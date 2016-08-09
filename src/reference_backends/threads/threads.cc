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

  // global
  size_t n_ranks = 1;
  bool traceMode = false;
  bool depthFirstExpand = true;
  size_t bwidth = 100;

  // global publish
  std::mutex rank_publish;
  std::unordered_map<std::pair<types::key_t, types::key_t>, PublishedBlock*> published;

  ThreadsRuntime::ThreadsRuntime()
    : produced(0)
    , consumed(0)
  {
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
      ThreadsFlow const* const f_in  = static_cast<ThreadsFlow*>(dep->get_in_flow());
      ThreadsFlow const* const f_out = static_cast<ThreadsFlow*>(dep->get_out_flow());

      // create new trace dependency
      if (f_in != f_out) {
        taskTrace[f_out->inner] = thisLog;
      }

      findAddTraceDep(f_in->inner,thisLog);
    }
  }

  size_t
  ThreadsRuntime::check_dep_task(std::shared_ptr<TaskNode> t) {
    DEBUG_VERBOSE_PRINT("check_dep_task\n");

    size_t dep_count = 0;

    for (auto&& dep : t->task->get_dependencies()) {
      ThreadsFlow* const f_in  = static_cast<ThreadsFlow*>(dep->get_in_flow());
      ThreadsFlow* const f_out = static_cast<ThreadsFlow*>(dep->get_out_flow());

      DEBUG_PRINT("check_dep_task: f_in=%lu, f_out=%lu\n",
                  PRINT_LABEL(f_in),
                  PRINT_LABEL(f_out));

      if (f_in->inner->isFetch) {
        assert(threads_backend::depthFirstExpand);
        blocking_fetch(f_in->inner->handle, f_in->inner->version_key);
        f_in->inner->ready = true;
      }

      // set readiness of this flow based on symmetric flow structure
      const bool check_ready = f_in->inner->check_ready();

      f_in->inner->ready = check_ready;

      if (f_in == f_out) {
        f_in->inner->readers_jc++;
      }

      if (!f_in->inner->ready) {
        if (f_in == f_out) {
          f_in->inner->readers.push_back(t);
        } else {
          f_in->inner->node = t;
        }
        dep_count++;
      }

      // wait for anti-dep
      if (f_in->inner->ready &&
          f_in->inner->readers_jc != 0 &&
          f_in != f_out) {
        f_in->inner->node = t;
        dep_count++;
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
    ThreadsFlow* f_in  = static_cast<ThreadsFlow*>(u->get_in_flow());
    ThreadsFlow* f_out = static_cast<ThreadsFlow*>(u->get_out_flow());

    f_in->inner->uses++;
    f_out->inner->uses++;

    auto const handle = u->get_handle();
    const auto& key = handle->get_key();
    const auto version = f_in->inner->version_key;

    const bool ready = f_in->inner->check_ready();
    const bool data_exists = data.find({version,key}) != data.end();

    DEBUG_PRINT("%p: register use: ready=%s, data_exists=%s, key=%s, version=%s, "
                "handle=%p [in={%ld,use=%ld},out={%ld,use=%ld}], sched=%d, immed=%d\n",
                u,
                PRINT_BOOL_STR(ready),
                PRINT_BOOL_STR(data_exists),
                PRINT_KEY(key),
                PRINT_KEY(version),
                handle,
                PRINT_LABEL(f_in),
                f_in->inner->ref,
                PRINT_LABEL(f_out),
                f_out->inner->ref,
                u->scheduling_permissions(),
                u->immediate_permissions()
               );

    if (data_exists) {
      u->get_data_pointer_reference() = data[{version,key}]->data;

      DEBUG_PRINT("%p: use register, ptr=%p, key=%s, "
                  "in version=%s, out version=%s\n",
                  u,
                  data[{version,key}]->data,
                  PRINT_KEY(key),
                  PRINT_KEY(f_in->inner->version_key),
                  PRINT_KEY(f_out->inner->version_key));
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
                  PRINT_KEY(f_in->inner->version_key),
                  PRINT_KEY(f_out->inner->version_key));
    }

    // count references to a given handle
    handle_refs[handle]++;

    // save the data pointer for this use for future reference
    f_in->inner->deferred_data_ptr = data[{version,key}];
    f_in->inner->hasDeferred = true;
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
  Flow*
  ThreadsRuntime::make_initial_flow(darma_runtime::abstract::frontend::Handle* handle) {
    ThreadsFlow* f = new ThreadsFlow(handle);
    f->inner->ready = true;
    f->inner->handle = handle;
    handle_refs[handle]++;
    return f;
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

        const bool buffer_exists = data.find({version_key,key}) != data.end();
        void* unpack_to = buffer_exists ? data[{version_key,key}]->data : malloc(pub.data->size_);

        DEBUG_PRINT("fetch: unpacking data: buffer_exists = %s, handle = %p\n",
                    PRINT_BOOL_STR(buffer_exists),
                    handle);

        handle
          ->get_serialization_manager()
          ->unpack_data(unpack_to,pub.data->data_);

        if (!buffer_exists) {
          data[{version_key,key}] = new DataBlock(unpack_to);
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

      const bool buffer_exists = data.find({version_key,key}) != data.end();
      void* unpack_to = buffer_exists ? data[{version_key,key}]->data : malloc(pub.data->size_);

      DEBUG_PRINT("fetch: unpacking data: buffer_exists = %s, handle = %p\n",
                  PRINT_BOOL_STR(buffer_exists),
                  handle);

      handle
        ->get_serialization_manager()
        ->unpack_data(unpack_to, pub.data->data_);

      if (!buffer_exists) {
        data[{version_key,key}] = new DataBlock(unpack_to);
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
  Flow*
  ThreadsRuntime::make_fetching_flow(darma_runtime::abstract::frontend::Handle* handle,
                                     types::key_t const& version_key) {
    DEBUG_VERBOSE_PRINT("make fetching flow\n");

    ThreadsFlow* f = new ThreadsFlow(handle);

    handle_refs[handle]++;
    f->inner->handle = handle;
    f->inner->version_key = version_key;

    if (threads_backend::depthFirstExpand) {
      //blocking_fetch(handle, version_key);
      //f->inner->ready = true;
      f->inner->handle = handle;
      f->inner->version_key = version_key;
      f->inner->isFetch = true;
      f->inner->ready = false;
    } else {
      auto node = std::make_shared<FetchNode>(FetchNode{this,f->inner});
      const bool ready = add_fetcher(node,handle,version_key);

      if (ready) {
        ready_local.push_back(node);
      }
      f->inner->ready = false;
    }

    return f;
  }

  /*virtual*/
  Flow*
  ThreadsRuntime::make_null_flow(darma_runtime::abstract::frontend::Handle* handle) {
    DEBUG_VERBOSE_PRINT("make null flow\n");
    ThreadsFlow* f = new ThreadsFlow(handle);

    f->inner->isNull = true;
    f->inner->handle = handle;

    DEBUG_PRINT("null flow %lu\n", PRINT_LABEL(f));

    return f;
  }

  /*virtual*/
  Flow*
  ThreadsRuntime::make_forwarding_flow(Flow* from) {
    DEBUG_VERBOSE_PRINT("make forwarding flow\n");

    ThreadsFlow* f  = static_cast<ThreadsFlow*>(from);
    ThreadsFlow* f_forward = new ThreadsFlow(0);

    DEBUG_PRINT("forwarding flow from %lu to %lu\n",
                PRINT_LABEL(f),
                PRINT_LABEL(f_forward));

    f->inner->next->ref++;

    if (depthFirstExpand) {
      f_forward->inner->ready = true;
    }

    if (getTrace()) {
      task_forwards[f_forward->inner] = f->inner;
    }

    f->inner->forward = f_forward->inner;
    f_forward->inner->handle = f->inner->handle;
    return f_forward;
  }

  /*virtual*/
  Flow*
  ThreadsRuntime::make_next_flow(Flow* from) {
    DEBUG_VERBOSE_PRINT("make next flow: (from=%p)\n", from);

    ThreadsFlow* f  = static_cast<ThreadsFlow*>(from);
    ThreadsFlow* f_next = new ThreadsFlow(0);

    f->inner->next = f_next->inner;
    f_next->inner->handle = f->inner->handle;

    DEBUG_PRINT("next flow from %lu (%p) to %lu (%p)\n",
                PRINT_LABEL(f),
                f,
                PRINT_LABEL(f_next),
                f_next);

    return f_next;
  }

  void
  ThreadsRuntime::cleanup_handle(std::shared_ptr<InnerFlow> flow) {
    auto const* const handle = flow->handle;

    handle_refs[handle]--;

    assert(handle == alias[flow]->handle);
    assert(handle_refs[handle] >= 0);

    DEBUG_PRINT("cleanup_handle identity: %p to %p: refs=%d\n",
                flow->handle,
                alias[flow]->handle,
                handle_refs[handle]);

    if (handle_refs[handle] == 0) {
      if (handle_pubs.find(handle) != handle_pubs.end()) {
        for (auto delayed_pub = handle_pubs[handle].begin(), end = handle_pubs[handle].end();
             delayed_pub != end;
             ++delayed_pub) {

          (*delayed_pub)->node->execute();
          (*delayed_pub)->finished = true;

          // explicitly don't call clean up because we do it manually
          // TODO: fix this problem with iterator

          DEBUG_PRINT("cleanup handle: force publish of handle = %p\n", handle);
        }

        handle_pubs.erase(handle);
      }
      delete_handle_data(handle,
                         flow->version_key,
                         handle->get_key());
      handle_refs.erase(handle);
    }
  }

  void
  ThreadsRuntime::delete_handle_data(darma_runtime::abstract::frontend::Handle const* const handle,
                                     types::key_t version,
                                     types::key_t key) {
    DEBUG_PRINT("delete handle data\n");
    DataBlock* prev = data[{version,key}];
    data.erase({version,key});
    // TODO: is this correct
    if (prev) {
      // call the destructor
      handle
        ->get_serialization_manager()
        ->destroy(prev->data);
      delete prev;
    }
  }

  /*virtual*/
  void
  ThreadsRuntime::establish_flow_alias(Flow* from,
                                       Flow* to) {
    ThreadsFlow* f_from  = static_cast<ThreadsFlow*>(from);
    ThreadsFlow* f_to    = static_cast<ThreadsFlow*>(to);

    DEBUG_PRINT("establish flow alias %lu (ref=%ld,uses=%ld) to %lu (ref=%ld,uses=%ld)\n",
                PRINT_LABEL(f_from),
                f_from->inner->ref,
                f_from->inner->uses,
                PRINT_LABEL(f_to),
                f_to->inner->ref,
                f_to->inner->uses);

    alias[f_from->inner] = f_to->inner;

    // build inverse alias map tracing
    // TODO: free/remove entries from
    // this inverted alias map when appropiate
    if (getTrace()) {
      inverse_alias[f_to->inner] = f_from->inner;
    }

    if (f_from->inner->uses == 0 ||
        f_from->inner->ready) {
      f_to->inner->ref = 0;
      release_node(f_to->inner);
    }

    if (f_from->inner->uses == 0 &&
        f_to->inner->isNull) {
      cleanup_handle(f_from->inner);
    }

    if (f_from->inner->uses == 0) {
      DEBUG_PRINT("remove alias %ld to %ld\n",
                  PRINT_LABEL_INNER(f_from->inner),
                  PRINT_LABEL_INNER(alias[f_from->inner]));

      alias.erase(alias.find(f_from->inner));
      DEBUG_PRINT("establish deleting: %ld\n",
                  PRINT_LABEL(f_from));
      delete f_from;
    }

    if (f_to->inner->isNull) {
      DEBUG_PRINT("establish deleting null: %ld\n",
                  PRINT_LABEL(f_to));
      delete f_to;
    }
  }

  bool
  ThreadsRuntime::test_alias_null(std::shared_ptr<InnerFlow> flow) {
    if (flow->uses == 0 &&
        alias[flow]->isNull) {
      DEBUG_PRINT("remove alias to null %ld to %ld\n",
                  PRINT_LABEL_INNER(flow),
                  PRINT_LABEL_INNER(alias[flow]));
      cleanup_handle(flow);
      alias.erase(alias.find(flow));
      return true;
    }
    return false;
  }

  bool
  ThreadsRuntime::release_alias(std::shared_ptr<InnerFlow> flow,
                                size_t readers) {
    if (alias.find(flow) != alias.end()) {
      if (test_alias_null(flow)) {
        return true;
      }

      DEBUG_PRINT("release_use: releasing alias: %ld, ref=%ld, readers_jc=%ld\n",
                  PRINT_LABEL_INNER(alias[flow]),
                  alias[flow]->ref,
                  alias[flow]->readers_jc);

      alias[flow]->ref--;
      if (alias[flow]->ref == 0) {
        const size_t alias_readers = release_node(alias[flow]);
        alias[flow]->readers_jc += readers;
        DEBUG_PRINT("release_use: releasing alias: %ld, ref=%ld, readers_jc=%ld\n",
                    PRINT_LABEL_INNER(alias[flow]),
                    alias[flow]->ref,
                    alias[flow]->readers_jc);

        release_alias(alias[flow], readers);

        if (flow->readers_jc == 0 && flow->ref == 0) {
          DEBUG_PRINT("remove alias %ld to %ld\n",
                      PRINT_LABEL_INNER(flow),
                      PRINT_LABEL_INNER(alias[flow]));

          alias.erase(alias.find(flow));
        }
      }
      return true;
    }
    return false;
  }

  size_t
  ThreadsRuntime::release_node(std::shared_ptr<InnerFlow> flow) {
    DEBUG_PRINT("releasing flow: %ld, readers=%ld, reader_jc=%ld, ref=%ld\n",
                PRINT_LABEL_INNER(flow),
                flow->readers.size(),
                flow->readers_jc,
                flow->ref);

    if (flow->ref == 0) {
      if (flow->readers_jc == 0) {
        if (flow->node) {
          flow->node->release();
        }
      } else {
        const size_t size = flow->readers.size();
        for (auto iter = flow->readers.begin(),
             iter_end = flow->readers.end();
             iter != iter_end;
             ++iter) {
          DEBUG_PRINT("releasing READER on flow %ld, readers=%ld\n",
                      PRINT_LABEL_INNER(flow),
                      flow->readers.size());
          (*iter)->release();
        }
        flow->readers.clear();
        return size;
      }
    }
    return 0;
  }

  void
  ThreadsRuntime::release_node_p2(std::shared_ptr<InnerFlow> flow) {
    flow->readers_jc--;
    DEBUG_PRINT("releasing node p2: %ld, readers=%ld, reader_jc=%ld, ref=%ld\n",
                PRINT_LABEL_INNER(flow),
                flow->readers.size(),
                flow->readers_jc,
                flow->ref);
    assert(flow->ref == 0 &&
           "Flow must have been ready for readers to have been released");
    if (flow->readers_jc == 0) {
      if (flow->node) {
        flow->node->release();
      }
    }
  }

  bool
  ThreadsRuntime::release_alias_p2(std::shared_ptr<InnerFlow> flow) {
    DEBUG_PRINT("release_use: try find alias\n");

    if (alias.find(flow) != alias.end()) {
      DEBUG_PRINT("release_use: releasing alias: %ld, ref=%ld, readers_jc=%ld\n",
                  PRINT_LABEL_INNER(alias[flow]),
                  alias[flow]->ref,
                  alias[flow]->readers_jc);

      if (test_alias_null(flow)) {
        return true;
      }

      alias[flow]->readers_jc--;

      // TODO: does this assertion ever break?
      assert(alias[flow]->ref == 0 &&
             "Alias flow must have been ready for readers to have been released");

      if (alias[flow]->readers_jc == 0) {
        if (alias[flow]->node) {
          alias[flow]->node->release();
        }
      }

      release_alias_p2(alias[flow]);

      if (flow->readers_jc == 0) {
        DEBUG_PRINT("remove alias %ld to %ld\n",
                    PRINT_LABEL_INNER(flow),
                    PRINT_LABEL_INNER(alias[flow]));
        alias.erase(alias.find(flow));
      }
      return true;
    }
    return false;
  }

  /*virtual*/
  void
  ThreadsRuntime::release_use(darma_runtime::abstract::frontend::Use* u) {
    ThreadsFlow* f_in  = static_cast<ThreadsFlow*>(u->get_in_flow());
    ThreadsFlow* f_out = static_cast<ThreadsFlow*>(u->get_out_flow());

    // enable next forward flow
    if (f_in->inner->forward)
      f_in->inner->forward->ready = true;

    // enable each out flow
    f_out->inner->ready = true;

    auto handle = u->get_handle();
    const auto version = f_in->inner->version_key;
    const auto& key = handle->get_key();

    DEBUG_PRINT("%p: release use: hasDeferred=%s, handle=%p, version=%s, key=%s, data=%p\n",
                u,
                f_in->inner->hasDeferred ? "*** true" : "false",
                handle,
                PRINT_KEY(version),
                PRINT_KEY(key),
                data[{version,key}]->data);

    handle_refs[handle]--;

    // dereference flows
    f_in->inner->uses--;
    f_out->inner->uses--;

    DEBUG_PRINT("release_use: f_in=[%ld,{use=%ld}], f_out=[%ld,{use=%ld}]: handle_refs=%d\n",
                PRINT_LABEL(f_in),
                f_in->inner->uses,
                PRINT_LABEL(f_out),
                f_out->inner->uses,
                handle_refs[handle]);

    const bool same = f_in == f_out;
    bool deleted_out = false;

    if (f_in == f_out) {
      release_node_p2(f_out->inner);
      const bool hasAlias = release_alias_p2(f_out->inner);
      if (hasAlias &&
          f_out->inner->uses == 0) {
        DEBUG_PRINT("deleting redirect: %ld\n", PRINT_LABEL(f_out));
        delete f_out;
        deleted_out = true;
      }
    } else {
      const size_t readers = release_node(f_out->inner);
      if (f_out->inner->ref == 0) {
        const bool hasAlias = release_alias(f_out->inner, readers);
        if (hasAlias &&
            f_out->inner->uses == 0) {
          DEBUG_PRINT("deleting redirect: %ld\n", PRINT_LABEL(f_out));
          delete f_out;
          deleted_out = true;
        }
      }
    }

    if (!deleted_out &&
        f_out->inner->uses == 0 &&
        (f_out->inner->next || f_out->inner->forward)) {
      DEBUG_PRINT("deleting: %ld\n", PRINT_LABEL(f_out));
      delete f_out;
      deleted_out = true;
    }

    if ((!same || !deleted_out) &&
        (f_in->inner->next || f_in->inner->forward) &&
        f_in->inner->uses == 0) {
      DEBUG_PRINT("deleting: %ld\n", PRINT_LABEL(f_in));
      delete f_in;
    }
  }

  bool
  ThreadsRuntime::test_publish(std::shared_ptr<DelayedPublish> publish) {
    return publish->flow->check_ready();
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

      const size_t size = handle
        ->get_serialization_manager()
        ->get_packed_data_size(data_ptr);

      auto block = new PackedDataBlock();

      // set data block for published block
      block->data_ = malloc(size);
      block->size_ = handle
        ->get_serialization_manager()
        ->get_metadata_size();

      // call pack method to put it inside the allocated buffer
      handle
        ->get_serialization_manager()
        ->pack_data(data_ptr,
                    block->data_);

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
        for (auto iter = publish->waiting.begin(),
             iter_end = publish->waiting.end();
             iter != iter_end;
             ++iter) {
          (*iter)->activate();
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
  }

  /*virtual*/
  void
  ThreadsRuntime::publish_use(darma_runtime::abstract::frontend::Use* f,
                              darma_runtime::abstract::frontend::PublicationDetails* details) {

    ThreadsFlow* f_in  = static_cast<ThreadsFlow*>(f->get_in_flow());
    ThreadsFlow* f_out = static_cast<ThreadsFlow*>(f->get_out_flow());

    const darma_runtime::abstract::frontend::Handle* handle = f->get_handle();

    auto version = details->get_version_name();
    auto key = handle->get_key();

    // TODO: do not create this just to tear it down
    auto pub = std::make_shared<DelayedPublish>
      (DelayedPublish{f_in->inner,
          handle,
          f->get_data_pointer_reference(),
          details->get_n_fetchers(),
          key,
          details->get_version_name(),
          false
       });

    assert(f_in->inner->hasDeferred);

    auto p = std::make_shared<PublishNode>(PublishNode{this,pub});

    // set the node for early release
    pub->node = p;

    const bool ready = p->ready();

    DEBUG_PRINT("%p: publish_use: ptr=%p, hasDeferred=%s, key=%s, "
                "version=%s, handle=%p, ready=%s, f_in=%lu, f_out=%lu\n",
                f,
                f->get_data_pointer_reference(),
                PRINT_BOOL_STR(f_in->inner->hasDeferred),
                PRINT_KEY(key),
                PRINT_KEY(version),
                handle,
                PRINT_BOOL_STR(ready),
                PRINT_LABEL(f_in),
                PRINT_LABEL(f_out));

    if (!ready) {
      // publishes.push_back(p);
      // insert into the set of delayed

      p->set_join(1);
      f_in->inner->node = p;

      handle_pubs[handle].push_back(pub);
    } else {
      p->execute();
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
  bool
  ThreadsRuntime::schedule_from_deque(std::mutex* lock,
                                      std::deque<Node>& nodes) {
    if (nodes.size() > 0) {
      DEBUG_PRINT("scheduling from deque %p (local = %p): size = %lu\n",
                  &nodes, &ready_local, nodes.size());

      if (lock) lock->lock();
      auto node = nodes.front();
      nodes.pop_front();
      if (lock) lock->unlock();

      node->execute();
      node->cleanup();

      return true;
    }
    return false;
  }

  void
  ThreadsRuntime::schedule_next_unit() {
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

    while (this->produced != this->consumed) {
      /// DEBUG_PRINT("produced = %ld, consumed = %ld\n", this->produced, this->consumed);
      schedule_next_unit();
    }

    assert(ready_local.size() == 0 &&
           ready_remote.size() == 0 &&
           "TD failed if queues have work units in them.");

    STARTUP_PRINT("work units: produced=%ld, consumed=%ld\n",
                  this->produced,
                  this->consumed);

    for (auto iter = alias.begin();
         iter != alias.end();
         ++iter) {
      DEBUG_PRINT("alias flow[%ld] = %ld\n",
                  PRINT_LABEL_INNER(iter->first),
                  PRINT_LABEL_INNER(iter->second));
    }

    DEBUG_PRINT("handle_refs=%ld, "
                "handle_pubs=%ld, "
                "\n",
                handle_refs.size(),
                handle_pubs.size());

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
  darma_runtime::abstract::backend::Runtime *&backend_runtime,
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
  backend_runtime = runtime;

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

#endif /* _THREADS_BACKEND_RUNTIME_ */
