/*
//@HEADER
// ************************************************************************
//
//                      threads_interface.h
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

#if !defined(_THREADS_INTERFACE_BACKEND_RUNTIME_H_)
#define _THREADS_INTERFACE_BACKEND_RUNTIME_H_

#include <trace.h>

namespace threads_backend {
  struct InnerFlow;
  struct GraphNode;
  template <typename TaskType>
  struct TaskNode;
  struct FetchNode;
  struct PublishNode;
  struct DelayedPublish;
  struct CollectiveInfo;

  template <typename Impl>
  class ThreadsInterface {
  public:
    inline void
    produce() {
      return static_cast<Impl*>(this)->produce();
    }
    inline void
    consume() {
      return static_cast<Impl*>(this)->consume();
    }

    inline void
    addFetchDeps(FetchNode* node, TraceLog* log, TraceLog* pub_log) {
      return static_cast<Impl*>(this)->addFetchDeps(node,log,pub_log);
    }
    inline void
    addPublishDeps(PublishNode* node, TraceLog* log) {
      return static_cast<Impl*>(this)->addPublishDeps(node,log);
    }
    template <typename TaskType>
    inline void
    addTraceDeps(TaskNode<TaskType>* node, TraceLog* log) {
      return static_cast<Impl*>(this)->addTraceDeps(node,log);
    }
    inline TraceModule*
    getTrace() {
      return static_cast<Impl*>(this)->getTrace();
    }

    inline bool
    try_release_to_read(std::shared_ptr<InnerFlow> flow) {
      return static_cast<Impl*>(this)->try_release_to_read(flow);
    }
    inline void
    release_to_write(
      std::shared_ptr<InnerFlow> flow
    ) {
      return static_cast<Impl*>(this)->release_to_write(flow);
    }

    inline void
    add_remote(std::shared_ptr<GraphNode> task) {
      return static_cast<Impl*>(this)->add_remote(task);
    }
    inline void
    add_local(std::shared_ptr<GraphNode> task) {
      return static_cast<Impl*>(this)->add_local(task);
    }

    inline size_t
    get_rank() {
      return static_cast<Impl*>(this)->inside_rank;
    }

    // fetch interface methods
    inline TraceLog*
    fetch(
      Runtime::handle_t* handle,
      types::key_t const& version_key,
      std::shared_ptr<DataStoreHandle> const& store
    ) {
      return static_cast<Impl*>(this)->fetch(
        handle,
        version_key,
        store
      );
    }
    inline bool
    test_fetch(
      Runtime::handle_t* handle,
      types::key_t const& version_key,
      std::shared_ptr<DataStoreHandle> const& store
    ) {
      return static_cast<Impl*>(this)->test_fetch(
        handle,
        version_key,
        store
      );
    }

    // collective interface
    inline bool
    collective(std::shared_ptr<CollectiveInfo> info) {
      return static_cast<Impl*>(this)->collective(info);
    }
    inline void
    collective_finish(std::shared_ptr<CollectiveInfo> info) {
      return static_cast<Impl*>(this)->collective_finish(info);
    }
    inline void
    blocking_collective(std::shared_ptr<CollectiveInfo> info) {
      return static_cast<Impl*>(this)->blocking_collective(info);
    }

    // publish interface methods
    inline bool
    test_publish(std::shared_ptr<DelayedPublish> publish) {
      return static_cast<Impl*>(this)->test_publish(publish);
    }
    inline void
    publish(std::shared_ptr<DelayedPublish> publish,
            TraceLog* const log) {
      return static_cast<Impl*>(this)->publish(publish,log);
    }
    inline void
    publish_finished(std::shared_ptr<DelayedPublish> publish) {
      return static_cast<Impl*>(this)->publish_finished(publish);
    }

    // task interface methods
    template <typename TaskType>
    inline void
    run_task(TaskType* task) {
      return static_cast<Impl*>(this)->run_task(task);
    }
  };
}


#endif /*_THREADS_INTERFACE_BACKEND_RUNTIME_H_*/
