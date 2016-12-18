/*
//@HEADER
// ************************************************************************
//
//                        union_find.h
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

#if !defined(_UNION_FIND_BACKEND_RUNTIME_H_)
#define _UNION_FIND_BACKEND_RUNTIME_H_

namespace threads_backend { namespace union_find {
    template <typename UFArchetype>
    void make_set(
      UFArchetype const& node
    ) {
      node->alias = nullptr;
      node->uf_size = 1;
    }

    template <typename UFArchetype>
    void union_nodes(
      UFArchetype node1,
      UFArchetype node2
    ) {
      DEBUG_PRINT_THD(
        (size_t)0,
        "union_nodes: sz = %ld, sz2 = %ld\n",
        node1->uf_size,
        node2->uf_size
      );
      // optimization to pick the larger subtree
      // does not apply to this problem due to ordering problem
      // if (node1->uf_size < node2->uf_size) {
      //   node1.swap(node2);
      // }
      node2->alias = node1;
      node1->uf_size += node2->uf_size;
    }

    template <typename UFArchetype, typename Callable>
    UFArchetype find_call(
      UFArchetype const& node,
      Callable&& callable
    ) {
      DEBUG_PRINT_THD(
        (size_t)0,
        "find_call: node=%ld, alias=%ld\n",
        PRINT_LABEL(node),
        node->alias ? PRINT_LABEL(node->alias) : 0
      );
      if (node->alias != nullptr) {
        callable(node->alias);
        node->alias = find_call(
          node->alias,
          callable
        );
      }
      return node->alias ? node->alias : node;
    }
  }
}

#endif /*_UNION_FIND_BACKEND_RUNTIME_H_*/
