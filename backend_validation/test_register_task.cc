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
          std::make_unique<mock_frontend::FakeTask>();

      abstract::backend::darma_backend_initialize(
        argc_, argv_, detail::backend_runtime,
        std::move(top_level_task)
      );

      // set up some handles to be used for testing
    }

    virtual void TearDown() {
      if(not backend_finalized) {
        // Clean up from failed tests
        detail::backend_runtime->finalize();
      }
      // Delete the instance of backend_runtime (?!?)
      delete detail::backend_runtime;
      detail::backend_runtime = nullptr;
      delete[] argv_[0];
      delete[] argv_;
    }

    int argc_;
    char** argv_;
    std::string program_name;
    bool backend_finalized;

    std::shared_ptr<FakeDependencyHandle> handle_a_0;
    std::shared_ptr<MockDependencyHandle> ghandle_a_0;

    std::shared_ptr<FakeDependencyHandle> handle_a_1;

    virtual ~RegisterTask() noexcept { }
};

} // end anonymous namespace

////////////////////////////////////////////////////////////////////////////////
// some helper functions

template <typename MockDep, typename Lambda, bool needs_read, bool needs_write, bool IsNice=false>
void register_one_dep_capture(MockDep* captured, Lambda&& lambda) {
  using namespace ::testing;
  typedef typename std::conditional<IsNice, ::testing::NiceMock<MockTask>, MockTask>::type task_t;

  auto task_a = std::make_unique<task_t>();
  if(IsNice) task_a->delegate_to_fake();
  EXPECT_CALL(*task_a, get_dependencies())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(FakeTask::handle_container_t{ captured }));
  EXPECT_CALL(*task_a, needs_read_data(Eq(captured)))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(needs_read));
  EXPECT_CALL(*task_a, needs_write_data(Eq(captured)))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(needs_write));
  EXPECT_CALL(*task_a, run())
    .Times(Exactly(1))
    .WillOnce(Invoke(std::forward<Lambda>(lambda)));
  detail::backend_runtime->register_task(std::move(task_a));
}

template <typename MockDep, typename Lambda, bool IsNice=false>
void register_read_only_capture(MockDep* captured, Lambda&& lambda) {
  register_one_dep_capture<MockDep, Lambda, true, false, IsNice>(captured, std::forward<Lambda>(lambda));
}

template <typename MockDep, typename Lambda, bool IsNice=false>
void register_write_only_capture(MockDep* captured, Lambda&& lambda) {
  register_one_dep_capture<MockDep, Lambda, false, true, IsNice>(captured, std::forward<Lambda>(lambda));
}

template <typename MockDep, typename Lambda, bool IsNice=false>
void register_read_write_capture(MockDep* captured, Lambda&& lambda) {
  register_one_dep_capture<MockDep, Lambda, true, true, IsNice>(captured, std::forward<Lambda>(lambda));
}

template <typename T, bool IsNice, bool ExpectNewAlloc, typename Version, typename... KeyParts>
std::shared_ptr<typename std::conditional<IsNice, ::testing::NiceMock<MockDependencyHandle>, MockDependencyHandle>::type>
make_handle(
  Version v, KeyParts&&... kp
) {
  using namespace ::testing;
  typedef typename std::conditional<IsNice, ::testing::NiceMock<MockDependencyHandle>, MockDependencyHandle>::type handle_t;

  // Deleted in accompanying MockDependencyHandle shared pointer deleter
  auto ser_man = new MockSerializationManager();
  if (ExpectNewAlloc){
    EXPECT_CALL(*ser_man, get_metadata_size(IsNull()))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(sizeof(T)));
  }

  auto h_0 = std::shared_ptr<handle_t>(
    new handle_t(),
    [=](handle_t* to_delete){
      delete to_delete;
      delete ser_man;
    }
  );
  if(IsNice) h_0->delegate_to_fake();
  EXPECT_CALL(*h_0, get_key())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(detail::key_traits<FakeDependencyHandle::key_t>::maker()(std::forward<KeyParts>(kp)...)));
  EXPECT_CALL(*h_0, get_version())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(v));
  if (ExpectNewAlloc){
    EXPECT_CALL(*h_0, get_serialization_manager())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ser_man));
  }

  return h_0;
}

//
////////////////////////////////////////////////////////////////////////////////

// Try three different spots for release_only_usage(): before lambda created,
//   before task registered, and after task registered
TEST_F(RegisterTask, allocate_data_block) {
  using namespace ::testing;

  auto ser_man = std::make_shared<MockSerializationManager>();
  EXPECT_CALL(*ser_man, get_metadata_size(IsNull()))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(sizeof(double)));

  auto h_0 = std::make_shared<NiceMock<MockDependencyHandle>>();
  h_0->delegate_to_fake();
  EXPECT_CALL(*h_0, get_key())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(detail::key_traits<FakeDependencyHandle::key_t>::maker()("the_key")));
  EXPECT_CALL(*h_0, get_version())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(FakeDependencyHandle::version_t()));
  EXPECT_CALL(*h_0, get_serialization_manager())
    .Times(AtLeast(1))
    .WillRepeatedly(Return(ser_man.get()));

  detail::backend_runtime->register_handle(h_0.get());

  auto task_a = std::make_unique<MockTask>();
  task_a->delegate_to_fake();
  EXPECT_CALL(*task_a, get_dependencies())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(FakeTask::handle_container_t{ h_0.get() }));
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

// Same task, but using helpers...
TEST_F(RegisterTask, allocate_data_block_2) {
  using namespace ::testing;

  auto h_0 = make_handle<double, true, true>(FakeDependencyHandle::version_t(), "the_key");

  detail::backend_runtime->register_handle(h_0.get());

  register_write_only_capture(h_0.get(), [&]{
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

TEST_F(RegisterTask, release_satisfy) {
  using namespace ::testing;

  auto h_0 = make_handle<int, true, true>(FakeDependencyHandle::version_t(), "the_key");
  auto next_version = h_0->get_version();
  ++next_version;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_1.get());

  int value = 42;

  register_write_only_capture(h_0.get(), [&,value]{
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    memcpy(data, &value, sizeof(int));
    detail::backend_runtime->release_handle(h_0.get());
  });

  detail::backend_runtime->release_read_only_usage(h_0.get());
  detail::backend_runtime->release_read_only_usage(h_1.get());

  register_read_write_capture(h_1.get(), [&,value]{  // FIXME: this is not running but not reporting that it didn't run either
    ASSERT_THAT(*(int*)(h_1->get_data_block()->get_data()), Eq(value));
    detail::backend_runtime->release_handle(h_1.get());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
