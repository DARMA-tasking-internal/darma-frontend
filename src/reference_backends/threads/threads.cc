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

  // global
  size_t n_ranks = 1;
  bool depthFirstExpand = true;
  size_t bwidth = 100;
  
  // global publish
  std::mutex rank_publish;
  std::unordered_map<std::pair<types::key_t, types::key_t>, PublishedBlock*> published;

  ThreadsRuntime::ThreadsRuntime(const ThreadsRuntime& tr) {}
  
  ThreadsRuntime::ThreadsRuntime()
    : produced(0)
    , consumed(0)
  {
    std::atomic_init(&finished, false);
    std::atomic_init<size_t>(&ranks, 1);
  }

  size_t
  ThreadsRuntime::count_delayed_work() const {
    return ready_local.size();
  }

  void
  ThreadsRuntime::release_deps(std::shared_ptr<InnerFlow> inner) {
    if (inner->node) {
      inner->node->release();
    }
    for (auto dep = inner->deps.begin(),
           dep_end = inner->deps.end();
         dep != dep_end;
         ++dep) {
      if ((*dep)->node) {
        (*dep)->node->release();
      }
    }
    inner->deps.clear();
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
    if (count_delayed_work() > bwidth)
      schedule_next_unit();
  }
    
  /*virtual*/
  void
  ThreadsRuntime::register_task(types::unique_ptr_template<runtime_t::task_t>&& task) {
    DEBUG_VERBOSE_PRINT("register task\n");

    auto t = std::make_shared<TaskNode>(TaskNode{this,std::move(task)});
    t->join_counter = check_dep_task(t);
    this->produced++;

    // use depth-first scheduling policy
    if (threads_backend::depthFirstExpand) {
      assert(t->ready());
      DEBUG_VERBOSE_PRINT("running task\n");
      t->execute();
      this->consumed++;
    } else {
      if (t->ready()) {
        ready_local.push_back(t);
      }
    }
  }

  bool
  ThreadsRuntime::register_condition_task(types::unique_ptr_template<runtime_t::task_t>&& task) {
    auto t = std::make_shared<TaskNode>(TaskNode{this,std::move(task)});
    t->join_counter = check_dep_task(t);
    this->produced++;

    assert(threads_backend::depthFirstExpand);

    if (threads_backend::depthFirstExpand) {
      assert(t->ready());
      DEBUG_VERBOSE_PRINT("running task\n");

      this->consumed++;

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

  size_t
  ThreadsRuntime::check_dep_task(std::shared_ptr<TaskNode> t) {
    DEBUG_VERBOSE_PRINT("check_dep_task\n");

    size_t dep_count = 0;

    for (auto&& dep : t->task->get_dependencies()) {
      ThreadsFlow* f_in  = static_cast<ThreadsFlow*>(dep->get_in_flow());
      ThreadsFlow* f_out = static_cast<ThreadsFlow*>(dep->get_out_flow());

      DEBUG_PRINT("check_dep_task: f_in = %lu, f_out = %lu\n",
                  PRINT_LABEL(f_in),
                  PRINT_LABEL(f_out));
      
      // set readiness of this flow based on symmetric flow structure
      const bool check_ready = f_in->inner->check_ready();

      f_in->inner->ready = check_ready;

      if (!f_in->inner->ready) {
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

    auto const handle = u->get_handle();
    const auto& key = handle->get_key();
    const auto version = f_in->inner->version_key;

    const bool ready = f_in->inner->check_ready();
    const bool data_exists = data.find({version,key}) != data.end();

    DEBUG_PRINT("%p: register use: ready = %s, data_exists = %s, key = %s, version = %s, handle = %p\n",
                u,
                PRINT_BOOL_STR(ready),
                PRINT_BOOL_STR(data_exists),
                PRINT_KEY(key),
                PRINT_KEY(version),
                handle);

    if (data_exists) {
      // increase reference count on data
      data[{version,key}]->ref();
      u->get_data_pointer_reference() = data[{version,key}]->data;

      DEBUG_PRINT("%p: use register, refs = %d, ptr = %p, key = %s, "
                  "in version = %s, out version = %s\n",
                  u,
                  data[{version,key}]->get_refs(),
                  data[{version,key}]->data,
                  PRINT_KEY(key),
                  PRINT_KEY(f_in->inner->version_key),
                  PRINT_KEY(f_out->inner->version_key));
    } else {
      // allocate new deferred data block for this use
      auto block = allocate_block(handle);

      // insert into the hash
      data[{version,key}] = block;
      block->ref();
      u->get_data_pointer_reference() = block->data;

      DEBUG_PRINT("%p: use register: *deferred*: ptr = %p, key = %s, "
                  "in version = %s, out version = %s\n",
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

    // allocate a new data block
    auto block = allocate_block(handle);

    // insert new allocated block into the hash
    const auto& key = handle->get_key();
    const auto& version = darma_runtime::detail::SimpleKey();

    data[{version,key}] = block;

    DEBUG_PRINT("make initial flow: ptr = %p, key = %s, handle = %p, flow = %lu\n",
                block->data,
                PRINT_KEY(key),
                handle,
                PRINT_LABEL(f));

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
        } else {
          data[{version_key,key}]->ref();
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

  void
  ThreadsRuntime::fetch(darma_runtime::abstract::frontend::Handle* handle,
                        types::key_t const& version_key) {
    {
      // TODO: big lock to handling fetching
      std::lock_guard<std::mutex> guard(threads_backend::rank_publish);

      const auto& key = handle->get_key();
      const auto& iter = published.find({version_key,key});

      // published block found and ready
      assert(iter != published.end() &&
             std::atomic_load<bool>(&iter->second->ready));

      PublishedBlock* pub_ptr = iter->second;
      auto &pub = *pub_ptr;

      const bool buffer_exists = data.find({version_key,key}) != data.end();
      void* unpack_to = buffer_exists ? data[{version_key,key}]->data : malloc(pub.data->size_);

      DEBUG_PRINT("try_fetch: unpacking data: buffer_exists = %s, handle = %p\n",
                  PRINT_BOOL_STR(buffer_exists),
                  handle);

      handle
        ->get_serialization_manager()
        ->unpack_data(unpack_to, pub.data->data_);

      if (!buffer_exists) {
        data[{version_key,key}] = new DataBlock(unpack_to);
      } else {
        data[{version_key,key}]->ref();
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
    }
  }
    
  /*virtual*/
  Flow*
  ThreadsRuntime::make_fetching_flow(darma_runtime::abstract::frontend::Handle* handle,
                                     types::key_t const& version_key) {
    DEBUG_VERBOSE_PRINT("make fetching flow\n");

    ThreadsFlow* f = new ThreadsFlow(handle);

    f->inner->version_key = version_key;

    if (threads_backend::depthFirstExpand) {
      blocking_fetch(handle, version_key);
      f->inner->ready = true;
    } else {
      auto node = std::make_shared<FetchNode>(FetchNode{this,f->inner});
      const bool ready = add_fetcher(node,handle,version_key);
      this->produced++;

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

    DEBUG_PRINT("null flow %lu\n", PRINT_LABEL(f));

    return f;
  }
  
  /*virtual*/
  Flow*
  ThreadsRuntime::make_same_flow(Flow* from,
                                 flow_propagation_purpose_t purpose) {
    DEBUG_VERBOSE_PRINT("make same flow: %d\n", purpose);
    ThreadsFlow* f  = static_cast<ThreadsFlow*>(from);
    ThreadsFlow* f_same = new ThreadsFlow(0);

    f_same->inner = f->inner;
    
    return f_same;
  }

  /*virtual*/
  Flow*
  ThreadsRuntime::make_forwarding_flow(Flow* from,
                                       flow_propagation_purpose_t purpose) {
    DEBUG_VERBOSE_PRINT("make forwarding flow: %d\n", purpose);

    ThreadsFlow* f  = static_cast<ThreadsFlow*>(from);
    ThreadsFlow* f_forward = new ThreadsFlow(0);

    DEBUG_PRINT("forwarding flow from %lu to %lu\n",
                PRINT_LABEL(f),
                PRINT_LABEL(f_forward));
    
    if (depthFirstExpand) {
      f_forward->inner->ready = true;
    }
    
    f->inner->forward = f_forward->inner;
    return f_forward;
  }

  /*virtual*/
  Flow*
  ThreadsRuntime::make_next_flow(Flow* from,
                                 flow_propagation_purpose_t purpose) {
    DEBUG_VERBOSE_PRINT("make next flow: %d\n", purpose);

    ThreadsFlow* f  = static_cast<ThreadsFlow*>(from);
    ThreadsFlow* f_next = new ThreadsFlow(0);

    DEBUG_PRINT("next flow from %lu to %lu\n",
                PRINT_LABEL(f),
                PRINT_LABEL(f_next));

    return f_next;
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

    DEBUG_PRINT("%p: release use: hasDeferred = %s, handle = %p, version = %s, key = %s, data = %p\n",
                u,
                f_in->inner->hasDeferred ? "*** true" : "false",
                handle,
                PRINT_KEY(version),
                PRINT_KEY(key),
                data[{version,key}]->data);

    handle_refs[handle]--;

    if (handle_refs[handle] == 0) {
      if (handle_pubs.find(handle) != handle_pubs.end()) {
        for (auto delayed_pub = handle_pubs[handle].begin(), end = handle_pubs[handle].end();
             delayed_pub != end;
             ++delayed_pub) {

          this->consumed++;
          publish(*delayed_pub);
          (*delayed_pub)->finished = true;

          DEBUG_PRINT("%p: release use: force publish of handle = %p\n", u, handle);
        }
      }

      handle_pubs.erase(handle);
      handle_refs.erase(handle);
    }
      
    data[{version,key}]->deref();

    DEBUG_PRINT("release_use: f_in %ld, f_out %ld\n",
                PRINT_LABEL(f_in),
                PRINT_LABEL(f_out));
    
    release_deps(f_out->inner);

    // free released flows, shared ptrs manage inner flows
    delete f_in;
    delete f_out;

    const auto refs = data[{version,key}]->get_refs();
      
    DEBUG_PRINT("%p: release use: refs = %d, ptr = %p, key = %s, version = %s, handle = %p\n",
                u,
                refs,
                data[{version, key}]->data,
                PRINT_KEY(key),
                PRINT_KEY(version),
                handle);

    if (refs == 1) {
      DataBlock* prev = data[{version,key}];
      data.erase({version,key});
      delete prev;
      DEBUG_PRINT("use delete\n");
    }
  }

  bool
  ThreadsRuntime::test_publish(std::shared_ptr<DelayedPublish> publish) {
    return publish->flow->check_ready();
  }

  void
  ThreadsRuntime::publish(std::shared_ptr<DelayedPublish> publish) {
    if (publish->finished) return;
    
    {
      // TODO: big lock to handling publishing
      std::lock_guard<std::mutex> guard(threads_backend::rank_publish);

      const auto& expected = publish->fetchers;
      const auto& version = publish->version;
      const auto& key = publish->key;

      const darma_runtime::abstract::frontend::Handle* handle = publish->handle;
      void* data_ptr = publish->data_ptr;

      const auto refs = publish->flow->deferred_data_ptr->get_refs();
        
      DEBUG_PRINT("publish: key = %s, version = %s, data ptr = %p, handle = %p, refs = %d\n",
                  PRINT_KEY(key),
                  PRINT_KEY(version),
                  data_ptr,
                  handle,
                  refs);
        
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
    handle_pubs[publish->handle].remove(publish);
  }
  
  /*virtual*/
  void
  ThreadsRuntime::publish_use(darma_runtime::abstract::frontend::Use* f,
                              darma_runtime::abstract::frontend::PublicationDetails* details) {

    ThreadsFlow* f_in  = static_cast<ThreadsFlow*>(f->get_in_flow());

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

    const bool ready = p->ready();

    DEBUG_PRINT("%p: publish_use: ptr = %p, hasDeferred = %s, key = %s, "
                "version = %s, handle = %p, ready = %s, f_in = %lu\n",
                f,
                f->get_data_pointer_reference(),
                PRINT_BOOL_STR(f_in->inner->hasDeferred),
                PRINT_KEY(key),
                PRINT_KEY(version),
                handle,
                PRINT_BOOL_STR(ready),
                PRINT_LABEL(f_in));
    
    if (!ready) {
      // publishes.push_back(p);
      // insert into the set of delayed

      p->set_join(1);
      f_in->inner->node = p;

      this->produced++;
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
  ThreadsRuntime::schedule_from_deque(std::mutex* lock, std::deque<Node>& nodes) {
    if (nodes.size() > 0) {
      DEBUG_PRINT("scheduling from deque %p (local = %p): size = %lu\n",
                  &nodes, &ready_local, nodes.size());

      if (lock) lock->lock();
      auto node = nodes.front();
      nodes.pop_front();
      if (lock) lock->unlock();

      node->execute();
      this->consumed++;

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
    DEBUG_PRINT("finalize\n");

    while (this->produced != this->consumed) {
      //DEBUG_PRINT("produced = %ld, consumed = %ld\n", this->produced, this->consumed);
      schedule_next_unit();
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

  // run scheduler, when this returns we have terminated!
  scheduler_loop(thd, runtime);
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
                    "DF-Sched mode (depth-first, rank-threaded scheduler)\n",
                    threads_backend::n_ranks);
    } else {
      STARTUP_PRINT("DARMA: number of ranks = %zu, "
                    "BF-Sched mode (breadth-first (B=%zu), rank-threaded scheduler)\n",
                    threads_backend::n_ranks,
                    threads_backend::bwidth);
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
