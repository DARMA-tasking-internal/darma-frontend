/*
//@HEADER
// ************************************************************************
//
//                         node.h
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

#if !defined(_THREADS_NODE_BACKEND_RUNTIME_H_)
#define _THREADS_NODE_BACKEND_RUNTIME_H_

#include <darma/interface/backend/flow.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/defaults/darma_main.h>

#include <threads_interface.h>
#include <flow.h>

namespace threads_backend {
  using namespace darma_runtime;
  using namespace darma_runtime::abstract::backend;

  typedef ThreadsInterface<ThreadsRuntime> Runtime;
  
  struct GraphNode
    : public std::enable_shared_from_this<GraphNode> {

    Runtime* runtime;
    size_t join_counter;

    GraphNode(size_t join_counter_,
	      Runtime* runtime_)
      : join_counter(join_counter_)
      , runtime(runtime_)
    { }

    void set_join(size_t join_counter_) {
      join_counter = join_counter_;
    }
    
    virtual void release()  {
      DEBUG_PRINT("join counter is now %zu\n", join_counter - 1);

      if (--join_counter == 0) {
	runtime->add_local(this->shared_from_this());
      }
    }
    
    // execute the graph node
    virtual void execute() = 0;

    // check readiness of the node
    virtual bool ready() = 0;

    // post execution
    virtual void cleanup() { }

    virtual ~GraphNode() = default;
  };

  struct FetchNode
    : public GraphNode
  {
    std::shared_ptr<InnerFlow> fetch;

    FetchNode(Runtime* rt, std::shared_ptr<InnerFlow> fetch_)
      : GraphNode(-1, rt)
      , fetch(fetch_)
    { }

    void execute() {
      runtime->fetch(fetch->handle, fetch->version_key);
      fetch->ready = true;
      runtime->release_deps(fetch);
    }

    bool ready() {
      return runtime->test_fetch(fetch->handle, fetch->version_key);
    }

    virtual ~FetchNode() {
      fetch = nullptr;
    }

    void activate() {
      runtime->add_remote(this->shared_from_this());
    }
  };

  struct PublishNode
    : public GraphNode
  {
    std::shared_ptr<DelayedPublish> pub;

    PublishNode(Runtime* rt, std::shared_ptr<DelayedPublish> pub_)
      : GraphNode(-1, rt)
      , pub(pub_)
    { }

    void execute() {
      runtime->publish(pub);
    }

    void cleanup() {
      runtime->publish_finished(pub);
    }

    bool ready() {
      return runtime->test_publish(pub);
    }

    virtual ~PublishNode() {
      pub = nullptr;
    }
  };

  struct TaskNode
    : public GraphNode
  {
    types::unique_ptr_template<runtime_t::task_t> task;

    TaskNode(Runtime* rt,
	     types::unique_ptr_template<runtime_t::task_t>&& task_)
      : GraphNode(-1, rt)
      , task(std::move(task_))
    { }

    void execute() {
      runtime->run_task(std::move(task));
    }

    bool ready() {
      return join_counter == 0;
    }
  };
}

#endif /* _THREADS_NODE_BACKEND_RUNTIME_H_ */
