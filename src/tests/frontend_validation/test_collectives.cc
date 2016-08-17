/*
//@HEADER
// ************************************************************************
//
//                      test_collectives.cc
//                         DARMA
//              Copyright (C) 2016 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
// Questions? Contact David S. Hollman (dshollm@sandia.gov)
//
// ************************************************************************
//@HEADER
*/


#include <gtest/gtest.h>

#include <vector>

#include <darma/impl/collective/allreduce.h>

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

TEST_F(TestCollectives, simple_allreduce) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_collectives;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow f_init, f_null, f_task_out, f_collect_out;
  use_t* task_use = nullptr;
  use_t* reduce_use = nullptr;

  expect_initial_access(f_init, f_null, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(f_init, f_task_out, task_use);

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(task_use))));

  EXPECT_CALL(*mock_runtime, make_next_flow(&f_task_out))
    .WillOnce(Return(&f_collect_out));

  EXPECT_REGISTER_USE(reduce_use, f_task_out, f_collect_out, None, Modify);

  EXPECT_CALL(*mock_runtime, allreduce_use(
    Eq(ByRef(reduce_use)), Eq(ByRef(reduce_use)),
    IsCollectiveDetailsWith(0, 10),
    Eq(make_key("world")
  )));

  EXPECT_CALL(*mock_runtime, release_use(Eq(ByRef(reduce_use))))
    .WillOnce(Invoke([&](auto&&...){ reduce_use = nullptr; }));

  {
    auto tmp = initial_access<int>("hello");

    create_work([=]{
      tmp.set_value(42);
    });

    allreduce(tmp, piece=0, n_pieces=10, tag="world");

    // TODO check expectations on continuing context

  }

  EXPECT_CALL(*mock_runtime, release_use(Eq(ByRef(task_use))))
    .WillOnce(Invoke([&](auto&&...){ task_use = nullptr; }));

  mock_runtime->registered_tasks.clear();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCollectives, allreduce_different_input_output) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_collectives;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow f_init, f_out_init, f_null, f_out_null, f_task_out, f_collect_out;
  use_t* task_use = nullptr;
  use_t* reduce_in_use = nullptr;
  use_t* reduce_out_use = nullptr;

  EXPECT_INITIAL_ACCESS(f_init, f_null, make_key("in"));
  EXPECT_INITIAL_ACCESS(f_out_init, f_out_null, make_key("out"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(f_init, f_task_out, task_use);

  EXPECT_CALL(*mock_runtime,
    register_task_gmock_proxy(UseInGetDependencies(ByRef(task_use)))
  );

  EXPECT_CALL(*mock_runtime, make_next_flow(&f_out_init))
    .WillOnce(Return(&f_collect_out));

  EXPECT_REGISTER_USE(reduce_in_use, f_task_out, f_task_out, None, Read);

  EXPECT_REGISTER_USE(reduce_out_use, f_out_init, f_collect_out, None, Write);

  EXPECT_CALL(*mock_runtime, allreduce_use(
    Eq(ByRef(reduce_in_use)), Eq(ByRef(reduce_out_use)),
    IsCollectiveDetailsWith(0, 10),
    Eq(make_key("world")
  )));

  EXPECT_RELEASE_USE(reduce_in_use);
  EXPECT_RELEASE_USE(reduce_out_use);

  {
    auto tmp_in = initial_access<int>("in");
    auto tmp_out = initial_access<int>("out");

    create_work([=]{
      tmp_in.set_value(42);
    });

    allreduce(input=tmp_in, output=tmp_out,
      piece=0, n_pieces=10, tag="world"
    );

    // TODO check expectations on continuing context

  }

  EXPECT_RELEASE_USE(task_use);

  mock_runtime->registered_tasks.clear();

}
