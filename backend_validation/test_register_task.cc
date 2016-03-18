/*
//@HEADER
// ************************************************************************
//
//                          test_register_task.cc
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
#include <gmock/gmock.h>

#ifdef TEST_BACKEND_INCLUDE
#  include TEST_BACKEND_INCLUDE
#endif

#include "mock_frontend.h"
#include "helpers.h"

using namespace darma_runtime;
using namespace mock_frontend;

namespace {

class RegisterTask
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

    virtual ~RegisterTask() noexcept { }
};

} // end anonymous namespace

////////////////////////////////////////////////////////////////////////////////

// Try three different spots for release_only_usage(): before lambda created,
//   before task registered, and after task registered

// release_read_only_usage right before the lambda is created
TEST_F(RegisterTask, first_writer_post_register_handle) {
  using namespace ::testing;

  auto h_0 = make_handle<double, true, true>(MockDependencyHandle::version_t(), "the_key");
  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  detail::backend_runtime->register_handle(h_0.get());

  detail::backend_runtime->release_read_only_usage(h_0.get());

  register_write_only_capture(h_0.get(), [&]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;

  detail::backend_runtime->release_handle(h_0.get());
}

// release_read_only_usage before the task is registered
TEST_F(RegisterTask, first_writer_block_post_lambda) {
  using namespace ::testing;

  auto h_0 = make_handle<double, true, true>(MockDependencyHandle::version_t(), "the_key");
  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  detail::backend_runtime->register_handle(h_0.get());

  typedef typename std::conditional<true, ::testing::NiceMock<MockTask>, MockTask>::type task_t;
  auto task_a = std::make_unique<task_t>();
  EXPECT_CALL(*task_a, get_dependencies())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(MockTask::handle_container_t{ h_0.get() }));
  EXPECT_CALL(*task_a, needs_read_data(Eq(h_0.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(false));
  EXPECT_CALL(*task_a, needs_write_data(Eq(h_0.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*task_a, run())
    .Times(Exactly(1))
    .WillOnce(Invoke(
      [&]{
        ASSERT_TRUE(h_0->is_satisfied());
        ASSERT_TRUE(h_0->is_writable());
        abstract::backend::DataBlock* data_block = h_0->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
      }
    ));

  detail::backend_runtime->release_read_only_usage(h_0.get());

  detail::backend_runtime->register_task(std::move(task_a));

  detail::backend_runtime->finalize();
  backend_finalized = true;

  detail::backend_runtime->release_handle(h_0.get());
}

// release_read_only_usage after the task is registered
TEST_F(RegisterTask, first_writer_post_register_task) {
  using namespace ::testing;

  auto h_0 = make_handle<double, true, true>(MockDependencyHandle::version_t(), "the_key");
  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  detail::backend_runtime->register_handle(h_0.get());

  register_write_only_capture(h_0.get(), [&]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
  });

  detail::backend_runtime->release_read_only_usage(h_0.get());

  detail::backend_runtime->finalize();
  backend_finalized = true;

  detail::backend_runtime->release_handle(h_0.get());
}

////////////////////////////////////////////////////////////////////////////////

// register a task with no dependencies or anti-dependencies
TEST_F(RegisterTask, register_task_with_no_deps) {
  using namespace ::testing;
  typedef typename std::conditional<false, ::testing::NiceMock<MockTask>, MockTask>::type task_t;

  auto task_a = std::make_unique<task_t>();
  EXPECT_CALL(*task_a, get_dependencies())
    .Times(AtLeast(1));
  EXPECT_CALL(*task_a, run())
    .Times(Exactly(1));

  detail::backend_runtime->register_task(std::move(task_a));

  detail::backend_runtime->finalize();
  backend_finalized = true;
}

////////////////////////////////////////////////////////////////////////////////

// register a write_only task and then a read-only task on its subsequent
// to make sure its subsequent gets satisfied as expected
TEST_F(RegisterTask, release_satisfy_for_read_only) {
  using namespace ::testing;

  auto h_0 = make_handle<int, true, true>(MockDependencyHandle::version_t(), "the_key");
  auto next_version = h_0->get_version();
  ++next_version;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(2));  // when running write-only task and when releasing

  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));  // when running read-only task
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(Exactly(0));

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_1.get());

  detail::backend_runtime->release_read_only_usage(h_0.get());

  int value = 42;

  register_write_only_capture(h_0.get(), [&,value]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    memcpy(data, &value, sizeof(int));
    detail::backend_runtime->release_handle(h_0.get());
  });

  register_read_only_capture(h_1.get(), [&,value]{
    ASSERT_TRUE(h_1->is_satisfied());
    ASSERT_FALSE(h_1->is_writable());
    abstract::backend::DataBlock* data_block = h_1->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    ASSERT_THAT(*((int*)data), Eq(value));
    detail::backend_runtime->release_read_only_usage(h_1.get());
    detail::backend_runtime->release_handle(h_1.get());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;
}

// register a write_only task and then a read-write task on its subsequent
// to make sure its subsequent gets satisfied as expected
TEST_F(RegisterTask, release_satisfy_for_read_write) {
  using namespace ::testing;

  auto h_0 = make_handle<int, true, true>(MockDependencyHandle::version_t(), "the_key");
  auto next_version = h_0->get_version();
  ++next_version;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(2));  // when running write-only task and when releasing

  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));  // when running read-write task
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(Exactly(1));

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_1.get());

  detail::backend_runtime->release_read_only_usage(h_0.get());

  int value = 42;

  register_write_only_capture(h_0.get(), [&,value]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    memcpy(data, &value, sizeof(int));
    detail::backend_runtime->release_handle(h_0.get());
  });

  detail::backend_runtime->release_read_only_usage(h_1.get());

  register_read_write_capture(h_1.get(), [&,value]{
    ASSERT_TRUE(h_1->is_satisfied());
    ASSERT_TRUE(h_1->is_writable());
    abstract::backend::DataBlock* data_block = h_1->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    ASSERT_THAT(*((int*)data), Eq(value));
    detail::backend_runtime->release_handle(h_1.get());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;
}

////////////////////////////////////////////////////////////////////////////////

// write-only with multiple outputs
TEST_F(RegisterTask, first_writer_2outputs) {
  using namespace ::testing;

  auto h_0 = make_handle<double, true, true>(MockDependencyHandle::version_t(), "the_key");
  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  auto h_1 = make_handle<double, true, true>(MockDependencyHandle::version_t(), "other_key");
  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_1.get());

  typedef typename std::conditional<false, ::testing::NiceMock<MockTask>, MockTask>::type task_t;

  auto task_a = std::make_unique<task_t>();
  EXPECT_CALL(*task_a, get_dependencies())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(MockTask::handle_container_t{ h_0.get(), h_1.get() }));
  EXPECT_CALL(*task_a, needs_read_data(Eq(h_0.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(false));
  EXPECT_CALL(*task_a, needs_read_data(Eq(h_1.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(false));
  EXPECT_CALL(*task_a, needs_write_data(Eq(h_0.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*task_a, needs_write_data(Eq(h_1.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*task_a, run())
    .Times(Exactly(1))
    .WillOnce(Invoke([&]{
      ASSERT_TRUE(h_0->is_satisfied());
      ASSERT_TRUE(h_1->is_satisfied());
      ASSERT_TRUE(h_0->is_writable());
      ASSERT_TRUE(h_1->is_writable());
      {
        abstract::backend::DataBlock* data_block = h_0->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
      }
      {
        abstract::backend::DataBlock* data_block = h_1->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
      }
    }));

  detail::backend_runtime->register_task(std::move(task_a));

  detail::backend_runtime->release_read_only_usage(h_0.get());
  detail::backend_runtime->release_read_only_usage(h_1.get());

  detail::backend_runtime->finalize();
  backend_finalized = true;

  detail::backend_runtime->release_handle(h_0.get());
  detail::backend_runtime->release_handle(h_1.get());
}

// read-only task with multiple dependencies
TEST_F(RegisterTask, multiple_deps_rr) {
  using namespace ::testing;

  auto h_0 = make_handle<double, true, true>(MockDependencyHandle::version_t(), "the_key");
  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  auto next_version0 = h_0->get_version();
  ++next_version0;
  auto h_0s = make_handle<int, true, false>(next_version0, "the_key");
  EXPECT_CALL(*h_0s.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0s.get(), get_data_block())
    .Times(AtLeast(1));  // when running read-only task
  EXPECT_CALL(*h_0s.get(), allow_writes())
    .Times(Exactly(0));

  auto h_1 = make_handle<double, true, true>(MockDependencyHandle::version_t(), "other_key");
  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  auto next_version1 = h_1->get_version();
  ++next_version1;
  auto h_1s = make_handle<int, true, false>(next_version1, "other_key");
  EXPECT_CALL(*h_1s.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1s.get(), get_data_block())
    .Times(AtLeast(1));  // when running read-only task
  EXPECT_CALL(*h_1s.get(), allow_writes())
    .Times(Exactly(0));

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_0s.get());
  detail::backend_runtime->register_handle(h_1.get());
  detail::backend_runtime->register_handle(h_1s.get());

  int value = 42;

  typedef typename std::conditional<false, ::testing::NiceMock<MockTask>, MockTask>::type task_t;

  detail::backend_runtime->release_read_only_usage(h_0.get());
  detail::backend_runtime->release_read_only_usage(h_1.get());

  auto task_wo = std::make_unique<task_t>();
  EXPECT_CALL(*task_wo, get_dependencies())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(MockTask::handle_container_t{ h_0.get(), h_1.get() }));
  EXPECT_CALL(*task_wo, needs_read_data(Eq(h_0.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(false));
  EXPECT_CALL(*task_wo, needs_read_data(Eq(h_1.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(false));
  EXPECT_CALL(*task_wo, needs_write_data(Eq(h_0.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*task_wo, needs_write_data(Eq(h_1.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*task_wo, run())
    .Times(Exactly(1))
    .WillOnce(Invoke([&]{
      ASSERT_TRUE(h_0->is_satisfied());
      ASSERT_TRUE(h_1->is_satisfied());
      ASSERT_TRUE(h_0->is_writable());
      ASSERT_TRUE(h_1->is_writable());
      {
        abstract::backend::DataBlock* data_block = h_0->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        memcpy(data, &value, sizeof(int));
      }
      {
        abstract::backend::DataBlock* data_block = h_1->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        int value2 = value*2;
        memcpy(data, &value2, sizeof(int));
      }
      detail::backend_runtime->release_handle(h_0.get());
      detail::backend_runtime->release_handle(h_1.get());
    }));

  detail::backend_runtime->register_task(std::move(task_wo));

  auto task_rr = std::make_unique<task_t>();
  EXPECT_CALL(*task_rr, get_dependencies())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(MockTask::handle_container_t{ h_0s.get(), h_1s.get() }));
  EXPECT_CALL(*task_rr, needs_read_data(Eq(h_0s.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*task_rr, needs_read_data(Eq(h_1s.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*task_rr, needs_write_data(Eq(h_0s.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(false));
  EXPECT_CALL(*task_rr, needs_write_data(Eq(h_1s.get())))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(false));
  EXPECT_CALL(*task_rr, run())
    .Times(Exactly(1))
    .WillOnce(Invoke([&]{
      ASSERT_TRUE(h_0s->is_satisfied());
      ASSERT_TRUE(h_1s->is_satisfied());
      ASSERT_FALSE(h_0s->is_writable());
      ASSERT_FALSE(h_1s->is_writable());
      {
        abstract::backend::DataBlock* data_block = h_0s->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        ASSERT_THAT(*((int*)data), Eq(value));
      }
      {
        abstract::backend::DataBlock* data_block = h_1s->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        ASSERT_THAT(*((int*)data), Eq(value*2));
      }
      detail::backend_runtime->release_read_only_usage(h_0s.get());
      detail::backend_runtime->release_read_only_usage(h_1s.get());
      detail::backend_runtime->release_handle(h_0s.get());
      detail::backend_runtime->release_handle(h_1s.get());
    }));

  detail::backend_runtime->register_task(std::move(task_rr));

  detail::backend_runtime->finalize();
  backend_finalized = true;
}
