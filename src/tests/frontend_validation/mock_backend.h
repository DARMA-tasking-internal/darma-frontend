/*
//@HEADER
// ************************************************************************
//
//                          mock_backend.h
//                         dharma_new
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/


#ifndef SRC_TESTS_FRONTEND_VALIDATION_MOCK_BACKEND_H_
#define SRC_TESTS_FRONTEND_VALIDATION_MOCK_BACKEND_H_

#include <deque>

#include <gmock/gmock.h>

#include <darma_types.h>

#include <darma/impl/task/task.h>

#include <darma/interface/backend/runtime.h>
#include <darma/interface/frontend/handle.h>
#include <darma/interface/backend/region_context_handle.h>
#include <darma/interface/backend/legacy_runtime.h>

namespace mock_backend {

class MockRuntime
  : public darma_runtime::abstract::backend::LegacyFlowsFromMethodsRuntime,
    public darma_runtime::abstract::backend::Context,
    public darma_runtime::abstract::backend::MemoryManager
{
  public:

    using task_t = darma_runtime::abstract::backend::runtime_t::task_t;
    using task_unique_ptr = darma_runtime::abstract::backend::runtime_t::task_unique_ptr;
    using runtime_t = darma_runtime::abstract::backend::Runtime;
    using handle_t = darma_runtime::abstract::frontend::Handle;
    using use_t = darma_runtime::abstract::frontend::Use;
    using destructible_use_t = darma_runtime::abstract::frontend::DestructibleUse;
    using key_t = darma_runtime::types::key_t;
    using publication_details_t = darma_runtime::abstract::frontend::PublicationDetails;
    using flow_t = darma_runtime::types::flow_t;
    using top_level_task_t = darma_runtime::abstract::backend::runtime_t::top_level_task_t;
    using top_level_task_unique_ptr = darma_runtime::abstract::backend::runtime_t::top_level_task_unique_ptr;
    using task_collection_t = darma_runtime::abstract::backend::runtime_t::task_collection_t;
    using task_collection_unique_ptr = darma_runtime::abstract::backend::runtime_t::task_collection_unique_ptr;

    void
    register_task(
       task_unique_ptr&& task
    ) override {
      register_task_gmock_proxy(task.get());
      if(save_tasks) {
        registered_tasks.emplace_back(std::forward<task_unique_ptr>(task));
      }
    }

    void register_task_collection(
      task_collection_unique_ptr&& collection
    ) override {
      register_task_collection_gmock_proxy(collection.get());
      if(save_tasks) {
        task_collections.emplace_back(std::forward<task_collection_unique_ptr>(collection));
      }
    }

    void publish_use(
      std::unique_ptr<destructible_use_t>&& pub_use,
      darma_runtime::abstract::frontend::PublicationDetails* details
    ) override {
      publish_use_gmock_proxy(pub_use.get(), details);
      backend_owned_uses.emplace_back(std::move(pub_use));
    }

    void allreduce_use(
      std::unique_ptr<destructible_use_t>&& use_in_out,
      darma_runtime::abstract::frontend::CollectiveDetails const* details,
      key_t const& tag
    ) override {
      allreduce_use_gmock_proxy(use_in_out.get(), details, tag);
      backend_owned_uses.emplace_back(std::move(use_in_out));
    }

    void allreduce_use(
      std::unique_ptr<destructible_use_t>&& use_in,
      std::unique_ptr<destructible_use_t>&& use_out,
      darma_runtime::abstract::frontend::CollectiveDetails const* details,
      key_t const& tag
    ) override {
      allreduce_use_gmock_proxy(use_in.get(), use_out.get(), details, tag);
      backend_owned_uses.emplace_back(std::move(use_in));
      backend_owned_uses.emplace_back(std::move(use_out));
    }

    void reduce_collection_use(
      std::unique_ptr<destructible_use_t>&& use_collection_in,
      std::unique_ptr<destructible_use_t>&& use_out,
      darma_runtime::abstract::frontend::CollectiveDetails const* details,
      key_t const& tag
    ) override {
      reduce_collection_use_gmock_proxy(use_collection_in.get(), use_out.get(),
        details, tag
      );
      backend_owned_uses.emplace_back(std::move(use_collection_in));
      backend_owned_uses.emplace_back(std::move(use_out));
    }


#ifdef __clang__
#if __has_warning("-Winconsistent-missing-override")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif
#endif

    MOCK_CONST_METHOD1(get_execution_resource_count, size_t(size_t));

    MOCK_METHOD1(register_task_gmock_proxy, void(task_t* task));
    MOCK_METHOD1(register_task_collection_gmock_proxy, void(task_collection_t*));
    MOCK_METHOD0(make_data_store,
      std::shared_ptr<darma_runtime::abstract::backend::DataStoreHandle>()
    );

    MOCK_CONST_METHOD0(get_running_task, task_t*());

    MOCK_METHOD1(legacy_register_use, void(
      darma_runtime::abstract::frontend::DependencyUse*
    ));
    MOCK_METHOD1(reregister_migrated_use, void(
      darma_runtime::abstract::frontend::RegisteredUse*
    ));
    MOCK_METHOD1(legacy_release_use, void(
      darma_runtime::abstract::frontend::DependencyUse*
    ));

    MOCK_METHOD1(make_initial_flow, flow_t(std::shared_ptr<handle_t const> const&));
    MOCK_METHOD2(make_fetching_flow, flow_t(std::shared_ptr<handle_t const> const&, key_t const&));
    MOCK_METHOD1(make_null_flow, flow_t(std::shared_ptr<handle_t const> const&));

    MOCK_METHOD1(make_forwarding_flow, flow_t(flow_t&));
    MOCK_METHOD1(make_next_flow, flow_t(flow_t&));

    MOCK_METHOD2(establish_flow_alias, void(flow_t&, flow_t&));


    MOCK_METHOD2(publish_use_gmock_proxy, void(
      use_t*, publication_details_t*)
    );

    MOCK_METHOD1(allocate, void*(std::size_t));
    //MOCK_METHOD2(allocate, void*(size_t,
    //  darma_runtime::abstract::frontend::MemoryRequirementDetails const&));
    MOCK_METHOD2(deallocate, void(void*, size_t));


    MOCK_METHOD3(allreduce_use_gmock_proxy, void(use_t*,
      darma_runtime::abstract::frontend::CollectiveDetails const*,
      key_t const&
    ));
    MOCK_METHOD4(allreduce_use_gmock_proxy, void(use_t*, use_t*,
      darma_runtime::abstract::frontend::CollectiveDetails const*,
      key_t const&
    ));
    MOCK_METHOD4(reduce_collection_use_gmock_proxy, void(use_t*, use_t*,
      darma_runtime::abstract::frontend::CollectiveDetails const*,
      key_t const&
    ));
    MOCK_METHOD1(release_flow, void(flow_t&));

    MOCK_METHOD1(get_packed_flow_size, size_t(flow_t const&));
    MOCK_METHOD2(pack_flow, void(flow_t&, void*&));
    MOCK_METHOD1(make_unpacked_flow, flow_t(void const*&));

#if _darma_has_feature(anti_flows)
    using anti_flow_t = darma_runtime::types::anti_flow_t;

    MOCK_METHOD1(get_packed_anti_flow_size, size_t(anti_flow_t const&));
    MOCK_METHOD2(pack_anti_flow, void(anti_flow_t&, void*&));
    MOCK_METHOD1(make_unpacked_anti_flow, anti_flow_t(void const*&));
#endif // _darma_has_feature(anti_flows)

    MOCK_METHOD2(make_indexed_local_flow, flow_t(flow_t&, size_t));
    MOCK_METHOD3(make_indexed_fetching_flow, flow_t(flow_t&, key_t const&, size_t));
    MOCK_METHOD2(make_indexed_flow, flow_t(flow_t&, size_t));

    MOCK_METHOD1(make_initial_flow_collection, flow_t(std::shared_ptr<handle_t const> const&));
    MOCK_METHOD1(make_null_flow_collection, flow_t(std::shared_ptr<handle_t const> const&));
    MOCK_METHOD1(make_next_flow_collection, flow_t(flow_t&));

    // "New" methods
    MOCK_METHOD1(register_use, void(darma_runtime::abstract::frontend::UsePendingRegistration*));
    MOCK_METHOD1(release_use, void(darma_runtime::abstract::frontend::UsePendingRelease*));

#ifdef __clang__
#if __has_warning("-Winconsistent-missing-override")
#pragma clang diagnostic pop
#endif
#endif


    // "new" interface calls
    void setup_default_delegators(
      top_level_task_unique_ptr& top_level_task
    ) {
      ON_CALL(*this, get_running_task())
        .WillByDefault(::testing::Return(top_level_task.get()));
      //ON_CALL(*this, allocate(::testing::_, ::testing::_))
      //  .WillByDefault(::testing::Invoke([](auto size, auto const& details) {
      //    return ::operator new(size);
      //  }));
      ON_CALL(*this, allocate(::testing::_))
        .WillByDefault(::testing::Invoke([](auto size) {
          return ::operator new(size);
        }));
      ON_CALL(*this, deallocate(::testing::_,::testing:: _))
        .WillByDefault(::testing::Invoke([](auto* ptr, auto size) {
          #if defined(__cpp_sized_deallocation)
          ::operator delete(ptr, size);
          #else
          ::operator delete(ptr);
          #endif
        }));
      setup_legacy_delegators();
    }

    void setup_legacy_delegators() {
      ON_CALL(*this, register_use(::testing::_))
        .WillByDefault(::testing::Invoke([this](auto* use) {
          this->darma_runtime::abstract::backend::LegacyFlowsFromMethodsRuntime::register_use(use);
        }));
      ON_CALL(*this, release_use(::testing::_))
        .WillByDefault(::testing::Invoke([this](auto* use) {
          this->darma_runtime::abstract::backend::LegacyFlowsFromMethodsRuntime::release_use(use);
        }));
    }


    bool save_tasks = true;
    std::deque<task_unique_ptr> registered_tasks;
    std::deque<task_collection_unique_ptr> task_collections;
    std::deque<std::unique_ptr<use_t>> backend_owned_uses;
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
  : public darma_runtime::abstract::backend::TaskCollectionContextHandle
{
  public:
    MOCK_CONST_METHOD0(get_backend_index, size_t());
};

} // end namespace mock_backend

#include "mock_free_functions.h"

#endif /* SRC_TESTS_FRONTEND_VALIDATION_MOCK_BACKEND_H_ */
