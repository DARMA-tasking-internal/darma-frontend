/*
//@HEADER
// ************************************************************************
//
//                      index_range_traits.h
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

#ifndef DARMA_IMPL_INDEX_RANGE_INDEX_RANGE_TRAITS_H
#define DARMA_IMPL_INDEX_RANGE_INDEX_RANGE_TRAITS_H

#include <cstdlib>
#include <type_traits>

#include <darma/impl/meta/detection.h>
#include <tinympl/type_traits.hpp>

namespace darma_runtime {
namespace indexing {

template <typename T>
class index_range_traits {

  //============================================================================
  // <editor-fold desc="mapping_to_dense"> {{{1

  private:

    template <typename IndexRangeT>
    using _has_mapping_to_dense_archetype = decltype(
      get_mapping_to_dense(std::declval<tinympl::as_const_lvalue_reference_t<IndexRangeT>>())
    );

  public:

    using has_mapping_to_dense = meta::is_detected<
      _has_mapping_to_dense_archetype, T
    >;

    using mapping_to_dense_type = meta::detected_t<
      _has_mapping_to_dense_archetype, T
    >;

    static
    mapping_to_dense_type
    mapping_to_dense(T const& range) {
      return get_mapping_to_dense(range);
    }

  // </editor-fold> end mapping_to_dense }}}1
  //============================================================================

  //============================================================================
  // <editor-fold desc="size"> {{{1

  private:

    template <typename U>
    using _has_size_archetype = decltype(
      std::declval<tinympl::as_const_lvalue_reference_t<U>>().size()
    );

    using _has_size = meta::is_detected_convertible<
      std::size_t,
      _has_size_archetype, T
    >;

  public:

    template <
      typename _for_SFINAE_only,
      typename=std::enable_if_t<_has_size::value>
    >
    static std::size_t
    size(T const& range) {
        return range.size();
    }

  // </editor-fold> end size }}}1
  //============================================================================

  //==============================================================================
  // <editor-fold desc="index_type"> {{{1

  private:

    // Detection idiom used for SFINAE safety only; this is a requirement

    template <typename U>
    using _has_index_type_archetype = typename U::index_type;

  public:

    using index_type = meta::detected_t<
      _has_index_type_archetype, T
    >;

  // </editor-fold> end index_type }}}1
  //==============================================================================

};

} // end namespace indexing
} // end namespace darma_runtime

#endif //DARMA_IMPL_INDEX_RANGE_INDEX_RANGE_TRAITS_H
