/*
//@HEADER
// ************************************************************************
//
//                          mock_backend.h
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

#ifndef SRC_TESTS_FRONTEND_VALIDATION_MOCK_BACKEND_H_
#define SRC_TESTS_FRONTEND_VALIDATION_MOCK_BACKEND_H_

#include <deque>

#include <gmock/gmock.h>

#include <darma_types.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/backend/data_block.h>

namespace mock_backend {

class MockRuntime
  : public darma_runtime::abstract::backend::Runtime<
      darma_runtime::types::key_t,
      darma_runtime::types::version_t,
      darma_runtime::types::shared_ptr_template,
      darma_runtime::types::unique_ptr_template
    >
{
  public:

    typedef darma_runtime::types::key_t key_t;
    typedef darma_runtime::types::version_t version_t;
    typedef darma_runtime::abstract::frontend::DependencyHandle<key_t, version_t> handle_t;
    typedef darma_runtime::abstract::frontend::Task task_t;
    typedef darma_runtime::types::unique_ptr_template<task_t> task_unique_ptr;
    typedef darma_runtime::types::unique_ptr_template<const task_t> task_const_unique_ptr;

    void
    register_task(task_unique_ptr&& task) override {
      register_task_gmock_proxy(task.get());
      if(save_tasks) {
        registered_tasks.emplace_back(std::forward<task_unique_ptr>(task));
      }
    }


#ifdef __clang__
#if __has_warning("-Winconsistent-missing-override")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif
#endif

    MOCK_CONST_METHOD0(get_running_task, task_t* const());
    MOCK_METHOD1(register_handle, void(handle_t* const handle));
    MOCK_METHOD2(register_fetching_handle, void(handle_t* const, key_t const&));
    MOCK_METHOD1(release_read_only_usage, void(handle_t* const));
    MOCK_METHOD1(release_handle, void(const handle_t* const));
    MOCK_METHOD4(publish_handle, void(handle_t* const, key_t const&, size_t const, bool));
    MOCK_METHOD0(finalize, void());

    MOCK_METHOD1(register_task_gmock_proxy, void(task_t* task));

#ifdef __clang__
#if __has_warning("-Winconsistent-missing-override")
#pragma clang diagnostic pop
#endif
#endif

    bool save_tasks = true;
    std::deque<task_unique_ptr> registered_tasks;
};

class MockDataBlock
  : public darma_runtime::abstract::backend::DataBlock
{
  public:

    MOCK_METHOD0(get_data, void*());
};

} // end namespace mock_backend

#endif /* SRC_TESTS_FRONTEND_VALIDATION_MOCK_BACKEND_H_ */
