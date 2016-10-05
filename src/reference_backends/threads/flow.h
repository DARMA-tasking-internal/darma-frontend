/*
//@HEADER
// ************************************************************************
//
//                           flow.h
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

#if !defined(_THREADS_FLOW_BACKEND_RUNTIME_H_)
#define _THREADS_FLOW_BACKEND_RUNTIME_H_

#include <darma/interface/backend/flow.h>

#include <threads.h>
#include <node.h>

namespace threads_backend {
  extern __thread size_t flow_label;

  enum FlowState {
    FlowWaiting,
    FlowReadReady,
    FlowReadOnlyReady,
    FlowWriteReady,
    FlowAntiReady
  };

  struct InnerFlow {
    using handle_t = Runtime::handle_t;

    std::shared_ptr<InnerFlow> forward, next;
    std::shared_ptr<DataBlock> data_block = nullptr;
    types::key_t version_key, key;
    std::shared_ptr<handle_t> handle = nullptr;
    bool ready, isNull, isFetch, fromFetch, isCollective, isForward;
    bool  isWriteForward, fetcherAdded;
    std::shared_ptr<DataStoreHandle> data_store = nullptr;

    size_t* shared_reader_count = nullptr;
    size_t uses = 0;

    FlowState state{};

    #if __THREADS_DEBUG_MODE__
      size_t label;
    #endif

    size_t readers_jc, ref;
    std::vector<std::shared_ptr<GraphNode>> readers;

    // node in the graph to activate
    std::shared_ptr<GraphNode> node;

    // hack to put dfs info in here
    std::shared_ptr<CollectiveNode> dfsColNode;

    std::shared_ptr<InnerFlow> alias = nullptr;

    InnerFlow(const InnerFlow& in) = default;

    InnerFlow(
      std::shared_ptr<handle_t> handle_
    )
      : forward(nullptr)
      , next(nullptr)
      , shared_reader_count(nullptr)
      , version_key(darma_runtime::detail::SimpleKey())
      , isNull(false)
      , isFetch(false)
      , fetcherAdded(false)
      , fromFetch(false)
      , isCollective(false)
      , isForward(false)
      , isWriteForward(false)
      , ready(false)
      #if __THREADS_DEBUG_MODE__
      , label(++flow_label)
      #endif
      , readers_jc(0)
      , ref(0)
      , node(nullptr)
      , handle(handle_)
      , state(FlowWaiting)
    { }
  };
}

#endif /* _THREADS_FLOW_BACKEND_RUNTIME_H_ */
