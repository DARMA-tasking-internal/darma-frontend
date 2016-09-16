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

  // TL state
  __thread runtime_t::task_t* current_task = 0;
  __thread size_t this_rank = 0;

  #if __THREADS_DEBUG_MODE__
    __thread size_t flow_label = 100;
  #endif

  // tracing TL state for unique labeling
  __thread size_t task_label = 1;
  __thread size_t publish_label = 1;
  __thread size_t fetch_label = 1;

  __thread ThreadsRuntime* cur_runtime = nullptr;

  // global
  size_t n_ranks = 1;
  bool traceMode = false;
  bool depthFirstExpand = true;
  size_t bwidth = 100;

  // global publish
  std::mutex rank_publish;
  std::unordered_map<std::pair<types::key_t, types::key_t>, PublishedBlock*> published;

  // global collective
  std::mutex rank_collective;
  std::unordered_map<
    std::pair<CollectiveType, types::key_t>,
    CollectiveState
  > collective_state;

  ThreadsRuntime::ThreadsRuntime()
    : produced(0)
    , consumed(0)
  {
    //srand48(2918279);
    srand48(time(NULL));
    std::atomic_init(&finished, false);
    std::atomic_init<size_t>(&ranks, 1);
    trace = traceMode ? new TraceModule{this_rank,n_ranks,"base"} : nullptr;
  }

  void
  ThreadsRuntime::produce() { this->produced++; }

  void
  ThreadsRuntime::consume() { this->consumed++; }

  TraceModule*
  ThreadsRuntime::getTrace() { return trace; }

  size_t
  ThreadsRuntime::get_spmd_rank() const {
    return this_rank;
  }

  size_t
  ThreadsRuntime::get_spmd_size() const {
    return n_ranks;
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
      ready_remote.push_back(task);
    }
  }

  void
  ThreadsRuntime::add_local(std::shared_ptr<GraphNode> task) {
    ready_local.push_back(task);
  }

  void
  ThreadsRuntime::schedule_over_breadth() {
    if (this->produced - this->consumed > bwidth)
      schedule_next_unit();
  }

  /*virtual*/
  void
  ThreadsRuntime::register_task(types::unique_ptr_template<runtime_t::task_t>&& task) {
    DEBUG_VERBOSE_PRINT("register task\n");

    auto t = std::make_shared<TaskNode>(TaskNode{this,std::move(task)});
    t->join_counter = check_dep_task(t);

    DEBUG_VERBOSE_PRINT("task join counter: %ld\n", t->join_counter);

    // use depth-first scheduling policy
    if (threads_backend::depthFirstExpand) {
      assert(t->ready());
      DEBUG_VERBOSE_PRINT("running task\n");

      t->execute();
    } else {
      if (t->ready()) {
        ready_local.push_back(t);
      }
    }

    schedule_over_breadth();
  }

  bool
  ThreadsRuntime::register_condition_task(types::unique_ptr_template<runtime_t::task_t>&& task) {
    auto t = std::make_shared<TaskNode>(TaskNode{this,std::move(task)});
    t->join_counter = check_dep_task(t);

    assert(threads_backend::depthFirstExpand);

    if (threads_backend::depthFirstExpand) {
      assert(t->ready());
      DEBUG_VERBOSE_PRINT("running task\n");

      runtime_t::task_t* prev = current_task;
      types::unique_ptr_template<runtime_t::task_t> cur = std::move(t->task);
      current_task = cur.get();
      bool ret = cur.get()->run<bool>();
      current_task = prev;

      return ret;
    }

    return true;
  }

  void
  ThreadsRuntime::reregister_migrated_use(darma_runtime::abstract::frontend::Use* u) {
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
    if (threads_backend::depthFirstExpand) {
      return;
    }

    if (pub_log) {
      const auto& end = std::atomic_load<TraceLog*>(&pub_log->end);
      const auto& time = end != nullptr ? end->time : pub_log->time;
      const auto& entry = thisLog->entry;
      auto dep = getTrace()->depCreate(time,entry);

      dep->rank = this_rank;
      dep->event = thisLog->event;
      thisLog->rank = pub_log->rank;
      pub_log->insertDep(dep);
    }
  }

  void
  ThreadsRuntime::addPublishDeps(PublishNode* node,
                                 TraceLog* thisLog) {
    if (threads_backend::depthFirstExpand) {
      return;
    }

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

  void
  ThreadsRuntime::addTraceDeps(TaskNode* node,
                               TraceLog* thisLog) {
    if (threads_backend::depthFirstExpand) {
      return;
    }

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

  size_t
  ThreadsRuntime::check_dep_task(std::shared_ptr<TaskNode> t) {
    DEBUG_VERBOSE_PRINT("check_dep_task\n");

    size_t dep_count = 0;

    for (auto&& dep : t->task->get_dependencies()) {
      auto const f_in  = dep->get_in_flow();
      auto const f_out = dep->get_out_flow();

      if (f_in->isFetch &&
          threads_backend::depthFirstExpand) {
        blocking_fetch(f_in->handle, f_in->version_key);
        f_in->state = FlowWriteReady;
        f_in->ready = true;
      } else if (f_in->isFetch) {
        auto node = std::make_shared<FetchNode>(FetchNode{this,f_in});
        const bool ready = add_fetcher(node,
                                       f_in->handle,
                                       f_in->version_key);
        if (ready) {
          ready_local.push_back(node);
        }
      }

      if (f_in->isCollective &&
          threads_backend::depthFirstExpand) {
        if (!f_in->dfsColNode->finished) {
          f_in->dfsColNode->block_execute();
        }
      }

      DEBUG_PRINT("check_dep_task: f_in=%lu (ready=%s), f_out=%lu\n",
                  PRINT_LABEL(f_in),
                  PRINT_BOOL_STR(f_in->ready),
                  PRINT_LABEL(f_out));

      auto const flows_match = f_in == f_out;

      if (flows_match) {
        f_in->readers_jc++;
        if (f_in->state == FlowWaiting) {
          f_in->readers.push_back(t);
          dep_count++;
        } else {
          // increment shared count
          assert(f_in->shared_reader_count != nullptr);
          (*f_in->shared_reader_count)++;
        }
      } else {
        if (f_in->state == FlowWaiting ||
            f_in->state == FlowReadReady) {
          f_in->node = t;
          dep_count++;
        }
      }
    }

    return dep_count;
  }

  void
  ThreadsRuntime::run_task(types::unique_ptr_template<runtime_t::task_t>&& task) {
    runtime_t::task_t* prev = current_task;
    types::unique_ptr_template<runtime_t::task_t> cur = std::move(task);
    current_task = cur.get();
    cur.get()->run();
    current_task = prev;
  }

  /*virtual*/
  runtime_t::task_t*
  ThreadsRuntime::get_running_task() const {
    DEBUG_VERBOSE_PRINT("get running task\n");
    return current_task;
  }

  /*virtual*/
  void
  ThreadsRuntime::register_use(darma_runtime::abstract::frontend::Use* u) {
    auto f_in  = u->get_in_flow();
    auto f_out = u->get_out_flow();

    auto const handle = u->get_handle();
    const auto& key = handle->get_key();
    const auto version = f_in->version_key;

    const bool ready = f_in->ready;

    DEBUG_PRINT("%p: register use: ready=%s, key=%s, version=%s, "
                "handle=%p [in={%ld,ref=%ld,state=%s},out={%ld,ref=%ld,state=%s}], "
                "sched=%d, immed=%d\n",
                u,
                PRINT_BOOL_STR(ready),
                PRINT_KEY(key),
                PRINT_KEY(version),
                handle,
                PRINT_LABEL(f_in),
                f_in->ref,
                PRINT_STATE(f_in),
                PRINT_LABEL(f_out),
                f_out->ref,
                PRINT_STATE(f_out),
                u->scheduling_permissions(),
                u->immediate_permissions()
               );

    if (f_in->isForward) {
      auto const flows_match = f_in == f_out;
      f_in->isWriteForward = !flows_match;
    }

    if (!f_in->fromFetch) {
      const bool data_exists = data.find({version,key}) != data.end();
      if (data_exists) {
        u->get_data_pointer_reference() = data[{version,key}]->data;

        DEBUG_PRINT("%p: use register, ptr=%p, key=%s, "
                    "in version=%s, out version=%s\n",
                    u,
                    data[{version,key}]->data,
                    PRINT_KEY(key),
                    PRINT_KEY(f_in->version_key),
                    PRINT_KEY(f_out->version_key));
        data[{version,key}]->refs++;
      } else {
        // allocate new deferred data block for this use
        auto block = allocate_block(handle);

        // insert into the hash
        data[{version,key}] = block;
        u->get_data_pointer_reference() = block->data;

        DEBUG_PRINT("%p: use register: ptr=%p, key=%s, "
                    "in version=%s, out version=%s\n",
                    u,
                    block->data,
                    PRINT_KEY(key),
                    PRINT_KEY(f_in->version_key),
                    PRINT_KEY(f_out->version_key));
        block->refs++;
      }

      f_in->shared_reader_count = &data[{version,key}]->shared_ref_count;
      f_out->shared_reader_count = &data[{version,key}]->shared_ref_count;
    } else {
      const bool data_exists = fetched_data.find({version,key}) != fetched_data.end();
      if (data_exists) {
        u->get_data_pointer_reference() = fetched_data[{version,key}]->data;
        fetched_data[{version,key}]->refs++;
      } else {
        // FIXME: copy-paste of above code...

        // allocate new deferred data block for this use
        auto block = allocate_block(handle);
        // insert into the hash
        fetched_data[{version,key}] = block;
        u->get_data_pointer_reference() = block->data;
        block->refs++;
      }

      f_in->shared_reader_count = &fetched_data[{version,key}]->shared_ref_count;
      f_out->shared_reader_count = &fetched_data[{version,key}]->shared_ref_count;
    }

    DEBUG_PRINT("flow %ld, shared_reader_count=%p\n",
                PRINT_LABEL(f_in),
                f_in->shared_reader_count);

    // count references to a given handle
    handle_refs[handle]++;
  }

  DataBlock*
  ThreadsRuntime::allocate_block(darma_runtime::abstract::frontend::Handle const* handle) {
    // allocate some space for this object
    const size_t sz = handle->get_serialization_manager()->get_metadata_size();
    auto block = new DataBlock(1, sz);

    DEBUG_PRINT("allocating data block: size = %ld, ptr = %p\n",
                sz, block->data);

    // call default constructor for data block
    handle
      ->get_serialization_manager()
      ->default_construct(block->data);

    return block;
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_initial_flow(darma_runtime::abstract::frontend::Handle* handle) {
    auto f = std::make_shared<InnerFlow>(handle);
    f->state = FlowWriteReady;
    f->ready = true;
    f->handle = handle;
    handle_refs[handle]++;
    return f;
  }

  PackedDataBlock*
  ThreadsRuntime::serialize(darma_runtime::abstract::frontend::Handle const* const handle,
                            void* unpacked) {
    auto policy = new SerializationPolicy();
    const size_t size = handle
      ->get_serialization_manager()
      ->get_packed_data_size(unpacked,
                             policy);

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
                  policy);
    delete policy;

    return block;
  }

  void
  ThreadsRuntime::de_serialize(darma_runtime::abstract::frontend::Handle const* const handle,
                               void* packed,
                               void* unpack_to) {
    auto policy = new SerializationPolicy();
    handle
      ->get_serialization_manager()
      ->unpack_data(unpack_to,
                    packed,
                    policy);
    delete policy;
  }

  bool
  ThreadsRuntime::try_fetch(darma_runtime::abstract::frontend::Handle* handle,
                            types::key_t const& version_key) {
    bool found = false;

  retry_fetch:
    {
      // TODO: big lock to handling fetching
      std::lock_guard<std::mutex> guard(threads_backend::rank_publish);

      const auto& key = handle->get_key();
      const auto& iter = published.find({version_key, key});

      if (iter != published.end()) {
        PublishedBlock* pub_ptr = iter->second;
        auto &pub = *pub_ptr;

        const bool buffer_exists = fetched_data.find({version_key,key}) != fetched_data.end();
        void* unpack_to = buffer_exists ? fetched_data[{version_key,key}]->data : malloc(pub.data->size_);

        DEBUG_PRINT("fetch: unpacking data: buffer_exists = %s, handle = %p\n",
                    PRINT_BOOL_STR(buffer_exists),
                    handle);

        de_serialize(handle,
                     pub.data->data_,
                     unpack_to);

        if (!buffer_exists) {
          fetched_data[{version_key,key}] = new DataBlock(unpack_to);
        }

        DEBUG_PRINT("fetch: key = %s, version = %s, published data = %p, expected = %ld, data = %p\n",
                    PRINT_KEY(key),
                    PRINT_KEY(version_key),
                    pub.data->data_,
                    std::atomic_load<size_t>(&pub.expected),
                    unpack_to);

        assert(pub.expected > 0);

        ++(pub.done);
        --(pub.expected);

        if (std::atomic_load<size_t>(&pub.expected) == 0) {
          // remove from publication list
          published.erase({version_key, key});
          free(pub_ptr->data->data_);
          delete pub.data;
          delete pub_ptr;
        }

        found = true;
      }
    }

    if (!found && depthFirstExpand)
      goto retry_fetch;

    return found;
  }

  bool
  ThreadsRuntime::add_fetcher(std::shared_ptr<FetchNode> fetch,
                              darma_runtime::abstract::frontend::Handle* handle,
                              types::key_t const& version_key) {
    bool ready = false;
    {
      // TODO: unscalable lock to handling fetching
      std::lock_guard<std::mutex> guard(threads_backend::rank_publish);

      const auto& key = handle->get_key();
      const auto& iter = published.find({version_key,key});
      const bool found = iter != published.end();
      const bool avail = found && std::atomic_load<bool>(&iter->second->ready);

      DEBUG_PRINT("add_fetcher: key=%s, version=%s, found=%s, avail=%s\n",
                  PRINT_KEY(key),
                  PRINT_KEY(version_key),
                  PRINT_BOOL_STR(found),
                  PRINT_BOOL_STR(avail)
                  );

      if (!found) {
        auto pub = new PublishedBlock();
        pub->waiting.push_front(fetch);
        published[{version_key,key}] = pub;
      } else if (found && !avail) {
        published[{version_key,key}]->waiting.push_front(fetch);
      }

      ready = avail;
    }
    return ready;
  }

  bool
  ThreadsRuntime::test_fetch(darma_runtime::abstract::frontend::Handle* handle,
                             types::key_t const& version_key) {
    bool ready = false;
    {
      // TODO: unscalable lock to handling fetching
      std::lock_guard<std::mutex> guard(threads_backend::rank_publish);

      const auto& key = handle->get_key();
      const auto& iter = published.find({version_key,key});

      DEBUG_PRINT("test_fetch: trying to find a publish, key=%s, version=%s\n",
                  PRINT_KEY(key),
                  PRINT_KEY(version_key));

      ready =
        iter != published.end() &&
        std::atomic_load<bool>(&iter->second->ready);
    }
    return ready;
  }

  void
  ThreadsRuntime::blocking_fetch(darma_runtime::abstract::frontend::Handle* handle,
                                 types::key_t const& version_key) {

    DEBUG_PRINT("fetch_block: handle = %p\n", handle);

    while (!test_fetch(handle, version_key)) ;

    fetch(handle, version_key);
  }

  TraceLog*
  ThreadsRuntime::fetch(darma_runtime::abstract::frontend::Handle* handle,
                        types::key_t const& version_key) {
    {
      // TODO: big lock to handling fetching
      std::lock_guard<std::mutex> guard(threads_backend::rank_publish);

      const auto& key = handle->get_key();
      const auto& iter = published.find({version_key,key});

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

      const bool buffer_exists = fetched_data.find({version_key,key}) != fetched_data.end();
      void* unpack_to = buffer_exists ? fetched_data[{version_key,key}]->data : malloc(pub.data->size_);

      DEBUG_PRINT("fetch: unpacking data: buffer_exists = %s, handle = %p\n",
                  PRINT_BOOL_STR(buffer_exists),
                  handle);

      de_serialize(handle,
                   pub.data->data_,
                   unpack_to);

      if (!buffer_exists) {
        fetched_data[{version_key,key}] = new DataBlock(unpack_to);
      }

      DEBUG_PRINT("fetch: key = %s, version = %s, published data = %p, expected = %ld, data = %p\n",
                  PRINT_KEY(key),
                  PRINT_KEY(version_key),
                  pub.data->data_,
                  std::atomic_load<size_t>(&pub.expected),
                  unpack_to);

      assert(pub.expected > 0);

      ++(pub.done);
      --(pub.expected);

      if (std::atomic_load<size_t>(&pub.expected) == 0) {
        // remove from publication list
        published.erase({version_key, key});
        free(pub_ptr->data->data_);
        delete pub.data;
        delete pub_ptr;
      }

      return traceLog;
    }
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_fetching_flow(darma_runtime::abstract::frontend::Handle* handle,
                                     types::key_t const& version_key) {
    DEBUG_VERBOSE_PRINT("make fetching flow\n");

    auto f = std::make_shared<InnerFlow>(handle);

    handle_refs[handle]++;

    f->handle = handle;
    f->version_key = version_key;
    f->isFetch = true;
    f->fromFetch = true;
    f->state = FlowWaiting;
    f->ready = false;

    return f;
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_null_flow(darma_runtime::abstract::frontend::Handle* handle) {
    DEBUG_VERBOSE_PRINT("make null flow\n");
    auto f = std::make_shared<InnerFlow>(handle);

    f->isNull = true;
    f->handle = handle;

    DEBUG_PRINT("null flow %lu\n", PRINT_LABEL(f));

    return f;
  }

  /*virtual*/
  void
  ThreadsRuntime::release_flow(flow_t& to_release) {
    DEBUG_VERBOSE_PRINT("release_flow %ld: state=%s\n",
                        PRINT_LABEL(to_release),
                        PRINT_STATE(to_release));
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_forwarding_flow(flow_t& f) {
    DEBUG_VERBOSE_PRINT("make forwarding flow\n");

    auto f_forward = std::make_shared<InnerFlow>(nullptr);

    DEBUG_PRINT("forwarding flow from %lu to %lu\n",
                PRINT_LABEL(f),
                PRINT_LABEL(f_forward));

    f->next->ref++;
    f->next->state = FlowWaiting;

    if (depthFirstExpand) {
      f_forward->ready = true;
      f_forward->state = FlowWriteReady;
    }

    if (getTrace()) {
      task_forwards[f_forward] = f;
    }

    f->forward = f_forward;

    f_forward->isForward = true;
    f_forward->handle = f->handle;
    f_forward->fromFetch = f->fromFetch;
    return f_forward;
  }

  void
  ThreadsRuntime::create_next_subsequent(
    std::shared_ptr<InnerFlow> f
  ) {
    // creating subsequent allowing release
    if (f->state == FlowReadReady &&
        f->readers_jc == 0 &&
        *f->shared_reader_count == 0) {
      // can't have alias if has next subsequent
      assert(flow_has_alias(f) == false);
      release_to_write(
        f
      );
    }
  }

  /*virtual*/
  flow_t
  ThreadsRuntime::make_next_flow(flow_t& f) {
    DEBUG_VERBOSE_PRINT("make next flow: (from=%p)\n", from);

    auto f_next = std::make_shared<InnerFlow>(nullptr);

    f->next = f_next;
    f_next->handle = f->handle;
    f_next->fromFetch = f->fromFetch;

    DEBUG_PRINT("next flow from %lu to %lu\n",
                PRINT_LABEL(f),
                PRINT_LABEL(f_next));

    create_next_subsequent(f);

    return f_next;
  }

  void
  ThreadsRuntime::cleanup_handle(std::shared_ptr<InnerFlow> flow) {
    auto const* const handle = flow->handle;

    handle_refs[handle]--;

    DEBUG_PRINT("cleanup_handle identity: %p to %p: refs=%d\n",
                flow->handle,
                alias[flow]->handle,
                handle_refs[handle]);

    assert(handle == alias[flow]->handle);
    assert(handle_refs[handle] >= 0);

    if (handle_refs[handle] == 0) {
      if (handle_pubs.find(handle) != handle_pubs.end()) {
        for (auto pub : handle_pubs[handle]) {
          pub->node->execute();
          pub->finished = true;

          // explicitly don't call clean up because we do it manually
          // TODO: fix this problem with iterator

          DEBUG_PRINT("cleanup handle: force publish of handle = %p\n", handle);
        }

        handle_pubs.erase(handle);
      }
      delete_handle_data(handle,
                         flow->version_key,
                         handle->get_key(),
                         flow->fromFetch);
      handle_refs.erase(handle);
    }
  }

  void
  ThreadsRuntime::delete_handle_data(darma_runtime::abstract::frontend::Handle const* const handle,
                                     const types::key_t version,
                                     const types::key_t key,
                                     const bool fromFetch) {
    DataBlock* prev = nullptr;
    size_t ref = 0;

    if (fromFetch) {
      prev = fetched_data[{version,key}];
      if (prev) {
        ref = prev->refs;
      }
      if (prev && ref == 0) {
        fetched_data.erase({version,key});
      }
    } else {
      prev = data[{version,key}];
      if (prev) {
        ref = prev->refs;
      }
      if (prev && ref == 0) {
        data.erase({version,key});
      }
    }

    if (prev && ref == 0) {
      DEBUG_PRINT("delete handle data: fromFetch=%s, refs=%ld\n",
                  PRINT_BOOL_STR(fromFetch),
                  ref);

      // call the destructor
      handle
        ->get_serialization_manager()
        ->destroy(prev->data);
      delete prev;
    }
  }

  /*virtual*/
  void
  ThreadsRuntime::establish_flow_alias(flow_t& f_from,
                                       flow_t& f_to) {
    DEBUG_PRINT("establish flow alias %lu (ref=%ld) to %lu (ref=%ld)\n",
                PRINT_LABEL(f_from),
                f_from->ref,
                PRINT_LABEL(f_to),
                f_to->ref);

    alias[f_from] = f_to;

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
          release_to_write(
            alias_part
          );
        } else {
          DEBUG_PRINT("establish_flow_alias subsequent, %ld in state=%s does not have *subsequent*\n",
                      PRINT_LABEL(alias_part),
                      PRINT_STATE(alias_part));

        }
      }
    } else if (f_from->state == FlowReadReady) {
      f_from->state = FlowReadOnlyReady;
    }

    if (f_to->isNull) {
      // TODO!
      DEBUG_PRINT("establish deleting null: %ld\n",
                  PRINT_LABEL(f_to));
    }
  }

  bool
  ThreadsRuntime::test_alias_null(std::shared_ptr<InnerFlow> flow) {
    if (alias[flow]->isNull) {
      // DEBUG_PRINT("remove alias to null %ld to %ld\n",
      //             PRINT_LABEL_INNER(flow),
      //             PRINT_LABEL_INNER(alias[flow]));
      if (*flow->shared_reader_count == 0) {
        cleanup_handle(flow);
      }
      //alias.erase(alias.find(flow));
      return true;
    }
    return false;
  }

  bool
  ThreadsRuntime::try_release_to_read(
    std::shared_ptr<InnerFlow> flow
  ) {
    assert(flow->state == FlowWaiting);
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
      }

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
    return alias.find(flow) != alias.end();
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

    DEBUG_PRINT("try_release_alias_to_read: parent flow=%ld, state=%s\n",
                PRINT_LABEL(flow),
                PRINT_STATE(flow));

    if (flow_has_alias(flow)) {
      // TODO: swtich to test_null
      if (test_alias_null(flow)) {
        return {alias[flow],false};
      }

      assert(flow->state == FlowReadOnlyReady);

      DEBUG_PRINT("try_release_alias_to_read: aliased flow=%ld, state=%s, ref=%ld\n",
                  PRINT_LABEL(alias[flow]),
                  PRINT_STATE(alias[flow]),
                  alias[flow]->ref);

      if (alias[flow]->state == FlowWaiting) {
        assert(alias[flow]->state == FlowWaiting);
        assert(alias[flow]->ref > 0);

        alias[flow]->ref--;

        // is conditional needed here on ref?
        assert(alias[flow]->ref == 0);

        auto const has_read_phase = try_release_to_read(alias[flow]);
        auto const ret = try_release_alias_to_read(alias[flow]);
        return
          {
            std::get<0>(ret),
            std::get<1>(ret) || has_read_phase
          };
      } else {
        assert(
          alias[flow]->state == FlowReadOnlyReady ||
          alias[flow]->state == FlowReadReady
        );
        assert(alias[flow]->shared_reader_count != nullptr);

        auto const has_outstanding_reads =
          alias[flow]->readers_jc != 0 &&
          *alias[flow]->shared_reader_count == 0;
        auto const ret = try_release_alias_to_read(alias[flow]);
        return
          {
            std::get<0>(ret),
            std::get<1>(ret) || has_outstanding_reads
          };
      }
    }
    return {flow,false};
  }

  void
  ThreadsRuntime::release_to_write(
    std::shared_ptr<InnerFlow> flow
  ) {
    assert(flow->state == FlowReadReady);
    assert(flow->ref == 0);
    assert(flow->readers_jc == 0);
    assert(flow->readers.size() == flow->readers_jc);
    assert(flow->shared_reader_count != nullptr);
    assert(*flow->shared_reader_count == 0);

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
    assert(flow->readers_jc > 0);
    assert(flow->ref == 0);
    assert(flow->readers.size() == 0);
    assert(flow->shared_reader_count != nullptr);
    assert(*flow->shared_reader_count > 0);
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

    return flow->readers_jc == 0 && *flow->shared_reader_count == 0;
  }

  /*virtual*/
  void
  ThreadsRuntime::allreduce_use(
    darma_runtime::abstract::frontend::Use* use_in,
    darma_runtime::abstract::frontend::Use* use_out,
    darma_runtime::abstract::frontend::CollectiveDetails const* details,
    types::key_t const& tag) {

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

    auto node = std::make_shared<CollectiveNode>(CollectiveNode{this,info});

    // set the node
    info->node = node;

    if (!depthFirstExpand) {
      f_out->ready = false;
      f_out->state = FlowWaiting;
      f_out->ref++;
    }

    const auto& ready = node->ready();

    DEBUG_PRINT("allreduce_use: ready=%s, flow in=%ld, flow out=%ld\n",
                PRINT_BOOL_STR(ready),
                PRINT_LABEL_INNER(f_in),
                PRINT_LABEL_INNER(f_out));

    if (depthFirstExpand) {
      f_out->dfsColNode = node;
    }

    if (ready) {
      node->execute();
    } else {
      node->set_join(1);
      f_in->node = node;
    }
  }

  bool
  ThreadsRuntime::collective(std::shared_ptr<CollectiveInfo> info) {
    DEBUG_PRINT("collective operation, type=%d, flow in=%ld, flow out=%ld\n",
                info->type,
                PRINT_LABEL_INNER(info->flow),
                PRINT_LABEL_INNER(info->flow_out));

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

          if (state.current_pieces == 0) {
            assert(state.cur_buf == nullptr);

            auto block = serialize(info->handle,
                                   info->data_ptr_in);

            const size_t sz = info
              ->handle
              ->get_serialization_manager()
              ->get_metadata_size();

            // TODO: memory leaks here
            state.cur_buf = malloc(sz);

            de_serialize(info->handle,
                         block->data_,
                         state.cur_buf);
          } else {
            DEBUG_PRINT("performing op on data\n");

            // TODO: offset might be wrong
            const size_t n_elem = info
              ->handle
              ->get_array_concept_manager()
              ->n_elements(info->data_ptr_in);

            info->op->reduce_unpacked_into_unpacked(info->data_ptr_in,
                                                    state.cur_buf,
                                                    0,
                                                    n_elem);
          }

          info->incorporated_local = true;
          state.current_pieces++;

          finished = state.current_pieces == state.n_pieces;

          if (!finished) {
            if (!depthFirstExpand) {
              state.activations.push_back(info->node);
            }
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

        const auto& handle = info->handle;
        auto block = serialize(handle,
                               state->cur_buf);

        de_serialize(handle,
                     block->data_,
                     info->data_ptr_out);

        DEBUG_PRINT("result = %d\n", *(int*)info->data_ptr_out);

        delete block;

        //info->flow_out->ref--;
        //release_node(info->flow_out);
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

        std::pair<CollectiveType,types::key_t> key = {CollectiveType::AllReduce,
                                                      info->tag};

        {
          std::lock_guard<std::mutex> guard(threads_backend::rank_collective);
          assert(collective_state.find(key) != collective_state.end());
          state = &collective_state[key];
        }

        assert(state != nullptr);
        assert(state->current_pieces == state->n_pieces);

        const auto& handle = info->handle;
        auto block = serialize(handle,
                               state->cur_buf);

        de_serialize(handle,
                     block->data_,
                     info->data_ptr_out);

        delete block;

        if (!depthFirstExpand) {
          info->flow_out->ref--;
          release_to_write(info->flow_out);
        }
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

    auto const finishedAllReads = finish_read(flow);
    auto const last_found_alias = try_release_alias_to_read(flow);
    auto const alias_part = std::get<0>(last_found_alias);

    if (finishedAllReads &&
        std::get<1>(last_found_alias) == false) {
      auto const has_subsequent = alias_part->next != nullptr || flow_has_alias(alias_part);
      if (has_subsequent) {
        release_to_write(
          alias_part
        );
      } else {
        DEBUG_PRINT("transition_after_read, %ld in state=%s does not have *subsequent*\n",
                    PRINT_LABEL(alias_part),
                    PRINT_STATE(alias_part));
      }
    }
  }

  void
  ThreadsRuntime::transition_after_write(
    std::shared_ptr<InnerFlow> f_in,
    std::shared_ptr<InnerFlow> f_out
  ) {
    assert(f_in->state == FlowWriteReady);
    assert(f_out->state == FlowWaiting);

    f_in->state = FlowAntiReady;

    DEBUG_PRINT("transition_after_write, transition in=%ld to state=%s\n",
                PRINT_LABEL(f_in),
                PRINT_STATE(f_in));

    if (f_out->ref == 0) {
      auto const has_read_phase = try_release_to_read(f_out);
      auto const last_found_alias = try_release_alias_to_read(f_out);
      auto const alias_part = std::get<0>(last_found_alias);

      DEBUG_PRINT("transition_after_write, transition out=%ld to last_found_alias=%ld, state=%s\n",
                  PRINT_LABEL(f_out),
                  PRINT_LABEL(std::get<0>(last_found_alias)),
                  PRINT_STATE(std::get<0>(last_found_alias)));

      // out flow from release is ready to go
      if (!has_read_phase &&
          std::get<1>(last_found_alias) == false) {

        auto const has_subsequent = alias_part->next != nullptr || flow_has_alias(alias_part);
        if (has_subsequent) {
          release_to_write(
            alias_part
          );
        } else {
          DEBUG_PRINT("transition_after_write, %ld in state=%s does not have *subsequent*\n",
                      PRINT_LABEL(alias_part),
                      PRINT_STATE(alias_part));
        }
      }
    }

    DEBUG_PRINT("release_use write, transition out=%ld to state=%s\n",
                PRINT_LABEL(f_out),
                PRINT_STATE(f_out));
  }

  /*virtual*/
  void
  ThreadsRuntime::release_use(darma_runtime::abstract::frontend::Use* u) {
    auto f_in  = u->get_in_flow();
    auto f_out = u->get_out_flow();

    // enable next forward flow
    if (f_in->forward) {
      f_in->forward->state = f_in->forward->isWriteForward ? FlowWriteReady : FlowReadReady;
      f_in->forward->ready = true;
    }

    // enable each out flow
    if (depthFirstExpand) {
      f_out->ready = true;
    }

    auto handle = u->get_handle();
    const auto version = f_in->version_key;
    const auto& key = handle->get_key();

    DEBUG_PRINT("%p: release use: handle=%p, version=%s, key=%s\n",
                u,
                handle,
                PRINT_KEY(version),
                PRINT_KEY(key));

    handle_refs[handle]--;

    if (!f_in->fromFetch) {
      data[{version,key}]->refs--;
    } else {
      fetched_data[{version,key}]->refs--;
    }

    DEBUG_PRINT("release_use: f_in=[%ld,state=%s], f_out=[%ld,state=%s]: handle_refs=%d\n",
                PRINT_LABEL(f_in),
                PRINT_STATE(f_in),
                PRINT_LABEL(f_out),
                PRINT_STATE(f_out),
                handle_refs[handle]);

    auto const flows_match = f_in == f_out;

    if (flows_match &&
        f_out->state == FlowWaiting) {
      // do nothing
    } else if (flows_match) {
      transition_after_read(f_out);
    } else {
      transition_after_write(f_in, f_out);
    }
  }

  bool
  ThreadsRuntime::test_publish(std::shared_ptr<DelayedPublish> publish) {
    return publish->flow->ready;
  }

  void
  ThreadsRuntime::publish(std::shared_ptr<DelayedPublish> publish,
                          TraceLog* const log) {
    if (publish->finished) return;

    {
      // TODO: big lock to handling publishing
      std::lock_guard<std::mutex> guard(threads_backend::rank_publish);

      const auto& expected = publish->fetchers;
      const auto& version = publish->version;
      const auto& key = publish->key;

      const darma_runtime::abstract::frontend::Handle* handle = publish->handle;
      void* data_ptr = publish->data_ptr;

      DEBUG_PRINT("publish: key = %s, version = %s, data ptr = %p, handle = %p\n",
                  PRINT_KEY(key),
                  PRINT_KEY(version),
                  data_ptr,
                  handle);

      assert(expected >= 1);

      auto block = serialize(handle,
                             data_ptr);

      DEBUG_PRINT("publication: key = %s, version = %s, published data = %p, data ptr = %p\n",
                  PRINT_KEY(key),
                  PRINT_KEY(version),
                  block->data_,
                  data_ptr);

      const bool publish_exists = published.find({version,key}) != published.end();
      const auto& publish = publish_exists ?
        published[{version,key}] :
        (published[{version,key}] = new PublishedBlock(),
         published[{version,key}]);

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
                publish->handle);
    handle_pubs[publish->handle].remove(publish);

    transition_after_read(
      publish->flow
    );
  }

  /*virtual*/
  void
  ThreadsRuntime::publish_use(darma_runtime::abstract::frontend::Use* f,
                              darma_runtime::abstract::frontend::PublicationDetails* details) {

    auto f_in  = f->get_in_flow();
    auto f_out = f->get_out_flow();

    const darma_runtime::abstract::frontend::Handle* handle = f->get_handle();

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
          false
       });

    auto p = std::make_shared<PublishNode>(PublishNode{this,pub});

    // set the node for early release
    pub->node = p;

    const bool ready = p->ready();

    DEBUG_PRINT("%p: publish_use: ptr=%p, key=%s, "
                "version=%s, handle=%p, ready=%s, f_in=%lu, f_out=%lu\n",
                f,
                f->get_data_pointer_reference(),
                PRINT_KEY(key),
                PRINT_KEY(version),
                handle,
                PRINT_BOOL_STR(ready),
                PRINT_LABEL(f_in),
                PRINT_LABEL(f_out));

    auto const flow_in_read =
      f_in->state == FlowReadReady ||
      f_in->state == FlowReadOnlyReady;

    assert(f_in == f_out);

    if (flow_in_read) {
      p->execute();
    } else {
      f_in->readers_jc++;
      f_in->readers.push_back(p);
      p->set_join(1);
      handle_pubs[handle].push_back(pub);
    }

    assert(ready || !depthFirstExpand);

    schedule_over_breadth();
  }

  template <typename Node>
  void
  ThreadsRuntime::try_node(std::list<std::shared_ptr<Node> >& nodes) {
    if (nodes.size() > 0) {
      auto n = nodes.back();
      nodes.pop_back();
      if (n->ready(this)) {
        n->execute(this);
        n->cleanup(this);
      } else {
        nodes.push_front(n);
      }
    }
  }

  template <typename Node>
  void
  ThreadsRuntime::shuffle_deque(std::mutex* lock,
                                std::deque<Node>& nodes) {
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
  ThreadsRuntime::schedule_from_deque(std::mutex* lock,
                                      std::deque<Node>& nodes) {
    if (lock) lock->lock();

    if (nodes.size() > 0) {
      DEBUG_PRINT("scheduling from deque %p (local = %p): size = %lu\n",
                  &nodes, &ready_local, nodes.size());

      auto node = nodes.front();
      nodes.pop_front();

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
    shuffle_deque(nullptr, ready_local);

    // check local deque
    const int found = schedule_from_deque(nullptr, ready_local);
    // check remote deque
    if (!found) {
      schedule_from_deque(&lock_remote, ready_remote);
    }
  }

  /*virtual*/
  void
  ThreadsRuntime::finalize() {
    DEBUG_PRINT("finalize:  produced=%ld, consumed=%ld\n",
                this->produced,
                this->consumed);

    do {
      schedule_next_unit();
    } while (this->produced != this->consumed
             || ready_local.size() != 0
             || ready_remote.size() != 0);

    assert(ready_local.size() == 0 &&
           ready_remote.size() == 0 &&
           this->produced == this->consumed &&
           "TD failed if queues have work units in them.");

    STARTUP_PRINT("work units: produced=%ld, consumed=%ld\n",
                  this->produced,
                  this->consumed);

    #if __THREADS_BACKEND_DEBUG__
    for (auto iter = alias.begin();
         iter != alias.end();
         ++iter) {
      DEBUG_PRINT("alias flow[%ld] = %ld\n",
                  PRINT_LABEL_INNER(iter->first),
                  PRINT_LABEL_INNER(iter->second));
    }
    #endif

    DEBUG_PRINT("handle_refs=%ld, "
                "handle_pubs=%ld, "
                "data=%ld, "
                "fetched data=%ld, "
                "\n",
                handle_refs.size(),
                handle_pubs.size(),
                data.size(),
                fetched_data.size());

    // should call destructor for trace module if it exists to write
    // out the logs
    if (trace) {
      delete trace;
      trace = nullptr;
    }

    if (this_rank == 0) {
      DEBUG_PRINT("total threads to join is %zu\n", threads_backend::live_ranks.size());

      // TODO: memory consistency bug on live_ranks size here..with relaxed ordering
      for (size_t i = 0; i < threads_backend::live_ranks.size(); i++) {
        DEBUG_PRINT("main thread joining %zu\n", i);
        threads_backend::live_ranks[i].join();
        DEBUG_PRINT("join complete %zu\n", i);
      }
    }
  }
} // end namespace threads_backend


void
start_thread_handler(const size_t thd, threads_backend::ThreadsRuntime* runtime) {
  //std::cout << "thread handler running" << std::endl;
  DEBUG_PRINT("%ld: thread handler starting\n", thd);

  // set thread-local rank
  threads_backend::this_rank = thd;
}

void
start_rank_handler(const size_t rank,
                   const int argc,
                   char** argv) {
  DEBUG_PRINT("%ld: rank handler starting\n", rank);

  // set TL variables
  threads_backend::this_rank = rank;

  // call into main
  const int ret = (*(darma_runtime::detail::_darma__generate_main_function_ptr<>()))(argc, argv);
}

int main(int argc, char **argv) {
  int ret = (*(darma_runtime::detail::_darma__generate_main_function_ptr<>()))(argc, argv);
  // TODO: check if runtime finalized before deleting
  // if (darma_runtime::detail::backend_runtime) {
  //   delete darma_runtime::detail::backend_runtime;
  // }

  return ret;
}

void
darma_runtime::abstract::backend::darma_backend_initialize(
  int &argc, char **&argv,
  //darma_runtime::abstract::backend::Runtime *&backend_runtime,
  types::unique_ptr_template<
    typename darma_runtime::abstract::backend::Runtime::task_t
  >&& top_level_task
) {
  bool depth = true;
  size_t ranks = 1, n_threads = 1;

  detail::ArgParser args = {
    {"t", "threads", 1},
    {"r", "ranks",   1},
    {"m", "trace",   1},
    {"", "backend-n-ranks", 1},
    {"", "serial-backend-n-ranks", 1},
    {"", "bf",   1}
  };

  args.parse(argc, argv);

  // read number of threads from the command line
  if (args["threads"].as<bool>()) {
    n_threads = args["threads"].as<size_t>();

    // TODO: require this backend not to run with multiple threads per rank
    assert(n_threads == 1);
  }

  if (args["backend-n-ranks"].as<bool>()) {
    ranks = args["backend-n-ranks"].as<size_t>();
    assert(ranks > 0);
  }

  if (args["serial-backend-n-ranks"].as<bool>()) {
    ranks = args["serial-backend-n-ranks"].as<size_t>();
    assert(ranks > 0);
  }

  // read number of ranks from the command line
  if (args["ranks"].as<bool>()) {
    ranks = args["ranks"].as<size_t>();

    assert(ranks > 0);
    // TODO: write some other sanity assertions here about the size of ranks...
  }

  if (args["trace"].as<bool>()) {
    size_t traceInt = args["trace"].as<size_t>();
    if (traceInt) {
      threads_backend::traceMode = true;
    }
  }

  // read number of ranks from the command line
  if (args["bf"].as<bool>()) {
    auto bf = args["bf"].as<size_t>() != 0;
    if (threads_backend::this_rank == 0) {
      threads_backend::bwidth = args["bf"].as<size_t>();
    }
    depth = not bf;
  }

  if (threads_backend::this_rank == 0) {
    threads_backend::n_ranks = ranks;
    threads_backend::depthFirstExpand = depth;
  }

  if (threads_backend::this_rank == 0) {
    if (threads_backend::depthFirstExpand) {
      STARTUP_PRINT("DARMA: number of ranks = %zu, "
                    "DF-Sched mode (depth-first, rank-threaded scheduler): Tracing=%s\n",
                    threads_backend::n_ranks,
                    threads_backend::traceMode ? "ON" : "OFF");
    } else {
      STARTUP_PRINT("DARMA: number of ranks = %zu, "
                    "BF-Sched mode (breadth-first (B=%zu), rank-threaded scheduler), Tracing=%s\n",
                    threads_backend::n_ranks,
                    threads_backend::bwidth,
                    threads_backend::traceMode ? "ON" : "OFF");
    }
  }

  auto* runtime = new threads_backend::ThreadsRuntime();
  threads_backend::cur_runtime = runtime;

  if (threads_backend::this_rank == 0) {
    DEBUG_PRINT("rank = %zu, ranks = %zu, threads = %zu\n",
                threads_backend::this_rank,
                threads_backend::n_ranks,
                n_threads);

    // launch std::thread for each rank
    threads_backend::live_ranks.resize(threads_backend::n_ranks - 1);
    for (size_t i = 0; i < threads_backend::n_ranks - 1; ++i) {
      threads_backend::live_ranks[i] = std::thread(start_rank_handler, i + 1, argc, argv);
    }
  }

  // setup root task
  runtime->top_level_task = std::move(top_level_task);
  runtime->top_level_task->set_name(darma_runtime::make_key(DARMA_BACKEND_SPMD_NAME_PREFIX,
                                                            threads_backend::this_rank,
                                                            threads_backend::n_ranks));
  threads_backend::current_task = runtime->top_level_task.get();
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
