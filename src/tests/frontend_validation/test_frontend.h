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
        mock_runtime.reset();
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
      bool read_only = false
    ) {
      using namespace ::testing;

      if(read_only) {
        EXPECT_CALL(*mock_runtime, register_fetching_handle(Truly(same_handle), _))
          .InSequence(s);
      }
      else {
        EXPECT_CALL(*mock_runtime, register_handle(Truly(same_handle)))
          .InSequence(s);
      }

      EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(same_handle)))
        .InSequence(s);

      EXPECT_CALL(*mock_runtime, release_handle(Truly(same_handle)))
        .InSequence(s);

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


#endif /* SRC_TESTS_FRONTEND_VALIDATION_TEST_FRONTEND_H_ */
