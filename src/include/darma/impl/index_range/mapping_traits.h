/*
//@HEADER
// ************************************************************************
//
//                      mapping_traits.h
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

#ifndef DARMA_IMPL_INDEX_RANGE_MAPPING_TRAITS_H
#define DARMA_IMPL_INDEX_RANGE_MAPPING_TRAITS_H

#include <type_traits>

#include <darma/impl/meta/detection.h>
#include <src/include/tinympl/type_traits.hpp>
#include <darma/impl/util/optional_boolean.h>

namespace darma_runtime {

namespace indexing {

template <typename T>
struct mapping_traits {

  //============================================================================
  // <editor-fold desc="Member type determination"> {{{1

  private:

    template <typename T>
    friend
    struct darma_runtime::indexing::mapping_traits;

    template <typename U>
    using _index_type_archetype = typename U::index_type;

    template <typename U>
    using _multi_index_type_archetype = typename U::multi_index_type;

    template <typename U>
    using _from_index_type_archetype = typename U::from_index_type;

    template <typename U>
    using _from_multi_index_type_archetype = typename U::from_multi_index_type;

    template <typename U>
    using _to_index_type_archetype = typename U::to_index_type;

    template <typename U>
    using _to_multi_index_type_archetype = typename U::to_multi_index_type;

    static constexpr auto _has_from_index_type = meta::is_detected<
      _from_index_type_archetype, T
    >::value;

    static constexpr auto _has_to_index_type = meta::is_detected<
      _to_index_type_archetype, T
    >::value;

    static constexpr auto _has_multi_index_type = meta::is_detected<
      _multi_index_type_archetype, T
    >::value;

    static constexpr auto _has_from_multi_index_type = meta::is_detected<
      _from_multi_index_type_archetype, T
    >::value;

    static constexpr auto _has_to_multi_index_type = meta::is_detected<
      _to_multi_index_type_archetype, T
    >::value;

    // Note: must have index_type defined if either from_index_type or to_index_type are missing

  public:

    using from_index_type = std::conditional_t<
      _has_from_index_type,
      meta::detected_t<_from_index_type_archetype, T>,
      meta::detected_t<_index_type_archetype, T>
    >;

    using to_index_type = std::conditional_t<
      _has_from_index_type,
      meta::detected_t<_to_index_type_archetype, T>,
      meta::detected_t<_index_type_archetype, T>
    >;

    using from_multi_index_type = std::conditional_t<
      _has_from_multi_index_type,
      meta::detected_t<_from_multi_index_type_archetype, T>,
      std::conditional_t<
        _has_multi_index_type,
        meta::detected_t<_multi_index_type_archetype, T>,
        from_index_type
      >
    >;

    using to_multi_index_type = std::conditional_t<
      _has_to_multi_index_type,
      meta::detected_t<_to_multi_index_type_archetype, T>,
      std::conditional_t<
        _has_multi_index_type,
        meta::detected_t<_multi_index_type_archetype, T>,
        to_index_type
      >
    >;

  // </editor-fold> end Member type determination }}}1
  //============================================================================

  //============================================================================
  // <editor-fold desc="known_same_mapping() and same_mapping_is_known"> {{{1

  private:

    template <typename U, typename OtherMapping>
    using _is_same_archetype = decltype(
      std::declval<U>().is_same(
        std::declval<tinympl::as_const_lvalue_reference_t<OtherMapping>>()
      )
    );

    template <typename OtherMapping>
    using _mapping_is_same_return_t =
      meta::detected_t<_is_same_archetype, T, OtherMapping>;

  public:


    template <typename OtherMapping>
    using same_mapping_is_known = tinympl::bool_<
      std::is_convertible<_mapping_is_same_return_t<OtherMapping>, bool>::value
        or std::is_convertible<
          _mapping_is_same_return_t<OtherMapping>,
          optional_boolean_t
        >::value
        or std::is_convertible<
          typename darma_runtime::indexing::mapping_traits<OtherMapping>
          ::template _mapping_is_same_return_t<T>,
          optional_boolean_t
        >::value
        or std::is_convertible<
          typename darma_runtime::indexing::mapping_traits<OtherMapping>
          ::template _mapping_is_same_return_t<T>,
          bool
        >::value
    >;

    template <typename OtherMapping>
    static bool
    known_same_mapping(
      std::enable_if_t<
        std::is_convertible<
          _mapping_is_same_return_t<OtherMapping>, optional_boolean_t
        >::value,
        T const&
      > mapping,
      OtherMapping const& other_mapping
    ) {
      return mapping.is_same(other_mapping) == OptionalBoolean::KnownTrue;
    }

    template <typename OtherMapping>
    static bool
    known_same_mapping(
      std::enable_if_t<
        std::is_convertible<
          _mapping_is_same_return_t<OtherMapping>,
          bool
        >::value
          and not std::is_convertible<
            _mapping_is_same_return_t<OtherMapping>, optional_boolean_t
          >::value,
        T const&
      > mapping,
      OtherMapping const& other_mapping
    ) {
      return mapping.is_same(other_mapping);
    }

    template <typename OtherMapping>
    static bool
    known_same_mapping(
      std::enable_if_t<
        not std::is_convertible<
          _mapping_is_same_return_t<OtherMapping>,
          bool
        >::value
          and not std::is_convertible<
            _mapping_is_same_return_t<OtherMapping>,
            optional_boolean_t
          >::value
          and std::is_convertible<
            typename darma_runtime::indexing::mapping_traits<OtherMapping>
            ::template _mapping_is_same_return_t<T>,
            optional_boolean_t
          >::value,
        T const&
      > mapping,
      OtherMapping const& other_mapping
    ) {
      return other_mapping.is_same(mapping) == OptionalBoolean::KnownTrue;
    }

    template <typename OtherMapping>
    static bool
    known_same_mapping(
      std::enable_if_t<
        not std::is_convertible<
          _mapping_is_same_return_t<OtherMapping>,
          bool
        >::value
          and not std::is_convertible<
            _mapping_is_same_return_t<OtherMapping>,
            optional_boolean_t
          >::value
          and not std::is_convertible<
            typename darma_runtime::indexing::mapping_traits<OtherMapping>
            ::template _mapping_is_same_return_t<T>,
            optional_boolean_t
          >::value
          and std::is_convertible<
            typename darma_runtime::indexing::mapping_traits<OtherMapping>
            ::template _mapping_is_same_return_t<T>,
            bool
          >::value,
        T const&
      > mapping,
      OtherMapping const& other_mapping
    ) {
      return other_mapping.is_same(mapping);
    }


    template <typename OtherMapping>
    static bool
    known_same_mapping(
      std::enable_if_t<
        not same_mapping_is_known<OtherMapping>::value,
        T const&
      > mapping,
      OtherMapping const& other_mapping
    ) {
      return false; // we don't know that it's the same
    }

  // </editor-fold> end known_same_mapping() and same_mapping_is_known }}}1
  //==========================================================================

  //==============================================================================
  // <editor-fold desc="Generic map_forward and map_backward"> {{{1

  private:

    template <typename U>
    using _map_forward_no_ranges_archetype = decltype(
      std::declval<U>().map_forward(std::declval<to_index_type const&>())
    );

    template <typename U, typename Range1>
    using _map_forward_one_range_archetype = decltype(
      std::declval<U>().map_forward(
        std::declval<to_index_type const&>(),
        std::declval<Range1 const&>()
      )
    );

    template <typename U, typename Range1, typename Range2>
    using _map_forward_two_ranges_archetype = decltype(
      std::declval<U>().map_forward(
        std::declval<to_index_type const&>(),
        std::declval<Range1 const&>(),
        std::declval<Range2 const&>()
      )
    );

    template <typename U>
    using _map_backward_no_ranges_archetype = decltype(
      std::declval<U>().map_backward(std::declval<from_index_type const&>())
    );

    template <typename U, typename Range1>
    using _map_backward_one_range_archetype = decltype(
      std::declval<U>().map_backward(
        std::declval<from_index_type const&>(),
        std::declval<Range1 const&>()
      )
    );

    template <typename U, typename Range1, typename Range2>
    using _map_backward_two_ranges_archetype = decltype(
      std::declval<U>().map_backward(
        std::declval<from_index_type const&>(),
        std::declval<Range1 const&>(),
        std::declval<Range2 const&>()
      )
    );

  public:



  // </editor-fold> end Generic map_forward and map_backward }}}1
  //==============================================================================

};


} // end namespace indexing


} // end namespace darma_runtime

#endif //DARMA_IMPL_INDEX_RANGE_MAPPING_TRAITS_H
