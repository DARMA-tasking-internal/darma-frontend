/*
//@HEADER
// ************************************************************************
//
//                      access_handle_collection_traits.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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
// Questions? Contact David S. Hollman (dshollm@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMA_IMPL_TASK_COLLECTION_ACCESS_HANDLE_COLLECTION_TRAITS_H
#define DARMA_IMPL_TASK_COLLECTION_ACCESS_HANDLE_COLLECTION_TRAITS_H


#include <darma/impl/access_handle/access_handle_traits.h>

#include <darma/impl/util/optional_boolean.h>

namespace darma_runtime {

namespace detail {

// TODO special members (e.g., for allocator)

namespace ahc_traits {

template <
  OptionalBoolean IsOuter = OptionalBoolean::Unknown,
  typename AHSemanticTraits = access_handle_semantic_traits<>
>
struct semantic_traits {
  static constexpr auto is_outer = IsOuter;
  using handle_semantic_traits = AHSemanticTraits;
};

} // end namespace ahc_traits

template <typename T, typename IndexRangeT,
  typename PermissionsTraits = access_handle_permissions_traits<>,
  typename SemanticTraits = ahc_traits::semantic_traits<>,
  typename AllocationTraits = access_handle_allocation_traits<T>
>
struct access_handle_collection_traits {
  using permissions_traits = PermissionsTraits;
  using semantic_traits = SemanticTraits;
  using allocation_traits = AllocationTraits;
};

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_TASK_COLLECTION_ACCESS_HANDLE_COLLECTION_TRAITS_H
