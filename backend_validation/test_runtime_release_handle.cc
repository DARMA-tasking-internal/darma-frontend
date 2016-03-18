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
          std::make_unique<::testing::NiceMock<mock_frontend::MockTask>>();

      abstract::backend::darma_backend_initialize(
        argc_, argv_, detail::backend_runtime,
        std::move(top_level_task)
      );
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

    virtual ~RuntimeRelease() { }
};

} // end anonymous namespace

TEST_F(RuntimeRelease, satisfy_next) {
  using namespace mock_frontend;
  using namespace ::testing;

  auto ser_man = std::make_shared<MockSerializationManager>();
  ser_man->get_metadata_size_return = sizeof(double);

  EXPECT_CALL(*ser_man, get_metadata_size(_))
    .Times(AtLeast(1));

  auto handle_a = std::make_shared<NiceMock<MockDependencyHandle>>();
  handle_a->get_key_return = detail::key_traits<MockDependencyHandle::key_t>::maker()("the_key");
  handle_a->get_version_return = MockDependencyHandle::version_t();
  handle_a->get_serialization_manager_return = ser_man.get();

  EXPECT_CALL(*handle_a, satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*handle_a, get_data_block())
    .Times(AtLeast(2));
  EXPECT_CALL(*handle_a, get_key())
    .Times(AtLeast(1));
  EXPECT_CALL(*handle_a, get_version())
    .Times(AtLeast(1));
  EXPECT_CALL(*handle_a, get_serialization_manager())
    .Times(AtLeast(1));
  EXPECT_CALL(*handle_a, allow_writes())
    .Times(Exactly(1));

  auto handle_b = std::make_shared<NiceMock<MockDependencyHandle>>();
  handle_b->get_key_return = detail::key_traits<MockDependencyHandle::key_t>::maker()("the_key");
  handle_b->get_version_return = ++MockDependencyHandle::version_t();
  handle_b->get_serialization_manager_return = ser_man.get();

  EXPECT_CALL(*handle_b, satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*handle_b, get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*handle_b, get_key())
    .Times(AtLeast(1));
  EXPECT_CALL(*handle_b, get_version())
    .Times(AtLeast(1));

  detail::backend_runtime->register_handle(handle_a.get());
  detail::backend_runtime->register_handle(handle_b.get());
  detail::backend_runtime->release_read_only_usage(handle_a.get());

  double test_value = 3.14;

  auto task_a = std::make_unique<NiceMock<MockTask>>();
  ON_CALL(*task_a, get_dependencies())
    .WillByDefault(ReturnRefOfCopy(MockTask::handle_container_t{ handle_a.get() }));
  ON_CALL(*task_a, needs_write_data(_))
    .WillByDefault(Return(true));
  ON_CALL(*task_a, run())
    .WillByDefault(Invoke([&](){
      abstract::backend::DataBlock* data_block = handle_a->get_data_block();
      ASSERT_NE(data_block, nullptr);
      void* data = data_block->get_data();
      ASSERT_NE(data, nullptr);
      memcpy(data, (void*)&test_value, sizeof(double));
      detail::backend_runtime->release_handle(handle_a.get());
    }));

  EXPECT_CALL(*task_a, run())
    .Times(Exactly(1));

  detail::backend_runtime->register_task(std::move(task_a));

  auto task_b = std::make_unique<NiceMock<MockTask>>();
  ON_CALL(*task_b, get_dependencies())
    .WillByDefault(ReturnRefOfCopy(MockTask::handle_container_t{ handle_b.get() }));
  ON_CALL(*task_b, needs_read_data(_))
    .WillByDefault(Return(true));
  ON_CALL(*task_b, run())
    .WillByDefault(Invoke([&](){
      abstract::backend::DataBlock* data_block = handle_b->get_data_block();
      ASSERT_NE(data_block, nullptr);
      void* data = data_block->get_data();
      ASSERT_NE(data, nullptr);
      ASSERT_THAT(*((double*)data), DoubleEq(test_value));
      detail::backend_runtime->release_read_only_usage(handle_b.get());
      detail::backend_runtime->release_handle(handle_b.get());
    }));

  EXPECT_CALL(*task_b, run())
    .Times(Exactly(1));

  detail::backend_runtime->register_task(std::move(task_b));

  detail::backend_runtime->finalize();
  backend_finalized = true;
}
