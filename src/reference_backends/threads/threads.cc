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

/*
 * Debugging prints with mutex
 */
#define __THREADS_BACKEND_DEBUG__         0
#define __THREADS_BACKEND_DEBUG_VERBOSE__ 0

std::mutex __output_mutex;
#define THREADS_PRINTER(fmt, arg...)					\
  do {									\
    std::unique_lock<std::mutex> __output_lg(__output_mutex);		\
    printf("(%zu): DEBUG threads: " fmt,				\
	   threads_backend::this_rank, ##arg);				\
    fflush(stdout);							\
  } while (0);

#define STARTUP_PRINT(fmt, arg...)					\
  do {									\
    std::unique_lock<std::mutex> __output_lg(__output_mutex);		\
    printf("(%zu) THREADS : " fmt, threads_backend::this_rank, ##arg);	\
    fflush(stdout);							\
  } while (0);

#if __THREADS_BACKEND_DEBUG_VERBOSE__
  #define DEBUG_VERBOSE_PRINT(fmt, arg...) THREADS_PRINTER(fmt, ##arg)
  #define DEBUG_PRINT(fmt, arg...)         THREADS_PRINTER(fmt, ##arg)
#else
  #define DEBUG_VERBOSE_PRINT(fmt, arg...)
#endif

#if !defined(DEBUG_PRINT)
  #if __THREADS_BACKEND_DEBUG__
    #define DEBUG_PRINT(fmt, arg...) THREADS_PRINTER(fmt, ##arg)
  #else
    #define DEBUG_PRINT(fmt, arg...)
  #endif
#endif

// TODO: hack this in here for now
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

  // TODO: threads not implemented
  std::vector<std::thread> live_threads;
  std::vector<std::thread> live_ranks;

  // TL state
  __thread runtime_t::task_t* current_task = 0;
  __thread size_t this_rank = 0;
  __thread size_t flow_label = 100;

  // global
  size_t n_ranks = 1;
  bool depthFirstExpand = true;
  size_t bwidth = 100;
  
  struct PackedDataBlock {
    virtual void *get_data() { return data_; }
    size_t size_;
    void *data_ = nullptr;
  };
  
  struct PublishedBlock {
    types::key_t key;
    PackedDataBlock* data = nullptr;
    std::atomic<size_t> expected = {0}, done = {0};

    PublishedBlock() = default;
  };

  // global publish
  std::mutex rank_publish;
  std::unordered_map<std::pair<types::key_t, types::key_t>, PublishedBlock*> published;

  struct DelayedPublish;
  
  struct DataBlock {
    void* data;

    DataBlock(const DataBlock& in) = delete;

    DataBlock(void* data_)
      : refs(1)
      , data(data_)
    { }
    
    DataBlock(int refs_, size_t sz)
      : refs(refs_)
      , data(malloc(sz))
    { }

    int get_refs() const { return refs; }
    void ref(int num = 1) { refs += num; }
    void deref() { refs--; }

    virtual ~DataBlock() { free(data); }

  private:
    int refs;
  };

  struct InnerFlow {
    std::shared_ptr<InnerFlow> same, next, forward, backward;
    types::key_t version_key;
    darma_runtime::abstract::frontend::Handle* handle;
    bool ready, hasDeferred;
    DataBlock* deferred_data_ptr;

    InnerFlow(const InnerFlow& in) = default;
    
    InnerFlow()
      : same(nullptr)
      , next(nullptr)
      , forward(nullptr)
      , backward(nullptr)
      , version_key(darma_runtime::detail::SimpleKey())
      , ready(false)
      , hasDeferred(false)
      , deferred_data_ptr(nullptr)
    { }

    bool check_ready() { return ready || (same ? check_same() : false); }
    bool check_same()  { return same->check_ready(); }
  };

  struct ThreadsFlow
    : public abstract::backend::Flow {

    std::shared_ptr<InnerFlow> inner;

    ThreadsFlow(darma_runtime::abstract::frontend::Handle* handle_)
      : inner(std::make_shared<InnerFlow>())
    {
      inner->handle = handle_;
    }
  };

  struct DelayedPublish {
    std::shared_ptr<InnerFlow> flow;
    const darma_runtime::abstract::frontend::Handle* handle;
    void* data_ptr;
    size_t fetchers;
    types::key_t key;
    types::key_t version;
  };
  
  class ThreadsRuntime
    : public abstract::backend::Runtime {

  public:
    ThreadsRuntime(const ThreadsRuntime& tr) {}

    // TODO: fix any memory consistency bugs with data coordination we
    // need a fence at the least
    std::unordered_map<std::pair<types::key_t,types::key_t>, DataBlock*> data;

    // TODO: multi-threaded half-implemented not working..
    std::vector<std::atomic<size_t>*> deque_counter;
    std::vector<std::mutex> deque_mutex;
    std::vector<std::deque<types::unique_ptr_template<runtime_t::task_t> > > deque;
    std::atomic<bool> finished;
    std::atomic<size_t> ranks;

    types::unique_ptr_template<runtime_t::task_t> top_level_task;

    std::list<types::unique_ptr_template<runtime_t::task_t> > tasks;
    std::list<std::shared_ptr<InnerFlow> > fetches;
    std::list<std::shared_ptr<DelayedPublish> > publishes;

    std::unordered_map<const darma_runtime::abstract::frontend::Handle*, int> handle_refs;
    std::unordered_map<const darma_runtime::abstract::frontend::Handle*,
		         std::list<
			   std::shared_ptr<DelayedPublish>
			 >
		       > handle_pubs;

    ThreadsRuntime() {
      std::atomic_init(&finished, false);
      std::atomic_init<size_t>(&ranks, 1);
    }

  protected:
    size_t count_delayed_work() const { return tasks.size() + fetches.size() + publishes.size(); }
    void schedule_over_breadth() { if (count_delayed_work() > bwidth) cyclic_schedule_work_until_done(); }
    
    virtual void
    register_task(types::unique_ptr_template<runtime_t::task_t>&& task) {
      DEBUG_VERBOSE_PRINT("register task\n");

      #if defined(_BACKEND_MULTITHREADED_RUNTIME)
        // scoped lock for insertion
        {
	  std::lock_guard<std::mutex> guard(deque_mutex[this_rank]);
	  deque[this_rank].emplace_front(std::move(task));
        }

        std::atomic_fetch_add<size_t>(deque_counter[this_rank], 1);
      #endif

      // use depth-first scheduling policy
      if (threads_backend::depthFirstExpand) {
	const bool task_ready = check_dep_task(task);

	if (task_ready)
	  run_task(std::move(task));

	assert(task_ready);
      } else {
	tasks.emplace_back(std::move(task));
	schedule_over_breadth();
      }
    }

    bool register_condition_task(types::unique_ptr_template<runtime_t::task_t>&& task) {
      assert(false);
      return true;
    }

    void reregister_migrated_use(darma_runtime::abstract::frontend::Use* u) {
      assert(false);
    }


    bool check_dep_task(const types::unique_ptr_template<runtime_t::task_t>& task) {
      DEBUG_VERBOSE_PRINT("check_dep_task\n");

      bool ready = true;

      for (auto&& dep : task->get_dependencies()) {
	ThreadsFlow* f_in  = static_cast<ThreadsFlow*>(dep->get_in_flow());
	ThreadsFlow* f_out = static_cast<ThreadsFlow*>(dep->get_out_flow());

	// set readiness of this flow based on symmetric flow structure
	const bool check_ready = f_in->inner->check_ready();
	// try to set readiness if we can find a backward flow that is ready
	const bool check_back_ready = f_in->inner->backward ? f_in->inner->backward->ready : false;

	f_in->inner->ready = check_ready || check_back_ready;
	
	ready &= f_in->inner->ready;
      }

      return ready;
    }

    void run_task(types::unique_ptr_template<runtime_t::task_t>&& task) {
      runtime_t::task_t* prev = current_task;
      types::unique_ptr_template<runtime_t::task_t> cur = std::move(task);
      current_task = cur.get();
      cur.get()->run();
      current_task = prev;
    }

    virtual runtime_t::task_t*
    get_running_task() const {
      DEBUG_VERBOSE_PRINT("get running task\n");
      return current_task;
    }

    virtual void
    register_use(darma_runtime::abstract::frontend::Use* u) {
      ThreadsFlow* f_in  = static_cast<ThreadsFlow*>(u->get_in_flow());
      ThreadsFlow* f_out = static_cast<ThreadsFlow*>(u->get_out_flow());

      auto const handle = u->get_handle();
      const auto& key = handle->get_key();
      const auto version = f_in->inner->same ? f_in->inner->same->version_key : f_in->inner->version_key;

      const bool ready = f_in->inner->check_ready();
      const bool data_exists = data.find({version,key}) != data.end();

      DEBUG_PRINT("%p: register use: ready = %s, data_exists = %s, key = %s, version = %s, handle = %p\n",
		  u,
		  ready ? "true" : "false",
		  data_exists ? "true" : "false",
		  key.human_readable_string().c_str(),
		  version.human_readable_string().c_str(),
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
		    key.human_readable_string().c_str(),
		    f_in->inner->version_key.human_readable_string().c_str(),
		    f_out->inner->version_key.human_readable_string().c_str());
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
		    key.human_readable_string().c_str(),
		    f_in->inner->version_key.human_readable_string().c_str(),
		    f_out->inner->version_key.human_readable_string().c_str());
      }

      // count references to a given handle
      handle_refs[handle]++;

      // save the data pointer for this use for future reference
      f_in->inner->deferred_data_ptr = data[{version,key}];
      f_in->inner->hasDeferred = true;
    }

    DataBlock*
    allocate_block(darma_runtime::abstract::frontend::Handle const* handle) {
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

    virtual Flow*
    make_initial_flow(darma_runtime::abstract::frontend::Handle* handle) {
      ThreadsFlow* f = new ThreadsFlow(handle);
      f->inner->ready = true;

      // allocate a new data block
      auto block = allocate_block(handle);

      // insert new allocated block into the hash
      const auto& key = handle->get_key();
      const auto& version = darma_runtime::detail::SimpleKey();

      data[{version,key}] = block;

      DEBUG_PRINT("make initial flow: ptr = %p, key = %s, handle = %p\n",
		  block->data,
		  key.human_readable_string().c_str(),
		  handle);

      return f;
    }

    bool
    try_fetch(darma_runtime::abstract::frontend::Handle* handle,
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

	  DEBUG_PRINT("try_fetch: unpacking data: buffer_exists = %s, handle = %p\n",
		      buffer_exists ? "true" : "false",
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
		      key.human_readable_string().c_str(),
		      version_key.human_readable_string().c_str(),
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
    
    virtual Flow*
    make_fetching_flow(darma_runtime::abstract::frontend::Handle* handle,
		       types::key_t const& version_key) {
      DEBUG_VERBOSE_PRINT("make fetching flow\n");

      const bool found = try_fetch(handle, version_key);

      ThreadsFlow* f = new ThreadsFlow(handle);

      f->inner->version_key = version_key;
      f->inner->ready = found;

      if (!found) {
	fetches.push_back(f->inner);
      }

      assert(found || !depthFirstExpand);

      return f;
    }

    virtual Flow*
    make_null_flow(darma_runtime::abstract::frontend::Handle* handle) {
      DEBUG_VERBOSE_PRINT("make null flow\n");
      ThreadsFlow* f = new ThreadsFlow(handle);
      return f;
    }
  
    virtual Flow*
    make_same_flow(Flow* from,
		   flow_propagation_purpose_t purpose) {
      DEBUG_VERBOSE_PRINT("make same flow: %d\n", purpose);
      ThreadsFlow* f  = static_cast<ThreadsFlow*>(from);
      ThreadsFlow* f_same = new ThreadsFlow(0);

      // skip list of same for constant lookup.
      f_same->inner->same = f->inner->same ? f->inner->same : f->inner;

      return f_same;
    }

    virtual Flow*
    make_forwarding_flow(Flow* from,
			 flow_propagation_purpose_t purpose) {
      DEBUG_VERBOSE_PRINT("make forwarding flow: %d\n", purpose);

      ThreadsFlow* f  = static_cast<ThreadsFlow*>(from);
      ThreadsFlow* f_forward = new ThreadsFlow(0);
      f->inner->forward = f_forward->inner;
      f_forward->inner->backward = f->inner;
      return f_forward;
    }

    virtual Flow*
    make_next_flow(Flow* from,
		   flow_propagation_purpose_t purpose) {
      DEBUG_VERBOSE_PRINT("make next flow: %d\n", purpose);

      ThreadsFlow* f  = static_cast<ThreadsFlow*>(from);
      ThreadsFlow* f_next = new ThreadsFlow(0);
      f->inner->next = f_next->inner;
      return f_next;
    }
  
    virtual void
    release_use(darma_runtime::abstract::frontend::Use* u) {
      ThreadsFlow* f_in  = static_cast<ThreadsFlow*>(u->get_in_flow());
      ThreadsFlow* f_out = static_cast<ThreadsFlow*>(u->get_out_flow());

      // enable next forward flow
      if (f_in->inner->forward)
	f_in->inner->forward->ready = true;

      // enable each out flow
      f_out->inner->ready = true;

      auto handle = u->get_handle();
      const auto version =
	f_in->inner->same ? f_in->inner->same->version_key : f_in->inner->version_key;
      const auto& key = handle->get_key();

      DEBUG_PRINT("%p: release use: hasDeferred = %s, handle = %p, version = %s, key = %s, data = %p\n",
		  u,
		  f_in->inner->hasDeferred ? "*** true" : "false",
		  handle,
		  version.human_readable_string().c_str(),
		  key.human_readable_string().c_str(),
		  data[{version,key}]->data);

      handle_refs[handle]--;

      if (handle_refs[handle] == 0) {
	if (handle_pubs.find(handle) != handle_pubs.end()) {
	  for (auto delayed_pub = handle_pubs[handle].begin();
	       delayed_pub != handle_pubs[handle].end();
	       ++delayed_pub) {

	    const bool result = try_publish(*delayed_pub, false);

	    DEBUG_PRINT("%p: release use: force publish of handle = %p, publish = %p\n",
			u, handle, *delayed_pub);
	  
	    publishes.remove(*delayed_pub);
	    assert(result);
	  }
	}

	handle_pubs.erase(handle);
	handle_refs.erase(handle);
      }
      
      data[{version,key}]->deref();

      // free released flows, shared ptrs manage inner flows
      delete f_in;
      delete f_out;

      const auto refs = data[{version,key}]->get_refs();
      
      DEBUG_PRINT("%p: release use: refs = %d, ptr = %p, key = %s, version = %s, handle = %p\n",
		  u,
		  refs,
		  data[{version, key}]->data,
		  key.human_readable_string().c_str(),
		  version.human_readable_string().c_str(),
		  handle);

      if (refs == 1) {
	DataBlock* prev = data[{version,key}];
	data.erase({version,key});
	delete prev;
	DEBUG_PRINT("use delete\n");
      }
    }

    bool try_publish(std::shared_ptr<DelayedPublish> publish, const bool remove) {
      const bool ready = publish->flow->check_ready();

      if (ready) {
	// TODO: big lock to handling publishing
	std::lock_guard<std::mutex> guard(threads_backend::rank_publish);

	const auto &expected = publish->fetchers;
	auto &key = publish->version;

	const darma_runtime::abstract::frontend::Handle* handle = publish->handle;
	void* data_ptr = publish->data_ptr;

	const auto refs = publish->flow->deferred_data_ptr->get_refs();
	
	DEBUG_PRINT("try_publish: key = %s, version = %s, data ptr = %p, handle = %p, refs = %d\n",
		    publish->key.human_readable_string().c_str(),
		    key.human_readable_string().c_str(),
		    data_ptr,
		    handle,
		    refs);
	
	assert(expected >= 1);

	const size_t size = handle
	  ->get_serialization_manager()
	  ->get_packed_data_size(data_ptr);

	PublishedBlock* pub = new PublishedBlock();
	pub->data = new PackedDataBlock();

	// set data block for published block
	pub->data->data_ = malloc(size);
	pub->data->size_ = handle
	  ->get_serialization_manager()
	  ->get_metadata_size();

	// call pack method to put it inside the allocated buffer
	handle
	  ->get_serialization_manager()
	  ->pack_data(data_ptr,
		      pub->data->data_);

	pub->expected = expected;
	pub->key = publish->key;

	DEBUG_PRINT("publication: key = %s, version = %s, published data = %p, data ptr = %p\n",
		    pub->key.human_readable_string().c_str(),
		    key.human_readable_string().c_str(),
		    pub->data->data_,
		    data_ptr);

	// free any replacement of this publish version
	if (published.find({key,pub->key}) != published.end()) {
	  PublishedBlock* prev = published[{key,pub->key}];
	  published.erase({key,pub->key});
	  free(prev->data->data_);
	  delete prev->data;
	  delete prev;
	}

	published[{key,pub->key}] = pub;

	if (remove) {
	  // remove from the set of delayed
	  handle_pubs[handle].remove(publish);
	}
      }
      
      return ready;
    }
		     
    virtual void
    publish_use(darma_runtime::abstract::frontend::Use* f,
		darma_runtime::abstract::frontend::PublicationDetails* details) {

      ThreadsFlow* f_in  = static_cast<ThreadsFlow*>(f->get_in_flow());

      const darma_runtime::abstract::frontend::Handle* handle = f->get_handle();

      auto version = details->get_version_name();
      auto key = handle->get_key();

      DEBUG_PRINT("publish_use: ptr = %p, hasDeferred = %s, key = %s, version = %s, handle = %p\n",
		  f->get_data_pointer_reference(),
		  f_in->inner->hasDeferred ? "true" : "false",
		  key.human_readable_string().c_str(),
		  version.human_readable_string().c_str(),
		  handle);

      // TODO: do not create this just to tear it down
      auto pub = std::make_shared<DelayedPublish>
	(DelayedPublish{f_in->inner,
	    handle,
	    f->get_data_pointer_reference(),
	    details->get_n_fetchers(),
	    key,
	    details->get_version_name()}
	);

      assert(f_in->inner->hasDeferred);

      const bool ready = try_publish(pub, false);

      if (!ready) {
	publishes.push_back(pub);
	// insert into the set of delayed
	handle_pubs[handle].push_back(pub);
      }

      assert(ready || !depthFirstExpand);

      schedule_over_breadth();
    }

    // implement a very slow scheduler that just loops over everything
    // possible to find something that can run
    void
    cyclic_schedule_work_until_done() {
      while (tasks.size()     > 0 ||
	     publishes.size() > 0 ||
	     fetches.size()   > 0) {
	if (tasks.size() > 0) {
	  DEBUG_VERBOSE_PRINT("tasks size = %lu\n", tasks.size());
	
	  types::unique_ptr_template<runtime_t::task_t> task = std::move(tasks.back());
	  tasks.pop_back();
	
	  const bool ready = check_dep_task(task);

	  DEBUG_VERBOSE_PRINT("task ready = %s\n", ready ? "true" : "false");
	
	  if (!ready)
	    tasks.emplace_front(std::move(task));
	  else
	    run_task(std::move(task));
	}

	if (publishes.size() > 0) {
	  DEBUG_VERBOSE_PRINT("publishes size = %lu\n", publishes.size());

	  auto pub = publishes.back();
	  publishes.pop_back();

	  const bool ready = try_publish(pub, true);

	  DEBUG_VERBOSE_PRINT("publish ready = %s\n", ready ? "true" : "false");
	  
	  if (!ready)
	    publishes.push_front(pub);
	}

	if (fetches.size() > 0) {
	  DEBUG_VERBOSE_PRINT("fetches size = %lu\n", fetches.size());

	  auto fetch = fetches.back();
	  fetches.pop_back();

	  const bool ready = try_fetch(fetch->handle, fetch->version_key);

	  DEBUG_VERBOSE_PRINT("fetch ready = %s\n", ready ? "true" : "false");
	  
	  if (!ready) {
	    fetches.push_front(fetch);
	  } else {
	    fetch->ready = true;
	  }
	}
      }
    }

    virtual void
    finalize() {
      DEBUG_PRINT("finalize\n");

      cyclic_schedule_work_until_done();
      
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
  };

  // HACK: put an instance of the backend runtime here for the main thread..
  threads_backend::ThreadsRuntime* runtime_inst;
  
} // end namespace threads_backend

// TODO: not used, for multi-threaded runtime
void
scheduler_loop(size_t thd, threads_backend::ThreadsRuntime* runtime) {
  // read rank from TL variable
  const size_t local_rank = threads_backend::this_rank;
  // read number of ranks
  const size_t num_ranks = std::atomic_load<size_t>(&runtime->ranks);

  DEBUG_PRINT("starting scheduler for %zu, number of ranks %zu\n",
	      local_rank, num_ranks);
  
  // spin on termination
  while (!std::atomic_load(&runtime->finished)) {
    if (std::atomic_load<size_t>(runtime->deque_counter[local_rank]) > 0) {
      // scoped lock for removal attempt
      {
	std::lock_guard<std::mutex> guard(runtime->deque_mutex[local_rank]);
      }
    }
  }
}

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

  //scheduler_loop(0, threads_backend::runtime_inst);

  #if defined(_BACKEND_MULTITHREADED_RUNTIME)
    for (size_t i = 0; i < threads_backend::live_threads.size(); i++) {
      DEBUG_PRINT("main thread joining %zu\n", i);
      threads_backend::live_threads[i].join();
    }
  #endif

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
    
    #if defined(_BACKEND_MULTITHREADED_RUNTIME)
      // store the launched threads in a vector
      threads_backend::live_threads.resize(n_threads - 1);
      threads_backend::runtime_inst = runtime;
    
      // init atomics/deques used to coordinate threads
      {
        std::vector<std::mutex> local_mutex(n_threads);
        runtime->deque_mutex.swap(local_mutex);
      }
    
      runtime->deque_counter.resize(n_threads);
      runtime->deque.resize(n_threads);
      std::atomic_store(&runtime->ranks, n_threads);
    
      for (size_t i = 0; i < n_threads; ++i) {
        runtime->deque_counter[i] = new std::atomic<size_t>();
        std::atomic_init<size_t>(runtime->deque_counter[i], 0);
      }
    
      // launch and save each std::thread
      for (size_t i = 0; i < n_threads - 1; ++i) {
        threads_backend::live_threads[i] = std::thread(start_thread_handler, i + 1, runtime);
      }
    #endif
  }

  // setup root task
  runtime->top_level_task = std::move(top_level_task);
  runtime->top_level_task->set_name(darma_runtime::make_key(DARMA_BACKEND_SPMD_NAME_PREFIX,
							    threads_backend::this_rank,
							    threads_backend::n_ranks));
  threads_backend::current_task = runtime->top_level_task.get();
}

#endif /* _THREADS_BACKEND_RUNTIME_ */
