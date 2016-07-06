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

namespace threads_backend {
  struct InnerFlow;
  struct GraphNode;
  struct DelayedPublish;

  template <typename Impl>
  class ThreadsInterface {
  public:
    inline void
    add_remote(std::shared_ptr<GraphNode> task) {
      return static_cast<Impl*>(this)->add_remote(task);
    }
    inline void
    add_local(std::shared_ptr<GraphNode> task) {
      return static_cast<Impl*>(this)->add_local(task);
    }

    // fetch interface methods
    inline void
    fetch(darma_runtime::abstract::frontend::Handle* handle,
          types::key_t const& version_key) {
      return static_cast<Impl*>(this)->fetch(handle, version_key);
    }
    inline bool
    test_fetch(darma_runtime::abstract::frontend::Handle* handle,
               types::key_t const& version_key) {
      return static_cast<Impl*>(this)->test_fetch(handle, version_key);
    }

    // publish interface methods
    inline bool
    test_publish(std::shared_ptr<DelayedPublish> publish) {
      return static_cast<Impl*>(this)->test_publish(publish);
    }
    inline void
    publish(std::shared_ptr<DelayedPublish> publish) {
      return static_cast<Impl*>(this)->publish(publish);
    }
    inline void
    publish_finished(std::shared_ptr<DelayedPublish> publish) {
      return static_cast<Impl*>(this)->publish_finished(publish);
    }
    
    // task interface methods
    inline void
    run_task(types::unique_ptr_template<runtime_t::task_t>&& task) {
      return static_cast<Impl*>(this)->run_task(std::forward<types::unique_ptr_template<runtime_t::task_t>>(task));
    }
  };
}


#endif /*_THREADS_INTERFACE_BACKEND_RUNTIME_H_*/
