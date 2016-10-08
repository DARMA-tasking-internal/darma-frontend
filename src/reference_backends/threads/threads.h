/*
//@HEADER
// ************************************************************************
//
//                         threads.h
//                           darma
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
// JL => Jonathan Lifflander (jliffla@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#if !defined(_THREADS_BACKEND_RUNTIME_H_)
#define _THREADS_BACKEND_RUNTIME_H_

#include <thread>
#include <atomic>
#include <mutex>

#include <iostream>
#include <deque>
#include <vector>
#include <list>
#include <utility>
#include <memory>
#include <unordered_map>
#include <cstring>

#include <threads_interface.h>
#include <common.h>
#include <trace.h>

namespace std {
  using darma_runtime::detail::SimpleKey;
  using darma_runtime::detail::key_traits;

  template<>
  struct hash<SimpleKey> {
    size_t operator()(SimpleKey const& in) const {
      return key_traits<SimpleKey>::hasher()(in);
    }
  };
}

namespace threads_backend {
  using namespace darma_runtime;
  using namespace darma_runtime::abstract::backend;

  struct InnerFlow;
  struct ThreadsFlow;

  struct GraphNode;
  // template <typename TaskType>
  // struct TaskNode;
  struct FetchNode;
  struct PublishNode;
  struct CollectiveNode;

  enum CollectiveType {
    AllReduce = 0
  };
}

namespace std {
  template<>
  struct hash<threads_backend::CollectiveType> {
    size_t operator()(threads_backend::CollectiveType const& in) {
      return std::hash<
        std::common_type_t<size_t, threads_backend::CollectiveType>
      >()(in);
    }
  };
}

namespace threads_backend {
  using flow_t = darma_runtime::types::flow_t;

  void global_produce();
  void global_consume();

  struct CollectiveState {
    size_t n_pieces{0};
    std::atomic<size_t> current_pieces{0};
    void* cur_buf = nullptr;
    std::list<std::shared_ptr<CollectiveNode>> activations;
  };

  struct PackedDataBlock {
    virtual void *get_data() { return data_; }
    size_t size_;
    void *data_ = nullptr;
  };

  struct SerializationPolicy :
    darma_runtime::abstract::backend::SerializationPolicy
  {
    virtual std::size_t
    packed_size_contribution_for_blob(void const* data_begin,
                                      std::size_t n_bytes) const override {
      return n_bytes;
    }

    virtual void
    pack_blob(void*& indirect_pack_buffer,
              void const* data_begin,
              std::size_t n_bytes) const override {
      std::memcpy(indirect_pack_buffer, data_begin, n_bytes);
      (char*&)indirect_pack_buffer += n_bytes;
    }

    virtual void
    unpack_blob(void*& indirect_packed_buffer,
                void* dest,
                std::size_t n_bytes) const override {
      std::memcpy(dest, indirect_packed_buffer, n_bytes);
      (char*&)indirect_packed_buffer += n_bytes;
    }
  };

  struct DelayedPublish {
    using handle_t = Runtime::handle_t;

    std::shared_ptr<InnerFlow> flow;
    std::shared_ptr<handle_t const> handle;
    void* data_ptr;
    size_t fetchers;
    types::key_t key;
    types::key_t version;
    bool finished;
    std::shared_ptr<DataStoreHandle> data_store;
  };

  struct CollectiveInfo {
    using handle_t = Runtime::handle_t;

    std::shared_ptr<InnerFlow> flow, flow_out;
    CollectiveType type;
    types::key_t tag;
    abstract::frontend::ReduceOp const* op;
    size_t this_piece;
    size_t num_pieces;
    void* data_ptr_in;
    void* data_ptr_out;
    std::shared_ptr<handle_t const> handle;
    bool incorporated_local;
    std::shared_ptr<CollectiveNode> node;
  };

  class ThreadsRuntime
    : public abstract::backend::Runtime
    , public abstract::backend::Context
    , public abstract::backend::MemoryManager
    , public ThreadsInterface<ThreadsRuntime> {

  public:
    size_t inside_rank = 0;
    size_t inside_num_ranks = 0;
    runtime_t::task_t* current_task = nullptr;

    ThreadsRuntime(const ThreadsRuntime& tr) = delete;

    std::condition_variable cv_remote_awake{};

    size_t data_store_counter = 0;

    std::unordered_map<
      std::pair<types::key_t, types::key_t>,
      std::shared_ptr<DataBlock>
    > data, fetched_data;

    top_level_task_unique_ptr top_level_task;

    std::deque<std::shared_ptr<GraphNode>> ready_local;

    std::mutex lock_remote{};
    std::deque<std::shared_ptr<GraphNode>> ready_remote;

    virtual size_t
    get_execution_resource_count(size_t depth) const;

    // used for tracing to follow the dependency back to the proper
    // traced block
    std::unordered_map<
      std::shared_ptr<InnerFlow>,
      std::shared_ptr<InnerFlow>
    > inverse_alias, task_forwards;

    std::unordered_map<
      darma_runtime::abstract::frontend::Use*,
      size_t
    > publish_uses;

    std::unordered_map<
      std::shared_ptr<InnerFlow>,
      TraceLog*
    > taskTrace;

    bool inScheduler = false;

    TraceModule* trace = nullptr;

    size_t produced = 0, consumed = 0;

    ThreadsRuntime(
      size_t const in_inside_rank,
      size_t const in_inside_num_ranks,
      top_level_task_unique_ptr&& in_top_level_task = nullptr
    );

    virtual ~ThreadsRuntime();

    template <typename TaskType>
    void addTraceDeps(
      TaskNode<TaskType>* node,
      TraceLog* log
    );

    void
    addPublishDeps(PublishNode* node,
                   TraceLog* log);
    void
    addFetchDeps(FetchNode* node,
                 TraceLog* log,
                 TraceLog* pub_log);

    TraceModule*
    getTrace();

    void
    produce();

    void
    consume();

    size_t
    get_spmd_size() const;

    size_t
    get_spmd_rank() const;

    void
    de_serialize(handle_t const* handle,
                 void* packed,
                 void* unpack_to);

    PackedDataBlock*
    serialize(handle_t const* handle,
              void* unpacked);

    void
    add_remote(std::shared_ptr<GraphNode> task);

    void
    add_local(std::shared_ptr<GraphNode> task);

    void
    cleanup_handle(
      std::shared_ptr<InnerFlow> flow
    );

    void
    delete_handle_data(
      handle_t const* handle,
      types::key_t const& version,
      types::key_t const& key,
      bool const fromFetch
    );

    void
    findAddTraceDep(std::shared_ptr<InnerFlow> flow,
                    TraceLog* thisLog);

    template <typename Key>
    Key
    follow(const std::unordered_map<Key, Key>& map,
                           const Key& flow);

    bool
    test_alias_null(std::shared_ptr<InnerFlow> flow);

    void
    release_to_write(
      std::shared_ptr<InnerFlow> flow
    );

    bool
    finish_read(
      std::shared_ptr<InnerFlow> flow
    );

    void
    create_next_subsequent(
      std::shared_ptr<InnerFlow> flow
    );

    template <typename NodeType>
    bool
    add_reader_to_flow(
      std::shared_ptr<NodeType> node,
      std::shared_ptr<InnerFlow> flow
    );

    void
    transition_after_read(
      std::shared_ptr<InnerFlow> flow
    );

    void
    transition_after_write(
      std::shared_ptr<InnerFlow> f_in,
      std::shared_ptr<InnerFlow> f_out
    );

    bool
    flow_has_alias(
      std::shared_ptr<InnerFlow> flow
    );

    std::tuple<
      std::shared_ptr<InnerFlow>,
      bool
    >
    try_release_alias_to_read(
      std::shared_ptr<InnerFlow> flow
    );

    bool
    try_release_to_read(std::shared_ptr<InnerFlow> flow);

    size_t
    count_ready_work() const;

    void
    schedule_over_breadth();

    virtual void
    register_task(
      types::unique_ptr_template<runtime_t::task_t>&& task
    );

    bool
    register_condition_task(
      condition_task_unique_ptr&& task
    );

    void
    reregister_migrated_use(darma_runtime::abstract::frontend::Use* u);

    template <typename TaskType>
    size_t
    check_dep_task(
      std::shared_ptr<TaskNode<TaskType>> t
    );

    template <typename TaskType>
    void
    run_task(
      TaskType* task
    );

    virtual runtime_t::task_t*
    get_running_task() const;

    virtual void
    register_use(darma_runtime::abstract::frontend::Use* u);

    virtual void
    register_concurrent_region(
      concurrent_region_task_unique_ptr&& task,
      size_t n_indices, std::shared_ptr<DataStoreHandle> const& data_store = nullptr
    );

    virtual std::shared_ptr<DataStoreHandle>
    make_data_store();

    std::shared_ptr<DataBlock>
    allocate_block(
      std::shared_ptr<handle_t const> handle,
      bool fromFetch = false
    );

    virtual flow_t
    make_initial_flow(
      std::shared_ptr<handle_t> const& handle
    );

    bool
    add_fetcher(
      std::shared_ptr<FetchNode> fetch,
      handle_t* handle,
      types::key_t const& version_key,
      std::shared_ptr<DataStoreHandle> const& store
    );

    bool
    try_fetch(
      handle_t* handle,
      types::key_t const& version_key,
      std::shared_ptr<DataStoreHandle> const& store
    );

    bool
    test_fetch(
      handle_t* handle,
      types::key_t const& version_key,
      std::shared_ptr<DataStoreHandle> const& store
    );

    void
    blocking_fetch(
      handle_t* handle,
      types::key_t const& version_key,
      std::shared_ptr<DataStoreHandle> const& store
    );

    TraceLog*
    fetch(
      handle_t* handle,
      types::key_t const& version_key,
      std::shared_ptr<DataStoreHandle> const& store
    );

    virtual flow_t
    make_fetching_flow(
      std::shared_ptr<handle_t> const& handle,
      types::key_t const& version_key,
      std::shared_ptr<DataStoreHandle> const& data_store
    );

    virtual flow_t
    make_null_flow(
      std::shared_ptr<handle_t> const& handle
    );

    virtual flow_t
    make_forwarding_flow(flow_t& from);

    virtual flow_t
    make_next_flow(flow_t& from);

    virtual void
    establish_flow_alias(flow_t& from,
                         flow_t& to);

    virtual void
    release_flow(
      flow_t& to_release
    );

    virtual void
    release_use(darma_runtime::abstract::frontend::Use* u);

    virtual void
    allreduce_use(darma_runtime::abstract::frontend::Use* use_in,
                  darma_runtime::abstract::frontend::Use* use_out,
                  darma_runtime::abstract::frontend::CollectiveDetails const* details,
                  types::key_t const& tag);

    bool
    collective(std::shared_ptr<CollectiveInfo> info);

    void
    blocking_collective(std::shared_ptr<CollectiveInfo> info);

    void
    collective_finish(std::shared_ptr<CollectiveInfo> info);

    bool
    test_publish(std::shared_ptr<DelayedPublish> publish);

    void
    publish(std::shared_ptr<DelayedPublish> publish,
            TraceLog* const log);

    void
    publish_finished(std::shared_ptr<DelayedPublish> publish);

    virtual void
    publish_use(darma_runtime::abstract::frontend::Use* f,
		darma_runtime::abstract::frontend::PublicationDetails* details);

    template <typename Node>
    void
    try_node(std::list<std::shared_ptr<Node>>& nodes);

    template <typename Node>
    bool schedule_from_deque(std::mutex* lock, std::deque<Node>& nodes);

    template <typename Node>
    void shuffle_deque(std::mutex* lock, std::deque<Node>& nodes);

    template <typename... Args>
    void try_release(Args... args);

    void
    schedule_next_unit();

    virtual void
    finalize();

    virtual void*
    allocate(size_t n_bytes,
             abstract::frontend::MemoryRequirementDetails const& details) {
      return malloc(n_bytes);
    }

    virtual void
    deallocate(void* ptr, size_t n_bytes) {
      free(ptr);
    }
  };
}

#endif /* _THREADS_BACKEND_RUNTIME_H_ */
