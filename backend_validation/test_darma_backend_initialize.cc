/*
//@HEADER
// ************************************************************************
//
//                          test_darma_backend_initialize.cc
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

#include <string>

#include <gtest/gtest.h>

#ifdef TEST_BACKEND_INCLUDE
#  include TEST_BACKEND_INCLUDE
#endif

#include "mock_frontend.h"
#include "main.h"

using namespace darma_runtime;

namespace {

class DARMABackendInitialize
  : public ::testing::Test
{
  protected:

    virtual void SetUp() {
      // Emulate argc and argv
      argc_ = 1;
      argv_ = new char*[1];
      argv_[0] = new char[256];
      sprintf(argv_[0], "<mock frontend test>");
      backend_finalized = false;
    }

    virtual void TearDown() {
      if(!backend_finalized) {
        // Clean up from failed tests
        detail::backend_runtime->finalize();
      }
      delete detail::backend_runtime;
      detail::backend_runtime = 0;
      delete[] argv_[0];
      delete[] argv_;
    }

    int argc_;
    char** argv_;
    std::string program_name;

    bool backend_finalized;

    virtual ~DARMABackendInitialize() { }
};

} // end anonymous namespace

// check that the top-level task's name is set correctly
TEST_F(DARMABackendInitialize, rank_size) {
  using namespace mock_frontend;
  using namespace ::testing;
  // Make a mock task pointer
  std::unique_ptr<NiceMock<MockTask>> top_level_task =
      std::make_unique<NiceMock<MockTask>>();

  // runtime must call set_name
  EXPECT_CALL(*top_level_task, set_name(_))
    .Times(Exactly(1));

  abstract::backend::darma_backend_initialize(
    argc_, argv_, detail::backend_runtime,
    std::move(top_level_task)
  );
  // Get the return of get_running_task()
  typename abstract::backend::runtime_t::task_t* top_level_task_ptr =
      detail::backend_runtime->get_running_task();
  auto name = top_level_task_ptr->get_name();

  // TODO: check that the name has the right number of parts

  // Test that the first component must equal the specified prefix
  ASSERT_EQ(
    DARMA_BACKEND_SPMD_NAME_PREFIX,
    name.component<0>().as<std::string>()
  );
  // Rank must be less than size
  ASSERT_LT(
    name.component<1>().as<size_t>(),
    name.component<2>().as<size_t>()
  );
  detail::backend_runtime->finalize();
  backend_finalized = true;
}

TEST_F(DARMABackendInitialize, top_level_run_not_called) {
  using namespace mock_frontend;
  using namespace ::testing;
  std::unique_ptr<NiceMock<MockTask>> top_level_task =
      std::make_unique<NiceMock<MockTask>>();

  // runtime must call set_name
  EXPECT_CALL(*top_level_task, set_name(_))
    .Times(Exactly(1));
  // run can never be called on a top-level task
  EXPECT_CALL(*top_level_task, run())
    .Times(Exactly(0));

  abstract::backend::darma_backend_initialize(
    argc_, argv_, detail::backend_runtime,
    std::move(top_level_task)
  );
  detail::backend_runtime->finalize();
  backend_finalized = true;
}
