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
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  Sequence s_hm2;
  auto hm2 = make_same_handle_matcher();
  expect_handle_life_cycle(hm2, s_hm2);
  Sequence s_hm1;
  auto hm1 = make_same_handle_matcher();
  expect_handle_life_cycle(hm1, s_hm1);

  {
    InSequence s;
    EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
      in_get_dependencies(hm1), needs_write_of(hm1), Not(needs_read_of(hm1))
    )));
  }

  // TODO finish this!!!

  {
    auto tmp = initial_access<int>("hello");
    create_work([=,&hm1]{
      ASSERT_THAT(tmp.dep_handle_.get(), Eq(hm1.handle));
    });
    ASSERT_THAT(tmp.dep_handle_.get(), Eq(hm2.handle));
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
