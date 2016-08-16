/*
//@HEADER
// ************************************************************************
//
//                      is_iterator.h
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

#ifndef DARMA_IMPL_META_IS_ITERATOR_H
#define DARMA_IMPL_META_IS_ITERATOR_H

#include <iterator>
#include <cassert>

#include <tinympl/logical_and.hpp>

#include <darma/impl/meta/detection.h>

#include "is_equality_comparable.h"

namespace darma_runtime {
namespace meta {


namespace _impl {

template <typename T>
using _iterator_traits_has_value_type_archetype =
  typename std::iterator_traits<T>::value_type;

template <typename T>
using _iterator_traits_has_difference_type_archetype =
  typename std::iterator_traits<T>::difference_type;

template <typename T>
using _iterator_traits_has_pointer_archetype =
  typename std::iterator_traits<T>::pointer;

template <typename T>
using _iterator_traits_has_reference_archetype =
  typename std::iterator_traits<T>::reference;

template <typename T>
using _iterator_traits_has_iterator_category_archetype =
  typename std::iterator_traits<T>::iterator_category;

template <typename T>
using _iterator_traits_has_value_type =
  is_detected<_iterator_traits_has_value_type_archetype, T>;

template <typename T>
using _iterator_traits_has_difference_type =
  is_detected<_iterator_traits_has_difference_type_archetype, T>;

template <typename T>
using _iterator_traits_has_pointer =
  is_detected<_iterator_traits_has_pointer_archetype, T>;

template <typename T>
using _iterator_traits_has_reference =
  is_detected<_iterator_traits_has_reference_archetype, T>;

template <typename T>
using _iterator_traits_has_iterator_category =
  is_detected<_iterator_traits_has_iterator_category_archetype, T>;

template <typename T>
using has_valid_std_iterator_traits = typename tinympl::and_<
  _iterator_traits_has_value_type<T>,
  _iterator_traits_has_difference_type<T>,
  _iterator_traits_has_pointer<T>,
  _iterator_traits_has_reference<T>,
  _iterator_traits_has_iterator_category<T>
>::type;

template <typename T, typename Enable=void>
struct iterator_traits_enabled_if {
  static constexpr auto is_valid_iterator = false;
};

template <typename T>
struct iterator_traits_enabled_if<T,
  std::enable_if_t<has_valid_std_iterator_traits<T>::value>
>
{
  private:


    using _traits = std::iterator_traits<T>;

    using iterator_reference = std::add_lvalue_reference_t<T>;
    using const_iterator_reference = std::add_lvalue_reference_t<
      std::add_const_t<T>
    >;

  public:


    using difference_type = typename _traits::difference_type;
    using value_type = typename _traits::value_type;
    using pointer = typename _traits::pointer;
    using reference = typename _traits::reference;
    using iterator_category = typename _traits::iterator_category;

  private:

    // Iterator requirements

    template <typename U>
    using _has_dereference_op_archetype = decltype( *(std::declval<U>()) );

    using _has_dereference_op = meta::is_detected<
      _has_dereference_op_archetype, T
    >;

    template <typename U>
    using _has_increment_op_archetype = decltype( ++(std::declval<U>()) );

    using _has_increment_op = meta::is_detected_convertible<
      iterator_reference, _has_increment_op_archetype, T
    >;

  public:

    static constexpr auto is_valid_iterator = _has_dereference_op::value
      and _has_increment_op::value;

    static constexpr auto is_iterator = is_valid_iterator;


  private:

    // Input Iterator requirements

    template <typename U>
    using _has_not_equal_archetype = decltype(
      std::declval<U>() != std::declval<U>()
    );

    using _has_not_equal = meta::is_detected_convertible<
      bool, _has_not_equal_archetype, T
    >;

    using _dereference_op_returns_reference = meta::is_detected_exact<
      reference, _has_dereference_op_archetype, T
    >;

    template <typename U>
    using _has_post_increment_op_archetype = decltype( (std::declval<U>())++ );

    using _has_post_increment_op = meta::is_detected<
      _has_post_increment_op_archetype, T
    >;

    using _post_increment_op_is_dereferencable = meta::is_detected_convertible<
      value_type, _has_dereference_op_archetype,
      typename _has_post_increment_op::type
    >;

  public:

    static constexpr auto is_input_iterator =
      is_iterator
      and is_equality_comparable<T>::value
      and _has_not_equal::value
      and _dereference_op_returns_reference::value
      and _has_post_increment_op::value
      and _post_increment_op_is_dereferencable::value
      and std::is_base_of<
        std::input_iterator_tag,
        typename _traits::iterator_category
      >::value;

  private:

    // Forward Iterator additional requirements

    using _post_increment_op_dereference_returns_reference = is_detected_exact<
        reference, _has_dereference_op_archetype,
        typename _has_post_increment_op::type
      >;

    using _post_increment_op_returns_iterator = is_detected_exact<
      T, _has_post_increment_op_archetype, T
    >;

    // TODO reference is exactly value_type& or value_type const& requirement

    // Untestable: multipass guarantee

  public:

    static constexpr auto is_forward_iterator =
      is_iterator
      and std::is_base_of<
        std::forward_iterator_tag,
        typename _traits::iterator_category
      >::value
      and std::is_default_constructible<T>::value
      and _post_increment_op_returns_iterator::value
    ;

    // Bidirectional Iterator

    template <typename U>
    using _has_decrement_op_archetype = decltype( --(std::declval<U>()) );

    using _has_decrement_op = is_detected_convertible<
      iterator_reference, _has_decrement_op_archetype, T
    >;

    template <typename U>
    using _has_post_decrement_op_archetype = decltype( (std::declval<U>())-- );


    using _has_post_decrement_op = is_detected_convertible<
      const_iterator_reference, _has_decrement_op_archetype, T
    >;

    // Not working for now...
    // TODO fix this

    //using _post_decrement_op_is_dereferencable = is_detected_convertible<
    //  reference, _has_dereference_op_archetype,
    //  typename _has_post_decrement_op::type
    //>;

  public:

    static constexpr auto is_bidirectional_iterator =
      is_forward_iterator
      and std::is_base_of<
        std::bidirectional_iterator_tag,
        typename _traits::iterator_category
      >::value
      and _has_decrement_op::value
      and _has_post_decrement_op::value
      //and _post_decrement_op_is_dereferencable::value
    ;


  private:

    // Random access iterator

    template <typename U>
    using _has_plus_equal_difference_type_archetype = decltype(
      std::declval<U>() += std::declval<difference_type>()
    );

    using _has_plus_equal_difference_type = is_detected_exact<
      iterator_reference, _has_plus_equal_difference_type_archetype, T
    >;

    template <typename U>
    using _has_plus_difference_type_archetype = decltype(
      std::declval<U>() + std::declval<difference_type>()
    );

    template <typename U>
    using _has_difference_type_plus_archetype = decltype(
      std::declval<difference_type>() + std::declval<U>()
    );

    using _has_plus_difference_type = is_detected_exact<T,
      _has_plus_difference_type_archetype, T
    >;

    using _has_difference_type_plus = is_detected_exact<T,
      _has_difference_type_plus_archetype, T
    >;

    template <typename U>
    using _has_minus_equal_difference_type_archetype = decltype(
      std::declval<U>() -= std::declval<difference_type>()
    );

    using _has_minus_equal_difference_type = is_detected_exact<
      iterator_reference, _has_minus_equal_difference_type_archetype, T
    >;

    template <typename U>
    using _has_minus_difference_type_archetype = decltype(
      std::declval<U>() - std::declval<difference_type>()
    );

    using _has_minus_difference_type = is_detected_exact<T,
      _has_minus_difference_type_archetype, T
    >;

    template <typename U>
    using _has_minus_archetype = decltype(
      std::declval<U>() - std::declval<U>()
    );

    using _has_minus = is_detected_convertible<difference_type,
      _has_minus_archetype, T
    >;

    template <typename U>
    using _has_subscript_archetype = decltype(
      std::declval<U>()[std::declval<difference_type>()]
    );

    using _has_subscript = is_detected_convertible<reference,
      _has_subscript_archetype, T
    >;

    template <typename U>
    using _has_less_archetype = decltype(
      std::declval<U>() < std::declval<U>()
    );

    using _has_less = is_detected_convertible<bool,
      _has_less_archetype, T
    >;

    template <typename U>
    using _has_less_equal_archetype = decltype(
      std::declval<U>() <= std::declval<U>()
    );

    using _has_less_equal = is_detected_convertible<bool,
      _has_less_equal_archetype, T
    >;

    template <typename U>
    using _has_greater_archetype = decltype(
      std::declval<U>() > std::declval<U>()
    );

    using _has_greater = is_detected_convertible<bool,
      _has_greater_archetype, T
    >;

    template <typename U>
    using _has_greater_equal_archetype = decltype(
    std::declval<U>() >= std::declval<U>()
    );

    using _has_greater_equal = is_detected_convertible<bool,
      _has_greater_equal_archetype, T
    >;

  public:

    static constexpr bool is_random_access_iterator =
      is_bidirectional_iterator
      and std::is_base_of<
        std::random_access_iterator_tag,
        typename _traits::iterator_category
      >::value
      and _has_plus_equal_difference_type::value
      and _has_plus_difference_type::value
      and _has_difference_type_plus::value
      and _has_minus_equal_difference_type::value
      and _has_minus_difference_type::value
      and _has_minus::value
      and _has_subscript::value
      and _has_less::value
      and _has_less_equal::value
      and _has_greater::value
      and _has_greater_equal::value
    ;

};

} // end namespace _impl

/** @brief SFINAE-friendly analog of std::iterator_traits, with some extra bells
 *  and whistles.
 *
 *  Note that some of this functionality duplicates additions to the C++17
 *  standard, but we can't rely on this being available to our users.
 */
template <typename T>
class iterator_traits
  : public _impl::iterator_traits_enabled_if<T>
{ };

} // end namespace meta
} // end namespace darma_runtime

#endif //DARMA_IMPL_META_IS_ITERATOR_H
