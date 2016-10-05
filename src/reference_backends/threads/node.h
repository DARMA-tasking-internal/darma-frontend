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
  // tracing externs for automatic labeling
  extern __thread size_t task_label;
  extern __thread size_t publish_label;
  extern __thread size_t fetch_label;

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
    {
      runtime->produce();
    }

    void set_join(size_t join_counter_) {
      join_counter = join_counter_;
    }

    virtual void release()  {
      DEBUG_PRINT("%p join counter is now %zu\n",
                  this,
                  join_counter - 1);

      if (--join_counter == 0) {
        runtime->add_local(this->shared_from_this());
      }
    }

    // execute the graph node
    virtual void execute() {
      runtime->consume();
    }

    // check readiness of the node
    virtual bool ready() = 0;

    // post execution
    virtual void cleanup() { }

    // another thread waking this
    virtual void activate() { }

    virtual ~GraphNode() = default;
  };

  struct FetchNode
    : public GraphNode
  {
    std::shared_ptr<InnerFlow> fetch;

    FetchNode(Runtime* rt,
              std::shared_ptr<InnerFlow> fetch_)
      : GraphNode(-1, rt)
      , fetch(fetch_)
    { }

    void execute() override {
      std::string genName = "";

      TraceLog* log = nullptr;
      if (runtime->getTrace()) {
        genName = "fetch-" + std::to_string(fetch_label++);
        log = runtime->getTrace()->eventStartNow(genName);
      }

      TraceLog* pub_log = runtime->fetch(
        fetch->handle.get(),
        fetch->version_key
      );

      if (runtime->getTrace()) {
        runtime->addFetchDeps(this,log,pub_log);
      }

      if (runtime->getTrace()) {
        runtime->getTrace()->eventStopNow(genName);
      }

      fetch->ready = true;

      DEBUG_PRINT("=== EXECUTING === fetch node\n");

      runtime->try_release_to_read(fetch);

      GraphNode::execute();
    }

    bool ready() override {
      return runtime->test_fetch(
        fetch->handle.get(),
        fetch->version_key
      );
    }

    virtual ~FetchNode() {
      DEBUG_PRINT("destructing fetch node, flow=%ld\n",
                  PRINT_LABEL(fetch)
                  );
    }

    void activate() override {
      DEBUG_PRINT("=== ACTIVATING === fetch node\n");
      runtime->add_remote(this->shared_from_this());
    }
  };

  struct PublishNode
    : public GraphNode
  {
    std::shared_ptr<DelayedPublish> pub;

    PublishNode(Runtime* rt,
                std::shared_ptr<DelayedPublish> pub_)
      : GraphNode(-1, rt)
      , pub(pub_)
    { }

    void execute() {
      DEBUG_PRINT("=== EXECUTING === publish node\n");

      if (!pub->finished) {
        std::string genName = "";
        TraceLog* log = nullptr;
        if (runtime->getTrace()) {
          genName = "publish-" + std::to_string(publish_label++);
          log = runtime->getTrace()->eventStartNow(genName);
          runtime->addPublishDeps(this,log);
        }

        runtime->publish(pub,log);

        if (runtime->getTrace()) {
          runtime->getTrace()->eventStopNow(genName);
        }

        GraphNode::execute();
      }
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

  struct CollectiveNode
    : GraphNode {

    std::shared_ptr<CollectiveInfo> info;
    bool finished = false;

    CollectiveNode(Runtime* rt,
                   std::shared_ptr<CollectiveInfo> info)
      : GraphNode(-1, rt)
      , info(info)
    { }

    void block_execute() {
      runtime->blocking_collective(info);
      GraphNode::execute();
      finished = true;
    }

    void execute() override {
      if (!info->incorporated_local) {
        const bool isFinished = runtime->collective(info);
        if (isFinished) {
          GraphNode::execute();
          finished = true;
        }
      } else {
        runtime->collective_finish(info);
        GraphNode::execute();
        finished = true;
      }
    }

    bool ready() override {
      return info->flow->ready;
    }

    virtual ~CollectiveNode() { }

    void activate() override {
      runtime->add_remote(this->shared_from_this());
    }
  };

  template <typename TaskType>
  struct TaskNode
    : public GraphNode
  {
    types::unique_ptr_template<TaskType> task;

    TaskNode(Runtime* rt,
             types::unique_ptr_template<TaskType>&& task_)
      : GraphNode(-1, rt)
      , task(std::move(task_))
    { }

    virtual void execute() {
      // start trace event
      std::string genName = "";
      if (runtime->getTrace()) {
        TraceLog* log = nullptr;
        if (task->get_name() == darma_runtime::detail::SimpleKey()) {
          genName = "task-" + std::to_string(task_label++);
        } else {
          genName = task->get_name().human_readable_string("_","","");
        }
        log = runtime->getTrace()->eventStartNow(genName);
        runtime->addTraceDeps(this,log);
      }

      runtime->run_task(std::move(task));

      // end trace event
      if (runtime->getTrace()) {
        runtime->getTrace()->eventStopNow(genName);
      }

      GraphNode::execute();
    }

    virtual bool ready() {
      return join_counter == 0;
    }
  };

  // struct ConditionTaskNode
  //   : TaskNode
  // {
  //   ThreadsRuntime::condition_task_unique_ptr task;

  //   ConditionTaskNode(Runtime* rt,
  //                     ThreadsRuntime::condition_task_unique_ptr&& task_)
  //     : GraphNode(-1, rt)
  //     , task(std::move(task_))
  //   { }
  // };
}

#endif /* _THREADS_NODE_BACKEND_RUNTIME_H_ */