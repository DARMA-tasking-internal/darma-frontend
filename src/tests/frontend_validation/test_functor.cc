/*
//@HEADER
// ************************************************************************
//
//                        test_functor
//                           DARMA
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

#include <darma/interface/app/access_handle.h>
#include <src/include/darma/interface/app/create_work.h>
#include <src/include/darma/interface/app/initial_access.h>

using namespace darma_runtime;

////////////////////////////////////////////////////////////////////////////////

class TestFunctor
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

struct SimpleFunctor {
  void
  operator()(
    int arg,
    AccessHandle<int> handle
  ) const {
    std::cout << "Hello World" << std::endl;
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimplerFunctor {
  void
  operator()() const {
    std::cout << "Hello World" << std::endl;
  }
};

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simple) {
  using namespace ::testing;
  //testing::internal::CaptureStdout();

  mock_runtime->save_tasks = true;

  handle_t* h0, *h1;
  h0 = h1 = nullptr;

  InSequence s;

  EXPECT_CALL(*mock_runtime, register_handle(_))
    .Times(2)
    .WillOnce(SaveArg<0>(&h0))
    .WillOnce(SaveArg<0>(&h1));

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    handle_in_get_dependencies(h0), needs_write_handle(h0), Not(needs_read_handle(h0))
  )));

  EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h1))));

  EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h0))));

  {
    auto tmp = initial_access<int>("hello");
    EXPECT_VERSION_EQ(tmp, {0});
    create_work<SimpleFunctor>(15, tmp);
    EXPECT_VERSION_EQ(tmp, {1});
  }

  run_all_tasks();

  //ASSERT_EQ(testing::internal::GetCapturedStdout(),
  //  "Hello World\n"
  //);
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simpler) {
  using namespace ::testing;
  testing::internal::CaptureStdout();

  mock_runtime->save_tasks = true;

  create_work<SimplerFunctor>();

  run_all_tasks();

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "Hello World\n"
  );
}
