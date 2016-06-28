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

#include <threads_interface.h>
#include <common.h>

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
  struct TaskNode;
  struct FetchNode;
  struct PublishNode;
  
  struct DelayedPublish {
    std::shared_ptr<InnerFlow> flow;
    const darma_runtime::abstract::frontend::Handle* handle;
    void* data_ptr;
    size_t fetchers;
    types::key_t key;
    types::key_t version;
    bool finished;
  };
  
  class ThreadsRuntime
    : public abstract::backend::Runtime
    , public ThreadsInterface<ThreadsRuntime> {

  public:
    ThreadsRuntime(const ThreadsRuntime& tr);

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

    std::deque<std::shared_ptr<GraphNode> > ready_local;

    std::mutex lock_remote;
    std::deque<std::shared_ptr<GraphNode> > ready_remote;

    std::unordered_map<const darma_runtime::abstract::frontend::Handle*, int> handle_refs;
    std::unordered_map<const darma_runtime::abstract::frontend::Handle*,
		         std::list<
			   std::shared_ptr<DelayedPublish>
			 >
		       > handle_pubs;

    size_t produced, consumed;
    
    ThreadsRuntime();

    void
    release_deps(std::shared_ptr<InnerFlow> flow);

    void
    add_remote(std::shared_ptr<GraphNode> task);

    void
    add_local(std::shared_ptr<GraphNode> task);
    
    size_t
    count_delayed_work() const;

    void
    schedule_over_breadth();
    
    virtual void
    register_task(types::unique_ptr_template<runtime_t::task_t>&& task);

    bool
    register_condition_task(types::unique_ptr_template<runtime_t::task_t>&& task);

    void
    reregister_migrated_use(darma_runtime::abstract::frontend::Use* u);

    size_t
    check_dep_task(std::shared_ptr<TaskNode> t);

    void
    run_task(types::unique_ptr_template<runtime_t::task_t>&& task);

    virtual runtime_t::task_t*
    get_running_task() const;

    virtual void
    register_use(darma_runtime::abstract::frontend::Use* u);

    DataBlock*
    allocate_block(darma_runtime::abstract::frontend::Handle const* handle);
    
    virtual Flow*
    make_initial_flow(darma_runtime::abstract::frontend::Handle* handle);

    bool
    add_fetcher(std::shared_ptr<FetchNode> fetch,
		darma_runtime::abstract::frontend::Handle* handle,
		types::key_t const& version_key);
    bool
    try_fetch(darma_runtime::abstract::frontend::Handle* handle,
	      types::key_t const& version_key);
    bool
    test_fetch(darma_runtime::abstract::frontend::Handle* handle,
	       types::key_t const& version_key);
    void
    blocking_fetch(darma_runtime::abstract::frontend::Handle* handle,
		   types::key_t const& version_key);
    void
    fetch(darma_runtime::abstract::frontend::Handle* handle,
	  types::key_t const& version_key);

    virtual Flow*
    make_fetching_flow(darma_runtime::abstract::frontend::Handle* handle,
		       types::key_t const& version_key);
    virtual Flow*
    make_null_flow(darma_runtime::abstract::frontend::Handle* handle);
 
    virtual Flow*
    make_same_flow(Flow* from,
		   flow_propagation_purpose_t purpose);

    virtual Flow*
    make_forwarding_flow(Flow* from,
			 flow_propagation_purpose_t purpose);
    virtual Flow*
    make_next_flow(Flow* from,
		   flow_propagation_purpose_t purpose);
    virtual void
    release_use(darma_runtime::abstract::frontend::Use* u);

    bool
    test_publish(std::shared_ptr<DelayedPublish> publish);

    void
    publish(std::shared_ptr<DelayedPublish> publish);

    void
    publish_finished(std::shared_ptr<DelayedPublish> publish);
		     
    virtual void
    publish_use(darma_runtime::abstract::frontend::Use* f,
		darma_runtime::abstract::frontend::PublicationDetails* details);

    template <typename Node>
    void
    try_node(std::list<std::shared_ptr<Node> >& nodes);

    template <typename Node>
    bool schedule_from_deque(std::mutex* lock, std::deque<Node>& nodes);

    template <typename... Args>
    void try_release(Args... args);
    
    void
    schedule_next_unit();

    virtual void
    finalize();
  };
}

#endif /* _THREADS_BACKEND_RUNTIME_H_ */
