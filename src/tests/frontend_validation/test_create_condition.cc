/*
//@HEADER
// ************************************************************************
//
//                      test_create_condition.cc
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

#include "mock_backend.h"
#include "test_frontend.h"

#include <darma/interface/app/initial_access.h>
#include <darma/interface/app/read_access.h>
#include <darma/interface/app/create_condition.h>

////////////////////////////////////////////////////////////////////////////////

class TestCreateCondition
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

TEST_F(TestCreateCondition, ro_capture_MN) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow fl_init, fl_null;
  use_t* task_use;

  int value = 42;

  EXPECT_INITIAL_ACCESS(fl_init, fl_null, make_key("hello"));

  EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR(fl_init, task_use);

  EXPECT_CALL(*mock_runtime, register_condition_task_gmock_proxy(
    UseInGetDependencies(ByRef(task_use))
  )).Times(1).WillOnce(Invoke([&](auto* cond_task) {
    task_use->get_data_pointer_reference() = (void*)&value;
    return cond_task->template run<bool>();
  }));

  EXPECT_CALL(*mock_runtime, release_use(Eq(ByRef(task_use))));

  //============================================================================
  // Actual code being tested
  {
    auto tmp = initial_access<int>("hello");

    if(not create_condition([=]{
      return tmp.get_value() == 42;
    })) {
      FAIL() << "create_condition should have returned true";
    }

  }
  //============================================================================

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateCondition, explicit_ro_capture_MN) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow fl_init, fl_null;
  use_t* task_use;

  int value = 42;

  EXPECT_INITIAL_ACCESS(fl_init, fl_null, make_key("hello"));

  EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR(fl_init, task_use);

  EXPECT_CALL(*mock_runtime, register_condition_task_gmock_proxy(
    UseInGetDependencies(ByRef(task_use))
  )).Times(1).WillOnce(Invoke([&](auto* cond_task) {
    task_use->get_data_pointer_reference() = (void*)&value;
    return cond_task->template run<bool>();
  }));

  EXPECT_CALL(*mock_runtime, release_use(Eq(ByRef(task_use))));

  //============================================================================
  // Actual code being tested
  {
    auto tmp = initial_access<int>("hello");

    if(not create_condition(reads(tmp), [=]{ return tmp.get_value() == 42; })) {
      FAIL() << "create_condition should have returned true";
    }
  }
  //============================================================================

  mock_runtime->registered_tasks.clear();
}
