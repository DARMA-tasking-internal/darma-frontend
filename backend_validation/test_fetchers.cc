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
#include "main.h"

using namespace darma_runtime;
using namespace mock_frontend;

namespace {

class TestFetchers
  : public ::testing::Test
{
  protected:

    virtual void SetUp() {

      detail::backend_runtime = darma_runtime::detail::_gen_backend_runtime_ptr<>();

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

    virtual ~TestFetchers() noexcept { }
};

} // end anonymous namespace

////////////////////////////////////////////////////////////////////////////////

// satisfy subsequent ++v and its fetcher when publish comes before fetch
TEST_F(TestFetchers, satisfy_fetcher_pub_then_reg) {
  using namespace ::testing;

  auto h_0 = make_handle<int, true, true>(MockDependencyHandle::version_t(), "the_key");
  auto next_version = h_0->get_version();
  ++next_version;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");

  auto h_1f = make_fetching_handle<int, true, false>(MockDependencyHandle::version_t(), "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(2));  // when running write-only task and when releasing

  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(AtMost(1));

  EXPECT_CALL(*h_1f.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1f.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1f.get(), allow_writes())
    .Times(Exactly(0));

  auto user_ver = make_key("alpha");

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

  detail::backend_runtime->publish_handle(h_1.get(), user_ver);

  detail::backend_runtime->register_fetching_handle(h_1f.get(), user_ver);

  detail::backend_runtime->finalize();
  backend_finalized = true;

  ASSERT_TRUE(h_1->is_satisfied());
  {
    abstract::backend::DataBlock* data_block = h_1->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    ASSERT_THAT(*((int*)data), Eq(value));
  }

  ASSERT_TRUE(h_1f->is_satisfied());
  ASSERT_FALSE(h_1f->is_writable());
  {
    abstract::backend::DataBlock* data_block = h_1f->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    ASSERT_THAT(*((int*)data), Eq(value));
  }

  detail::backend_runtime->release_read_only_usage(h_1f.get());
  detail::backend_runtime->release_handle(h_1f.get());

  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
}

// satisfy subsequent ++v and its fetchers when publish comes before fetch
TEST_F(TestFetchers, satisfy_fetcher_pub_then_reg2) {
  using namespace ::testing;

  auto h_0 = make_handle<int, true, true>(MockDependencyHandle::version_t(), "the_key");
  auto next_version = h_0->get_version();
  ++next_version;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");

  auto h_1f = make_fetching_handle<int, true, false>(MockDependencyHandle::version_t(), "the_key");
  auto h_1f2 = make_fetching_handle<int, true, false>(MockDependencyHandle::version_t(), "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(2));  // when running write-only task and when releasing

  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(AtMost(1));

  EXPECT_CALL(*h_1f.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1f.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1f.get(), allow_writes())
    .Times(Exactly(0));

  EXPECT_CALL(*h_1f2.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1f2.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1f2.get(), allow_writes())
    .Times(Exactly(0));

  auto user_ver = make_key("alpha");

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

  detail::backend_runtime->publish_handle(h_1.get(), user_ver, 2);

  detail::backend_runtime->register_fetching_handle(h_1f.get(), user_ver);
  detail::backend_runtime->register_fetching_handle(h_1f2.get(), user_ver);

  detail::backend_runtime->finalize();
  backend_finalized = true;

  ASSERT_TRUE(h_1->is_satisfied());
  {
    abstract::backend::DataBlock* data_block = h_1->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    ASSERT_THAT(*((int*)data), Eq(value));
  }

  ASSERT_TRUE(h_1f->is_satisfied());
  ASSERT_FALSE(h_1f->is_writable());
  {
    abstract::backend::DataBlock* data_block = h_1f->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    ASSERT_THAT(*((int*)data), Eq(value));
  }

  ASSERT_TRUE(h_1f2->is_satisfied());
  ASSERT_FALSE(h_1f2->is_writable());
  {
    abstract::backend::DataBlock* data_block = h_1f2->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    ASSERT_THAT(*((int*)data), Eq(value));
  }

  detail::backend_runtime->release_read_only_usage(h_1f.get());
  detail::backend_runtime->release_handle(h_1f.get());

  detail::backend_runtime->release_read_only_usage(h_1f2.get());
  detail::backend_runtime->release_handle(h_1f2.get());

  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
}

// satisfy subsequent ++v and its fetchers (with different user version
// tags) when publish comes before fetch
TEST_F(TestFetchers, satisfy_fetcher_pub2_then_reg2) {
  using namespace ::testing;

  auto h_0 = make_handle<int, true, true>(MockDependencyHandle::version_t(), "the_key");
  auto next_version = h_0->get_version();
  ++next_version;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");

  auto h_1f = make_fetching_handle<int, true, false>(MockDependencyHandle::version_t(), "the_key");
  auto h_1f2 = make_fetching_handle<int, true, false>(MockDependencyHandle::version_t(), "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(2));  // when running write-only task and when releasing

  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(AtMost(1));

  EXPECT_CALL(*h_1f.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1f.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1f.get(), allow_writes())
    .Times(Exactly(0));

  EXPECT_CALL(*h_1f2.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1f2.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1f2.get(), allow_writes())
    .Times(Exactly(0));

  auto user_ver = make_key("alpha");
  auto user_ver2 = make_key("beta");

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

  detail::backend_runtime->publish_handle(h_1.get(), user_ver);
  detail::backend_runtime->publish_handle(h_1.get(), user_ver2);

  detail::backend_runtime->register_fetching_handle(h_1f.get(), user_ver);
  detail::backend_runtime->register_fetching_handle(h_1f2.get(), user_ver2);

  detail::backend_runtime->finalize();
  backend_finalized = true;

  ASSERT_TRUE(h_1->is_satisfied());
  {
    abstract::backend::DataBlock* data_block = h_1->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    ASSERT_THAT(*((int*)data), Eq(value));
  }

  ASSERT_TRUE(h_1f->is_satisfied());
  ASSERT_FALSE(h_1f->is_writable());
  {
    abstract::backend::DataBlock* data_block = h_1f->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    ASSERT_THAT(*((int*)data), Eq(value));
  }

  ASSERT_TRUE(h_1f2->is_satisfied());
  ASSERT_FALSE(h_1f2->is_writable());
  {
    abstract::backend::DataBlock* data_block = h_1f2->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    ASSERT_THAT(*((int*)data), Eq(value));
  }

  detail::backend_runtime->release_read_only_usage(h_1f.get());
  detail::backend_runtime->release_handle(h_1f.get());

  detail::backend_runtime->release_read_only_usage(h_1f2.get());
  detail::backend_runtime->release_handle(h_1f2.get());

  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
}

// satisfy subsequent ++v and its fetcher when publish comes before fetch,
// in the case where we release the primary handle before checking the fetcher
TEST_F(TestFetchers, satisfy_fetcher_pub_then_reg_release) {
  using namespace ::testing;

  auto h_0 = make_handle<int, true, true>(MockDependencyHandle::version_t(), "the_key");
  auto next_version = h_0->get_version();
  ++next_version;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");

  auto h_1f = make_fetching_handle<int, true, false>(MockDependencyHandle::version_t(), "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(2));  // when running write-only task and when releasing

  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(AtMost(1));

  EXPECT_CALL(*h_1f.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1f.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1f.get(), allow_writes())
    .Times(Exactly(0));

  auto user_ver = make_key("alpha");

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

  detail::backend_runtime->publish_handle(h_1.get(), user_ver);

  detail::backend_runtime->register_fetching_handle(h_1f.get(), user_ver);

  detail::backend_runtime->finalize();
  backend_finalized = true;

  ASSERT_TRUE(h_1->is_satisfied());
  {
    abstract::backend::DataBlock* data_block = h_1->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    ASSERT_THAT(*((int*)data), Eq(value));
  }

  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());

  ASSERT_TRUE(h_1f->is_satisfied());
  ASSERT_FALSE(h_1f->is_writable());
  {
    abstract::backend::DataBlock* data_block = h_1f->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    ASSERT_THAT(*((int*)data), Eq(value));
  }

  detail::backend_runtime->release_read_only_usage(h_1f.get());
  detail::backend_runtime->release_handle(h_1f.get());
}

// satisfy subsequent ++v and its fetcher when fetch comes before publish
TEST_F(TestFetchers, satisfy_fetcher_reg_then_pub) {
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
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(AtMost(1));

  auto user_ver = make_key("alpha");

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

  register_read_only_capture(h_1.get(), [&,value,user_ver]{
    register_read_only_capture(h_1.get(), [&,value,user_ver]{
      ASSERT_TRUE(h_1->is_satisfied());
      {
        abstract::backend::DataBlock* data_block = h_1->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        ASSERT_THAT(*((int*)data), Eq(value));
      } 
      detail::backend_runtime->publish_handle(h_1.get(), user_ver);
    });
  });

  register_nodep_task([&,value,user_ver]{
    auto h_1f = make_fetching_handle<int, true, false>(
        MockDependencyHandle::version_t(), "the_key");

    EXPECT_CALL(*h_1f.get(), satisfy_with_data_block(_))
      .Times(Exactly(1));
    EXPECT_CALL(*h_1f.get(), get_data_block())
      .Times(AtLeast(1));
    EXPECT_CALL(*h_1f.get(), allow_writes())
      .Times(Exactly(0));  // since it's a fetching handle

    detail::backend_runtime->register_fetching_handle(h_1f.get(), user_ver);

    register_read_only_capture(h_1f.get(), [&,value]{
      ASSERT_TRUE(h_1f->is_satisfied());
      ASSERT_FALSE(h_1f->is_writable());
      {
        abstract::backend::DataBlock* data_block = h_1f->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        ASSERT_THAT(*((int*)data), Eq(value));
      }
      detail::backend_runtime->release_read_only_usage(h_1f.get());
      detail::backend_runtime->release_handle(h_1f.get());
    });
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;

  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
}

// satisfy subsequent ++v and its fetcher when fetch comes before publish,
// in the case where we release the primary handle before checking the fetcher
TEST_F(TestFetchers, satisfy_fetcher_reg_then_pub_release) {
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
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(AtMost(1));

  auto user_ver = make_key("alpha");

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

  register_read_only_capture(h_1.get(), [&,value,user_ver]{
    register_read_only_capture(h_1.get(), [&,value,user_ver]{
      ASSERT_TRUE(h_1->is_satisfied());
      {
        abstract::backend::DataBlock* data_block = h_1->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        ASSERT_THAT(*((int*)data), Eq(value));
      } 
      detail::backend_runtime->publish_handle(h_1.get(), user_ver);
      detail::backend_runtime->release_read_only_usage(h_1.get());
      detail::backend_runtime->release_handle(h_1.get());
    });
  });

  register_nodep_task([&,value,user_ver]{
    auto h_1f = make_fetching_handle<int, true, false>(
        MockDependencyHandle::version_t(), "the_key");

    EXPECT_CALL(*h_1f.get(), satisfy_with_data_block(_))
      .Times(Exactly(1));
    EXPECT_CALL(*h_1f.get(), get_data_block())
      .Times(AtLeast(1));
    EXPECT_CALL(*h_1f.get(), allow_writes())
      .Times(Exactly(0));  // since it's a fetching handle

    detail::backend_runtime->register_fetching_handle(h_1f.get(), user_ver);

    register_read_only_capture(h_1f.get(), [&,value]{
      ASSERT_TRUE(h_1f->is_satisfied());
      ASSERT_FALSE(h_1f->is_writable());
      {
        abstract::backend::DataBlock* data_block = h_1f->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        ASSERT_THAT(*((int*)data), Eq(value));
      }
      detail::backend_runtime->release_read_only_usage(h_1f.get());
      detail::backend_runtime->release_handle(h_1f.get());
    });
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;
}

// satisfy subsequent ++v and its fetchers when fetches come before publish
TEST_F(TestFetchers, satisfy_fetcher_reg2_then_pub) {
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
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(AtMost(1));

  auto user_ver = make_key("alpha");

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

  register_read_only_capture(h_1.get(), [&,value,user_ver]{
    register_read_only_capture(h_1.get(), [&,value,user_ver]{
      ASSERT_TRUE(h_1->is_satisfied());
      {
        abstract::backend::DataBlock* data_block = h_1->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        ASSERT_THAT(*((int*)data), Eq(value));
      } 
      detail::backend_runtime->publish_handle(h_1.get(), user_ver, 2);
    });
  });

  register_nodep_task([&,value,user_ver]{
    auto h_1f = make_fetching_handle<int, true, false>(
        MockDependencyHandle::version_t(), "the_key");

    EXPECT_CALL(*h_1f.get(), satisfy_with_data_block(_))
      .Times(Exactly(1));
    EXPECT_CALL(*h_1f.get(), get_data_block())
      .Times(AtLeast(1));
    EXPECT_CALL(*h_1f.get(), allow_writes())
      .Times(Exactly(0));  // since it's a fetching handle

    detail::backend_runtime->register_fetching_handle(h_1f.get(), user_ver);

    register_read_only_capture(h_1f.get(), [&,value]{
      ASSERT_TRUE(h_1f->is_satisfied());
      ASSERT_FALSE(h_1f->is_writable());
      {
        abstract::backend::DataBlock* data_block = h_1f->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        ASSERT_THAT(*((int*)data), Eq(value));
      }
      detail::backend_runtime->release_read_only_usage(h_1f.get());
      detail::backend_runtime->release_handle(h_1f.get());
    });
  });

  register_nodep_task([&,value,user_ver]{
    auto h_2f = make_fetching_handle<int, true, false>(
        MockDependencyHandle::version_t(), "the_key");

    EXPECT_CALL(*h_2f.get(), satisfy_with_data_block(_))
      .Times(Exactly(1));
    EXPECT_CALL(*h_2f.get(), get_data_block())
      .Times(AtLeast(1));
    EXPECT_CALL(*h_2f.get(), allow_writes())
      .Times(Exactly(0));  // since it's a fetching handle

    detail::backend_runtime->register_fetching_handle(h_2f.get(), user_ver);

    register_read_only_capture(h_2f.get(), [&,value]{
      ASSERT_TRUE(h_2f->is_satisfied());
      ASSERT_FALSE(h_2f->is_writable());
      {
        abstract::backend::DataBlock* data_block = h_2f->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        ASSERT_THAT(*((int*)data), Eq(value));
      }
      detail::backend_runtime->release_read_only_usage(h_2f.get());
      detail::backend_runtime->release_handle(h_2f.get());
    });
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;

  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
}

// satisfy subsequent ++v and its 2 fetchers (with different user
// version tags)
TEST_F(TestFetchers, satisfy_fetcher_reg2_then_pub2) {
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
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(AtMost(1));

  auto user_ver = make_key("alpha");
  auto user_ver2 = make_key("beta");

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

  register_read_only_capture(h_1.get(), [&,value,user_ver,user_ver2]{
    register_read_only_capture(h_1.get(), [&,value,user_ver,user_ver2]{
      ASSERT_TRUE(h_1->is_satisfied());
      {
        abstract::backend::DataBlock* data_block = h_1->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        ASSERT_THAT(*((int*)data), Eq(value));
      } 
      detail::backend_runtime->publish_handle(h_1.get(), user_ver);
      detail::backend_runtime->publish_handle(h_1.get(), user_ver2);
    });
  });

  register_nodep_task([&,value,user_ver]{
    auto h_1f = make_fetching_handle<int, true, false>(
        MockDependencyHandle::version_t(), "the_key");

    EXPECT_CALL(*h_1f.get(), satisfy_with_data_block(_))
      .Times(Exactly(1));
    EXPECT_CALL(*h_1f.get(), get_data_block())
      .Times(AtLeast(1));
    EXPECT_CALL(*h_1f.get(), allow_writes())
      .Times(Exactly(0));  // since it's a fetching handle

    detail::backend_runtime->register_fetching_handle(h_1f.get(), user_ver);

    register_read_only_capture(h_1f.get(), [&,value]{
      ASSERT_TRUE(h_1f->is_satisfied());
      ASSERT_FALSE(h_1f->is_writable());
      {
        abstract::backend::DataBlock* data_block = h_1f->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        ASSERT_THAT(*((int*)data), Eq(value));
      }
      detail::backend_runtime->release_read_only_usage(h_1f.get());
      detail::backend_runtime->release_handle(h_1f.get());
    });
  });

  register_nodep_task([&,value,user_ver2]{
    auto h_2f = make_fetching_handle<int, true, false>(
        MockDependencyHandle::version_t(), "the_key");

    EXPECT_CALL(*h_2f.get(), satisfy_with_data_block(_))
      .Times(Exactly(1));
    EXPECT_CALL(*h_2f.get(), get_data_block())
      .Times(AtLeast(1));
    EXPECT_CALL(*h_2f.get(), allow_writes())
      .Times(Exactly(0));  // since it's a fetching handle

    detail::backend_runtime->register_fetching_handle(h_2f.get(), user_ver2);

    register_read_only_capture(h_2f.get(), [&,value]{
      ASSERT_TRUE(h_2f->is_satisfied());
      ASSERT_FALSE(h_2f->is_writable());
      {
        abstract::backend::DataBlock* data_block = h_2f->get_data_block();
        ASSERT_THAT(data_block, NotNull());
        void* data = data_block->get_data();
        ASSERT_THAT(data, NotNull());
        ASSERT_THAT(*((int*)data), Eq(value));
      }
      detail::backend_runtime->release_read_only_usage(h_2f.get());
      detail::backend_runtime->release_handle(h_2f.get());
    });
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;

  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
}

