/*
//@HEADER
// ************************************************************************
//
//                          test_death.cc
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
#include "helpers.h"

using namespace darma_runtime;

namespace {

class DARMADeathTest
  : public ::testing::Test
{
  protected:

    static void
    init_backend() {
      int argc = 1;
      char **argv = new char*[1];
      argv[0] = new char[256];
      sprintf(argv[0], "<mock frontend test>");

      std::unique_ptr<typename abstract::backend::runtime_t::task_t>
          top_level_task = std::make_unique<
          ::testing::NiceMock<mock_frontend::MockTask>>();

      abstract::backend::darma_backend_initialize(
        argc, argv, detail::backend_runtime,
        std::move(top_level_task)
      );
    }

    template <typename Version, typename... KeyParts>
    static std::shared_ptr<NiceMock<MockDependencyHandle>>
    make_handle(Version v, KeyParts&&... kp) {
      typedef NiceMock<MockSerializationManager> ser_man_t;
      typedef NiceMock<MockDependencyHandle> handle_t;
      auto ser_man = new ser_man_t;
      EXPECT_CALL(*ser_man, get_metadata_size())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(sizeof(int)));
      auto new_handle = std::shared_ptr<handle_t>(
        new handle_t(),
        [=](handle_t* to_delete){
          delete to_delete;
          delete ser_man;
        }
      );
      new_handle->get_key_return = detail::key_traits<MockDependencyHandle::key_t>::maker()(
          std::forward<KeyParts>(kp)...);
      new_handle->get_version_return = v;
      EXPECT_CALL(*new_handle, get_serialization_manager())
        .Times(AnyNumber())
        .WillRepeatedly(Return(ser_man));
      return new_handle;
    }

    template <typename... KeyParts>
    static std::shared_ptr<NiceMock<MockDependencyHandle>>
    make_pending_handle(KeyParts&&... kp) {
      typedef NiceMock<MockSerializationManager> ser_man_t;
      typedef NiceMock<MockDependencyHandle> handle_t;
      auto ser_man = new ser_man_t;
      EXPECT_CALL(*ser_man, get_metadata_size())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(sizeof(int)));
      auto new_handle = std::shared_ptr<handle_t>(
        new handle_t(),
        [=](handle_t* to_delete){
          delete to_delete;
          delete ser_man;
        }
      );
      new_handle->get_key_return = detail::key_traits<MockDependencyHandle::key_t>::maker()(
          std::forward<KeyParts>(kp)...);
      new_handle->get_version_return = MockDependencyHandle::version_t();
      new_handle->version_is_pending_return = true;
      EXPECT_CALL(*new_handle, get_serialization_manager())
        .Times(AnyNumber())
        .WillRepeatedly(Return(ser_man));
      return new_handle;
    }

    virtual ~DARMADeathTest() { }
};

} // end anonymous namespace

#if defined(DEBUG) || !defined(NDEBUG)

TEST_F(DARMADeathTest, unregistered_dependency_returned_from_get_dependencies) {
  using namespace ::testing;

  typedef ::testing::NiceMock<MockTask> task_t;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");

      auto task_a = std::make_unique<task_t>();
      EXPECT_CALL(*task_a, get_dependencies())
        .Times(AtLeast(1))
        .WillRepeatedly(ReturnRefOfCopy(MockTask::handle_container_t{ h_0.get() }));
      EXPECT_CALL(*task_a, needs_read_data(Eq(h_0.get())))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
      EXPECT_CALL(*task_a, needs_write_data(Eq(h_0.get())))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));

      detail::backend_runtime->register_task(std::move(task_a));

      detail::backend_runtime->release_read_only_usage(h_0.get());
      detail::backend_runtime->finalize();
      detail::backend_runtime->release_handle(h_0.get());
    },
    ""
  );
}

TEST_F(DARMADeathTest, released_dependency_returned_from_get_dependencies) {
  using namespace ::testing;

  typedef ::testing::NiceMock<MockTask> task_t;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");

      detail::backend_runtime->register_handle(h_0.get());
      detail::backend_runtime->release_read_only_usage(h_0.get());
      detail::backend_runtime->release_handle(h_0.get());

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

      detail::backend_runtime->register_task(std::move(task_a));

      detail::backend_runtime->finalize();
    },
    ""
  );
}

TEST_F(DARMADeathTest, same_key_version_registered_twice) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");
      auto h_1 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");

      detail::backend_runtime->register_handle(h_0.get());
      detail::backend_runtime->register_handle(h_1.get());
    },
    ""
  );
}

TEST_F(DARMADeathTest, register_handle_called_twice_same_handle) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");

      detail::backend_runtime->register_handle(h_0.get());
      detail::backend_runtime->register_handle(h_0.get());
    },
    ""
  );
}

TEST_F(DARMADeathTest, register_handle_called_with_pending_version) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_pending_handle("the_key");

      detail::backend_runtime->register_handle(h_0.get());
    },
    ""
  );
}

TEST_F(DARMADeathTest, register_fetching_handle_called_with_nonpending_version) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");

      auto user_ver = make_key("alpha");
      detail::backend_runtime->register_fetching_handle(h_0.get(), user_ver);
    },
    ""
  );
}

TEST_F(DARMADeathTest, release_read_only_usage_before_register_handle) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");
      detail::backend_runtime->release_read_only_usage(h_0.get());
    },
    "release_read_only_usage"
  );
}

TEST_F(DARMADeathTest, release_read_only_usage_after_release_handle) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");

      detail::backend_runtime->register_handle(h_0.get());
      detail::backend_runtime->release_read_only_usage(h_0.get());
      detail::backend_runtime->release_handle(h_0.get());

      detail::backend_runtime->release_read_only_usage(h_0.get());
    },
    "release_read_only_usage"
  );
}

TEST_F(DARMADeathTest, publish_handle_after_release_read_only_usage) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");

      detail::backend_runtime->register_handle(h_0.get());
      detail::backend_runtime->release_read_only_usage(h_0.get());

      auto user_ver = make_key("alpha");
      detail::backend_runtime->publish_handle(h_0.get(), user_ver);

      detail::backend_runtime->release_handle(h_0.get());
    },
    "publish_handle"
  );
}

TEST_F(DARMADeathTest, publish_handle_after_release_handle) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");

      detail::backend_runtime->register_handle(h_0.get());
      detail::backend_runtime->release_read_only_usage(h_0.get());
      detail::backend_runtime->release_handle(h_0.get());

      auto user_ver = make_key("alpha");
      detail::backend_runtime->publish_handle(h_0.get(), user_ver);
    },
    "publish_handle"
  );
}

TEST_F(DARMADeathTest, register_too_many_fetching_handles) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");

      auto user_ver = make_key("alpha");
      detail::backend_runtime->register_handle(h_0.get());
      detail::backend_runtime->publish_handle(h_0.get(), user_ver, 2);

      auto h_0f1 = this->make_pending_handle("the_key");
      auto h_0f2 = this->make_pending_handle("the_key");
      auto h_0f3 = this->make_pending_handle("the_key");

      detail::backend_runtime->register_fetching_handle(h_0f1.get(), user_ver);
      detail::backend_runtime->register_fetching_handle(h_0f2.get(), user_ver);
      detail::backend_runtime->register_fetching_handle(h_0f3.get(), user_ver);
    },
    ""
  );
}

TEST_F(DARMADeathTest, register_too_few_fetching_handles) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");

      auto user_ver = make_key("alpha");
      detail::backend_runtime->register_handle(h_0.get());
      detail::backend_runtime->publish_handle(h_0.get(), user_ver, 2);

      auto h_0f1 = this->make_pending_handle("the_key");

      detail::backend_runtime->register_fetching_handle(h_0f1.get(), user_ver);

      detail::backend_runtime->release_read_only_usage(h_0f1.get());
      detail::backend_runtime->release_handle(h_0f1.get());

      detail::backend_runtime->release_read_only_usage(h_0.get());
      detail::backend_runtime->release_handle(h_0.get());

      detail::backend_runtime->finalize();

      delete detail::backend_runtime;
    },
    ""
  );
}

TEST_F(DARMADeathTest, register_task_after_finalize) {
  using namespace ::testing;

  typedef ::testing::NiceMock<MockTask> task_t;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");
      detail::backend_runtime->register_handle(h_0.get());
      detail::backend_runtime->release_read_only_usage(h_0.get());

      auto task_a = std::make_unique<task_t>();
      EXPECT_CALL(*task_a, get_dependencies())
        .Times(AtLeast(1))
        .WillRepeatedly(ReturnRefOfCopy(MockTask::handle_container_t{ h_0.get() }));
      EXPECT_CALL(*task_a, needs_read_data(Eq(h_0.get())))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
      EXPECT_CALL(*task_a, needs_write_data(Eq(h_0.get())))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));

      detail::backend_runtime->finalize();

      detail::backend_runtime->register_task(std::move(task_a));
    },
    "finalize"
  );
}

TEST_F(DARMADeathTest, get_running_task_after_finalize) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();

      detail::backend_runtime->finalize();

      detail::backend_runtime->get_running_task();
    },
    "finalize"
  );
}

TEST_F(DARMADeathTest, register_handle_after_finalize) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");

      detail::backend_runtime->finalize();

      detail::backend_runtime->register_handle(h_0.get());
    },
    "finalize"
  );
}

TEST_F(DARMADeathTest, register_fetching_handle_after_finalize) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();
      auto user_ver = make_key("alpha");
      auto h_0 = this->make_pending_handle("the_key");

      detail::backend_runtime->finalize();

      detail::backend_runtime->register_fetching_handle(h_0.get(), user_ver);
    },
    "finalize"
  );
}

TEST_F(DARMADeathTest, publish_handle_after_finalize) {
  using namespace ::testing;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");
      auto user_ver = make_key("alpha");
      detail::backend_runtime->register_handle(h_0.get());

      detail::backend_runtime->finalize();

      detail::backend_runtime->publish_handle(h_0.get(), user_ver);
    },
    "finalize"
  );
}

TEST_F(DARMADeathTest, dependency_not_used_for_read_or_write) {
  using namespace ::testing;

  typedef ::testing::NiceMock<MockTask> task_t;

  EXPECT_DEATH(
    {
      init_backend();
      auto h_0 = this->make_handle(
          MockDependencyHandle::version_t(), "the_key");

      detail::backend_runtime->register_handle(h_0.get());

      auto task_a = std::make_unique<task_t>();
      EXPECT_CALL(*task_a, get_dependencies())
        .Times(AtLeast(1))
        .WillRepeatedly(ReturnRefOfCopy(MockTask::handle_container_t{ h_0.get() }));
      EXPECT_CALL(*task_a, needs_read_data(Eq(h_0.get())))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
      EXPECT_CALL(*task_a, needs_write_data(Eq(h_0.get())))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));

      detail::backend_runtime->register_task(std::move(task_a));

      detail::backend_runtime->release_read_only_usage(h_0.get());
      detail::backend_runtime->finalize();
      detail::backend_runtime->release_handle(h_0.get());
    },
    ""
  );
}

#endif
