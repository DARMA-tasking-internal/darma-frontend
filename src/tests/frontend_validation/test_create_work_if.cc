/*
//@HEADER
// ************************************************************************
//
//                      test_create_work_if.cc
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#include "mock_backend.h"
#include "test_frontend.h"

#include <darma.h>
#include <darma/impl/create_if_then.h>

////////////////////////////////////////////////////////////////////////////////

class TestCreateWorkIf
  : public TestFrontend
{
  protected:

    virtual void SetUp() {
      using namespace ::testing;

      setup_mock_runtime<::testing::NiceMock>();
      TestFrontend::SetUp();
      ON_CALL(*mock_runtime, get_running_task())
        .WillByDefault(Return(top_level_task.get()));
    }

    virtual void TearDown() {
      TestFrontend::TearDown();
    }

};

using namespace darma_runtime;
using namespace darma_runtime::experimental;

////////////////////////////////////////////////////////////////////////////////

//struct TestBasicSame
//  : TestCreateWorkIf,
//    ::testing::WithParamInterface<bool>
//{ };

TEST_F(TestCreateWorkIf, basic_same_always_false) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_if_out
  );
  use_t* if_use, *if_cont_use, *init_use, *outer_cont_use;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, init_use, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_if_out));


  {
    InSequence two_if_uses;

    EXPECT_REGISTER_USE_AND_SET_BUFFER(if_use, f_init, f_if_out, Modify, Read, value);
  }

  EXPECT_REGISTER_USE(outer_cont_use, f_if_out, f_null, Modify, None);

  EXPECT_RELEASE_USE(init_use);

  EXPECT_REGISTER_TASK(if_use);

  EXPECT_FLOW_ALIAS(f_if_out, f_null);

  EXPECT_RELEASE_USE(outer_cont_use);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");

    create_work_if([=]{
      return tmp.get_value() != 0; // should always be False
    }).then_([=]{
      tmp.set_value(73);
      FAIL() << "Ran then clause when if should have been false";
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_RELEASE_USE(if_use);

  run_all_tasks();


}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkIf, basic_same_always_true) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_if_out, f_then_fwd, f_then_out
  );
  use_t* if_use, *then_use, *use_outer_cont, *use_init, *then_cont_use;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_init, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_if_out));

  EXPECT_REGISTER_USE(use_outer_cont, f_if_out, f_null, Modify, None);

  {
    InSequence reg_before_release;

    EXPECT_REGISTER_USE_AND_SET_BUFFER(if_use, f_init, f_if_out, Modify, Read, value);

    EXPECT_RELEASE_USE(use_init);

    EXPECT_REGISTER_TASK(if_use);
  }

  EXPECT_FLOW_ALIAS(f_if_out, f_null);

  EXPECT_RELEASE_USE(use_outer_cont);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");

    create_work_if([=]{
      return tmp.get_value() == 0; // should always be true
    }).then_([=]{
      tmp.set_value(73);
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  {
    InSequence seq;

    // TODO decide if this should be a forwarding flow call
    EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
      .WillOnce(Return(f_then_out));

    EXPECT_REGISTER_USE_AND_SET_BUFFER(then_use, f_init, f_then_out, Modify, Modify, value);

    // TODO potentially this shouldn't be registered
    EXPECT_REGISTER_USE(then_cont_use, f_then_out, f_if_out, Modify, None);

    EXPECT_RELEASE_USE(if_use);

    EXPECT_REGISTER_TASK(then_use);

    // TODO eventually get rid of these next two calls
    EXPECT_FLOW_ALIAS(f_then_out, f_if_out);

    EXPECT_RELEASE_USE(then_cont_use);
  }

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  EXPECT_RELEASE_USE(then_use);

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_THAT(value, Eq(73));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkIf, basic_same_always_true_functor) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_if_out, f_then_fwd, f_then_out
  );
  use_t* if_use, *then_use, *use_outer_cont, *use_init, *then_cont_use;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_init, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_if_out));

  EXPECT_REGISTER_USE(use_outer_cont, f_if_out, f_null, Modify, None);

  {
    InSequence reg_before_release;

    EXPECT_REGISTER_USE_AND_SET_BUFFER(if_use, f_init, f_if_out, Modify, Read, value);

    EXPECT_RELEASE_USE(use_init);

    EXPECT_REGISTER_TASK(if_use);
  }

  EXPECT_FLOW_ALIAS(f_if_out, f_null);

  EXPECT_RELEASE_USE(use_outer_cont);

  //============================================================================
  // actual code being tested
  {
    struct IfFunctor {
      bool operator()(
        int const& v
      ) const {
        return v == 0; // should always be true
      }
    };

    auto tmp = initial_access<int>("hello");

    create_work_if<IfFunctor>(tmp).then_([=]{
      tmp.set_value(73);
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  {
    InSequence seq;

    // TODO decide if this should be a forwarding flow call
    EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
      .WillOnce(Return(f_then_out));

    EXPECT_REGISTER_USE_AND_SET_BUFFER(then_use, f_init, f_then_out, Modify, Modify, value);

    // TODO potentially this shouldn't be registered
    EXPECT_REGISTER_USE(then_cont_use, f_then_out, f_if_out, Modify, None);

    EXPECT_RELEASE_USE(if_use);

    EXPECT_REGISTER_TASK(then_use);

    // TODO eventually get rid of these next two calls
    EXPECT_FLOW_ALIAS(f_then_out, f_if_out);

    EXPECT_RELEASE_USE(then_cont_use);
  }

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  EXPECT_RELEASE_USE(then_use);

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_THAT(value, Eq(73));
}


////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkIf, basic_different_always_true) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_cond_init, f_cond_null, f_cap_init, f_cap_null,
    f_cap_if_out, f_cap_cont
  );
  use_t* cond_use = nullptr;
  use_t* cap_if_use = nullptr;
  use_t* cap_then_use = nullptr;
  use_t* cap_then_cont_use = nullptr;
  use_t* cond_init_use, *cap_init_use;
  use_t* cond_cont_use, *cap_cont_use;

  int cond_value = 0;
  int cap_value = 0;

  EXPECT_INITIAL_ACCESS(f_cond_init, f_cond_null, cond_init_use, make_key("hello"));

  EXPECT_INITIAL_ACCESS(f_cap_init, f_cap_null, cap_init_use, make_key("world"));

  EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR_AND_SET_BUFFER(f_cond_init, cond_use, cond_value);

  EXPECT_CALL(*mock_runtime, make_next_flow(f_cap_init))
    .WillOnce(Return(f_cap_if_out));

  EXPECT_REGISTER_USE(cap_if_use, f_cap_init, f_cap_if_out, Modify, None);

  {
    InSequence release_after;

    EXPECT_REGISTER_USE(cap_cont_use, f_cap_if_out, f_cap_null, Modify, None);

    EXPECT_RELEASE_USE(cap_init_use);
  }

  {
    InSequence release_after;

    EXPECT_REGISTER_TASK(cond_use, cap_if_use);

    EXPECT_RELEASE_USE(cond_init_use);
  }

  EXPECT_FLOW_ALIAS(f_cond_init, f_cond_null);

  EXPECT_FLOW_ALIAS(f_cap_if_out, f_cap_null);

  EXPECT_RELEASE_USE(cap_cont_use);

  //============================================================================
  // actual code being tested
  {

    auto cond_val = initial_access<int>("hello");
    auto cap_val = initial_access<int>("world");

    create_work_if([=]{
      EXPECT_THAT(cond_val.get_value(), Eq(0));
      return cond_val.get_value() != 42; // should always be true
    }).then_([=]{
      cap_val.set_value(73);
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_next_flow(f_cap_init))
    .WillOnce(Return(f_cap_cont));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(cap_then_use, f_cap_init, f_cap_cont, Modify, Modify, cap_value);

  // TODO get rid of this in the code
  EXPECT_REGISTER_USE(cap_then_cont_use, f_cap_cont, f_cap_if_out, Modify, None);

  EXPECT_REGISTER_TASK(cap_then_use);

  EXPECT_RELEASE_USE(cond_use);
  EXPECT_RELEASE_USE(cap_if_use);
  EXPECT_RELEASE_USE(cap_then_cont_use);

  EXPECT_FLOW_ALIAS(f_cap_cont, f_cap_if_out);

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_RELEASE_USE(cap_then_use);

  run_one_task();

  EXPECT_THAT(cap_value, Eq(73));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkIf, if_else_same_different_always_true) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_if_out, f_then_fwd, f_then_out,
    f_init2, f_null2, f_if_out_2
  );
  use_t* if_use = nullptr;
  use_t* then_use = nullptr;
  use_t* else_sched_use = nullptr;
  use_t* use_init, *use_init2, *use_cont, *use_cont2, *then_cont_use;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_init, make_key("hello"));
  EXPECT_INITIAL_ACCESS(f_init2, f_null2, use_init2, make_key("world"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_if_out));
  EXPECT_CALL(*mock_runtime, make_next_flow(f_init2))
    .WillOnce(Return(f_if_out_2));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(if_use, f_init, f_if_out, Modify, Read, value);
  EXPECT_REGISTER_USE(use_cont, f_if_out, f_null, Modify, None);

  EXPECT_REGISTER_USE(else_sched_use, f_init2, f_if_out_2, Modify, None);
  EXPECT_REGISTER_USE(use_cont2, f_if_out_2, f_null2, Modify, None);

  EXPECT_RELEASE_USE(use_init);
  EXPECT_RELEASE_USE(use_init2);

  EXPECT_REGISTER_TASK(if_use, else_sched_use);

  EXPECT_FLOW_ALIAS(f_if_out, f_null);
  EXPECT_RELEASE_USE(use_cont);

  EXPECT_FLOW_ALIAS(f_if_out_2, f_null2);
  EXPECT_RELEASE_USE(use_cont2);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");
    auto tmp2 = initial_access<int>("world");

    create_work_if([=]{
      return tmp.get_value() == 0; // should always be true
    }).then_([=]{
      tmp.set_value(73);
    }).else_([=]{
      tmp2.set_value(42);
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  {
    InSequence seq;

    // TODO decide if this should be a forwarding flow call
    EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
      .WillOnce(Return(f_then_out));

    EXPECT_REGISTER_USE_AND_SET_BUFFER(then_use, f_init, f_then_out, Modify, Modify, value);
    // TODO get rid of this register, maybe?
    EXPECT_REGISTER_USE(then_cont_use, f_then_out, f_if_out, Modify, None);

    EXPECT_RELEASE_USE(if_use);

    EXPECT_REGISTER_TASK(then_use);

  }

  // TODO FIXME: this needs to be ordered before the registration of then_use
  EXPECT_RELEASE_USE(else_sched_use);


  // TODO eventually get rid of these next two calls
  EXPECT_FLOW_ALIAS(f_then_out, f_if_out);
  EXPECT_RELEASE_USE(then_cont_use);

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  EXPECT_RELEASE_USE(then_use);

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_THAT(value, Eq(73));

}


////////////////////////////////////////////////////////////////////////////////

// TODO make this and the next one into a parameterized test
TEST_F(TestCreateWorkIf, if_else_same_same_always_true) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_if_out, f_then_fwd, f_then_out
  );
  use_t* if_use = nullptr;
  use_t* then_use = nullptr;
  use_t* use_init, *use_cont, *then_cont_use;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_init, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_if_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(if_use, f_init, f_if_out, Modify, Read, value);

  {
    InSequence reg_before_release;
    EXPECT_REGISTER_USE(use_cont, f_if_out, f_null, Modify, None);

    EXPECT_RELEASE_USE(use_init);
  }

  EXPECT_REGISTER_TASK(if_use);

  EXPECT_FLOW_ALIAS(f_if_out, f_null);
  EXPECT_RELEASE_USE(use_cont);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");

    create_work_if([=]{
      return tmp.get_value() == 0; // should always be true
    }).then_([=]{
      tmp.set_value(73);
    }).else_([=]{
      tmp.set_value(42);
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  {
    InSequence seq;

    EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
      .WillOnce(Return(f_then_out));

    EXPECT_REGISTER_USE_AND_SET_BUFFER(then_use, f_init, f_then_out, Modify, Modify, value);
    EXPECT_REGISTER_USE(then_cont_use, f_then_out, f_if_out, Modify, None);

    EXPECT_RELEASE_USE(if_use);

    EXPECT_REGISTER_TASK(then_use);

  }

  EXPECT_FLOW_ALIAS(f_then_out, f_if_out);
  EXPECT_RELEASE_USE(then_cont_use);

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  run_one_task();

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  EXPECT_RELEASE_USE(then_use);

  run_one_task();

  EXPECT_THAT(value, Eq(73));

}


////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkIf, if_else_same_same_always_false) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_if_out, f_then_fwd, f_then_out
  );
  use_t* if_use = nullptr;
  use_t* if_cont_use = nullptr;
  use_t* use_init = nullptr;
  use_t* then_use = nullptr;
  use_t* then_cont_use = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_init, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_if_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(if_use, f_init, f_if_out, Modify, Read, value);
  EXPECT_REGISTER_USE(if_cont_use, f_if_out, f_null, Modify, None);
  EXPECT_RELEASE_USE(use_init);

  EXPECT_REGISTER_TASK(if_use);

  EXPECT_FLOW_ALIAS(f_if_out, f_null);
  EXPECT_RELEASE_USE(if_cont_use);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");

    create_work_if([=]{
      return tmp.get_value() != 0; // should always be false
    }).then_([=]{
      tmp.set_value(73);
    }).else_([=]{
      tmp.set_value(42);
    });

  }
  //============================================================================

  {
    InSequence seq;

    EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
      .WillOnce(Return(f_then_out));

    EXPECT_REGISTER_USE_AND_SET_BUFFER(then_use, f_init, f_then_out, Modify, Modify, value);
    EXPECT_REGISTER_USE(then_cont_use, f_then_out, f_if_out, Modify, None);

    EXPECT_RELEASE_USE(if_use);

    EXPECT_REGISTER_TASK(then_use);

  }

  EXPECT_FLOW_ALIAS(f_then_out, f_if_out);
  EXPECT_RELEASE_USE(then_cont_use);

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  run_one_task();

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  EXPECT_RELEASE_USE(then_use);

  run_one_task();

  EXPECT_THAT(value, Eq(42));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkIf, basic_same_read_only) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null
  );
  use_t* if_use = nullptr;
  use_t* use_init = nullptr;
  use_t* then_use = nullptr;

  int value = 73;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_init, make_key("hello"));

#if _darma_has_feature(anti_flows)
  EXPECT_REGISTER_USE_AND_SET_BUFFER(if_use, f_init, nullptr, Read, Read, value);
#else
  EXPECT_REGISTER_USE_AND_SET_BUFFER(if_use, f_init, f_init, Read, Read, value);
#endif // _darma_has_feature(anti_flows)

  EXPECT_REGISTER_TASK(if_use);

  EXPECT_FLOW_ALIAS(f_init, f_null);
  EXPECT_RELEASE_USE(use_init);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");

    create_work_if([=]{
      return tmp.get_value() == 73; // should always be true
    }).then_(reads(tmp), [=]{
      ASSERT_THAT(tmp.get_value(), Eq(73));
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, establish_flow_alias(_, _)).Times(0);

  {
    InSequence seq;

#if _darma_has_feature(anti_flows)
    EXPECT_REGISTER_USE_AND_SET_BUFFER(then_use, f_init, nullptr, Read, Read, value);
#else
    EXPECT_REGISTER_USE_AND_SET_BUFFER(then_use, f_init, f_init, Read, Read, value);
#endif // _darma_has_feature(anti_flows)

    EXPECT_REGISTER_TASK(then_use);
  }

  // TODO FIXME this should be released before the then_use task is registered
  EXPECT_RELEASE_USE(if_use);

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  run_one_task();

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  EXPECT_RELEASE_USE(then_use);

  run_one_task();

  EXPECT_THAT(value, Eq(73));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkIf, basic_same_read_only_write_else) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_if_out
  );
  use_t* if_use = nullptr;
  use_t* then_use = nullptr;
  use_t* use_init;
  use_t* use_cont;

  int value = 73;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_init, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_if_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(if_use, f_init, f_if_out, Modify, Read, value);
  EXPECT_REGISTER_USE(use_cont, f_if_out, f_null, Modify, None);
  EXPECT_RELEASE_USE(use_init);

  EXPECT_REGISTER_TASK(if_use);

  EXPECT_FLOW_ALIAS(f_if_out, f_null);
  EXPECT_RELEASE_USE(use_cont);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");

    create_work_if([=]{
      return tmp.get_value() == 73; // should always be true
    }).then_(reads(tmp), [=]{
      ASSERT_THAT(tmp.get_value(), Eq(73));
    }).else_([=]{
      tmp.set_value(42);
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  {
    InSequence seq;

#if _darma_has_feature(anti_flows)
    EXPECT_REGISTER_USE_AND_SET_BUFFER(then_use, f_init, nullptr, Read, Read, value);
#else
    EXPECT_REGISTER_USE_AND_SET_BUFFER(then_use, f_init, f_init, Read, Read, value);
#endif // _darma_has_feature(anti_flows)

    EXPECT_REGISTER_TASK(then_use);
  }

  // TODO FIXME this should be released before the then_use task is registered
  EXPECT_RELEASE_USE(if_use);

  // TODO eventually get rid of these next two calls
  EXPECT_FLOW_ALIAS(f_init, f_if_out);

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  EXPECT_RELEASE_USE(then_use);

  run_one_task();

  EXPECT_THAT(value, Eq(73));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkIf, basic_same_true_if_lambda_then_functor) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_if_out, f_then_fwd, f_then_out
  );
  use_t* if_use = nullptr;
  use_t* use_initial = nullptr;
  use_t* use_cont = nullptr;
  use_t* then_use = nullptr;
  use_t* then_cont_use = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_initial, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_if_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(if_use, f_init, f_if_out, Modify, Read, value);
  EXPECT_REGISTER_USE(use_cont, f_if_out, f_null, Modify, None);
  EXPECT_RELEASE_USE(use_initial);

  EXPECT_REGISTER_TASK(if_use);

  EXPECT_FLOW_ALIAS(f_if_out, f_null);
  EXPECT_RELEASE_USE(use_cont);

  //============================================================================
  // actual code being tested
  {
    struct ThenFunctor {
      void operator()(int& v) const {
        v = 73;
      }
    };

    auto tmp = initial_access<int>("hello");

    create_work_if([=]{
      return tmp.get_value() == 0; // should be always true
    }).then_<ThenFunctor>(tmp);

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  {
    InSequence seq;

    // TODO decide if this should be a forwarding flow call
    EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
      .WillOnce(Return(f_then_out));

    EXPECT_REGISTER_USE_AND_SET_BUFFER(then_use, f_init, f_then_out, Modify, Modify, value);
    // TODO Eventually don't even register this
    EXPECT_REGISTER_USE(then_cont_use, f_then_out, f_if_out, Modify, None);

    EXPECT_RELEASE_USE(if_use);

    EXPECT_REGISTER_TASK(then_use);

    // TODO eventually get rid of these next two calls
    EXPECT_FLOW_ALIAS(f_then_out, f_if_out);

    EXPECT_RELEASE_USE(then_cont_use);
  }


  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  run_one_task();

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  EXPECT_RELEASE_USE(then_use);

  run_one_task();

  EXPECT_THAT(value, Eq(73));

}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestCreateWorkIf, same_false_if_lambda_then_lambda_else_functor) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_if_out, f_then_fwd, f_then_out
  );
  use_t* if_use = nullptr;
  use_t* use_init = nullptr;
  use_t* use_cont = nullptr;
  use_t* then_use = nullptr;
  use_t* then_use_cont = nullptr;


  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_init, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_if_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(if_use, f_init, f_if_out, Modify, Read, value);
  EXPECT_REGISTER_USE(use_cont, f_if_out, f_null, Modify, None);
  EXPECT_RELEASE_USE(use_init);

  EXPECT_REGISTER_TASK(if_use);

  EXPECT_FLOW_ALIAS(f_if_out, f_null);
  EXPECT_RELEASE_USE(use_cont);

  //============================================================================
  // actual code being tested
  {
    struct ElseFunctor {
      void operator()(int& v) const {
        v = 73;
      }
    };

    auto tmp = initial_access<int>("hello");

    create_work_if([=]{
      return tmp.get_value() != 0; // should be always false
    }).then_([=]{
      tmp.set_value(42);
      FAIL() << "took then branch on always-false predicate";
    }).else_<ElseFunctor>(tmp);

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  {
    InSequence seq;

    // TODO decide if this should be a forwarding flow call
    EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
      .WillOnce(Return(f_then_out));

    EXPECT_REGISTER_USE_AND_SET_BUFFER(then_use, f_init, f_then_out, Modify, Modify, value);
    EXPECT_REGISTER_USE(then_use_cont, f_then_out, f_if_out, Modify, None);

    EXPECT_RELEASE_USE(if_use);

    EXPECT_REGISTER_TASK(then_use);

    // TODO eventually get rid of these next two calls
    EXPECT_FLOW_ALIAS(f_then_out, f_if_out);

    EXPECT_RELEASE_USE(then_use_cont);
  }


  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  run_one_task();

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  EXPECT_RELEASE_USE(then_use);

  run_one_task();

  EXPECT_THAT(value, Eq(73));

}

////////////////////////////////////////////////////////////////////////////////


struct TestFunctorLambdaTrueFalse
  : TestCreateWorkIf,
    ::testing::WithParamInterface<std::tuple<bool, bool, bool, bool>>
{ };

TEST_P(TestFunctorLambdaTrueFalse, same_if_then_else) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  bool if_is_true = std::get<0>(GetParam());
  bool if_use_functor = std::get<1>(GetParam());
  bool then_use_functor = std::get<2>(GetParam());
  bool else_use_functor = std::get<3>(GetParam());

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_if_out, f_then_fwd, f_then_out
  );

  use_t* use_init = nullptr;
  use_t* use_cont = nullptr;
  use_t* if_use = nullptr;
  use_t* then_use = nullptr;
  use_t* then_use_cont = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_init, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_if_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(if_use, f_init, f_if_out, Modify, Read, value);
  EXPECT_REGISTER_USE(use_cont, f_if_out, f_null, Modify, None);

  EXPECT_RELEASE_USE(use_init);

  EXPECT_REGISTER_TASK(if_use);

  EXPECT_FLOW_ALIAS(f_if_out, f_null);
  EXPECT_RELEASE_USE(use_cont);

  //============================================================================
  // actual code being tested
  {
    struct IfFunctor {
      bool operator()(int const& v, bool should_be_true) const {
        return should_be_true ? (v == 0) : (v != 0);
      }
    };

    struct ThenFunctor {
      void operator()(int& v) const {
        v = 42;
      }
    };

    struct ElseFunctor {
      void operator()(int& v) const {
        v = 73;
      }
    };

    auto tmp = initial_access<int>("hello");

    if(if_use_functor and then_use_functor and else_use_functor) {
      create_work_if<IfFunctor>(tmp, if_is_true)
        .then_<ThenFunctor>(tmp)
        .else_<ElseFunctor>(tmp);
    }
    else if(!if_use_functor and then_use_functor and else_use_functor) {
      create_work_if([=]{ return if_is_true ? (tmp.get_value() == 0) : (tmp.get_value() != 0); })
        .then_<ThenFunctor>(tmp)
        .else_<ElseFunctor>(tmp);
    }
    else if(!if_use_functor and !then_use_functor and else_use_functor) {
      create_work_if([=]{ return if_is_true ? (tmp.get_value() == 0) : (tmp.get_value() != 0); })
        .then_([=]{ tmp.set_value(42); })
        .else_<ElseFunctor>(tmp);
    }
    else if(!if_use_functor and then_use_functor and !else_use_functor) {
      create_work_if([=]{ return if_is_true ? (tmp.get_value() == 0) : (tmp.get_value() != 0); })
        .then_<ThenFunctor>(tmp)
        .else_([=]{ tmp.set_value(73); });
    }
    else if(!if_use_functor and !then_use_functor and !else_use_functor) {
      create_work_if([=]{ return if_is_true ? (tmp.get_value() == 0) : (tmp.get_value() != 0); })
        .then_([=]{ tmp.set_value(42); })
        .else_([=]{ tmp.set_value(73); });
    }
    else if(if_use_functor and !then_use_functor and else_use_functor) {
      create_work_if<IfFunctor>(tmp, if_is_true)
        .then_([=]{ tmp.set_value(42); })
        .else_<ElseFunctor>(tmp);
    }
    else if(if_use_functor and !then_use_functor and !else_use_functor) {
      create_work_if<IfFunctor>(tmp, if_is_true)
        .then_([=]{ tmp.set_value(42); })
        .else_([=]{ tmp.set_value(73); });
    }
    else if(if_use_functor and then_use_functor and !else_use_functor) {
      create_work_if<IfFunctor>(tmp, if_is_true)
        .then_<ThenFunctor>(tmp)
        .else_([=]{ tmp.set_value(73); });
    }
    else {
      FAIL() << "Huh?";
    }

  }
  //============================================================================

  {
    InSequence seq;

    // TODO decide if this should be a forwarding flow call
    EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
      .WillOnce(Return(f_then_out));

    EXPECT_REGISTER_USE_AND_SET_BUFFER(then_use, f_init, f_then_out, Modify, Modify, value);
    EXPECT_REGISTER_USE(then_use_cont, f_then_out, f_if_out, Modify, None);

    EXPECT_RELEASE_USE(if_use);

    EXPECT_REGISTER_TASK(then_use);

    // TODO eventually get rid of these next two calls
    EXPECT_FLOW_ALIAS(f_then_out, f_if_out);

    EXPECT_RELEASE_USE(then_use_cont);
  }


  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  run_one_task();

  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  EXPECT_RELEASE_USE(then_use);

  run_one_task();

  EXPECT_THAT(value, Eq(if_is_true ? 42 : 73));

}


INSTANTIATE_TEST_CASE_P(
  all_cases_same_if_then_else,
  TestFunctorLambdaTrueFalse,
  ::testing::Combine(
    ::testing::Bool(),
    ::testing::Bool(),
    ::testing::Bool(),
    ::testing::Bool()
  )
);

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkIf, multiple_different_always_true) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_cond_init, f_cond_null, f_cap_init, f_cap_null,
    f_cap_if_out, f_cap_cont
  );
  use_t* cond_use = nullptr;
  use_t* cond_init_use = nullptr;
  use_t* cap_init_use = nullptr;
  use_t* cap_if_use = nullptr;
  use_t* cap_then_use = nullptr;
  use_t* cap_then_cont_use = nullptr;
  use_t* outer_cont_use = nullptr;
  use_t* cond_after_use = nullptr;


  int cond_value = 0;
  int cap_value = 0;

  EXPECT_INITIAL_ACCESS(f_cond_init, f_cond_null, cond_init_use, make_key("hello"));

  EXPECT_INITIAL_ACCESS(f_cap_init, f_cap_null, cap_init_use, make_key("world"));

  EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR_AND_SET_BUFFER(f_cond_init, cond_use, cond_value);

  {
    // TODO decide if this should be done differently...

    InSequence two_nexts;
    EXPECT_CALL(*mock_runtime, make_next_flow(f_cap_init))
      .WillOnce(Return(f_cap_if_out));

    EXPECT_REGISTER_USE(cap_if_use, f_cap_init, f_cap_if_out, Modify, None);
    EXPECT_REGISTER_USE(outer_cont_use, f_cap_if_out, f_cap_null, Modify, None);
    EXPECT_RELEASE_USE(cap_init_use);

    EXPECT_REGISTER_TASK(cond_use, cap_if_use);

  }



  EXPECT_FLOW_ALIAS(f_cap_if_out, f_cap_null);
  EXPECT_RELEASE_USE(outer_cont_use);

  EXPECT_FLOW_ALIAS(f_cond_init, f_cond_null);
  EXPECT_RELEASE_USE(cond_init_use);

  //============================================================================
  // actual code being tested
  {

    auto cond_val = initial_access<int>("hello");
    auto cap_val = initial_access<int>("world");

    create_work_if([=]{
      return cond_val.get_value() != 42; // should always be true
    }).then_([=]{
      cap_val.set_value(73);
    });

    EXPECT_REGISTER_USE_AND_SET_BUFFER(cond_after_use, f_cond_init, f_cond_init, Read, Read, cond_value);

    EXPECT_REGISTER_TASK(cond_after_use);

    create_work(reads(cond_val), [=]{
      EXPECT_THAT(cond_val.get_value(), Eq(0));
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_next_flow(f_cap_init))
    .WillOnce(Return(f_cap_cont));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(cap_then_use, f_cap_init, f_cap_cont, Modify, Modify, cap_value);
  EXPECT_REGISTER_USE(cap_then_cont_use, f_cap_cont, f_cap_if_out, Modify, None);

  EXPECT_REGISTER_TASK(cap_then_use);

  EXPECT_RELEASE_USE(cond_use);
  EXPECT_RELEASE_USE(cap_if_use);

  EXPECT_FLOW_ALIAS(f_cap_cont, f_cap_if_out);
  EXPECT_RELEASE_USE(cap_then_cont_use);

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, legacy_register_use(_)).Times(0);
  EXPECT_CALL(*mock_runtime, establish_flow_alias(_, _)).Times(0);

  EXPECT_RELEASE_USE(cap_then_use);

  run_most_recently_added_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, legacy_register_use(_)).Times(0);
  EXPECT_CALL(*mock_runtime, establish_flow_alias(_, _)).Times(0);

  EXPECT_RELEASE_USE(cond_after_use);

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, legacy_register_use(_)).Times(0);
  EXPECT_CALL(*mock_runtime, establish_flow_alias(_, _)).Times(0);

  EXPECT_THAT(cap_value, Eq(73));

}



////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkIf, collection_then_always_true) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_if_out, f_then_fwd, f_then_out,
    f_init_c, f_null_c, f_if_out_c, fout_coll, f_then_out_c
  );
  MockFlow f_in_idx[4] = { "f_in_idx[0]", "f_in_idx[1]", "f_in_idx[2]", "f_in_idx[3]"};
  MockFlow f_out_idx[4] = { "f_out_idx[0]", "f_out_idx[1]", "f_out_idx[2]", "f_out_idx[3]"};
  use_t* use_idx[4] = { nullptr, nullptr, nullptr, nullptr };
  use_t* use_init = nullptr;
  use_t* use_init_coll = nullptr;
  use_t* use_cont = nullptr;
  use_t* use_outer_coll_cont = nullptr;
  use_t* use_coll = nullptr;
  use_t* use_coll_cont = nullptr;
  use_t* if_use = nullptr;
  use_t* if_c_use = nullptr;
  use_t* then_use = nullptr;
  use_t* then_use_cont = nullptr;
  use_t* then_c_use = nullptr;
  use_t* then_c_use_cont = nullptr;

  int value = 0;
  int values[4] = {0, 0, 0, 0};

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_init, make_key("hello"));
  EXPECT_INITIAL_ACCESS_COLLECTION(f_init_c, f_null_c, use_init_coll, make_key("world"), 4);

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_if_out));

  EXPECT_CALL(*mock_runtime, make_next_flow_collection(f_init_c))
    .WillOnce(Return(f_if_out_c));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(if_use, f_init, f_if_out, Modify, Read, value);
  EXPECT_REGISTER_USE(if_c_use, f_init_c, f_if_out_c, Modify, None);
  EXPECT_REGISTER_USE(use_cont, f_if_out, f_null, Modify, None);
  EXPECT_REGISTER_USE(use_outer_coll_cont, f_if_out_c, f_null_c, Modify, None);
  EXPECT_RELEASE_USE(use_init);
  EXPECT_RELEASE_USE(use_init_coll);

  EXPECT_REGISTER_TASK(if_use, if_c_use);

  EXPECT_FLOW_ALIAS(f_if_out, f_null);
  EXPECT_RELEASE_USE(use_cont);

  EXPECT_FLOW_ALIAS(f_if_out_c, f_null_c);
  EXPECT_RELEASE_USE(use_outer_coll_cont);

  //============================================================================
  // actual code being tested
  {

    struct Foo {
      void operator()(Index1D<int> index,
        AccessHandleCollection<int, Range1D<int>> coll
      ) const {
        coll[index].local_access().set_value(42);
      }
    };

    auto tmp = initial_access<int>("hello");
    auto tmp_c = initial_access_collection<int>("world",
      index_range=Range1D<int>(4)
    );

    create_work_if([=]{
      return tmp.get_value() == 0; // should always be true
    }).then_([=]{
      tmp.set_value(73);
      create_concurrent_work<Foo>(tmp_c,
        index_range=Range1D<int>(4)
      );
    });

  }
  //============================================================================

  // Assert that there's only one task registered
  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  // Stuff that should happen in the if clause

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_then_out));
  EXPECT_CALL(*mock_runtime, make_next_flow_collection(f_init_c))
    .WillOnce(Return(f_then_out_c));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(then_use, f_init, f_then_out, Modify, Modify, value);
  EXPECT_REGISTER_USE(then_use_cont, f_then_out, f_if_out, Modify, None);

  // Expect register schedule only use for the collection
  EXPECT_REGISTER_USE_COLLECTION(then_c_use, f_init_c, f_then_out_c, Modify, None, 4);

  {
    InSequence reg_before_release;

    // Expect register continuation use for the collection
    EXPECT_REGISTER_USE_COLLECTION(then_c_use_cont,
      f_then_out_c, f_if_out_c, Modify, None, 4);

    // Expect release of the collection use for the if clause
    EXPECT_RELEASE_USE(if_c_use);

  }

  // Expect release use for the regular handle
  EXPECT_RELEASE_USE(if_use);

  // Expect register task  with both uses
  EXPECT_REGISTER_TASK(then_use, then_c_use);

  // Expect aliases of both continuation uses
  EXPECT_FLOW_ALIAS(f_then_out, f_if_out);
  EXPECT_RELEASE_USE(then_use_cont);

  // Expect release of the continuation use
  // We can do this before or after the register task call, but preferrably
  // before
  EXPECT_FLOW_ALIAS(f_then_out_c, f_if_out_c);
  EXPECT_RELEASE_USE(then_c_use_cont);

  run_one_task(); // run the "if" part

  // Assert that there's still only one task
  ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));  // The "then" task should be registered

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  EXPECT_CALL(*mock_runtime, make_next_flow_collection(f_init_c))
    .WillOnce(Return(fout_coll));

  EXPECT_REGISTER_USE_COLLECTION(use_coll, f_init_c, fout_coll, Modify, Modify, 4);

  {
    InSequence reg_before_rel;

    EXPECT_REGISTER_USE_COLLECTION(use_coll_cont, fout_coll, f_then_out_c, Modify, None, 4);

    EXPECT_RELEASE_USE(then_c_use);
  }

  EXPECT_RELEASE_USE(use_coll_cont);

  EXPECT_CALL(*mock_runtime, register_task_collection_gmock_proxy(
    CollectionUseInGetDependencies(ByRef(use_coll))
  ));

  EXPECT_FLOW_ALIAS(fout_coll, f_then_out_c);

  EXPECT_RELEASE_USE(then_use);

  run_one_task(); // run the "then" part

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  for(int i = 0; i < 4; ++i) {
    values[i] = 0;

    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(f_init_c, i))
      .WillOnce(Return(f_in_idx[i]));
    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(fout_coll, i))
      .WillOnce(Return(f_out_idx[i]));
    EXPECT_REGISTER_USE_AND_SET_BUFFER(use_idx[i], f_in_idx[i], f_out_idx[i],
      Modify, Modify, values[i]);

    auto created_task = mock_runtime->task_collections.front()->create_task_for_index(i);

    EXPECT_THAT(created_task.get(), UseInGetDependencies(use_idx[i]));

    EXPECT_RELEASE_USE(use_idx[i]);

    created_task->run();

    EXPECT_THAT(values[i], Eq(42));

  }

  EXPECT_RELEASE_USE(use_coll);

  mock_runtime->task_collections.front().reset(nullptr);

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  EXPECT_THAT(value, Eq(73));

}

