/*
//@HEADER
// ************************************************************************
//
//                          test_runtime_release_handle.cc
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

#ifdef TEST_BACKEND_INCLUDE
#  include TEST_BACKEND_INCLUDE
#endif

#include "mock_frontend.h"

using namespace darma_runtime;

namespace {

class RuntimeRelease
  : public ::testing::Test
{
  protected:

    virtual void SetUp() {
      // Emulate argc and argv
      argc_ = 1;
      argv_ = new char*[1];
      argv_[0] = new char[256];
      sprintf(argv_[0], "<mock frontend test>");
      // Make a mock task pointer
      std::unique_ptr<typename abstract::backend::runtime_t::task_t> top_level_task =
          std::make_unique<mock_frontend::FakeTask>();

      abstract::backend::darma_backend_initialize(
        argc_, argv_, detail::backend_runtime,
        std::move(top_level_task)
      );
      backend_finalized = false;
    }

    virtual void TearDown() {
      if(not backend_finalized) {
        // Clean up from failed tests
        detail::backend_runtime->finalize();
      }
      delete[] argv_[0];
      delete[] argv_;
    }

    int argc_;
    char** argv_;
    std::string program_name;

    bool backend_finalized;

    virtual ~RuntimeRelease() { }
};

} // end anonymous namespace

TEST_F(RuntimeRelease, satisfy_next) {
  using namespace mock_frontend;

  auto ser_man = std::make_shared<MockSerializationManager>();

  auto handle_a = std::make_shared<FakeDependencyHandle>();
  handle_a->get_key_return = detail::key_traits<FakeDependencyHandle::key_t>::maker()("the_key");
  handle_a->get_version_return = FakeDependencyHandle::version_t();
  handle_a->get_serialization_manager_return = ser_man.get();

  auto handle_b = std::make_shared<FakeDependencyHandle>();
  handle_b->get_key_return = detail::key_traits<FakeDependencyHandle::key_t>::maker()("the_key");
  handle_b->get_version_return = ++FakeDependencyHandle::version_t();
  handle_b->get_serialization_manager_return = ser_man.get();

  detail::backend_runtime->register_handle(handle_a.get());
  detail::backend_runtime->register_handle(handle_b.get());
  detail::backend_runtime->release_read_only_usage(handle_a.get());

  double test_value = 3.14;

  auto task_a = std::make_unique<FakeTask>();
  task_a->get_dependencies_return = FakeTask::handle_container_t();
  task_a->get_dependencies_return.insert(handle_a.get());
  task_a->needs_read_data_return = false;
  task_a->needs_write_data_return = true;
  std::function<void(const FakeTask*)> task_a_run = [&](auto* _){
    abstract::backend::DataBlock* data_block = handle_a->get_data_block();
    ASSERT_NE(data_block, nullptr);
    void* data = data_block->get_data();
    ASSERT_NE(data, nullptr);
    memcpy(data, (void*)&test_value, sizeof(double));
    detail::backend_runtime->release_handle(handle_a.get());
  };
  task_a->replace_run = &task_a_run;

  detail::backend_runtime->register_task(std::move(task_a));

  auto task_b = std::make_unique<FakeTask>();
  task_b->get_dependencies_return = FakeTask::handle_container_t();
  task_b->get_dependencies_return.insert(handle_b.get());
  task_b->needs_read_data_return = true;
  task_b->needs_write_data_return = false;
  std::function<void(const FakeTask*)> task_b_run = [&](auto* _){
    abstract::backend::DataBlock* data_block = handle_b->get_data_block();

    FakeDependencyHandle* handle_b_ptr = handle_b.get();

    ASSERT_EQ(*(double*)(handle_b_ptr->get_data_block()->get_data()), test_value);

    detail::backend_runtime->release_read_only_usage(handle_b.get());
    detail::backend_runtime->release_handle(handle_b.get());
  };
  task_b->replace_run = &task_b_run;

  detail::backend_runtime->register_task(std::move(task_a));

  detail::backend_runtime->finalize();
  backend_finalized = true;
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
