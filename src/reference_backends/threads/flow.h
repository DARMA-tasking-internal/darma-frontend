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
  
  struct InnerFlow {
    std::shared_ptr<InnerFlow> forward;
    types::key_t version_key;
    darma_runtime::abstract::frontend::Handle* handle;
    bool ready, hasDeferred;
    DataBlock* deferred_data_ptr;
    size_t label;

    std::list<std::shared_ptr<InnerFlow> > deps;
    
    // node in the graph to activate
    std::shared_ptr<GraphNode> node;
    
    InnerFlow(const InnerFlow& in) = default;
    
    InnerFlow()
      : forward(nullptr)
      , version_key(darma_runtime::detail::SimpleKey())
      , ready(false)
      , hasDeferred(false)
      , deferred_data_ptr(nullptr)
      , label(++flow_label)
      , node(nullptr)
    { }

    bool check_ready() { return ready; }
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
}

#endif /* _THREADS_FLOW_BACKEND_RUNTIME_H_ */
