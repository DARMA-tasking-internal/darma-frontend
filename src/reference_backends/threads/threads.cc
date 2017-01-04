/*
//@HEADER
// ************************************************************************
//
//                         threads.cc
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

#if !defined(_THREADS_BACKEND_RUNTIME_)
#define _THREADS_BACKEND_RUNTIME_

#include <darma/interface/frontend.h>

#ifndef DARMA_HAS_FRONTEND_TYPES_H
#include <darma.h>
#endif

#include <darma/interface/backend/flow.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/defaults/darma_main.h>
#include "common.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <iostream>
#include <deque>
#include <vector>
#include <list>
#include <utility>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <cstdlib>

#include <common.h>
#include <threads.h>
#include <node.h>
#include <publish.h>
#include <flow.h>

namespace threads_backend {
  using namespace darma_runtime;
  using namespace darma_runtime::abstract::backend;

  using flow_t = darma_runtime::types::flow_t;

  std::vector<std::thread> live_ranks;
  std::vector<ThreadsRuntime*> shared_ranks;
  std::atomic<int> terminate_counter = {0};

  #if __THREADS_DEBUG_MODE__
    __thread size_t flow_label = 100;
  #endif

  // tracing TL state for unique labeling
  __thread size_t task_label = 1;
  __thread size_t publish_label = 1;
  __thread size_t fetch_label = 1;

  __thread ThreadsRuntime* cur_runtime = nullptr;

  size_t start_barrier = 0;
  std::condition_variable cv_start_barrier{};
  std::mutex mutex_start_barrier{};

  std::atomic<size_t> global_produced{};
  std::atomic<size_t> global_consumed{};

  void global_produce() { global_produced++; }
  void global_consume() { global_consumed++; }

  // global
  size_t n_ranks = 1;
  bool traceMode = false;
  size_t bwidth = 100;

  // global publish
  std::mutex rank_publish;
  std::unordered_map<
    std::tuple<CollectionID, types::key_t, types::key_t>,
    PublishedBlock*
  > published;

  // global collective
  std::mutex rank_collective;
  std::unordered_map<
    std::pair<CollectiveType, types::key_t>,
    CollectiveState
  > collective_state;

  ThreadsRuntime::ThreadsRuntime(
    size_t const in_inside_rank,
    size_t const in_inside_num_ranks,
    top_level_task_unique_ptr&& in_top_level_task
  )
    : produced(0)
    , consumed(0)
    , inside_rank(in_inside_rank)
    , inside_num_ranks(in_inside_num_ranks)
    , top_level_task(in_top_level_task ? std::move(in_top_level_task) : nullptr)
  {
    #if __THREADS_BACKEND_DEBUG__ || __THREADS_BACKEND_SHUFFLE__
      //srand48(2918279);
      srand48(time(NULL));
    #endif
    trace = traceMode ? new TraceModule{inside_rank,n_ranks,"base"} : nullptr;
  }

  /*virtual*/
  ThreadsRuntime::~ThreadsRuntime() {
    DEBUG_PRINT("TERMIANTED: calling the destructor on runtime\n");
  }

  void
  ThreadsRuntime::produce() { this->produced++; }

  void
  ThreadsRuntime::consume() { this->consumed++; }

  TraceModule*
  ThreadsRuntime::getTrace() { return trace; }

  size_t
  ThreadsRuntime::get_spmd_size() const {
    return inside_num_ranks;
  }

  /*virtual*/
  size_t
  ThreadsRuntime::get_execution_resource_count(size_t depth) const {
    return inside_num_ranks;
  }

  size_t
  ThreadsRuntime::get_spmd_rank() const {
    return inside_rank;
  }

  size_t
  ThreadsRuntime::count_ready_work() const {
    return ready_local.size();
  }

  void
  ThreadsRuntime::add_remote(std::shared_ptr<GraphNode> task) {
    // this may be called from other threads
    {
      std::lock_guard<std::mutex> lock(lock_remote);
      cv_remote_awake.notify_one();
      ready_remote.push_back(task);
    }
  }

  void
  ThreadsRuntime::add_local(std::shared_ptr<GraphNode> task) {
    ready_local.push_back(task);
  }

  void
  ThreadsRuntime::schedule_over_breadth() {
    DEBUG_PRINT("schedule_over_breadth: prod=%ld, cons=%ld, bwidth=%ld\n",
                this->produced,
                this->consumed,
                bwidth);
    // ensure that the scheduler does not reenter recursively, which will stack
    // overflow
    if (!inScheduler &&
        this->produced - this->consumed > bwidth) {
      schedule_next_unit();
    }
  }

  /*virtual*/
  void
  ThreadsRuntime::register_task(
    types::unique_ptr_template<runtime_t::task_t>&& task
  ) {
    DEBUG_PRINT("register task\n");

    auto t = std::make_shared<TaskNode<task_t>>(
      this,std::move(task)
    );

    create_task(t);
  }

  template <typename TaskNode>
  void
  ThreadsRuntime::create_task(
    std::shared_ptr<TaskNode> t, int rank
  ) {
    t->join_counter = check_dep_task(t);

    DEBUG_PRINT("task check_dep results: jc=%ld\n", t->join_counter);

    if (t->ready()) {
      if (rank == -1) {
        ready_local.push_back(t);
      } else {
        shared_ranks[rank]->add_remote(t);
      }
    }

    schedule_over_breadth();
  }

  bool
  ThreadsRuntime::register_condition_task(
    condition_task_unique_ptr&& task
  ) {
    DEBUG_PRINT("register condition task\n");

    auto t = std::make_shared<TaskConditionNode<condition_task_t>>(
      this, std::move(task)
    );

    t->join_counter = check_dep_task(t);

    DEBUG_PRINT(
      "register condition task: jc=%ld, ready=%s\n",
      t->join_counter, PRINT_BOOL_STR(t->ready())
    );

    while (!t->ready()) {
      schedule_next_unit();
    }

    DEBUG_PRINT(
      "register condition task (after loop): jc=%ld, ready=%s\n",
      t->join_counter, PRINT_BOOL_STR(t->ready())
    );

    assert(t->ready());

    t->execute();
    t->cleanup();

    auto const ret = t->get_result<bool>();

    DEBUG_PRINT(
      "register condition task (after loop): ret=%s\n",
      PRINT_BOOL_STR(ret)
    );

    return ret;
  }

  void
  ThreadsRuntime::reregister_migrated_use(use_t* u) {
    assert(false);
  }

  template <typename Key>
  Key
  ThreadsRuntime::follow(const std::unordered_map<Key, Key>& map,
                         const Key& flow) {
    if (map.find(flow) != map.end()) {
      return follow(map, map.find(flow)->second);
    } else {
      return flow;
    }
  }

  void
  ThreadsRuntime::addFetchDeps(FetchNode* node,
                               TraceLog* thisLog,
                               TraceLog* pub_log) {
    if (pub_log) {
      const auto& end = std::atomic_load<TraceLog*>(&pub_log->end);
      const auto& time = end != nullptr ? end->time : pub_log->time;
      const auto& entry = thisLog->entry;
      auto dep = getTrace()->depCreate(time,entry);

      dep->rank = inside_rank;
      dep->event = thisLog->event;
      thisLog->rank = pub_log->rank;
      pub_log->insertDep(dep);
    }
  }

  void
  ThreadsRuntime::addPublishDeps(PublishNode* node,
                                 TraceLog* thisLog) {
    const auto& flow = node->pub->flow;
    findAddTraceDep(flow,thisLog);
  }

  void
  ThreadsRuntime::findAddTraceDep(std::shared_ptr<InnerFlow> flow,
                                  TraceLog* thisLog) {
    // find dependency for flow
    if (taskTrace.find(flow) != taskTrace.end()) {
      std::shared_ptr<InnerFlow> prev = flow;

      if (inverse_alias.find(flow) !=
          inverse_alias.end()) {
        const auto& inverse = follow(inverse_alias, flow);
        prev = inverse;
        DEBUG_TRACE("f_in=%ld, inverse alias=%ld\n",
                    PRINT_LABEL_INNER(flow),
                    PRINT_LABEL_INNER(inverse));
      }

      assert(taskTrace.find(prev) != taskTrace.end());

      const auto& parent = taskTrace[prev];
      const auto& end = std::atomic_load<TraceLog*>(&parent->end);
      auto dep = getTrace()->depCreate(end != nullptr ? end->time : parent->time,
                                       thisLog->entry);
      dep->event = thisLog->event;
      parent->insertDep(dep);
    } else if (task_forwards.find(flow) !=
               task_forwards.end()) {
      assert(taskTrace.find(task_forwards[flow]->next) != taskTrace.end());
      const auto& parent = taskTrace[task_forwards[flow]->next];
      const auto& end = std::atomic_load<TraceLog*>(&parent->end);
      auto dep = getTrace()->depCreate(end != nullptr ? end->time : parent->time,
                                       thisLog->entry);
      dep->event = thisLog->event;
      parent->insertDep(dep);
    }
  }

  template <typename TaskType>
  void
  ThreadsRuntime::addTraceDeps(TaskNode<TaskType>* node,
                               TraceLog* thisLog) {
    for (auto&& dep : node->task->get_dependencies()) {
      auto const f_in  = dep->get_in_flow();
      auto const f_out = dep->get_out_flow();

      // create new trace dependency
      if (f_in != f_out) {
        taskTrace[f_out] = thisLog;
      }

      findAddTraceDep(f_in,thisLog);
    }
  }

  void
  ThreadsRuntime::add_fetch_node_flow(
    std::shared_ptr<InnerFlow> f_in
  ) {
    if (f_in->isFetch && !f_in->fetcherAdded) {
      auto node = std::make_shared<FetchNode>(this,f_in);
      node->acquire = f_in->acquire;
      DEBUG_PRINT(
        "add_fetch_node_flow: handle=%p\n", f_in->handle.get()
      );
      const bool ready = add_fetcher(
        node,
        f_in->handle.get(),
        f_in->version_key,
        f_in->cid
      );
      if (ready) {
        DEBUG_PRINT("check_dep_task: adding fetch node to deque\n");
        ready_local.push_back(node);
      }
    }
  }

  template <typename TaskType>
  size_t
  ThreadsRuntime::check_dep_task(
    std::shared_ptr<TaskType> t
  ) {
    DEBUG_PRINT("check_dep_task\n");

    size_t dep_count = 0;

    for (auto&& dep : t->task->get_dependencies()) {
      auto const f_in  = dep->get_in_flow();
      auto const f_out = dep->get_out_flow();

      DEBUG_PRINT("check_dep_task dep f_in=%ld\n", PRINT_LABEL(f_in));

      add_fetch_node_flow(f_in);

      DEBUG_PRINT("check_dep_task: f_in=%lu (ready=%s), f_out=%lu\n",
                  PRINT_LABEL(f_in),
                  PRINT_BOOL_STR(f_in->ready),
                  PRINT_LABEL(f_out));

      auto const flows_match = f_in == f_out;

      if (flows_match) {
        auto const flow_in_reading = add_reader_to_flow(
          t,
          f_in
        );

        if (!flow_in_reading) {
          dep_count++;
        }
      } else {
        if (f_in->state == FlowWaiting ||
            f_in->state == FlowReadReady) {
          f_in->node = t;
          dep_count++;
        }
        if (f_in->state == FlowScheduleOnly) {
          if (!f_in->scheduleOnlyNeeded) {
            f_in->node = t;
            dep_count++;
          }
        }
      }
    }

    return dep_count;
  }

  template <typename NodeType>
  bool
  ThreadsRuntime::add_reader_to_flow(
    std::shared_ptr<NodeType> node,
    std::shared_ptr<InnerFlow> flow
  ) {
    flow->readers_jc++;

    DEBUG_PRINT("add_reader_to_flow: %ld, state=%s\n",
                PRINT_LABEL(flow),
                PRINT_STATE(flow));

    if (flow->state == FlowWaiting ||
        flow->state == FlowScheduleOnly ||
        flow->state == FlowWriteReady) {
      flow->readers.push_back(node);
      // shared is incremented when readers are released
      return false;
    } else {
      assert(flow->state == FlowReadReady ||
             flow->state == FlowReadOnlyReady);
      assert(flow->shared_reader_count != nullptr);
      (*flow->shared_reader_count)++;
      // readers already released

      DEBUG_PRINT("add_reader_to_flow: %ld INCREMENT shared\n",
                  PRINT_LABEL_INNER(flow));

      return true;
    }
  }

  template <typename TaskType>
  void
  ThreadsRuntime::run_task(
    TaskType* in_task
  ) {
    auto* prev = current_task;
    current_task = in_task;
    in_task->run();
    current_task = prev;
  }

  /*virtual*/
  runtime_t::task_t*
  ThreadsRuntime::get_running_task() const {
    DEBUG_VERBOSE_PRINT("get running task\n");
    return current_task;
  }

  void
  ThreadsRuntime::assign_data_ptr(
    use_t* u, std::shared_ptr<DataBlock> data_block
  ) {
    auto f_in  = u->get_in_flow();
    auto f_out = u->get_out_flow();

    f_in->data_block = data_block;
    f_out->data_block = data_block;

    u->get_data_pointer_reference() = data_block->data;

    f_in->shared_reader_count = &data_block->shared_ref_count;
    f_out->shared_reader_count = &data_block->shared_ref_count;
  }

  template <typename DataMap>
  void
  ThreadsRuntime::set_up_data(
    use_t* u, std::shared_ptr<handle_t const> handle, DataMap& data,
    types::key_t const& key, types::key_t const& version, CollectionID const& cid
  ) {
    auto f_in  = u->get_in_flow();
    auto f_out = u->get_out_flow();
    auto const lookup = std::make_tuple(cid,version,key);
    auto const search_iter = data.find(lookup);
    auto const data_exists = search_iter != data.end();

    if (data_exists) {
      auto data_block = search_iter->second;
      assign_data_ptr(u, data_block);
    } else {
      auto data_block = allocate_block(handle);
      data[lookup] = data_block;
      assign_data_ptr(u, data_block);
    }

    auto const& data_block = data[lookup];

    DEBUG_PRINT(
      "set_up_data: ptr=%p, key=%s, version=%s\n",
      data_block->data, PRINT_KEY(key), PRINT_KEY(f_in->version_key)
    );
  }

  /*virtual*/
  void
  ThreadsRuntime::register_use(use_t* u) {
    auto f_in  = u->get_in_flow();
    auto f_out = u->get_out_flow();

    auto handle = u->get_handle();
    auto const& key = handle->get_key();
    auto const version = f_in->version_key;
    auto const& cid = f_in->cid;

    bool const ready = f_in->ready;

    auto flows_same = f_in == f_out;

    if (!flows_same &&
        u->immediate_permissions() == abstract::frontend::Use::Permissions::None) {
      f_out->state = FlowScheduleOnly;
    }

    if (u->immediate_permissions() == abstract::frontend::Use::Permissions::None) {
      f_in->scheduleOnlyNeeded = true;
    } else {
      f_in->scheduleOnlyNeeded = false;
    }

    DEBUG_PRINT(
      "%p: register use: ready=%s, key=%s, version=%s, "
      "handle=%p [in={%ld,ref=%ld,state=%s,son=%s},out={%ld,ref=%ld,state=%s,son=%s}], "
      "sched=%d, immed=%d, fromFetch=%s\n",
      u, PRINT_BOOL_STR(ready), PRINT_KEY(key), PRINT_KEY(version), handle.get(),
      PRINT_LABEL(f_in), f_in->ref, PRINT_STATE(f_in), PRINT_BOOL_STR(f_in->scheduleOnlyNeeded),
      PRINT_LABEL(f_out), f_out->ref, PRINT_STATE(f_out), PRINT_BOOL_STR(f_out->scheduleOnlyNeeded),
      u->scheduling_permissions(), u->immediate_permissions(),
      PRINT_BOOL_STR(f_in->fromFetch)
    );

    if (f_in->isForward) {
      auto const flows_match = f_in == f_out;
      if (!f_in->writeForwardSet) {
        f_in->isWriteForward = !flows_match;
        f_in->writeForwardSet = true;
      }
    }

    f_in->uses++;
    f_out->uses++;

    if (not flows_same and f_in->is_indexed) {
      auto const from = f_in->collection;
      std::lock_guard<std::mutex> lg1(from->collection_mutex);
      auto const backend_index = f_in->collection_index;
      from->collection_child[backend_index] = std::make_pair(f_out,f_in);
      DEBUG_PRINT(
        "register_use: setting collection child index=%lu, from=%ld, "
        "fst=%ld, snd=%ld\n",
        backend_index, PRINT_LABEL(from), PRINT_LABEL(f_out), PRINT_LABEL(f_in)
      );
      f_in->next = f_out;
    }

    if (!f_in->fromFetch) {
      auto const data_exists = data.find(std::make_tuple(cid,version,key)) != data.end();
      if (!data_exists && f_in->is_indexed && !f_in->is_initial) {
        f_in->perform_transfer = true;
      } else {
        set_up_data(u, handle, data, key, version, cid);
      }
    } else {
      set_up_data(u, handle, fetched_data, key, version, cid);
    }

    // save keys
    f_in->key = key;
    f_out->key = key;

    DEBUG_PRINT(
      "flow {%ld,%s}, shared_reader_count=%p [%ld]\n",
      PRINT_LABEL(f_in), PRINT_STATE(f_in),
      f_in->shared_reader_count,
      f_in->shared_reader_count ? *f_in->shared_reader_count : -1
    );
  }

  std::shared_ptr<DataBlock>
  ThreadsRuntime::allocate_block(
    std::shared_ptr<handle_t const> handle,
    bool fromFetch
  ) {
    // allocate some space for this object
    const size_t sz = handle->get_serialization_manager()->get_metadata_size();
    auto data_block = new DataBlock(1, sz);
    auto block = std::shared_ptr<DataBlock>(data_block, [handle,this](DataBlock* block) {
      DEBUG_PRINT(
        "DataBlock deleter running, block data=%p, calling destructor from allocate\n",
        block->data
      );

      handle
        ->get_serialization_manager()
        ->destroy(block->data);
      delete block;
    });

    DEBUG_PRINT("allocating data block: size=%ld, ptr=%p, block=%p\n",
                sz,
                block->data,
                block.get());

    if (!fromFetch) {
      // call default constructor for data block
      handle
        ->get_serialization_manager()
        ->default_construct(block->data);
    }

    return block;
  }

  void
  ThreadsRuntime::assign_key(
    std::shared_ptr<handle_t> const& handle
  ) {
    // For now, cram things into a uint64_t
    assert(inside_rank < std::numeric_limits<uint16_t>::max());
    assert(assigned_key_offset < (1ull<<48ull));
    handle->set_key(
      darma_runtime::detail::key_traits<darma_runtime::types::key_t>::backend_maker()(
        ((uint64_t)(inside_rank) << 48ull) ^ (uint64_t)(assigned_key_offset++)
      )
    );
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_initial_flow(
    std::shared_ptr<handle_t> const& handle
  ) {
    if(not handle->has_user_defined_key()) {
      assign_key(handle);
    }
    auto f = std::shared_ptr<InnerFlow>(new InnerFlow(handle), [this](InnerFlow* flow){
      DEBUG_PRINT("make_initial_flow: deleter running %ld\n",
                  PRINT_LABEL(flow));
      delete flow;
    });
    f->state = FlowReadReady;
    f->ready = true;
    f->handle = handle;
    f->is_initial = true;
    return f;
  }

  PackedDataBlock*
  ThreadsRuntime::serialize(
    handle_t const* handle,
    void* unpacked
  ) {
    auto policy = std::make_unique<SerializationPolicy>();

    const size_t size = handle
      ->get_serialization_manager()
      ->get_packed_data_size(unpacked,
                             policy.get());

    auto block = new PackedDataBlock();

    // set data block for published block
    block->data_ = malloc(size);
    block->size_ = handle
      ->get_serialization_manager()
      ->get_metadata_size();

    // call pack method to put it inside the allocated buffer
    handle
      ->get_serialization_manager()
      ->pack_data(unpacked,
                  block->data_,
                  policy.get());

    return block;
  }

  void
  ThreadsRuntime::de_serialize(
    handle_t const* handle,
    void* packed,
    void* unpack_to
  ) {
    auto policy = std::make_unique<SerializationPolicy>();

    handle
      ->get_serialization_manager()
      ->unpack_data(unpack_to,
                    packed,
                    policy.get());
  }

  bool
  ThreadsRuntime::add_fetcher(
    std::shared_ptr<FetchNode> fetch,
    handle_t* handle,
    types::key_t const& version_key,
    CollectionID cid
  ) {
    bool ready = false;
    {
      // TODO: unscalable lock to handling fetching
      std::lock_guard<std::mutex> guard(threads_backend::rank_publish);

      auto const& cid = fetch->fetch->cid;
      auto const& key = handle->get_key();
      auto const& iter = published.find(std::make_tuple(cid,version_key,key));
      bool const found = iter != published.end();
      bool const avail = found && std::atomic_load<bool>(&iter->second->ready);

      fetch->fetch->fetcherAdded = true;

      DEBUG_PRINT(
        "add_fetcher: key=%s, version=%s, found=%s, avail=%s, collection=%ld, index=%ld\n",
        PRINT_KEY(key), PRINT_KEY(version_key),
        PRINT_BOOL_STR(found), PRINT_BOOL_STR(avail),
        cid.collection, cid.index
      );

      if (!found) {
        auto pub = new PublishedBlock();
        pub->waiting.push_front(fetch);
        published[std::make_tuple(cid,version_key,key)] = pub;
      } else if (found && !avail) {
        published[std::make_tuple(cid,version_key,key)]->waiting.push_front(fetch);
      }

      ready = avail;
    }
    return ready;
  }

  bool
  ThreadsRuntime::test_fetch(
    handle_t* handle,
    types::key_t const& version_key,
    CollectionID cid
  ) {
    bool ready = false;
    {
      // TODO: unscalable lock to handling fetching
      std::lock_guard<std::mutex> guard(threads_backend::rank_publish);

      const auto& key = handle->get_key();
      const auto& iter = published.find(std::make_tuple(cid,version_key,key));

      DEBUG_PRINT("test_fetch: trying to find a publish, key=%s, version=%s\n",
                  PRINT_KEY(key),
                  PRINT_KEY(version_key));

      ready =
        iter != published.end() &&
        std::atomic_load<bool>(&iter->second->ready);
    }
    return ready;
  }

  TraceLog*
  ThreadsRuntime::fetch(
    handle_t* handle,
    types::key_t const& version_key,
    CollectionID cid
  ) {
    {
      // TODO: big lock to handling fetching
      std::lock_guard<std::mutex> guard(threads_backend::rank_publish);

      const auto& key = handle->get_key();
      const auto& iter = published.find(std::make_tuple(cid,version_key,key));

      DEBUG_PRINT("fetch: trying to find a publish, assume existance: handle=%p, key=%s, version=%s\n",
                  handle,
                  PRINT_KEY(key),
                  PRINT_KEY(version_key));

      // published block found and ready
      assert(iter != published.end() &&
             std::atomic_load<bool>(&iter->second->ready));

      PublishedBlock* pub_ptr = iter->second;
      auto &pub = *pub_ptr;
      auto traceLog = pub.pub_log;

      auto const buffer_exists = fetched_data.find(std::make_tuple(cid,version_key,key)) != fetched_data.end();
      auto block = buffer_exists ? fetched_data[std::make_tuple(cid,version_key,key)] :
        std::shared_ptr<DataBlock>(new DataBlock(0, pub.data->size_), [handle,this](DataBlock* b) {
          DEBUG_PRINT(
            "DataBlock deleter running, block data=%p, calling destructor\n",
            b->data
          );

          handle
            ->get_serialization_manager()
            ->destroy(b->data);
          delete b;
       });

      if (!buffer_exists) {
        fetched_data[std::make_tuple(cid,version_key,key)] = block;
      } else {
        // call destructor since it was previously default constructed
        handle
          ->get_serialization_manager()
          ->destroy(block->data);
      }

      DEBUG_PRINT(
        "fetch: unpacking data: buffer_exists=%s, handle=%p\n",
        PRINT_BOOL_STR(buffer_exists), handle
      );

      de_serialize(
        handle, pub.data->data_, block->data
      );

      DEBUG_PRINT(
        "fetch: key = %s, version = %s, published data = %p, expected = %ld, data = %p\n",
        PRINT_KEY(key),
        PRINT_KEY(version_key),
        pub.data->data_,
        std::atomic_load<size_t>(&pub.expected),
        block->data
      );

      assert(pub.expected > 0);

      ++(pub.done);
      --(pub.expected);

      DEBUG_PRINT("expected=%ld\n", pub.expected.load());

      if (std::atomic_load<size_t>(&pub.expected) == 0) {
        // remove from publication list
        published.erase(std::make_tuple(cid,version_key,key));
        free(pub_ptr->data->data_);
        delete pub.data;
        delete pub_ptr;
      }

      return traceLog;
    }
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_fetching_flow(
    std::shared_ptr<handle_t> const& handle,
    types::key_t const& version_key,
    bool acquired
  ) {
    DEBUG_VERBOSE_PRINT("make fetching flow\n");

    auto f = std::shared_ptr<InnerFlow>(new InnerFlow(handle), [this](InnerFlow* flow){
      DEBUG_PRINT("make_fetching_flow: deleter running %ld\n",
                  PRINT_LABEL(flow));
      delete flow;
    });

    f->version_key = version_key;
    f->isFetch = true;
    f->fromFetch = true;
    f->state = FlowWaiting;
    f->ready = false;
    f->acquire = true;

    return f;
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_null_flow(
    std::shared_ptr<handle_t> const& handle
 ) {
    DEBUG_VERBOSE_PRINT("make null flow\n");

    auto f = std::shared_ptr<InnerFlow>(new InnerFlow(handle), [this](InnerFlow* flow){
      DEBUG_PRINT("make_null_flow: deleter running %ld\n",
                  PRINT_LABEL(flow));
      delete flow;
    });

    f->isNull = true;

    DEBUG_PRINT("null flow %lu\n", PRINT_LABEL(f));

    return f;
  }

  /*virtual*/
  void
  ThreadsRuntime::release_flow(flow_t& to_release) {
    DEBUG_PRINT("release_flow %ld: state=%s\n",
                PRINT_LABEL(to_release),
                PRINT_STATE(to_release));
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_forwarding_flow(flow_t& f) {
    DEBUG_VERBOSE_PRINT("make forwarding flow\n");

    auto f_forward = std::shared_ptr<InnerFlow>(new InnerFlow(nullptr), [this](InnerFlow* flow){
      DEBUG_PRINT("make_forwarding_flow: deleter running %ld\n",
                  PRINT_LABEL(flow));
      delete flow;
    });

    DEBUG_PRINT(
      "forwarding flow from %lu to %lu, f->next=%ld\n",
      PRINT_LABEL(f), PRINT_LABEL(f_forward), f->next ? PRINT_LABEL(f->next) : -1
    );

    if (f->next != nullptr) {
      f->next->ref++;
      f->next->state = FlowWaiting;
    }

    if (getTrace()) {
      task_forwards[f_forward] = f;
    }

    f->forward = f_forward;

    f_forward->cid = f->cid;
    f_forward->isForward = true;
    f_forward->handle = f->handle;
    f_forward->fromFetch = f->fromFetch;
    return f_forward;
  }

  void
  ThreadsRuntime::create_next_subsequent(
    std::shared_ptr<InnerFlow> f
  ) {
    DEBUG_PRINT(
      "create_next_subsequent: f=%ld, state=%s\n",
      PRINT_LABEL(f), PRINT_STATE(f)
    );

    // creating subsequent allowing release
    if (f->state == FlowReadReady &&
        f->readers_jc == 0 &&
        (f->shared_reader_count == nullptr ||
         *f->shared_reader_count == 0)) {
      // can't have alias if has next subsequent
      assert(flow_has_alias(f) == false);
      release_to_write(f);
      indexed_alias_to_out(nullptr, f);
    }
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_next_flow(flow_t& f) {
    DEBUG_VERBOSE_PRINT("make next flow: (from=%p)\n", from);

    auto f_next = std::shared_ptr<InnerFlow>(new InnerFlow(nullptr), [this](InnerFlow* flow){
      DEBUG_PRINT("make_next_flow: deleter running %ld\n",
                  PRINT_LABEL(flow));
      delete flow;
    });

    f->next = f_next;
    f_next->handle = f->handle;
    f_next->fromFetch = f->fromFetch;
    f_next->version_key = f->version_key;
    f_next->cid = f->cid;

    DEBUG_PRINT("next flow from %lu to %lu\n",
                PRINT_LABEL(f),
                PRINT_LABEL(f_next));

    create_next_subsequent(f);

    return f_next;
  }

  void
  ThreadsRuntime::cleanup_handle(
    std::shared_ptr<InnerFlow> flow
  ) {
    DEBUG_PRINT(
      "cleanup_handle identity: flow=%ld, key=%s, version=%s, "
      "cid.index=%ld, fromFetch=%s, indexed_rank_owner=%d\n",
      PRINT_LABEL(flow), PRINT_KEY(flow->key), PRINT_KEY(flow->version_key),
      flow->cid.index, PRINT_BOOL_STR(flow->fromFetch),
      flow->indexed_rank_owner
    );

    auto const flow_data_owner = flow->indexed_rank_owner;
    if (flow_data_owner != -1 && flow_data_owner != inside_rank) {
      auto const& owning_rt = shared_ranks[flow_data_owner];
      auto delete_node = std::make_shared<DeleteNode>(owning_rt, flow);
      owning_rt->add_remote(delete_node);
    } else {
      delete_handle_data(
        flow->version_key, flow->key, flow->cid, flow->fromFetch
      );
    }
  }

  void
  ThreadsRuntime::delete_handle_data(
    types::key_t const& version, types::key_t const& key,
    CollectionID const& cid, bool const fromFetch
  ) {
    auto& data_store = fromFetch ? fetched_data : data;
    auto data_iter = data_store.find(std::make_tuple(cid, version, key));
    auto found = data_iter != data_store.end();

    if (found) {
      auto const& data_block = data_iter->second->data;

      DEBUG_PRINT(
        "delete_handle_data: map ptr=%p, data ptr=%p, fromFetch=%s, found=%s\n",
        &data_store, data_block, PRINT_BOOL_STR(fromFetch),
        PRINT_BOOL_STR(found)
      );

      data_store.erase(data_iter);
    }
  }

  void
  ThreadsRuntime::indexed_alias_to_out(
    flow_t const& f_in,
    flow_t const& f_alias
  ) {
    DEBUG_PRINT(
      "indexed_alias_to_out: f_in=%ld, f_alias=%ld\n",
      f_in ? PRINT_LABEL(f_in) : -1, PRINT_LABEL(f_alias)
    );

    if (f_alias->is_indexed) {
      auto index = f_alias->collection_index;

      DEBUG_PRINT(
        "indexed_alias_to_out: index=%ld\n", index
      );

      assert(f_alias->collection != nullptr);

      auto next_col = f_alias->collection;
      assert(f_alias->collection->next != nullptr);

      DEBUG_PRINT(
        "indexed_alias_to_out: next_col=%ld\n", PRINT_LABEL(next_col)
      );

      f_alias->indexed_alias_out = true;

      std::lock_guard<std::mutex> lg2(f_alias->collection->next->collection_mutex);
      std::lock_guard<std::mutex> lg1(f_alias->collection->collection_mutex);

      if (next_col != nullptr) {
        DEBUG_PRINT(
          "indexed_alias_to_out: next_col->collection_child[%ld]=%s\n",
          index, next_col->collection_child.find(index) != next_col->collection_child.end() ? "true" : "false"
        );

        if (next_col->collection_child.find(index) != next_col->collection_child.end()) {
          auto next_elm = next_col->collection_child[index];

          DEBUG_PRINT(
            "indexed_alias_to_out: fst=%ld, snd=%ld, has second=%s\n",
            next_elm.first ? PRINT_LABEL(next_elm.first) : -1,
            next_elm.second ? PRINT_LABEL(next_elm.second) : -1,
            next_elm.second ? "true" : "false"
          );

          if (next_elm.second != nullptr) {
            DEBUG_PRINT(
              "indexed_alias_to_out: first_elem=%ld, next_elem=%ld, "
              "next_elm.second->state=%s\n",
              PRINT_LABEL(next_elm.first), PRINT_LABEL(next_elm.second),
              PRINT_STATE(next_elm.second)
            );
            next_elm.second->prev_rank_owner = f_alias->indexed_rank_owner;
            if (next_elm.second->state == FlowWaiting) {
              auto const has_read = try_release_to_read(next_elm.second);
              if (!has_read) {
                release_to_write(next_elm.second);
              }
            } else if (next_elm.second->state == FlowReadReady) {
              release_to_write(next_elm.second);
            }
            next_elm.second->prev = f_alias;
          }
        }
      }
    }
  }

  /*virtual*/
  void
  ThreadsRuntime::establish_flow_alias(flow_t& f_from,
                                       flow_t& f_to) {
    DEBUG_PRINT(
      "establish flow alias %lu (state=%s,ref=%ld) to %lu (state=%s,ref=%ld)\n",
      PRINT_LABEL(f_from), PRINT_STATE(f_from), f_from->ref,
      PRINT_LABEL(f_to), PRINT_STATE(f_to), f_to->ref
    );

    union_find::union_nodes(f_to, f_from);

    if (getTrace()) {
      inverse_alias[f_to] = f_from;
    }

    // creating subsequent allowing release
    if (f_from->state == FlowReadReady &&
        f_from->readers_jc == 0) {
      f_from->state = FlowReadOnlyReady;

      auto const last_found_alias = try_release_alias_to_read(f_from);
      auto const alias_part = std::get<0>(last_found_alias);
      if (std::get<1>(last_found_alias) == false) {
        auto const has_subsequent = alias_part->next != nullptr || flow_has_alias(alias_part);

        DEBUG_PRINT("establish_flow_alias alias_part, %ld in state=%s\n",
                    PRINT_LABEL(alias_part),
                    PRINT_STATE(alias_part));

        if (has_subsequent) {
          release_to_write(alias_part);
          if (alias_part != f_from) {
            indexed_alias_to_out(f_from, alias_part);
          }
        } else {
          DEBUG_PRINT("establish_flow_alias subsequent, %ld in state=%s does not have *subsequent*\n",
                      PRINT_LABEL(alias_part),
                      PRINT_STATE(alias_part));
        }
      }
    } else if (f_from->state == FlowReadReady) {
      f_from->state = FlowReadOnlyReady;
    }
  }

  bool
  ThreadsRuntime::test_alias_null(
    std::shared_ptr<InnerFlow> flow,
    std::shared_ptr<InnerFlow> alias
  ) {
    auto const is_col = flow->is_collection;

    DEBUG_PRINT(
      "test_alias_null: is_collection=%s, flow=%ld, alias=%ld\n",
      PRINT_BOOL_STR(is_col), PRINT_LABEL(flow), PRINT_LABEL(alias)
    );

    if (is_col) {
      assert(alias->is_collection);

      if (flow->prev) {
        std::lock_guard<std::mutex> lg1(flow->prev->collection_mutex);

        for (auto&& c : flow->prev->collection_child) {
          DEBUG_PRINT(
            "test_alias_null: fst=%ld, snd=%ld\n",
            c.second.first ? PRINT_LABEL(c.second.first) : -1,
            c.second.second ? PRINT_LABEL(c.second.second) : -1
          );

          if (c.second.first) {
            cleanup_handle(c.second.first);
          }
          if (c.second.second) {
            cleanup_handle(c.second.second);
          }
        }
      }
    }

    if (alias->isNull) {
      if (flow->shared_reader_count != nullptr &&
          *flow->shared_reader_count == 0) {
        cleanup_handle(flow);
      }
      return true;
    }
    return false;
  }

  bool
  ThreadsRuntime::try_release_to_read(
    std::shared_ptr<InnerFlow> flow
  ) {
    assert(flow->state == FlowWaiting || flow->state == FlowScheduleOnly);
    assert(flow->ref == 0);
    assert(flow->readers.size() == flow->readers_jc);

    DEBUG_PRINT("try_release_to_read: %ld, readers=%ld, reader_jc=%ld, ref=%ld\n",
                PRINT_LABEL_INNER(flow),
                flow->readers.size(),
                flow->readers_jc,
                flow->ref);

    flow->state = flow_has_alias(flow) ? FlowReadOnlyReady : FlowReadReady;

    DEBUG_PRINT("try_release_to_read: %ld, new state=%s\n",
                PRINT_LABEL(flow),
                PRINT_STATE(flow));

    if (flow->readers.size() > 0) {
      for (auto reader : flow->readers) {
        DEBUG_PRINT("releasing READER on flow %ld, readers=%ld\n",
                    PRINT_LABEL_INNER(flow),
                    flow->readers.size());
        reader->release();

        // increment shared count
        assert(flow->shared_reader_count != nullptr);
        (*flow->shared_reader_count)++;

        DEBUG_PRINT("try_release_to_read: %ld INCREMENT shared\n",
                    PRINT_LABEL_INNER(flow));
      }

      DEBUG_PRINT("try_release_to_read: %ld, shared=%ld\n",
                  PRINT_LABEL(flow),
                  (*flow->shared_reader_count));

      flow->readers.clear();
      return true;
    } else {
      return false;
    }
  }

  bool
  ThreadsRuntime::flow_has_alias(
    std::shared_ptr<InnerFlow> flow
  ) {
    return flow->alias != nullptr;
  }

  std::tuple<
    std::shared_ptr<InnerFlow>,
    bool
  >
  ThreadsRuntime::try_release_alias_to_read(
    std::shared_ptr<InnerFlow> flow
  ) {
    assert(
      flow->state == FlowReadReady ||
      flow->state == FlowReadOnlyReady
    );

    DEBUG_PRINT("try_release_alias_to_read: parent flow=%ld, state=%s, alias=%p\n",
                PRINT_LABEL(flow),
                PRINT_STATE(flow),
                flow->alias ? flow->alias.get() : nullptr);

    if (flow_has_alias(flow)) {
      bool has_read_phase = false;
      auto aliased = union_find::find_call(flow, [&](std::shared_ptr<InnerFlow> alias){
        if (alias->isNull) {
          return;
        } else if (alias->state == FlowWaiting) {
          assert(alias->state == FlowWaiting);
          assert(alias->ref > 0);

          alias->ref--;

          assert(alias->ref == 0);

          has_read_phase |= try_release_to_read(alias);
        } else if (alias->state == FlowScheduleOnly) {
          has_read_phase |= try_release_to_read(alias);
        } else {
          assert(
            alias->state == FlowReadOnlyReady ||
            alias->state == FlowReadReady
          );

          assert(alias->shared_reader_count != nullptr);

          auto const has_outstanding_reads =
            alias->readers_jc != 0 && *alias->shared_reader_count == 0;

          has_read_phase |= has_outstanding_reads;
        }
      });

      DEBUG_PRINT("try_release_alias_to_read: aliased flow=%ld, state=%s, ref=%ld\n",
                  PRINT_LABEL(aliased),
                  PRINT_STATE(aliased),
                  aliased->ref);

      if (test_alias_null(flow, aliased)) {
        return std::make_tuple(aliased,false);
      } else {
        return std::make_tuple(aliased, has_read_phase);
      }
    }
    return std::make_tuple(flow,false);
  }

  void
  ThreadsRuntime::release_to_write(
    std::shared_ptr<InnerFlow> flow
  ) {
    assert(flow->state == FlowReadReady);
    assert(flow->ref == 0);
    assert(flow->readers_jc == 0);
    assert(flow->readers.size() == flow->readers_jc);
    assert(flow->shared_reader_count == nullptr ||
           *flow->shared_reader_count == 0);

    DEBUG_PRINT("release_to_write: %ld, readers=%ld, reader_jc=%ld, ref=%ld\n",
                PRINT_LABEL_INNER(flow),
                flow->readers.size(),
                flow->readers_jc,
                flow->ref);

    flow->state = FlowWriteReady;
    flow->ready = true;

    DEBUG_PRINT("release_to_write: %ld, new state=%s\n",
                PRINT_LABEL_INNER(flow),
                PRINT_STATE(flow));

    if (flow->node) {
      flow->node->release();
      flow->node = nullptr;
    }
  }

  bool
  ThreadsRuntime::finish_read(std::shared_ptr<InnerFlow> flow) {
    assert(flow->shared_reader_count != nullptr);
    assert(*flow->shared_reader_count > 0);
    assert(flow->readers_jc > 0);
    assert(flow->ref == 0);
    assert(flow->readers.size() == 0);
    assert(
      flow->state == FlowReadReady ||
      flow->state == FlowReadOnlyReady
    );

    DEBUG_PRINT("finish read: %ld, reader_jc=%ld, ref=%ld, state=%s, shared=%ld\n",
                PRINT_LABEL_INNER(flow),
                flow->readers_jc,
                flow->ref,
                PRINT_STATE(flow),
                *flow->shared_reader_count);

    flow->readers_jc--;
    (*flow->shared_reader_count)--;

    DEBUG_PRINT("finish read: %ld DECREMENT shared\n",
                PRINT_LABEL_INNER(flow));

    return flow->readers_jc == 0 && *flow->shared_reader_count == 0;
  }

  /*virtual*/
  void
  ThreadsRuntime::allreduce_use(
    use_t* use_in,
    use_t* use_out,
    collective_details_t const* details,
    types::key_t const& tag
  ) {

    auto f_in = use_in->get_in_flow();
    auto f_out = use_in->get_out_flow();

    const auto this_piece = details->this_contribution();
    const auto num_pieces = details->n_contributions();

    // TODO: relax this assumption
    // use_in != use_out
    assert(f_in != f_out);

    f_out->isCollective = true;

    auto info = std::make_shared<CollectiveInfo>
      (CollectiveInfo{
        f_in,
        f_out,
        CollectiveType::AllReduce,
        tag,
        details->reduce_operation(),
        this_piece,
        num_pieces,
        //use_in->get_handle()->get_key(),
        //f_out->version_key, // should be the out for use_out
        use_in->get_data_pointer_reference(),
        use_out->get_data_pointer_reference(),
        use_in->get_handle(),
        false
    });

    auto node = std::make_shared<CollectiveNode>(this,info);

    // set the node
    info->node = node;

    f_out->ready = false;
    f_out->state = FlowWaiting;
    f_out->ref++;

    const auto& ready = node->ready();

    DEBUG_PRINT("allreduce_use: ready=%s, flow in=%ld, flow out=%ld\n",
                PRINT_BOOL_STR(ready),
                PRINT_LABEL_INNER(f_in),
                PRINT_LABEL_INNER(f_out));

    publish_uses[use_in]++;

    add_fetch_node_flow(f_in);

    if (f_in->state == FlowWaiting ||
          f_in->state == FlowReadReady) {
      f_in->node = node;
      node->set_join(1);
    } else {
      node->execute();
    }

    // TODO: for the future this should be two-stage starting with a read
    // auto const flow_in_reading = add_reader_to_flow(
    //   node,
    //   f_in
    // );
  }

  bool
  ThreadsRuntime::collective(std::shared_ptr<CollectiveInfo> info) {
    DEBUG_PRINT("collective operation, type=%d, flow in={%ld,state=%s}, "
                "flow out={%ld,state=%s}\n",
                info->type,
                PRINT_LABEL(info->flow),
                PRINT_STATE(info->flow),
                PRINT_LABEL(info->flow_out),
                PRINT_STATE(info->flow_out)
                );

    switch (info->type) {
    case CollectiveType::AllReduce:
      {
        std::pair<CollectiveType,types::key_t> key = {CollectiveType::AllReduce,
                                                      info->tag};

        std::list<std::shared_ptr<CollectiveNode>>* act = nullptr;
        bool finished = false;
        {
          std::lock_guard<std::mutex> guard(threads_backend::rank_collective);

          if (collective_state.find(key) == collective_state.end()) {
            collective_state[key];
            collective_state[key].n_pieces = info->num_pieces;
          }

          auto& state = collective_state[key];

          // make sure that all contributions have the same expectation
          assert(state.n_pieces == info->num_pieces);
          assert(info->incorporated_local == false);

          DEBUG_PRINT("collective: current_pieces = %ld\n",
                      state.current_pieces.load());

          if (state.current_pieces == 0) {
            assert(state.cur_buf == nullptr);

            auto block = serialize(
              info->handle.get(),
              info->data_ptr_in
            );

            const size_t sz = info
              ->handle
              ->get_serialization_manager()
              ->get_metadata_size();

            DEBUG_PRINT("collective: size = %ld\n", sz);

            // TODO: memory leaks here
            state.cur_buf = malloc(sz);

            de_serialize(
              info->handle.get(),
              block->data_,
              state.cur_buf
            );
          } else {
            DEBUG_PRINT("performing op on data\n");

            // TODO: offset might be wrong
            size_t const n_elem = info
              ->handle
              ->get_array_concept_manager()
              ->n_elements(info->data_ptr_in);

            info->op->reduce_unpacked_into_unpacked(
              info->data_ptr_in,
              state.cur_buf,
              0,
              n_elem
            );
          }

          info->incorporated_local = true;
          state.current_pieces++;

          finished = state.current_pieces == state.n_pieces;

          if (!finished) {
            state.activations.push_back(info->node);
          } else {
            act = &state.activations;
          }
        }

        if (finished) {
          collective_finish(info);

          for (auto&& n : *act) {
            n->activate();
          }
          return true;
        }
      }
      break;
    default:
      DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
    }

    return false;
  }

  void
  ThreadsRuntime::blocking_collective(std::shared_ptr<CollectiveInfo> info) {
    switch (info->type) {
    case CollectiveType::AllReduce:
      {
        CollectiveState* state = nullptr;

        std::pair<CollectiveType,types::key_t> key = {CollectiveType::AllReduce,
                                                      info->tag};

      block_try_again:
        {
          std::lock_guard<std::mutex> guard(threads_backend::rank_collective);
          assert(collective_state.find(key) != collective_state.end());
          state = &collective_state[key];

          if (state->current_pieces != state->n_pieces) {
            goto block_try_again;
          }
        }

        assert(state != nullptr);
        assert(state->current_pieces == state->n_pieces);

        auto const& handle = info->handle;

        auto block = serialize(
          handle.get(),
          state->cur_buf
        );

        de_serialize(
          handle.get(),
          block->data_,
          info->data_ptr_out
        );

        delete block;
      }
      break;
    default:
      DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
    }
  }

  void
  ThreadsRuntime::collective_finish(std::shared_ptr<CollectiveInfo> info) {
    switch (info->type) {
    case CollectiveType::AllReduce:
      {
        CollectiveState* state = nullptr;

        std::pair<CollectiveType,types::key_t> key = {
          CollectiveType::AllReduce,
          info->tag
        };

        DEBUG_PRINT("collective finish, type=%d, flow in={%ld,state=%s}, "
                    "flow out={%ld,state=%s}\n",
                    info->type,
                    PRINT_LABEL(info->flow),
                    PRINT_STATE(info->flow),
                    PRINT_LABEL(info->flow_out),
                    PRINT_STATE(info->flow_out)
                    );

        {
          std::lock_guard<std::mutex> guard(threads_backend::rank_collective);
          assert(collective_state.find(key) != collective_state.end());
          state = &collective_state[key];
        }

        assert(state != nullptr);
        assert(state->current_pieces == state->n_pieces);

        auto const& handle = info->handle;

        auto block = serialize(
          handle.get(),
          state->cur_buf
        );

        de_serialize(
          handle.get(),
          block->data_,
          info->data_ptr_out
        );

        delete block;

        info->flow_out->ref--;
        transition_after_write(
          info->flow,
          info->flow_out
        );
      }
      break;
    default:
      DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
    }
  }

  void
  ThreadsRuntime::transition_after_read(
    std::shared_ptr<InnerFlow> flow
  ) {
    assert(
      flow->state == FlowReadReady ||
      flow->state == FlowReadOnlyReady
   );

    DEBUG_PRINT("transition_after_read, current state %ld is state=%s\n",
                PRINT_LABEL(flow),
                PRINT_STATE(flow));

    auto const finishedAllReads = *flow->shared_reader_count == 0 || finish_read(flow);

    if (finishedAllReads) {
      auto const last_found_alias = try_release_alias_to_read(flow);
      auto const alias_part = std::get<0>(last_found_alias);

      if (std::get<1>(last_found_alias) == false) {
        auto const has_subsequent = alias_part->next != nullptr || flow_has_alias(alias_part);
        if (has_subsequent) {
          release_to_write(alias_part);
          if (alias_part != flow) {
            indexed_alias_to_out(flow, alias_part);
          }
        } else {
          DEBUG_PRINT("transition_after_read, %ld in state=%s does not have *subsequent*\n",
                      PRINT_LABEL(alias_part),
                      PRINT_STATE(alias_part));
        }
      }
    }
  }

  void
  ThreadsRuntime::transition_after_write(
    std::shared_ptr<InnerFlow> f_in,
    std::shared_ptr<InnerFlow> f_out
  ) {
    //assert(f_in->state == FlowWriteReady);
    assert(f_out->state == FlowWaiting);

    f_in->state = FlowAntiReady;

    DEBUG_PRINT(
      "transition_after_write, transition in=%ld to state=%s, out ref=%ld\n",
      PRINT_LABEL(f_in), PRINT_STATE(f_in), f_out->ref
    );

    if (f_out->ref == 0) {
      auto const has_read_phase = try_release_to_read(f_out);
      auto const last_found_alias = try_release_alias_to_read(f_out);
      auto const alias_part = std::get<0>(last_found_alias);

      DEBUG_PRINT(
        "transition_after_write, transition out=%ld to last_found_alias=%ld,, "
        "state=%s, last_has_read_phase=%s, has_read_phase=%s, "
        "last_found_alias->next=%ld\n",
        PRINT_LABEL(f_out),
        PRINT_LABEL(std::get<0>(last_found_alias)),
        PRINT_STATE(std::get<0>(last_found_alias)),
        PRINT_BOOL_STR(std::get<1>(last_found_alias)),
        PRINT_BOOL_STR(has_read_phase),
        alias_part->next ? PRINT_LABEL(alias_part->next) : -1
      );

      // out flow from release is ready to go
      if (!has_read_phase &&
          std::get<1>(last_found_alias) == false) {

        auto const has_subsequent = alias_part->next != nullptr || flow_has_alias(alias_part);
        if (has_subsequent) {
          release_to_write(alias_part);
          if (alias_part != f_in) {
            indexed_alias_to_out(f_in, alias_part);
          }
        } else {
          DEBUG_PRINT("transition_after_write, %ld in state=%s does not have *subsequent*\n",
                      PRINT_LABEL(alias_part),
                      PRINT_STATE(alias_part));
        }
      }
    }

    DEBUG_PRINT("transition_after_write, transition out=%ld to state=%s\n",
                PRINT_LABEL(f_out),
                PRINT_STATE(f_out));
  }

  /*virtual*/
  void
  ThreadsRuntime::release_use(use_t* u) {
    auto f_in  = u->get_in_flow();
    auto f_out = u->get_out_flow();

    // enable next forward flow
    if (f_in->forward) {
      f_in->forward->state = f_in->forward->isWriteForward ? FlowWriteReady : FlowReadReady;
      f_in->forward->ready = true;
    }

    auto handle = u->get_handle();
    auto const version = f_in->version_key;
    auto const& key = handle->get_key();
    auto const& cid = f_in->cid;

    DEBUG_PRINT(
      "release use: f_in=[%ld,state=%s], f_out=[%ld,state=%s], handle=%p, "
      "version=%s, key=%s, cid.index=%ld, u=%p, permissions=%d\n",
      PRINT_LABEL(f_in), PRINT_STATE(f_in),
      PRINT_LABEL(f_out), PRINT_STATE(f_out),
      handle.get(), PRINT_KEY(version), PRINT_KEY(key), cid.index, u,
      u->immediate_permissions()
    );

    f_in->uses--;
    f_out->uses--;

    auto const flows_match = f_in == f_out;

    // track release of publish uses so that they do not count toward a read
    // release
    if (publish_uses.find(u) != publish_uses.end()) {
      assert(publish_uses[u] > 0);
      publish_uses[u]--;
      if (publish_uses[u] == 0) {
        publish_uses.erase(publish_uses.find(u));
      }
      return;
    }

    if (flows_match) {
      if (u->immediate_permissions() != abstract::frontend::Use::Permissions::None) {
        transition_after_read(f_out);
      }
    } else {
      if (f_in->uses == 0) {
        if (u->immediate_permissions() == abstract::frontend::Use::Permissions::Modify) {
          transition_after_write(
            f_in,
            f_out
          );
        }
      }
    }
  }

  bool
  ThreadsRuntime::test_publish(
    std::shared_ptr<DelayedPublish> publish
  ) {
    return publish->flow->ready;
  }

  void
  ThreadsRuntime::publish(
    std::shared_ptr<DelayedPublish> publish,
    TraceLog* const log
  ) {
    if (publish->finished) return;

    {
      // TODO: big lock to handling publishing
      std::lock_guard<std::mutex> guard(threads_backend::rank_publish);

      auto const& expected = publish->fetchers;
      auto const& version = publish->version;
      auto const& key = publish->key;
      auto const& cid = publish->collection_id;

      auto handle = publish->handle;
      void* data_ptr = publish->data_ptr;

      DEBUG_PRINT(
        "publish: key = %s, version = %s, data ptr = %p, handle = %p, collection=%ld, index=%ld\n",
        PRINT_KEY(key), PRINT_KEY(version),
        data_ptr, handle.get(),
        cid.collection, cid.index
      );

      assert(expected >= 1);

      auto block = serialize(handle.get(), data_ptr);

      DEBUG_PRINT("publication: key = %s, version = %s, published data = %p, data ptr = %p\n",
                  PRINT_KEY(key),
                  PRINT_KEY(version),
                  block->data_,
                  data_ptr);

      auto const key_tup = std::make_tuple(cid,version,key);
      auto const publish_exists = published.find(key_tup) != published.end();
      auto const& publish = publish_exists ?
        published[key_tup] :
        (published[key_tup] = new PublishedBlock(),
         published[key_tup]);

      publish->expected = expected;
      publish->key = key;
      publish->ready = true;
      publish->data = block;
      publish->pub_log = log;

      if (publish_exists) {
        for (auto pub : publish->waiting) {
          pub->activate();
        }
        publish->waiting.clear();
      }
    }
  }

  void
  ThreadsRuntime::publish_finished(std::shared_ptr<DelayedPublish> publish) {
    DEBUG_PRINT("publish finished, removing from handle %p\n",
                publish->handle.get());
    transition_after_read(
      publish->flow
    );
  }

  /*virtual*/
  void
  ThreadsRuntime::register_task_collection(
    task_collection_unique_ptr&& task_collection
  ) {
    auto cr_task = std::move(task_collection);

    auto task_node = std::make_shared<TaskCollectionNode<task_collection_t>>(
      this, std::move(cr_task),
      inside_rank, inside_num_ranks
    );

    create_task(task_node, inside_rank);
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_initial_flow_collection(
    std::shared_ptr<handle_t> const& handle
  ) {
    auto flow = make_initial_flow(handle);
    flow->is_collection = true;
    flow->cid.collection = next_collection_id++;

    DEBUG_PRINT(
      "make_initial_flow_collection: id=%ld\n", flow->cid.collection
    );

    return flow;
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_null_flow_collection(
    std::shared_ptr<handle_t> const& handle
  ) {
    auto flow = make_null_flow(handle);
    flow->is_collection = true;

    DEBUG_PRINT("make_null_flow_collection\n");

    return flow;
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_next_flow_collection(
    flow_t& from
  ) {
    auto flow = make_next_flow(from);
    from->next = flow;
    flow->is_collection = true;
    flow->prev = from;
    flow->cid.collection = from->cid.collection;

    DEBUG_PRINT("make_next_flow_collection\n");

    return flow;
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_indexed_local_flow(
    flow_t& from,
    size_t backend_index
  ) {
    auto f = std::shared_ptr<InnerFlow>(new InnerFlow(nullptr), [this](InnerFlow* flow){
      DEBUG_PRINT(
        "make_indexed_local_flow: deleter running %ld\n",
        PRINT_LABEL(flow)
      );
      delete flow;
    });

    DEBUG_PRINT(
      "make_indexed_local_flow: flow=%ld, from=%ld, collection=%ld, index=%ld, from state=%s\n",
      PRINT_LABEL(f), PRINT_LABEL(from), from->cid.collection, backend_index,
      PRINT_STATE(from)
    );

    f->is_indexed = true;
    f->collection = from;
    f->cid = CollectionID(from->cid.collection, backend_index);
    f->is_initial = from->is_initial;
    f->indexed_rank_owner = inside_rank;
    f->collection_index = backend_index;

    flow_t sub_other = nullptr;
    {
      std::lock_guard<std::mutex> lg1(from->collection_mutex);

      if (from->is_initial) {
        f->state = from->state;
      }

      // set up next link inside indexed region between in and out flows for
      // correct forwarding
      if (from->prev != nullptr) {
        std::lock_guard<std::mutex> lg2(from->prev->collection_mutex);

        auto other = from->prev->collection_child[backend_index].first;
        if (other) {
          other->next = f;

          DEBUG_PRINT(
            "make_indexed_local_flow: subsequent flow=%ld, collection=%ld, "
            "index=%ld, from=%ld, other=%ld\n",
            PRINT_LABEL(f), from->cid.collection, backend_index,
            PRINT_LABEL(from), PRINT_LABEL(other)
          );

          sub_other = other;
        }
      }
    }

    if (sub_other != nullptr) {
      create_next_subsequent(sub_other);
    }

    std::lock_guard<std::mutex> lg1(from->collection_mutex);

    // set up next link inside indexed region between in and out flows for
    // correct forwarding
    if (from->prev != nullptr) {
      std::lock_guard<std::mutex> lg2(from->prev->collection_mutex);

      auto other = from->prev->collection_child[backend_index].first;

      DEBUG_PRINT(
        "make_indexed_local_flow: flow=%ld, from=%ld, collection=%ld, "
        "index=%ld, from state=%s, prev=%ld\n",
        PRINT_LABEL(f), PRINT_LABEL(from), from->cid.collection, backend_index,
        PRINT_STATE(from), PRINT_LABEL(from->prev)
      );

      if (from->prev->collection_child.find(backend_index) !=
          from->prev->collection_child.end()) {

        DEBUG_PRINT(
          "make_indexed_local_flow: flow=%ld, from=%ld, collection=%ld, "
          "index=%ld, from state=%s, prev fst=%ld, prev snd=%ld\n",
          PRINT_LABEL(f), PRINT_LABEL(from), from->cid.collection, backend_index,
          PRINT_STATE(from),
          from->prev->collection_child[backend_index].first ?
          PRINT_LABEL(from->prev->collection_child[backend_index].first) : -1,
          from->prev->collection_child[backend_index].second ?
          PRINT_LABEL(from->prev->collection_child[backend_index].second) : -1
        );

        if (from->prev->collection_child[backend_index].first != nullptr) {
          auto prev_matching_flow = from->prev->collection_child[backend_index].first;

          DEBUG_PRINT(
            "make_indexed_local_flow: flow=%ld, from=%ld, index=%ld, prev_matching_flow=%ld, "
            "prev_matching_flow->indexed_alias_out=%s\n",
            PRINT_LABEL(f), PRINT_LABEL(from), backend_index, PRINT_LABEL(prev_matching_flow),
            prev_matching_flow->indexed_alias_out ? "true" : "false"
          );

          if (prev_matching_flow != nullptr) {
            f->prev_rank_owner = prev_matching_flow->indexed_rank_owner;

            if (prev_matching_flow->indexed_alias_out) {
              f->state = FlowWriteReady;
              f->prev = prev_matching_flow;
            }

            prev_matching_flow->next = f;
          }
        }
      }
    }

    return f;
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_indexed_fetching_flow(
    flow_t& from,
    types::key_t const& version_key,
    size_t backend_index
  ) {
    auto f = std::shared_ptr<InnerFlow>(new InnerFlow(from->handle), [this](InnerFlow* flow){
      DEBUG_PRINT(
        "make_indexed_fetching_flow: deleter running %ld\n",
        PRINT_LABEL(flow)
      );
      delete flow;
    });

    DEBUG_PRINT(
      "make_indexed_fetching_flow: flow=%ld, from=%ld, collection=%ld, index=%ld, from state=%s\n",
      PRINT_LABEL(f), PRINT_LABEL(from), from->cid.collection, backend_index,
      PRINT_STATE(from)
    );

    f->is_indexed = true;
    f->collection = from;
    f->collection_index = backend_index;
    f->cid = CollectionID(from->cid.collection, backend_index);

    f->version_key = version_key;
    f->isFetch = true;
    f->fromFetch = true;
    f->state = FlowWaiting;
    f->ready = false;
    f->acquire = true;

    return f;
  }

  /*virtual*/
  void
  ThreadsRuntime::publish_use(
    use_t* f,
    pub_details_t* details
  ) {

    auto f_in  = f->get_in_flow();
    auto f_out = f->get_out_flow();
    auto handle = f->get_handle();
    auto version = details->get_version_name();
    auto key = handle->get_key();

    // TODO: do not create this just to tear it down
    auto pub = std::make_shared<DelayedPublish>
      (DelayedPublish{f_in,
          handle,
          f->get_data_pointer_reference(),
          details->get_n_fetchers(),
          key,
          details->get_version_name(),
          false,
          f_in->cid
       });

    auto p = std::make_shared<PublishNode>(this,pub);

    const bool ready = p->ready();

    DEBUG_PRINT(
      "%p: publish_use: shared=%ld, ptr=%p, key=%s, "
      "version=%s, handle=%p, ready=%s, f_in={%lu,%s}, f_out=%lu\n",
      f,
      *f_in->shared_reader_count,
      f->get_data_pointer_reference(),
      PRINT_KEY(key),
      PRINT_KEY(version),
      handle.get(),
      PRINT_BOOL_STR(ready),
      PRINT_LABEL(f_in),
      PRINT_STATE(f_in),
      PRINT_LABEL(f_out)
    );

    assert(f_in == f_out);

    // save that this use is a publish so the release is meaningless
    publish_uses[f]++;

    auto const flow_in_reading = add_reader_to_flow(
      p,
      f_in
    );

    if (flow_in_reading) {
      p->execute();
      p->cleanup();
    } else {
      p->set_join(1);
    }

    schedule_over_breadth();
  }

  template <typename Node>
  void
  ThreadsRuntime::shuffle_deque(
    std::mutex* lock,
    std::deque<Node>& nodes
  ) {
    if (lock) lock->lock();

    if (nodes.size() > 0) {
      DEBUG_PRINT("shuffle deque %p (local = %p): size = %lu\n",
                  &nodes, &ready_local, nodes.size());

      for (auto i = 0; i < nodes.size(); i++) {
        auto const i1 = static_cast<size_t>(drand48() * nodes.size());
        auto const i2 = static_cast<size_t>(drand48() * nodes.size());

        DEBUG_PRINT("shuffle deque i1=%ld, i2=%ld\n", i1, i2);

        if (i1 != i2) {
          std::swap(nodes[i1], nodes[i2]);
        }
      }
    }

    if (lock) lock->unlock();
  }

  template <typename Node>
  bool
  ThreadsRuntime::schedule_from_deque(
    std::mutex* lock,
    std::deque<Node>& nodes
  ) {
    if (lock) lock->lock();

    if (nodes.size() > 0) {
      DEBUG_PRINT("scheduling from deque %p (local = %p): size = %lu, prod=%ld, cons=%ld\n",
                  &nodes, &ready_local, nodes.size(),
                  this->produced,
                  this->consumed);

      largest_deque_size = std::max(largest_deque_size, nodes.size());

      auto node = nodes.back();
      nodes.pop_back();

      if (lock) lock->unlock();

      node->execute();
      node->cleanup();

      return true;
    }

    if (lock) lock->unlock();
    return false;
  }

  void
  ThreadsRuntime::schedule_next_unit() {
    inScheduler = true;
    #if __THREADS_BACKEND_DEBUG__ || __THREADS_BACKEND_SHUFFLE__
      shuffle_deque(nullptr, ready_local);
    #endif

    // check local deque
    const int found = schedule_from_deque(nullptr, ready_local);
    // check remote deque
    if (!found) {
      schedule_from_deque(&lock_remote, ready_remote);
    }
    inScheduler = false;
  }

  /*virtual*/
  void
  ThreadsRuntime::finalize() {
    using namespace std::chrono_literals;

    if (top_level_task) {
      auto t = std::make_shared<TaskNode<top_level_task_t>>(
        this,std::move(top_level_task)
      );
      add_local(t);
      top_level_task = nullptr;
    }

    DEBUG_PRINT("finalize:  produced=%ld, consumed=%ld\n",
                this->produced,
                this->consumed);

    do {
      do {
        schedule_next_unit();
      } while (
        this->produced != this->consumed ||
        ready_local.size() != 0 ||
        ready_remote.size() != 0
      );

      //std::this_thread::yield();

      {
        std::unique_lock<std::mutex> _lock1(lock_remote);
        auto now = std::chrono::system_clock::now();
        cv_remote_awake.wait_until(
          _lock1,
          now + 10ms,
          [this]{
            return ready_remote.size() > 0;
          }
        );
      }

    } while (
      global_produced.load() < 1 ||
      global_produced.load() != global_consumed.load()
    );

    DEBUG_PRINT("thread is TERMIANTED\n");

    assert(ready_local.size() == 0 &&
           ready_remote.size() == 0 &&
           this->produced == this->consumed &&
           "TD failed if queues have work units in them.");

    STARTUP_PRINT(
      "work units: produced=%ld, consumed=%ld, max deque count=%ld, data={%ld,%ld}\n",
      this->produced, this->consumed, largest_deque_size,
      data.size(), fetched_data.size()
    );

    DEBUG_PRINT(
      "data(%p)=%ld, fetched data(%p)=%ld\n",
      &data, data.size(), &fetched_data, fetched_data.size()
    );

    // should call destructor for trace module if it exists to write
    // out the logs
    if (trace) {
      delete trace;
      trace = nullptr;
    }

    if (inside_rank == 0) {
      DEBUG_PRINT("total threads to join is %zu\n", threads_backend::live_ranks.size());

      // TODO: memory consistency bug on live_ranks size here..with relaxed ordering
      for (size_t i = 0; i < threads_backend::live_ranks.size(); i++) {
        DEBUG_PRINT("main thread joining %zu\n", i);
        threads_backend::live_ranks[i].join();
        DEBUG_PRINT("join complete %zu\n", i);
      }

      // reset barrier
      threads_backend::start_barrier = 0;
      terminate_counter = 0;
    }
  }
} // end namespace threads_backend

void barrier(size_t const rank) {
  DEBUG_PRINT_THD(
    rank,
    "start barrier rank=%ld\n", rank
  );
  {
    std::unique_lock<std::mutex> _lock1(threads_backend::mutex_start_barrier);
    threads_backend::start_barrier++;
  }
  threads_backend::cv_start_barrier.notify_all();
  {
    std::unique_lock<std::mutex> _lock1(threads_backend::mutex_start_barrier);
    threads_backend::cv_start_barrier.wait(_lock1, []{
      return threads_backend::start_barrier == threads_backend::n_ranks;
    });
  }
}

void
start_rank_handler(
  size_t const system_rank,
  size_t const num_system_ranks
) {
  DEBUG_PRINT_THD(
    system_rank,
    "%ld: rank handler starting\n",
    system_rank
  );

  auto runtime = std::make_unique<threads_backend::ThreadsRuntime>(
    system_rank,
    num_system_ranks
  );
  threads_backend::cur_runtime = runtime.get();
  threads_backend::shared_ranks[system_rank] = runtime.get();

  barrier(system_rank);

  runtime->finalize();

  return;
}

enum ValidArgs {
  SystemRank,
  NumSystemRanks,
  Ranks,
  Threads,
  Trace,
  BreadthFirst,
  Help,
  AppArgv,
  AppHelp,
};

static ArgsConfig arg_config[] = {
  {SystemRank, NO_SHORT_NAME, "system_rank", REQUIRED_VALUE, "the system rank (system-use only)"},
  {NumSystemRanks, NO_SHORT_NAME, "num_system_ranks", REQUIRED_VALUE, "the number of system ranks (system-use only)"},
  {Ranks, 'r', "ranks", REQUIRED_VALUE, "the number of ranks to launch for concurrent regions - data partition only, no reference to physical resources"},
  {Threads, 't', "threads", REQUIRED_VALUE, "the number of physical threads to run"},
  {BreadthFirst, 'b', "bf", REQUIRED_VALUE, "the degree of breadth-first lookahead in the scheduler. If not given, will use system default"},
  {Trace, NO_SHORT_NAME, "trace", NO_VALUE, "whether to activate DAG tracing"},
  {Help, 'h', "help", NO_VALUE, "print usage of application command-line flags"},
  {AppArgv, 'a', "app-argv", MULTIPLE_VALUES, "the list of arguments to be used by the application"},
  {AppHelp, NO_SHORT_NAME, "app-help", NO_VALUE, "print application options (pass --help to app argv)"},
  {0, 0, nullptr, 0}
};

namespace threads_backend {

/**
 * Parse command line arguments
 * @param argc The number of arguments
 * @param argv The array of string arguments
 * @param system_rank [out] The rank being initialized
 * @param num_system_ranks [out] The total number of ranks being initialized
 * @return A string containing the command line arguments for the application
 */
void
backend_parse_arguments(
  int argc, char** argv,
  size_t& system_rank,
  size_t& num_system_ranks,
  std::vector<std::string>& app_argv
) {
  size_t n_threads = 1;
  size_t n_ranks = 1;
  bool trace = false;
  size_t bwidth = 1;
  ArgsHolder holder(arg_config);
  holder.parse(argc, argv);
  for (auto& entry : holder){
    switch (entry.argEnum){
      case SystemRank:
        entry.get<size_t>(system_rank);
        break;
      case NumSystemRanks:
        entry.get<size_t>(num_system_ranks);
        break;
      case Ranks:
        entry.get<size_t>(n_ranks);
        break;
      case Threads:
        entry.get<size_t>(n_threads);
        break;
      case AppHelp:
        app_argv.push_back("--help");
        break;
      case BreadthFirst:
        entry.get<size_t>(bwidth);
        break;
      case Help:
        holder.usage(std::cout);
        _Exit(0);
      case AppArgv:
        for (auto& arg : entry){
          app_argv.push_back(arg.get());
        }
        break;
    }
  }

  bool const depth =  bwidth == 0 ? true : false;

  threads_backend::bwidth = bwidth;
  threads_backend::n_ranks = n_ranks;

  if (num_system_ranks == 1 && n_ranks != num_system_ranks) {
    num_system_ranks = n_ranks;
  }

  if (system_rank == 0) {
    STARTUP_PRINT(
      "DARMA: number of ranks = %zu, "
      "BF-Sched mode (breadth-first (B=%zu), rank-threaded scheduler), Tracing=%s\n",
      n_ranks,
      bwidth,
      trace ? "ON" : "OFF"
    );

    DEBUG_PRINT_THD(
      system_rank,
      "rank=%zu, ranks=%zu, threads=%zu\n",
      system_rank,
      n_ranks,
      n_threads
    );

    // launch std::thread for each rank
    threads_backend::shared_ranks.resize(n_ranks);
    threads_backend::live_ranks.resize(n_ranks - 1);
    for (size_t i = 0; i < n_ranks - 1; ++i) {
      threads_backend::live_ranks[i] = std::thread(
        start_rank_handler,
        i + 1,
        n_ranks
      );
    }
  }
}

}



void backend_init_finalize(int argc, char **argv) {
  size_t system_rank = 0;
  size_t num_system_ranks = 1;
  std::vector<std::string> app_argv;
  app_argv.push_back(argv[0]);
  threads_backend::backend_parse_arguments(
      argc, argv, system_rank, num_system_ranks, app_argv);

  if (system_rank == 0) {
    auto task = darma_runtime::frontend::darma_top_level_setup(std::move(app_argv));

    auto runtime = std::make_unique<threads_backend::ThreadsRuntime>(
      system_rank,
      num_system_ranks,
      std::move(task)
    );
    threads_backend::cur_runtime = runtime.get();
    threads_backend::shared_ranks[system_rank] = runtime.get();

    barrier(system_rank);

    runtime->finalize();
  } else {
    auto runtime = std::make_unique<threads_backend::ThreadsRuntime>(
      system_rank,
      num_system_ranks
    );
    threads_backend::cur_runtime = runtime.get();
    runtime->finalize();
  }
}


int main(int argc, char **argv) {
  backend_init_finalize(argc, argv);
  return 0;
}


namespace darma_runtime {
  abstract::backend::Context*
  abstract::backend::get_backend_context() {
    return threads_backend::cur_runtime;
  }

  abstract::backend::MemoryManager*
  abstract::backend::get_backend_memory_manager() {
    return threads_backend::cur_runtime;
  }

  abstract::backend::Runtime*
  abstract::backend::get_backend_runtime() {
    return threads_backend::cur_runtime;
  }
}

#endif /* _THREADS_BACKEND_RUNTIME_ */
