/*
//@HEADER
// ************************************************************************
//
//                          test_create_work.cc
//                         dharma_new
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

class TestCreateWork_mod_capture_MM_Test;
#define DARMA_TEST_FRONTEND_VALIDATION_CREATE_WORK 1

#include "mock_backend.h"
#include "test_frontend.h"
#include "test_make_key_functor.h"

#include <darma/interface/app/initial_access.h>
#include <darma/interface/app/read_access.h>
#include <darma/interface/app/create_work.h>

////////////////////////////////////////////////////////////////////////////////

class TestMakeKeyFunctor
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

TEST_F(TestMakeKeyFunctor, test_make_key_functor_int) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(f_initial, f_null, f_task_out);
  use_t* task_use = nullptr;
  use_t* use_initial = nullptr;
  use_t* use_cont = nullptr;

  EXPECT_INITIAL_ACCESS(f_initial, f_null, use_initial, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(f_initial, f_task_out, task_use, f_null, use_cont);

  int value;

  {
    InSequence rel_before_reg;

    EXPECT_RELEASE_USE(use_initial);

    EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(task_use))))
    .WillOnce(Invoke([&](auto* task) {
      for(auto&& dep : task->get_dependencies()) {
        dep->get_data_pointer_reference() = (void*)(&value);
      }
    }));

    EXPECT_FLOW_ALIAS(f_task_out, f_null);

    EXPECT_RELEASE_USE(use_cont);
  }

  //============================================================================
  // Actual code being tested
  {
    auto tmp = initial_access<int>("hello");

    create_work([=]{

      tmp.set_value(10);
      auto created_key = darma_runtime::check_test_make_key_functor(version(1, tmp));

      ASSERT_EQ(created_key.component<0>().as<int>(), 1);
      ASSERT_EQ(created_key.component<1>().as<int>(), 10);

    });

  } // tmp deleted
  //============================================================================

  EXPECT_RELEASE_USE(task_use);

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestMakeKeyFunctor, test_make_key_functor_string) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(f_initial, f_null, f_task_out);
  use_t* task_use = nullptr;
  use_t* use_initial = nullptr;
  use_t* use_cont = nullptr;

  EXPECT_INITIAL_ACCESS(f_initial, f_null, use_initial, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(f_initial, f_task_out, task_use, f_null, use_cont);

  std::string value;

  {
    InSequence rel_before_reg;

    EXPECT_RELEASE_USE(use_initial);

    EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(task_use))))
    .WillOnce(Invoke([&](auto* task) {
      for(auto&& dep : task->get_dependencies()) {
        dep->get_data_pointer_reference() = (void*)(&value);
      }
    }));

    EXPECT_FLOW_ALIAS(f_task_out, f_null);

    EXPECT_RELEASE_USE(use_cont);
  }

  //============================================================================
  // Actual code being tested
  {
    auto tmp = initial_access<std::string>("hello");

    create_work([=]{

      tmp.set_value("Hello DARMA user");
      auto created_key = darma_runtime::check_test_make_key_functor(version(1, tmp));

      ASSERT_EQ(created_key.component<0>().as<int>(), 1);
      ASSERT_EQ(created_key.component<1>().as<std::string>(), "Hello DARMA user");

    });

  } // tmp deleted
  //============================================================================

  EXPECT_RELEASE_USE(task_use);

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

#if defined(DEBUG) || !defined(NDEBUG)
TEST_F(TestMakeKeyFunctor, test_key_functor_death) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(f_initial, f_null, f_task_out);
  use_t* task_use = nullptr;
  use_t* use_initial = nullptr;
  use_t* use_cont = nullptr;

  EXPECT_INITIAL_ACCESS(f_initial, f_null, use_initial, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(f_initial, f_task_out, task_use, f_null, use_cont);

  std::string value;

  {
    InSequence rel_before_reg;

    EXPECT_RELEASE_USE(use_initial);

    EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(task_use))))
    .WillOnce(Invoke([&](auto* task) {
      for(auto&& dep : task->get_dependencies()) {
        dep->get_data_pointer_reference() = (void*)(&value);
      }
    }));

    EXPECT_FLOW_ALIAS(f_task_out, f_null);

    EXPECT_RELEASE_USE(use_cont);
  }

  //============================================================================
  // Actual code being tested
  {
    auto tmp = initial_access<std::string>("hello");

    create_work([=]{

      tmp.set_value("Hello");
      auto created_key = darma_runtime::check_test_make_key_functor(version(1, tmp));

      ASSERT_EQ(created_key.component<0>().as<int>(), 1);
      ASSERT_EQ(created_key.component<1>().as<std::string>(), "Hello");

    });

    EXPECT_DEATH(
      {
        auto created_key = darma_runtime::check_test_make_key_functor(version(1,tmp));
      },
      "`get_value\\(\\)` performed on AccessHandle"
    );

  } // tmp deleted
  //============================================================================

  EXPECT_RELEASE_USE(task_use);

  run_all_tasks();

}
#endif
