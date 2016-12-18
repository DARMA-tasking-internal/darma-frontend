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

#include <flow.h>

namespace threads_backend {
  // tracing externs for automatic labeling
  extern __thread size_t task_label;
  extern __thread size_t publish_label;
  extern __thread size_t fetch_label;

  using namespace darma_runtime;
  using namespace darma_runtime::abstract::backend;

  using runtime_t = ThreadsRuntime;

  extern std::vector<ThreadsRuntime*> shared_ranks;

  struct GraphNode
    : std::enable_shared_from_this<GraphNode> {
    runtime_t* runtime;
    size_t join_counter;
    int for_rank = -1;

    GraphNode(size_t join_counter_,
              runtime_t* runtime_)
      : join_counter(join_counter_)
      , runtime(runtime_)
    {
      runtime->produce();
      threads_backend::global_produce();
    }

    void set_join(size_t join_counter_) {
      join_counter = join_counter_;
    }

    virtual void release()  {
      DEBUG_PRINT_THD(
        runtime->inside_rank,
        "%p: join counter is now %zu\n",
        this,
        join_counter - 1
      );

      if (--join_counter == 0) {
        if (for_rank == -1) {
          runtime->add_remote(this->shared_from_this());
        } else {
          threads_backend::shared_ranks[for_rank]->add_remote(this->shared_from_this());
        }
      }
    }

    // execute the graph node
    virtual void execute() {
      runtime->consume();
      threads_backend::global_consume();
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
    : GraphNode
  {
    std::shared_ptr<InnerFlow> fetch;
    bool acquire = false;

    FetchNode(runtime_t* rt,
              std::shared_ptr<InnerFlow> fetch_)
      : GraphNode(-1, rt)
      , fetch(fetch_)
    {
      DEBUG_PRINT_THD(
        runtime->inside_rank,
        "constructing fetch node, flow=%ld, this=%p\n",
        PRINT_LABEL(fetch), this
      );
    }

    void execute() override {
      std::string genName = "";

      TraceLog* log = nullptr;
      if (runtime->getTrace()) {
        genName = "fetch-" + std::to_string(fetch_label++);
        log = runtime->getTrace()->eventStartNow(genName);
      }

      TraceLog* pub_log = runtime->fetch(
        fetch->handle.get(),
        fetch->version_key,
        fetch->cid
      );

      if (runtime->getTrace()) {
        runtime->addFetchDeps(this,log,pub_log);
      }

      if (runtime->getTrace()) {
        runtime->getTrace()->eventStopNow(genName);
      }

      fetch->ready = true;

      DEBUG_PRINT_THD(
        runtime->inside_rank,
        "=== EXECUTING === fetch node\n"
      );

      auto const has_read_phase = runtime->try_release_to_read(fetch);
      if (acquire && !has_read_phase) {
        runtime->release_to_write(fetch);
      }

      GraphNode::execute();
    }

    bool ready() override {
      return runtime->test_fetch(
        fetch->handle.get(),
        fetch->version_key,
        fetch->cid
      );
    }

    virtual ~FetchNode() {
      DEBUG_PRINT_THD(
        runtime->inside_rank,
        "destructing fetch node, flow=%ld, this=%p\n",
        PRINT_LABEL(fetch), this
      );
    }

    void activate() override {
      DEBUG_PRINT_THD(
        runtime->inside_rank,
        "=== ACTIVATING === fetch node\n"
      );
      runtime->add_remote(this->shared_from_this());
    }
  };

  struct PublishNode
    : GraphNode
  {
    std::shared_ptr<DelayedPublish> pub;

    PublishNode(runtime_t* rt,
                std::shared_ptr<DelayedPublish> pub_)
      : GraphNode(-1, rt)
      , pub(pub_)
    { }

    void execute() {
      DEBUG_PRINT_THD(
        runtime->inside_rank,
        "=== EXECUTING === publish node\n"
      );

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
    : GraphNode
  {
    std::shared_ptr<CollectiveInfo> info;
    bool finished = false;

    CollectiveNode(runtime_t* rt,
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

  template <typename MetaTask>
  struct MetaTaskNode : GraphNode {
    std::shared_ptr<MetaTask> shared_task = nullptr;
    int lo = 0, hi = 0, cur = 0;
    int rank = -1;

    MetaTaskNode(
      runtime_t* rt, std::shared_ptr<MetaTask> meta_shared_task,
      int lo_in, int hi_in, int in_rank
    ) : GraphNode(-1, rt), shared_task(meta_shared_task)
      , lo(lo_in), hi(hi_in), cur(lo_in), rank(in_rank)
    { }

    virtual void execute() override {
      if (cur < hi) {
        auto task = std::move(shared_task->create_task_for_index(cur));
        cur++;
        auto task_node = std::make_shared<TaskNode<types::task_collection_task_t>>(
          runtime, std::move(task)
        );
        runtime->create_task(
          task_node, rank
        );
        if (cur < hi) {
          runtime->add_remote(this->shared_from_this());
        } else {
          GraphNode::execute();
        }
      }
    }

    virtual bool ready() override {
      return true;
    }
  };

  template <typename TaskType>
  struct TaskNode
    : GraphNode
  {
    std::unique_ptr<TaskType> task = nullptr;

    TaskNode(runtime_t* rt,
             std::unique_ptr<TaskType>&& task_)
      : GraphNode(-1, rt)
      , task(std::move(task_))
    { }

    virtual void execute() {
      // start trace event
      std::string genName = "";
      if (runtime->getTrace()) {
        TraceLog* log = nullptr;
        if (task->get_name() == darma_runtime::types::key_t()) {
          genName = "task-" + std::to_string(task_label++);
        } else {
          genName = task->get_name().human_readable_string("_","","");
        }
        log = runtime->getTrace()->eventStartNow(genName);
        runtime->addTraceDeps(this,log);
      }

      for (auto&& use : task->get_dependencies()) {
        auto const& f_in = use->get_in_flow();
        if (f_in->perform_transfer) {
          DEBUG_PRINT_THD(
            runtime->inside_rank,
            "perform transfer: f_in=%ld, owner=%d, prev_owner=%d, prev=%ld\n",
            PRINT_LABEL(f_in), f_in->indexed_rank_owner, f_in->prev_rank_owner,
            f_in->prev ? PRINT_LABEL(f_in->prev) : -1
          );

          assert(f_in->prev_rank_owner != -1);

          if (f_in->prev != nullptr) {
            runtime->assign_data_ptr(use, f_in->prev->data_block);
          }
        }
      }

      runtime->run_task(task.get());

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

  template <typename TaskType>
  struct TaskCollectionNode : GraphNode {

    std::unique_ptr<TaskType> task;
    size_t inside_rank = 0, inside_num_ranks = 0;

    TaskCollectionNode(
      runtime_t* rt, std::unique_ptr<TaskType>&& task_, size_t rank, size_t num_ranks
    )
      : GraphNode(-1, rt)
      , task(std::move(task_))
      , inside_rank(rank)
      , inside_num_ranks(num_ranks)
    { }

    virtual void execute() override {
      auto shared_task_collection = std::shared_ptr<ThreadsRuntime::task_collection_t>(std::move(task));

      int const blocks = shared_task_collection->size();
      int const ranks = inside_num_ranks;
      int const num_per_rank = std::max(1, blocks / ranks);

      DEBUG_PRINT(
        "register_task_collection: indicies=%d, ranks=%d, num_per_rank=%d\n",
        blocks, ranks, num_per_rank
      );

      for (auto cur = 0; cur < std::max(blocks,num_per_rank*ranks); cur += num_per_rank) {
        int const rank = (cur / num_per_rank) % ranks;

        auto task_node = std::make_shared<MetaTaskNode<ThreadsRuntime::task_collection_t>>(
          rank == inside_rank ? runtime : shared_ranks[rank],
          shared_task_collection,
          cur,
          std::min(cur+num_per_rank,blocks),
          rank
        );

        if (rank == inside_rank) {
          runtime->add_remote(task_node);
        } else {
          shared_ranks[rank]->add_remote(task_node);
        }
      }

      GraphNode::execute();
    }

    virtual bool ready() override {
      return join_counter == 0;
    }
  };
}

#endif /* _THREADS_NODE_BACKEND_RUNTIME_H_ */
