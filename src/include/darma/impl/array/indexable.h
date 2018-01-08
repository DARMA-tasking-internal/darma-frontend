/*
//@HEADER
// ************************************************************************
//
//                      indexable.h
//                         DARMA
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
// Questions? Contact David S. Hollman (dshollm@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMA_IMPL_ARRAY_INDEXABLE_H
#define DARMA_IMPL_ARRAY_INDEXABLE_H

#include "array_fwd.h"
#include "index_decomposition.h"


namespace darma_runtime {

namespace detail {

template <typename T>
struct IndexingTraits {

  public:

    using decomposition = IndexDecomposition<T>;
    using const_decomposition = IndexDecomposition<std::add_const_t<T>>;
    using element_type = typename decomposition::element_type;

    using subset_object_type = typename decomposition::subset_object_type;

  private:

    using _get_element_range_return_type = decltype(
      std::declval<decomposition>().get_element_range(
        std::declval<T&>(), size_t(), size_t()
      )
    );

  public:

    template <typename SubobjectType = T>
    static inline void
    make_subobject(
      void* allocd, T const& parent,
      size_t offset, size_t n_elem
    ) {
      const_decomposition().get_element_range(allocd, parent, offset, n_elem);
    }

    static inline size_t
    n_elements(T const& obj) {
      return const_decomposition().n_elements(obj);
    }

    static inline element_type&
    get_element(
      T& object, size_t i
    ) {
      return decomposition().get_element(object, i);
    }

    static inline subset_object_type
    get_element_range(
      T& object,
      size_t offset, size_t n_elem
    ) {
      return decomposition().get_element_range(object, offset, n_elem);
    }

    static inline void
    set_element_range(
      T& object, subset_object_type const& subset,
      size_t offset, size_t n_elem
    ) {
      decomposition().set_element_range(object, subset, offset, n_elem);
    }

    static inline bool
    is_deep_copy(T const& object) {
      return const_decomposition().subset_is_deep_copy(object);
    }

    // TODO these shouldn't reconstruct the sub-object except as a last resort

    template <typename ArchiveT>
    static inline void
    get_packed_size(
      T const& obj, ArchiveT& ar,
      size_t offset, size_t n_elem
    ) {
      // Simplest default: make an object with an element range and pack it
      ar % const_decomposition().get_element_range(obj, offset, n_elem);
    }

    template <typename ArchiveT>
    static inline void
    pack_elements(
      T const& obj, ArchiveT& ar,
      size_t offset, size_t n_elem
    ) {
      // Simplest default: make an object with an element range and pack it
      ar << const_decomposition().get_element_range(obj, offset, n_elem);
    }

    template <typename ArchiveT>
    static inline void
    unpack_elements(
      T& obj, ArchiveT& ar,
      size_t offset, size_t n_elem
    ) {
      // Simplest default: reconstruct the object, then
      T subobj;
      ar >> subobj;
      decomposition().set_element_range(obj, subobj, offset, n_elem);
    }

};

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_ARRAY_INDEXABLE_H
