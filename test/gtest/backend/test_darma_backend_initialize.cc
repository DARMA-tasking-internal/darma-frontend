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

#include <darma/darma.h>

#include "mock_frontend.h"

namespace {

class MockFrontendTest
  : public ::testing::Test
{
  protected:

    virtual void SetUp() {
      // Emulate argc and argv
      argc_ = 1;
      argv_ = new char*[1];
      argv_[0] = new char[256];
      sprintf(argv_[0], "<mock frontend test>");
    }

    virtual void TearDown() {
      delete[] argv_[0];
      delete[] argv_;
    }

    int argc_;
    char** argv_;
    std::string program_name;

    virtual ~MockFrontendTest() { }
};

} // end anonymous namespace

TEST_F(MockFrontendTest, initialize_rank_size) {
  using namespace darma_runtime;
  std::unique_ptr<typename abstract::backend::runtime_t::task_t> top_level_task =
      std::make_unique<mock_frontend::MockTask>();
  abstract::backend::darma_backend_initialize(
    argc_, argv_, detail::backend_runtime,
    std::move(top_level_task)
  );
  typename abstract::backend::runtime_t::task_t* top_level_task_ptr =
      detail::backend_runtime->get_running_task();
  ASSERT_EQ(
    DARMA_BACKEND_SPMD_NAME_PREFIX,
    top_level_task_ptr->get_name().component<0>().as<std::string>()
  );
  // Rank must be less than size
  ASSERT_LT(
    top_level_task_ptr->get_name().component<1>().as<size_t>(),
    top_level_task_ptr->get_name().component<2>().as<size_t>()
  );
  darma_runtime::detail::backend_runtime->finalize();
}

TEST_F(MockFrontendTest, top_level_run_not_called) {
  using namespace darma_runtime;
  using namespace mock_frontend;
  auto tmp = std::make_unique<mock_frontend::MockTask>();
  std::function<void(const MockTask*)> replace_run = [&](auto* _) {
    FAIL() << "run() shouldn't be called on top level task";
  };
  tmp->replace_run = &replace_run;
  std::unique_ptr<typename abstract::backend::runtime_t::task_t> top_level_task
    = std::move(tmp);
  abstract::backend::darma_backend_initialize(
    argc_, argv_, detail::backend_runtime,
    std::move(top_level_task)
  );
  darma_runtime::detail::backend_runtime->finalize();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
