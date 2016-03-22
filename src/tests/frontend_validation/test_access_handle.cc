/*
//@HEADER
// ************************************************************************
//
//                          test_access_handle.h
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

////////////////////////////////////////////////////////////////////////////////

// Forward declarations for friend usage
class TestAccessHandle;
class TestAccessHandle_get_value_Test;
#define DARMA_TEST_FRONTEND_VALIDATION 1

////////////////////////////////////////////////////////////////////////////////

#include <iomanip>
#include <iostream>

#include <gtest/gtest.h>

#include "mock_backend.h"
#include "test_frontend.h"

#include <darma/interface/app/access_handle.h>
#include <darma/interface/app/create_work.h>
#include <darma/interface/app/initial_access.h>

////////////////////////////////////////////////////////////////////////////////

class TestAccessHandle
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

TEST_F(TestAccessHandle, get_value) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  handle_t* h0 = nullptr, *h1 = nullptr;

  {
    InSequence s_handles;

    EXPECT_CALL(*mock_runtime, register_handle(_))
      .WillOnce(SaveArg<0>(&h0));
    EXPECT_CALL(*mock_runtime, register_handle(_))
      .WillOnce(SaveArg<0>(&h1));
  }

  int data = 0;
  MockDataBlock db;
  EXPECT_CALL(db, get_data())
    .Times(AnyNumber())
    .WillRepeatedly(Return(&data));

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    handle_in_get_dependencies(h0), Not(needs_read_handle(h0)), needs_write_handle(h0)
  )))
    .WillOnce(InvokeWithoutArgs([&h0,&db]{ h0->satisfy_with_data_block(&db); }));

  {
    auto tmp = initial_access<int>("hello");

    EXPECT_THAT(h0, NotNull());

    create_work([=,&h0,&h1]{
      EXPECT_THAT(tmp.dep_handle_.get(), Eq(h0));
      EXPECT_THAT(h1, NotNull());
    });
    EXPECT_THAT(tmp.dep_handle_.get(), Eq(h1));
  }

  while(not mock_runtime->registered_tasks.empty()) {
    mock_runtime->registered_tasks.front()->run();
    mock_runtime->registered_tasks.pop_front();
  }

}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
