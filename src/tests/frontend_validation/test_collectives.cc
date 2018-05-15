/*
//@HEADER
// ************************************************************************
//
//                      test_collectives.cc
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/


#include <gtest/gtest.h>

#include <vector>

#include <darma/impl/collective/allreduce.h>
#include <darma/interface/app/initial_access.h>
#include <darma/interface/app/create_work.h>

#include "test_frontend.h"

////////////////////////////////////////////////////////////////////////////////


MATCHER_P2(IsCollectiveDetailsWith, this_contribution, n_contributions,
  "is CollectiveDetails pointer with this_contribution()=[%(this_contribution)s]"
    " and n_contributions()=[%(n_contributions)s]"
) {
  using namespace ::testing;
  if(arg == nullptr) {
    *result_listener << "is null";
    return false;
  }

  *result_listener << "is CollectiveDetails pointer with this_contribution()="
                   << arg->this_contribution() << " and n_contributions()="
                   << arg->n_contributions();
  return arg->this_contribution() == this_contribution
    and arg->n_contributions() == n_contributions;
}

MATCHER_P3(IsCollectiveDetailsWithReduceOp,
  this_contribution, n_contributions, reduce_op,
  "is CollectiveDetails pointer with this_contribution()=[%(this_contribution)s]"
    ", n_contributions()=[%(n_contributions)s], and reduce_operation()=[%(reduce_op)s]"
) {
  using namespace ::testing;
  if(arg == nullptr) {
    *result_listener << "is null";
    return false;
  }

  *result_listener << "is CollectiveDetails pointer with this_contribution="
                   << arg->this_contribution() << ", n_contributions="
                   << arg->n_contributions() << ", and reduce_operation()="
                   << PrintToString(arg->reduce_operation());
  return arg->this_contribution() == this_contribution
    and arg->n_contributions() == n_contributions
    and (intptr_t)arg->reduce_operation() == (intptr_t)reduce_op;
}

////////////////////////////////////////////////////////////////////////////////

class TestCollectives
  : public TestFrontend
{
  protected:

    virtual void SetUp() {
      setup_mock_runtime<::testing::NiceMock>();
      TestFrontend::SetUp();
      ON_CALL(*mock_runtime, get_running_task())
        .WillByDefault(Return(top_level_task.get()));
    }

    virtual void TearDown() {
      TestFrontend::TearDown();
    }
};

////////////////////////////////////////////////////////////////////////////////

struct Test_simple_allreduce
  : TestCollectives,
    ::testing::WithParamInterface<int>
{ };

TEST_P(Test_simple_allreduce, overloads) {
  using namespace ::testing;
  using namespace darma;
  using namespace darma::keyword_arguments_for_collectives;
  using namespace mock_backend;
  using darma::frontend::Permissions;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(f_init, f_null, f_task_out, f_collect_out);
  use_t* use_initial = nullptr;
  use_t* use_cont = nullptr;
  use_t* task_use = nullptr;
  use_t* reduce_use = nullptr;
  use_t* use_reduce_cont = nullptr;

  int overload = GetParam();
  ASSERT_THAT(overload, Lt(2));

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_initial, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(f_init, f_task_out, task_use, f_null, use_cont);
  EXPECT_RELEASE_USE(use_initial);

  EXPECT_REGISTER_TASK(task_use);

  EXPECT_CALL(*mock_runtime, make_next_flow(f_task_out))
    .WillOnce(Return(f_collect_out));

  { // scope for sequence
    InSequence seq;

    EXPECT_REGISTER_USE(reduce_use, f_task_out, f_collect_out, None, Modify);

    EXPECT_REGISTER_USE(use_reduce_cont, f_collect_out, f_null, Modify, None);

    EXPECT_RELEASE_USE(use_cont);

    EXPECT_CALL(*mock_runtime, allreduce_use_gmock_proxy(
      // Can't just use Eq(ByRef(reduce_use)), since address is allowed to change
      // upon transfer of ownership.
      IsUseWithFlows(f_task_out, f_collect_out, Permissions::None, Permissions::Modify),
      IsCollectiveDetailsWithReduceOp(0, 10,
        // Check that the correct reduce op is getting used also...
        detail::_impl::_get_static_reduce_op_instance<
          detail::ReduceOperationWrapper<Add, std::vector<int>>
        >()
      ),
      Eq(make_key("world"))
    ));

  } // end of sequence

  EXPECT_FLOW_ALIAS(f_collect_out, f_null);
  EXPECT_RELEASE_USE(use_reduce_cont);

  //============================================================================
  // actual code being tested
  {
    auto tmp = initial_access<std::vector<int>>("hello");

    create_work([=]{
      tmp.set_value(std::vector<int>{5, 6, 7});
    });


    if(overload == 0)
      allreduce(tmp, piece=0, n_pieces=10, tag="world");
    else if(overload == 1)
      allreduce(in_out=tmp, piece=0, n_pieces=10, tag="world");


    // TODO check expectations on continuing context
  }
  //============================================================================

  EXPECT_RELEASE_USE(task_use);

  mock_runtime->registered_tasks.clear();
  mock_runtime->backend_owned_uses.clear();

}

INSTANTIATE_TEST_CASE_P(
  AllOverloads,
  Test_simple_allreduce,
  ::testing::Range(0, 2);
);


////////////////////////////////////////////////////////////////////////////////


struct Test_different_inout_allreduce
  : TestCollectives,
    ::testing::WithParamInterface<int>
{ };

TEST_P(Test_different_inout_allreduce, overload) {
  using namespace ::testing;
  using namespace darma;
  using namespace darma::keyword_arguments_for_collectives;
  using namespace mock_backend;
  using darma::frontend::Permissions;

  mock_runtime->save_tasks = true;

  int overload = GetParam();

  ASSERT_THAT(overload, Lt(3));

  DECLARE_MOCK_FLOWS(f_init, f_out_init, f_null, f_out_null, f_task_out, f_collect_out);
  use_t* use_initial = nullptr;
  use_t* use_cont = nullptr;
  use_t* use_out_cont = nullptr;
  use_t* use_out_initial = nullptr;
  use_t* task_use = nullptr;
  use_t* reduce_in_use = nullptr;
  use_t* reduce_out_use = nullptr;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_initial, make_key("in"));
  EXPECT_INITIAL_ACCESS(f_out_init, f_out_null, use_out_initial, make_key("out"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(f_init, f_task_out, task_use, f_null, use_cont);
  EXPECT_RELEASE_USE(use_initial);

  EXPECT_REGISTER_TASK(task_use);

  EXPECT_CALL(*mock_runtime, make_next_flow(f_out_init))
    .WillOnce(Return(f_collect_out));

  EXPECT_REGISTER_USE(reduce_in_use, f_task_out, nullptr, None, Read);

  EXPECT_REGISTER_USE(reduce_out_use, f_out_init, f_collect_out, None, Modify);
  EXPECT_RELEASE_USE(use_out_initial);

  EXPECT_REGISTER_USE(use_out_cont, f_collect_out, f_out_null, Modify, None);

  EXPECT_CALL(*mock_runtime, allreduce_use_gmock_proxy(
    // Can't just use Eq(ByRef(reduce_in_use)), since address is allowed to change
    // upon transfer of ownership.
    IsUseWithFlows(f_task_out, nullptr, Permissions::None, Permissions::Read),
    // Can't just use Eq(ByRef(reduce_out_use)), since address is allowed to change
    // upon transfer of ownership.
    IsUseWithFlows(f_out_init, f_collect_out, Permissions::None, Permissions::Modify),
    IsCollectiveDetailsWithReduceOp(0, 10,
      // Check that the correct reduce op is getting used also...
      detail::_impl::_get_static_reduce_op_instance<
        detail::ReduceOperationWrapper<Union, std::set<int>>
      >()
    ),
    Eq(make_key("world")
  )));

  EXPECT_FLOW_ALIAS(f_task_out, f_null);
  EXPECT_RELEASE_USE(use_cont);

  EXPECT_FLOW_ALIAS(f_collect_out, f_out_null);
  EXPECT_RELEASE_USE(use_out_cont);

  //============================================================================
  // actual code being tested
  {
    auto tmp_in = initial_access<std::set<int>>("in");
    auto tmp_out = initial_access<std::set<int>>("out");

    create_work([=]{
      tmp_in->insert(42);
    });

    if(overload == 0) {
      allreduce(
        input=tmp_in, output=tmp_out,
        piece=0, n_pieces=10, tag="world"
      );
    } else if(overload == 1) {
      allreduce(
        tmp_in, output=tmp_out,
        piece=0, n_pieces=10, tag="world"
      );
    } else if(overload == 2) {
      // test out the explicitly specified version also
      allreduce<Union>(
        tmp_in, tmp_out,
        piece=0, n_pieces=10, tag="world"
      );
    }

    // TODO check expectations on continuing context

  }
  //============================================================================

  EXPECT_RELEASE_USE(task_use);

  mock_runtime->registered_tasks.clear();

}

INSTANTIATE_TEST_CASE_P(
  AllOverloads,
  Test_different_inout_allreduce,
  ::testing::Range(0, 3);
);

////////////////////////////////////////////////////////////////////////////////

//TEST(TestReduceOp, string) {
//  using namespace darma;
//  using namespace darma::detail;
//
//  auto* reduce_op = _impl::_get_static_reduce_op_instance<
//    detail::ReduceOperationWrapper< Add, std::string >
//  >();
//
//  std::string hello = "hello";
//  std::string _world = " world";
//
//  std::string result = hello;
//
//  reduce_op->reduce_unpacked_into_unpacked(
//    &_world, &hello, 0, 1
//  );
//
//}

