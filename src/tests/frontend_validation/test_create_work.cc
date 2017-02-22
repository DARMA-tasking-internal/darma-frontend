/*
//@HEADER
// ************************************************************************
//
//                          test_create_work.cc
//                         dharma_new
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

class TestCreateWork_mod_capture_MM_Test;
#define DARMA_TEST_FRONTEND_VALIDATION_CREATE_WORK 1

#include "mock_backend.h"
#include "test_frontend.h"

#include <darma/interface/app/initial_access.h>
#include <darma/interface/app/read_access.h>
#include <darma/interface/app/create_work.h>

////////////////////////////////////////////////////////////////////////////////

class TestCreateWork
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

////////////////////////////////////////////////////////////////////////////////

struct TestModCaptureMN
  : TestCreateWork,
    ::testing::WithParamInterface<bool>
{ };

TEST_P(TestModCaptureMN, mod_capture_MN) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  const bool use_helper = GetParam();

  DECLARE_MOCK_FLOWS(f_initial, f_null, f_task_out);
  use_t* task_use = nullptr;
  use_t* use_initial = nullptr;
  use_t* use_cont = nullptr;

  EXPECT_INITIAL_ACCESS(f_initial, f_null, use_initial, make_key("hello"));

  if(use_helper) {
    EXPECT_MOD_CAPTURE_MN_OR_MR(f_initial, f_task_out, task_use, f_null, use_cont);
  }
  else {
    EXPECT_CALL(*mock_runtime, make_next_flow(&f_initial))
      .WillOnce(Return(&f_task_out));

    EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows(
      f_initial, f_task_out, use_t::Modify, use_t::Modify
    ))).WillOnce(SaveArg<0>(&task_use));

    EXPECT_REGISTER_USE(use_cont, f_task_out, f_null, Modify, None);
  }

  {
    InSequence rel_before_reg;

    EXPECT_RELEASE_USE(use_initial);

    EXPECT_REGISTER_TASK(task_use);

    EXPECT_FLOW_ALIAS(f_task_out, f_null);

    EXPECT_RELEASE_USE(use_cont);
  }

  {
    auto tmp = initial_access<int>("hello");

    create_work([=]{
      // This code doesn't run in this example
      tmp.set_value(5);
      FAIL() << "This code block shouldn't be running in this example";
    });

  } // tmp deleted

  EXPECT_RELEASE_USE(task_use);

  mock_runtime->registered_tasks.clear();

}

INSTANTIATE_TEST_CASE_P(
  WithAndWithoutHelper,
  TestModCaptureMN,
  ::testing::Bool()
);


////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, mod_capture_MN_vector) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  DECLARE_MOCK_FLOWS(finit1, finit2, fnull1, fnull2, fout1, fout2);
  use_t *use_1, *use_2, *init_use_1, *init_use_2, *cont_use_1, *cont_use_2;

  EXPECT_INITIAL_ACCESS(finit1, fnull1, init_use_1, make_key("hello"));
  EXPECT_INITIAL_ACCESS(finit2, fnull2, init_use_2, make_key("world"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(finit1, fout1, use_1, fnull1, cont_use_1);

  EXPECT_RELEASE_USE(init_use_1);

  EXPECT_MOD_CAPTURE_MN_OR_MR(finit2, fout2, use_2, fnull2, cont_use_2);

  EXPECT_RELEASE_USE(init_use_2);

  EXPECT_REGISTER_TASK(use_1, use_2);

  EXPECT_FLOW_ALIAS(fout1, fnull1);

  EXPECT_RELEASE_USE(cont_use_1);

  EXPECT_FLOW_ALIAS(fout2, fnull2);

  EXPECT_RELEASE_USE(cont_use_2);


  {
    std::vector<AccessHandle<int>> handles;

    handles.push_back(initial_access<int>("hello"));
    handles.emplace_back(initial_access<int>("world"));

    create_work([=]{
      handles[0].set_value(5);
      FAIL() << "This code block shouldn't be running in this example";
    });

  } // handles deleted

  EXPECT_RELEASE_USE(use_1);

  EXPECT_RELEASE_USE(use_2);

  mock_runtime->registered_tasks.clear();
}

////////////////////////////////////////////////////////////////////////////////

#if 0 // Arbitrary Publish-Fetch deprecated

struct TestRoCaptureRN
  : TestCreateWork,
    ::testing::WithParamInterface<bool>
{ };

TEST_P(TestRoCaptureRN, ro_capture_RN) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  Sequence s1, s_release_read;

  DECLARE_MOCK_FLOWS(f_fetch, f_null);
  use_t* read_use = nullptr;
  EXPECT_READ_ACCESS(f_fetch, f_null, make_key("hello"), make_key("world"));

  bool use_helper = GetParam();

  if(use_helper) {
    EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR(f_fetch, read_use);
  }
  else {

    // ro-capture of RN
    EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows(
      &f_fetch, &f_fetch, use_t::Read, use_t::Read
    ))).WillOnce(SaveArg<0>(&read_use));

  }

  EXPECT_REGISTER_TASK(read_use);

  {

    auto tmp = read_access<int>("hello", version="world");
    create_work([=]{
      std::cout << tmp.get_value();
      FAIL() << "This code block shouldn't be running in this example";
    });

    Expectation fa1 = EXPECT_FLOW_ALIAS(f_fetch, f_null);
    EXPECT_RELEASE_FLOW(f_null).After(fa1);

  }

  Expectation rel_read = EXPECT_RELEASE_USE(read_use);

  EXPECT_RELEASE_FLOW(f_fetch).After(rel_read);

  mock_runtime->registered_tasks.clear();

}

INSTANTIATE_TEST_CASE_P(
  WithAndWithoutHelper,
  TestRoCaptureRN,
  ::testing::Bool()
);

#endif

////////////////////////////////////////////////////////////////////////////////

struct TestCaptureMM
  : TestCreateWork,
    ::testing::WithParamInterface<std::tuple<bool, bool>>
{ };

TEST_P(TestCaptureMM, capture_MM) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  bool ro_capture = std::get<0>(GetParam());
  bool use_vector = std::get<1>(GetParam());

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    finit, fnull, f_outer_out, f_forwarded, f_inner_out
  );
  use_t* use_outer, *use_inner, *use_continuing, *use_outer_cont, *use_initial;
  use_outer = use_inner = use_continuing = nullptr;

  EXPECT_INITIAL_ACCESS(finit, fnull, use_initial, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(finit, f_outer_out, use_outer, fnull, use_outer_cont);

  EXPECT_RELEASE_USE(use_initial);

  task_t* outer;
  int value = 0;

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(use_outer))))
    .WillOnce(Invoke([&](auto* task) {
      for(auto&& dep : task->get_dependencies()) {
        dep->get_data_pointer_reference() = (void*)(&value);
      }
      outer = task;
    }));

  EXPECT_FLOW_ALIAS(f_outer_out, fnull);

  EXPECT_RELEASE_USE(use_outer_cont);

  //============================================================================
  // Actual code being tested
  if(use_vector) {

    std::vector<AccessHandle<int>> tmp;
    tmp.push_back(initial_access<int>("hello"));

    create_work([=]{
      tmp[0].set_value(42);
      if(not ro_capture) {
        create_work([=]{
          ASSERT_THAT(tmp[0].get_value(), Eq(42));
        });
      }
      else {
        create_work(reads(tmp[0]), [=]{
          ASSERT_THAT(tmp[0].get_value(), Eq(42));
        });
        // State is MR, should work...
        ASSERT_THAT(tmp[0].get_value(), Eq(42));
      }
    });

  }

  //----------------------------------------------------------------------------

  else {

    auto tmp = initial_access<int>("hello");

    create_work([=]{
      tmp.set_value(42);
      if(not ro_capture) {
        create_work([=]{
          ASSERT_THAT(tmp.get_value(), Eq(42));
        });
      }
      else {
        create_work(reads(tmp), [=]{
          ASSERT_THAT(tmp.get_value(), Eq(42));
        });
        // State is MR, should work...
        ASSERT_THAT(tmp.get_value(), Eq(42));
      }
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  ON_CALL(*mock_runtime, get_running_task())
    .WillByDefault(Return(ByRef(outer)));

  EXPECT_CALL(*mock_runtime, make_forwarding_flow(&finit))
    .WillOnce(Return(&f_forwarded));

  if(not ro_capture) {
    //InSequence seq;

    EXPECT_CALL(*mock_runtime, make_next_flow(&f_forwarded))
      .WillOnce(Return(&f_inner_out));

    EXPECT_REGISTER_USE_AND_SET_BUFFER(use_inner, f_forwarded, f_inner_out, Modify, Modify, value);

    // Doesn't really need to be sequenced w.r.t. prev register
    EXPECT_REGISTER_USE(use_continuing, f_inner_out, f_outer_out, Modify, None);

    EXPECT_RELEASE_USE(use_outer);

  }

  else {

    InSequence seq;

    EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows(
      &f_forwarded, &f_forwarded, use_t::Read, use_t::Read
    )))
      .WillOnce(Invoke([&](auto&& use){
        use->get_data_pointer_reference() = (void*)(&value);
        use_inner = use;
      }));
    // Expect the continuing context use to be registered after the captured context
    EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows(
      &f_forwarded, &f_outer_out, use_t::Modify, use_t::Read
    ))).WillOnce(Invoke([&](auto&& use){
      // Shouldn't be necessary
      // use->get_data_pointer_reference() = (void*)(&value);
      use_continuing = use;
    }));

    EXPECT_RELEASE_USE(use_outer);
  }

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(use_inner))));

  if(not ro_capture) {
    EXPECT_FLOW_ALIAS(f_inner_out, f_outer_out);
  }
  else{
    // Note: Currently there is no ordering requirement on these two
    EXPECT_FLOW_ALIAS(f_forwarded, f_outer_out);
  }

  EXPECT_RELEASE_USE(use_continuing);

  EXPECT_RELEASE_USE(use_inner);
  //EXPECT_CALL(*mock_runtime, release_use(Eq(ByRef(use_inner))))
  //  .WillOnce(Assign(&use_inner, nullptr));

  run_all_tasks();

}

INSTANTIATE_TEST_CASE_P(
  ReadOrModWithAndWithoutVector,
  TestCaptureMM,
  ::testing::Combine(
    ::testing::Bool(),
    ::testing::Bool()
  )
);


////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, named_task) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace mock_backend;

  EXPECT_CALL(*mock_runtime,
    register_task_gmock_proxy(HasName(make_key("hello_task", "world", 42)))
  );

  {
    create_work( name("hello_task", "world", 42),
      [=] {
        // This code doesn't run in this example
        FAIL() << "This code block shouldn't be running in this example";
      }
    );
  }

  mock_runtime->registered_tasks.clear();
}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestCreateWork, handle_aliasing) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(finit, fnull, fcapt);
  use_t* use_capt, *use_init, *use_cont;

  EXPECT_INITIAL_ACCESS(finit, fnull, use_init, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(finit, fcapt, use_capt, fnull, use_cont);

  EXPECT_RELEASE_USE(use_init);

  EXPECT_REGISTER_TASK(use_capt);

  EXPECT_FLOW_ALIAS(fcapt, fnull);

  EXPECT_RELEASE_USE(use_cont);

  //============================================================================
  // actual code being tested
  {
    auto call_me = [](AccessHandle<int> a, AccessHandle<int> b) {
      create_work(allow_aliasing=true, [=]{
        std::cout << (a.get_value() * b.get_value()) << std::endl;
      });
    };

    auto tmp = initial_access<int>("hello");

    call_me(tmp, tmp);

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_RELEASE_USE(use_capt);

  mock_runtime->registered_tasks.clear();

}

////////////////////////////////////////////////////////////////////////////////

struct TestScheduleOnly
  : TestCreateWork,
    ::testing::WithParamInterface<bool>
{ };

TEST_P(TestScheduleOnly, schedule_only) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(finit, fnull, fcapt, f_sched_out, f_immed_out);
  use_t *use_immed_capt, *use_sched_capt, *use_read_capt, *use_sched_contin,
    *use_ro_sched, *use_init, *use_cont;
  use_read_capt = use_immed_capt = use_sched_capt = nullptr;
  use_ro_sched = nullptr;

  int value = 0;

  bool sched_capture_read = GetParam();

  EXPECT_INITIAL_ACCESS(finit, fnull, use_init, make_key("hello"));

  // First, expect the task that does a schedule-only capture

  // Expect a schedule-only mod capture
  EXPECT_CALL(*mock_runtime, make_next_flow(finit))
    .WillOnce(Return(f_sched_out));
  Expectation reg_sched =
    EXPECT_REGISTER_USE(use_sched_capt, finit, f_sched_out, Modify, None);

  EXPECT_RELEASE_USE(use_init);

  EXPECT_REGISTER_USE(use_sched_contin, f_sched_out, fnull, Modify, None);

  EXPECT_REGISTER_TASK(use_sched_capt).After(reg_sched);

  if(not sched_capture_read) {
    // Then expect the task that does a read-only capture
    EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR_AND_SET_BUFFER(f_sched_out, use_read_capt, value);
    EXPECT_REGISTER_TASK(use_read_capt);
  }
  else {
    // Expect a schedule-only ro capture
    EXPECT_REGISTER_USE(use_ro_sched, f_sched_out, f_sched_out, Read, None);
    EXPECT_REGISTER_TASK(use_ro_sched);
  }

  EXPECT_FLOW_ALIAS(f_sched_out, fnull);

  EXPECT_RELEASE_USE(use_sched_contin);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");

    create_work(schedule_only(tmp), [=]{
      create_work([=]{
        tmp.set_value(42);
      });
    });

    if(not sched_capture_read) {
      create_work(reads(tmp), [=] {
        EXPECT_THAT(tmp.get_value(), Eq(42));
      });
    }
    else {
      create_work(schedule_only(reads(tmp)), [=] {
        create_work(reads(tmp), [=] {
          EXPECT_THAT(tmp.get_value(), Eq(42));
        });
      });

    }

  }
  //============================================================================

  // Now expect the mod-immediate task to be registered once the first task is run
  // TODO technically I should change the return value of get_running_task here...

  EXPECT_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(finit, f_immed_out, use_immed_capt, value);
  EXPECT_REGISTER_TASK(use_immed_capt);

  // Also, a replacement schedule-only use should be registered
  EXPECT_REGISTER_USE(use_sched_contin, f_immed_out, f_sched_out, Modify, None);

  EXPECT_RELEASE_USE(use_sched_contin);
  EXPECT_FLOW_ALIAS(f_immed_out, f_sched_out);

  EXPECT_RELEASE_USE(use_sched_capt);

  mock_runtime->registered_tasks.front()->run();
  mock_runtime->registered_tasks.pop_front();

  EXPECT_RELEASE_USE(use_immed_capt);

  // now the inner task should be on the back, so run it next
  mock_runtime->registered_tasks.back()->run();
  mock_runtime->registered_tasks.pop_back();


  if(sched_capture_read) {
    // The outer task will be on the front now
    EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR_AND_SET_BUFFER(f_sched_out, use_read_capt, value);
    EXPECT_REGISTER_TASK(use_read_capt);

    EXPECT_RELEASE_USE(use_ro_sched);

    mock_runtime->registered_tasks.front()->run();
    mock_runtime->registered_tasks.pop_front();
  }

  EXPECT_RELEASE_USE(use_read_capt);

  // and finally the read task
  mock_runtime->registered_tasks.front()->run();
  mock_runtime->registered_tasks.pop_front();
  EXPECT_TRUE(mock_runtime->registered_tasks.empty());

}

INSTANTIATE_TEST_CASE_P(
  schedule_only,
  TestScheduleOnly,
  ::testing::Bool()
);


////////////////////////////////////////////////////////////////////////////////

#if defined(DEBUG) || !defined(NDEBUG)
TEST_F(TestCreateWork, death_schedule_only) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  MockFlow finit, fnull, f_sched_out;
  use_t* use_sched_capt = nullptr;
  use_t* use_sched_cont = nullptr;
  use_t* use_init = nullptr;

  mock_runtime->save_tasks = true;

  EXPECT_INITIAL_ACCESS(finit, fnull, use_init, make_key("hello"));

  // Expect a schedule-only mod capture
  EXPECT_CALL(*mock_runtime, make_next_flow(finit))
    .WillOnce(Return(f_sched_out));
  EXPECT_REGISTER_USE(use_sched_capt, finit, f_sched_out, Modify, None);
  EXPECT_REGISTER_USE(use_sched_cont, f_sched_out, fnull, Modify, None);
  EXPECT_RELEASE_USE(use_init);
  EXPECT_REGISTER_TASK(use_sched_capt);

  EXPECT_RELEASE_USE(use_sched_cont);

  //============================================================================
  // actual code being tested (that should fail when run)
  {
    auto tmp = initial_access<int>("hello");

    create_work(schedule_only(tmp), [=] {
      EXPECT_DEATH(
        {
          tmp.set_value(42);
        },
        "set_value\\(\\) called on handle not in immediately modifiable state.*"
      );
    });

  }
  //============================================================================

  EXPECT_RELEASE_USE(use_sched_capt);

  run_all_tasks();

}
#endif

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestCreateWork, mod_capture_MN_nested_MR) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(finit, fnull, ftask_out, fforw, finner_out);
  use_t* use_task, *use_pub, *use_pub_cont, *use_inner,
    *use_init, *use_outer_cont, *use_mod_cont;
  use_task = use_pub = use_pub_cont = use_inner = nullptr;

  EXPECT_INITIAL_ACCESS(finit, fnull, use_init, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(finit, ftask_out, use_task, fnull, use_outer_cont);

  {
    InSequence rel_before_task;

    EXPECT_RELEASE_USE(use_init);

    EXPECT_REGISTER_TASK(use_task);
  }

  EXPECT_FLOW_ALIAS(ftask_out, fnull);

  EXPECT_RELEASE_USE(use_outer_cont);

  //============================================================================
  // actual code being tested
  {
    auto tmp = initial_access<int>("hello");

    create_work([=]{

      tmp.publish();

      create_work([=]{
        tmp.set_value(42);
      });

    });

  }
  //
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  // Inside of outer task:

  EXPECT_CALL(*mock_runtime, make_forwarding_flow(finit))
    .WillOnce(Return(fforw));

  EXPECT_REGISTER_USE(use_pub, fforw, fforw, None, Read);

  {
    InSequence s;

    EXPECT_REGISTER_USE(use_pub_cont, fforw, ftask_out, Modify, Read);

    EXPECT_RELEASE_USE(use_task);

    EXPECT_CALL(*mock_runtime, publish_use(Eq(ByRef(use_pub)), _));

    EXPECT_RELEASE_USE(use_pub);

  }


  int value = 0;

  {
    InSequence s;

    EXPECT_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(fforw, finner_out, use_inner, value);

    EXPECT_REGISTER_USE(use_mod_cont, finner_out, ftask_out, Modify, None);

    EXPECT_RELEASE_USE(use_pub_cont);
  }

  EXPECT_REGISTER_TASK(use_inner);

  EXPECT_FLOW_ALIAS(finner_out, ftask_out);

  EXPECT_RELEASE_USE(use_mod_cont);

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  // Inside of inner task

  EXPECT_RELEASE_USE(use_inner);

  run_one_task();

  EXPECT_THAT(value, Eq(42));

}

////////////////////////////////////////////////////////////////////////////////
