/*
//@HEADER
// ************************************************************************
//
//                          test_frontend.h
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

#ifndef SRC_TESTS_FRONTEND_VALIDATION_TEST_FRONTEND_H_
#define SRC_TESTS_FRONTEND_VALIDATION_TEST_FRONTEND_H_

#define DEBUG_CREATE_WORK_HANDLES 0

#include <deque>
#include <iomanip>
#include <iostream>

#include <gtest/gtest.h>
#include <darma/impl/task.h>
#include <darma/impl/runtime.h>
#include <src/include/darma/interface/app/access_handle.h>
#include <src/include/darma/impl/handle_attorneys.h>

#include "mock_backend.h"
#include "darma_types.h"

class TestFrontend
  : public ::testing::Test
{
  public:

    typedef typename darma_runtime::abstract::backend::runtime_t::handle_t handle_t;
    typedef typename darma_runtime::abstract::backend::runtime_t::task_t task_t;
    typedef typename darma_runtime::abstract::backend::runtime_t::version_t version_t;
    typedef typename darma_runtime::abstract::backend::runtime_t::key_t key_t;

  protected:


    void setup_top_level_task() {
      top_level_task = std::make_unique<darma_runtime::detail::TopLevelTask>();
    }

    template <template <class...> class Strictness = ::testing::StrictMock>
    void setup_mock_runtime() {
      using namespace ::testing;

      if(!top_level_task) {
        setup_top_level_task();
      }

      mock_runtime = std::make_unique<Strictness<mock_backend::MockRuntime>>();
      ON_CALL(*mock_runtime, get_running_task())
        .WillByDefault(Return(top_level_task.get()));

      darma_runtime::detail::backend_runtime = mock_runtime.get();
      mock_runtime_setup_done = true;
    }

    virtual void SetUp() {
      if(not mock_runtime_setup_done) {
        setup_mock_runtime();
      }
    }

    virtual void TearDown() {
      top_level_task.reset();
      if(mock_runtime_setup_done) {
        mock_runtime = nullptr;
        mock_runtime_setup_done = false;
      }
      same_handles.clear();
      same_keys.clear();
      same_versions.clear();

    }

    struct SameHandleMatcher {
      SameHandleMatcher(
        mock_backend::MockRuntime::handle_t*& handle_to_track,
        darma_runtime::types::key_t& key_to_track,
        darma_runtime::types::version_t& version_to_track
      ) : handle(handle_to_track), key(key_to_track), version(version_to_track) { }
      mock_backend::MockRuntime::handle_t*& handle;
      darma_runtime::types::version_t& version;
      darma_runtime::types::key_t& key;
      bool operator()(
        const mock_backend::MockRuntime::handle_t* in_handle
      ) const {
        if(handle != nullptr) {
          return intptr_t(in_handle) == intptr_t(handle)
              and key == in_handle->get_key()
              and version == in_handle->get_version();
        }
        else {
          // Just const_cast so that one matcher can handle both the const and non-const cases
          handle = const_cast<mock_backend::MockRuntime::handle_t*>(in_handle);
          key = handle->get_key();
          version = handle->get_version();
          return true;
        }
      }
    };

    SameHandleMatcher
    make_same_handle_matcher() {
      same_handles.push_back(nullptr);
      same_keys.emplace_back();
      same_versions.emplace_back();
      return { same_handles.back(), same_keys.back(), same_versions.back() };
    }


    template <typename SameHandle>
    void expect_handle_life_cycle(SameHandle&& same_handle,
      ::testing::Sequence s = ::testing::Sequence(),
      bool read_only = false,
      bool calls_last_at_version_depth = true
    ) {
      using namespace ::testing;
      Sequence s2;

      if(read_only) {
        EXPECT_CALL(*mock_runtime, register_fetching_handle(Truly(same_handle), _))
          .InSequence(s, s2);
      }
      else {
        EXPECT_CALL(*mock_runtime, register_handle(Truly(same_handle)))
          .InSequence(s, s2);
      }

      EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(same_handle)))
        .InSequence(s);

      if(not read_only and calls_last_at_version_depth) {
        EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Truly(same_handle)))
          .InSequence(s2);
      }

      EXPECT_CALL(*mock_runtime, release_handle(Truly(same_handle)))
        .InSequence(s, s2);

    }

    template <typename SameHandle>
    auto in_get_dependencies(SameHandle&& same_handle) {
      return ::testing::Truly([&handle=same_handle.handle](auto* task_arg) {
        const auto& task_deps = task_arg->get_dependencies();
        return task_deps.find(handle) != task_deps.end();
      });
    }

    auto handle_in_get_dependencies(handle_t*& handle) {
      return ::testing::Truly([&handle](task_t const * const task_arg) {
        const auto& task_deps = task_arg->get_dependencies();
        bool rv = task_deps.find(handle) != task_deps.end();
#if DEBUG_CREATE_WORK_HANDLES
        std::cout << "handle_in_get_dependencies(0x" << std::hex << intptr_t(handle)
                  << ") for task " << std::hex << intptr_t(task_arg)
                  << ": " << std::boolalpha << rv << std::endl;
#endif
        return rv;
      });
    }

    template <typename SameHandle>
    auto needs_read_of(SameHandle&& same_handle) {
      return ::testing::Truly([&handle=same_handle.handle](auto* task_arg) {
        return task_arg->needs_read_data(handle);
      });
    }

    auto needs_read_handle(handle_t*& handle) {
      return ::testing::Truly([&handle](task_t const * const task_arg) {
        bool rv = task_arg->needs_read_data(handle);
#if DEBUG_CREATE_WORK_HANDLES
        std::cout << "needs_read_handle(0x" << std::hex << intptr_t(handle)
                  << ") for task " << std::hex << intptr_t(task_arg)
                  << ": " << std::boolalpha << rv << std::endl;
#endif
        return rv;
      });
    }

    template <typename SameHandle>
    auto needs_write_of(SameHandle&& same_handle) {
      return ::testing::Truly([&handle=same_handle.handle](auto* task_arg) {
        return task_arg->needs_write_data(handle);
      });
    }

    auto needs_write_handle(handle_t*& handle) {
      return ::testing::Truly([&handle](task_t const * const task_arg) {
        bool rv = task_arg->needs_write_data(handle);
#if DEBUG_CREATE_WORK_HANDLES
        std::cout << "needs_write_handle(0x" << std::hex << intptr_t(handle)
                  << ") for task " << std::hex << intptr_t(task_arg)
                  << ": " << std::boolalpha << rv << std::endl;
#endif
        return rv;
      });
    }

    void
    run_all_tasks() {
      while(not mock_runtime->registered_tasks.empty()) {
        mock_runtime->registered_tasks.front()->run();
        mock_runtime->registered_tasks.pop_front();
      }
    }

    mock_backend::MockRuntime::task_unique_ptr top_level_task;
    std::unique_ptr<mock_backend::MockRuntime> mock_runtime;
    bool mock_runtime_setup_done = false;
    std::deque<mock_backend::MockRuntime::handle_t*> same_handles;
    std::deque<darma_runtime::types::key_t> same_keys;
    std::deque<darma_runtime::types::version_t> same_versions;
};

namespace _impl {

using namespace darma_runtime;
using namespace darma_runtime::detail;
typedef typename abstract::backend::runtime_t::handle_t handle_t;

template <typename Handle, typename Enable=void>
struct assert_version_is_helper;

template <>
struct assert_version_is_helper<handle_t*> {
  ::testing::AssertionResult
  operator()(
    const char* v1_expr,
    const char* v2_expr,
    handle_t* h,
    darma_runtime::types::version_t v2,
    bool exact=false
  ) const {
    auto& v1 = h->get_version();

    bool success = v1 == v2;
    if(success and exact) success = v1.depth() == v2.depth();

    if(success) return ::testing::AssertionSuccess();

    return ::testing::AssertionFailure()
      << "Version associated with " << v1_expr << ", which is "
      << v1 << ", is not"
      << (exact ? std::string(" exactly ") : std::string(" "))
      << "equal to expected value of " << v2 << " (from expression "
      << v2_expr << ")";
  }
};

template <typename... Args>
struct assert_version_is_helper<AccessHandle<Args...>> {
  ::testing::AssertionResult
  operator()(
    const char* v1_expr,
    const char* v2_expr,
    AccessHandle<Args...> const& ah,
    darma_runtime::types::version_t v2,
    bool exact=false
  ) const {
    using darma_runtime::detail::create_work_attorneys::for_AccessHandle;
    handle_t* handle = for_AccessHandle::get_dep_handle(ah);
    return assert_version_is_helper<handle_t*>()(v1_expr, v2_expr, handle, v2);
  }


};

} // end namespace _impl

template <bool exact, typename VersionFrom>
::testing::AssertionResult
AssertHandleVersionIs(
  const char* v1_expr,
  const char* v2_expr,
  VersionFrom&& vf,
  darma_runtime::types::version_t v2
) {
  return _impl::assert_version_is_helper<std::decay_t<VersionFrom>>()(
    v1_expr, v2_expr, std::forward<VersionFrom>(vf), v2, exact
  );
}

#define EXPECT_VERSION_EQ(h, ...) EXPECT_PRED_FORMAT2(AssertHandleVersionIs<false>, h, TestFrontend::version_t(__VA_ARGS__))
#define EXPECT_VERSION_EQ_EXACT(h, ...) EXPECT_PRED_FORMAT2(AssertHandleVersionIs<true>, h, TestFrontend::version_t(__VA_ARGS__))
#define ASSERT_VERSION_EQ(h, ...) ASSERT_PRED_FORMAT2(AssertHandleVersionIs<false>, h, TestFrontend::version_t(__VA_ARGS__))
#define ASSERT_VERSION_EQ_EXACT(h, ...) ASSERT_PRED_FORMAT2(AssertHandleVersionIs<true>, h, TestFrontend::version_t(__VA_ARGS__))


#endif /* SRC_TESTS_FRONTEND_VALIDATION_TEST_FRONTEND_H_ */
