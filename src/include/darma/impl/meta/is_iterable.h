/*
//@HEADER
// ************************************************************************
//
//                      is_iterable.h
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

#ifndef DARMA_IMPL_META_IS_ITERABLE_H
#define DARMA_IMPL_META_IS_ITERABLE_H

#include <iterator>

#include <tinympl/delay.hpp>

#include <darma/impl/meta/detection.h>
#include <darma/impl/meta/is_iterator.h>

namespace darma_runtime {
namespace meta {

template <typename T>
class iterable_traits {
  private:

    // TODO decide if this is wise
    static_assert(
      std::is_same<T, std::remove_reference_t<T>>::value,
      "is_iterable<T> can't operate on reference types"
    );

    template <typename _U>
    using _has_begin_archetype = decltype( begin( std::declval<_U>() ) );

    using _has_begin = meta::is_detected<_has_begin_archetype, T>;

    template <typename _U>
    using _has_end_archetype = decltype( end( std::declval<_U>() ) );

    using _has_end = meta::is_detected<_has_end_archetype, T>;



  public:


    using begin_iterator = typename _has_begin::type;

    static constexpr auto has_begin = _has_begin::value
      and meta::iterator_traits<begin_iterator>::is_valid_iterator;

    using end_iterator = typename _has_end::type;

    static constexpr auto has_end = _has_end::value
      and meta::iterator_traits<end_iterator>::is_valid_iterator;

  private:

    // Used for lazy cases where meta::iterator_traits<begin_iterator> is
    // invalid
    template <typename>
    struct _nonesuch_iterator_traits {
      using type = _nonesuch_iterator_traits<T>;
      using value_type = nonesuch;
      using difference_type = nonesuch;
      using pointer = nonesuch;
      using reference = nonesuch;
      using iterator_category = nonesuch;
      static constexpr auto is_iterator = false;
      static constexpr auto is_forward_iterator = false;
      static constexpr auto is_input_iterator = false;
      static constexpr auto is_bidirectional_iterator = false;
      static constexpr auto is_random_access_iterator = false;
    };

    using _traits = std::conditional_t<
      meta::iterator_traits<begin_iterator>::is_valid_iterator,
      meta::iterator_traits<begin_iterator>,
      _nonesuch_iterator_traits<T>
    >;

  public:

    using iterator = begin_iterator;

    using value_type = typename _traits::value_type;
    using element_type = value_type;

    using difference_type = typename _traits::difference_type;

  private:

    template <typename _BeginI, typename _EndI>
    using _begin_end_inequality_comparable_archetype = decltype(
      std::declval<_BeginI>() != std::declval<_EndI>()
    );

    using _begin_end_inequality_comparable_t = is_detected_convertible<bool,
      _begin_end_inequality_comparable_archetype, begin_iterator, end_iterator
    >;

    template <typename U>
    using _has_iterator_ctor_archetype = decltype(
      U(std::declval<iterator>(), std::declval<iterator>())
    );

    using _has_iterator_ctor = is_detected<
      _has_iterator_ctor_archetype, T
    >;

  public:

    static constexpr auto begin_and_end_are_inequality_comparable =
      _begin_end_inequality_comparable_t::value;

    static constexpr auto is_iterable = has_begin and has_end
      and begin_and_end_are_inequality_comparable;

    static constexpr auto is_random_access_iterable = is_iterable
      and _traits::is_random_access_iterator;

    static constexpr auto has_iterator_constructor =
      _has_iterator_ctor::value;




};

} // end namespace meta
} // end namespace darma_runtime

#endif //DARMA_IMPL_META_IS_ITERABLE_H
