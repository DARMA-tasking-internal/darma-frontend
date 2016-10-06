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

#include <darma/impl/task.h>

#include <darma/interface/backend/runtime.h>
#include <darma/interface/frontend/handle.h>
#include <darma/interface/backend/region_context_handle.h>

namespace mock_backend {

class MockRuntime
  : public darma_runtime::abstract::backend::Runtime,
    public darma_runtime::abstract::backend::Context,
    public darma_runtime::abstract::backend::MemoryManager
{
  public:

    using task_t = darma_runtime::abstract::backend::runtime_t::task_t;
    using task_unique_ptr = darma_runtime::abstract::backend::runtime_t::task_unique_ptr;
    using condition_task_t = darma_runtime::abstract::backend::runtime_t::condition_task_t;
    using condition_task_unique_ptr = darma_runtime::abstract::backend::runtime_t::condition_task_unique_ptr;
    using runtime_t = darma_runtime::abstract::backend::Runtime;
    using handle_t = darma_runtime::abstract::frontend::Handle;
    using use_t = darma_runtime::abstract::frontend::Use;
    using key_t = darma_runtime::types::key_t;
    using publication_details_t = darma_runtime::abstract::frontend::PublicationDetails;
    using flow_t = darma_runtime::types::flow_t;
    using concurrent_region_task_t = runtime_t::concurrent_region_task_t;
    using concurrent_region_task_unique_ptr = runtime_t::concurrent_region_task_unique_ptr;
    using top_level_task_t = darma_runtime::abstract::backend::runtime_t::top_level_task_t;
    using top_level_task_unique_ptr = darma_runtime::abstract::backend::runtime_t::top_level_task_unique_ptr;

    struct CRTaskDetails {
      concurrent_region_task_unique_ptr task;
      size_t n_indices;
      std::shared_ptr<darma_runtime::abstract::backend::DataStoreHandle> data_store_handle;
      size_t n_run_so_far = 0;
    };

    void
    register_task(
       task_unique_ptr&& task
    ) override {
      register_task_gmock_proxy(task.get());
      if(save_tasks) {
        registered_tasks.emplace_back(std::forward<task_unique_ptr>(task));
      }
    }

    void
    register_concurrent_region(
      concurrent_region_task_unique_ptr&& task, size_t n_idxs,
      std::shared_ptr<darma_runtime::abstract::backend::DataStoreHandle> const& ds
    ) override {
      register_concurrent_region_gmock_proxy(task.get(), n_idxs, ds.get());
      concurrent_regions.emplace_back(CRTaskDetails{std::move(task), n_idxs, ds, 0});
    }

    bool
    register_condition_task(
      condition_task_unique_ptr&& task
    ) override {
      auto rv = register_condition_task_gmock_proxy(task.get());
      return rv;
    }


#ifdef __clang__
#if __has_warning("-Winconsistent-missing-override")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif
#endif

    MOCK_CONST_METHOD0(get_spmd_rank, size_t());
    MOCK_CONST_METHOD0(get_spmd_size, size_t());

    MOCK_METHOD1(register_task_gmock_proxy, void(task_t* task));
    MOCK_METHOD1(register_condition_task_gmock_proxy, bool(condition_task_t* task));
    MOCK_METHOD3(register_concurrent_region_gmock_proxy, void(
      concurrent_region_task_t*, size_t,
      darma_runtime::abstract::backend::DataStoreHandle*
    ));
    MOCK_METHOD0(make_data_store,
      std::shared_ptr<darma_runtime::abstract::backend::DataStoreHandle>()
    );

    MOCK_CONST_METHOD1(get_execution_resource_count, size_t(size_t));
    MOCK_CONST_METHOD0(get_running_task, task_t*());
    MOCK_METHOD0(finalize, void());
    MOCK_METHOD1(register_use, void(use_t*));
    MOCK_METHOD1(reregister_migrated_use, void(use_t*));
    MOCK_METHOD1(make_initial_flow, flow_t(std::shared_ptr<handle_t> const&));
    MOCK_METHOD3(make_fetching_flow, flow_t(std::shared_ptr<handle_t> const&, key_t const&,
      std::shared_ptr<darma_runtime::abstract::backend::DataStoreHandle> const&
    ));
    MOCK_METHOD1(make_null_flow, flow_t(std::shared_ptr<handle_t> const&));
    MOCK_METHOD1(make_forwarding_flow, flow_t(flow_t&));
    MOCK_METHOD1(make_next_flow, flow_t(flow_t&));
    MOCK_METHOD2(establish_flow_alias, void(flow_t&, flow_t&));
    MOCK_METHOD1(release_use, void(use_t*));
    MOCK_METHOD2(publish_use, void(use_t*, publication_details_t*));
    MOCK_METHOD2(allocate, void*(size_t,
      darma_runtime::abstract::frontend::MemoryRequirementDetails const&));
    MOCK_METHOD2(deallocate, void(void*, size_t));
    MOCK_METHOD4(allreduce_use, void(use_t*, use_t*,
      darma_runtime::abstract::frontend::CollectiveDetails const*,
      key_t const&
    ));
    MOCK_METHOD1(release_flow, void(flow_t&));


#ifdef __clang__
#if __has_warning("-Winconsistent-missing-override")
#pragma clang diagnostic pop
#endif
#endif


    bool save_tasks = true;
    std::deque<task_unique_ptr> registered_tasks;
    std::deque<CRTaskDetails> concurrent_regions;
};



class MockSerializationPolicy
  : public darma_runtime::abstract::backend::SerializationPolicy
{
  public:

    MOCK_CONST_METHOD2(packed_size_contribution_for_blob, size_t(void const*, size_t));
    MOCK_CONST_METHOD3(pack_blob, void(void*&, void const*, size_t));
    MOCK_CONST_METHOD3(unpack_blob, void(void*&, void*, size_t));

};

struct MockConcurrentRegionContextHandle
  : public darma_runtime::abstract::backend::ConcurrentRegionContextHandle
{
  public:
    MOCK_CONST_METHOD0(get_backend_index, size_t());
};

struct MockDataStoreHandle
  : public darma_runtime::abstract::backend::DataStoreHandle
{

};

} // end namespace mock_backend

#endif /* SRC_TESTS_FRONTEND_VALIDATION_MOCK_BACKEND_H_ */
