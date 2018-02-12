/*
//@HEADER
// ************************************************************************
//
//                      is_container.h
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMA_IMPL_META_IS_CONTAINER_H
#define DARMA_IMPL_META_IS_CONTAINER_H

#include <type_traits>
#include <cstddef>
#include <iterator>

#include "detection.h"
#include "detection_archetypes.h"
#include "any_convertible.h"
#include "is_equality_comparable.h"

namespace darma_runtime {
namespace meta {

// Some incomplete concept checking stubs to be filled in later
template <typename T>
using is_erasable = std::true_type; // TODO implement this
template <typename T>
using is_forward_iterator = std::true_type; // TODO implement this
template <typename T>
using is_copy_insertable = std::true_type; // TODO implement this
template <typename T>
using is_allocator = std::true_type; // TODO implement this

template <typename Iterator>
struct is_iterator {
  private:
    template <typename T>
    using _dereferencable_archetype = decltype( *std::declval<T&>() );
    template <typename T>
    using _preincrementable_archetype = decltype( ++std::declval<T&>() );

  public:

    using type = typename std::integral_constant<bool,
      is_detected<_dereferencable_archetype, Iterator>::value
      and is_detected_exact<Iterator&, _preincrementable_archetype, Iterator>::value
      and std::is_copy_constructible<Iterator>::value
      and std::is_copy_assignable<Iterator>::value
      and std::is_destructible<Iterator>::value
      // and std::is_swappable<Iterator&>::value // needs c++17
    >;
    static constexpr auto value = type::value;
};

template <typename Iterator>
struct is_output_iterator {
  private:

    template <typename T>
    using _assignable_archetype = decltype(
      std::declval<decltype(*std::declval<T&>())>() = std::declval<any_arg>()
    );

    template <typename T>
    using _preincrementable_archetype = decltype( ++std::declval<T&>() );

    template <typename T>
    using _postincrementable_archetype = decltype( std::declval<T&>()++ );

    template <typename T>
    using _postincrement_assign_archetype = decltype(
      *(std::declval<T&>()++) = std::declval<any_arg>()
    );

  public:
    using type = typename std::integral_constant<bool, true
      //and is_detected<_assignable_archetype, Iterator>::value //doesn't work for some reason
      and is_detected_exact<Iterator&, _preincrementable_archetype, Iterator>::value
      and is_detected_convertible<Iterator const&, _postincrementable_archetype, Iterator>::value
      //and is_detected<_postincrement_assign_archetype, Iterator>::value //doesn't work for some reason
      and is_iterator<Iterator>::value
    >;
    static constexpr auto value = type::value;
};

// Concept checking for stl container types

template <typename C>
struct is_container {
  public:
    // Must have a value_type member type
    using value_type = detected_t<has_value_type_archetype, C>;
    static constexpr auto has_value_type = is_detected<has_value_type_archetype, C>::value;

    // value_type must be erasable
    static constexpr auto value_type_is_valid = is_erasable<value_type>::value;

    // Must have reference and const_reference member types
  private:
    template <typename T>
    using has_reference_member_type_archetype = typename T::reference;
    template <typename T>
    using has_const_reference_member_type_archetype = typename T::const_reference;
  public:
    using reference = detected_t<has_reference_member_type_archetype, C>;
    using const_reference = detected_t<has_const_reference_member_type_archetype, C>;
    static constexpr auto has_reference = is_detected<has_value_type_archetype, C>::value;
    static constexpr auto has_const_reference = is_detected<has_value_type_archetype, C>::value;

    // Must have iterator and const_iterator member types
  private:
    template <typename T>
    using has_iterator_member_type_archetype = typename T::iterator;
    template <typename T>
    using has_const_iterator_member_type_archetype = typename T::const_iterator;
  public:
    using iterator = detected_t<has_iterator_member_type_archetype, C>;
    using const_iterator = detected_t<has_const_iterator_member_type_archetype, C>;
    static constexpr auto has_iterator = is_detected<has_value_type_archetype, C>::value;
    static constexpr auto has_const_iterator = is_detected<has_value_type_archetype, C>::value;

    // iterator and const_iterator member types must meet the ForwardIterator concept
    static constexpr auto iterator_is_valid = is_forward_iterator<iterator>::value;
    static constexpr auto const_iterator_is_valid = is_forward_iterator<const_iterator>::value;

    // must have difference_type and size_type member types
  private:
    template <typename T>
    using has_difference_type_member_type_archetype = typename T::difference_type;
    template <typename T>
    using has_size_type_member_type_archetype = typename T::size_type;
  public:
    using difference_type = detected_t<has_difference_type_member_type_archetype, C>;
    static constexpr auto has_difference_type =
      is_detected<has_difference_type_member_type_archetype, C>::value;
    using size_type = detected_t<has_size_type_member_type_archetype, C>;
    static constexpr auto has_size_type =
      is_detected<has_size_type_member_type_archetype, C>::value;

    // must be default constructible
    static constexpr auto is_default_constructible = std::is_default_constructible<C>::value;

    // TODO must be copy constructible if value_type is CopyInsertable

    // must be assignable
    static constexpr auto is_assignable = std::is_assignable<C, C>::value;

    // must be destructible
    static constexpr auto is_destructible = std::is_destructible<C>::value;

    // must have a begin() method that returns an iterator, and a begin() const method
    // that returns a const_iterator (and same for end).  Also has to have cbegin() and
    // cend() that return const_iterator instances
  private:
    template <typename T>
    using has_begin_method_archetype = decltype( std::declval<T>().begin() );
    template <typename T>
    using has_end_method_archetype = decltype( std::declval<T>().end() );
    template <typename T>
    using has_cbegin_method_archetype = decltype( std::declval<T>().cbegin() );
    template <typename T>
    using has_cend_method_archetype = decltype( std::declval<T>().cend() );
  public:
    static constexpr auto begin_method_works =
      is_detected_exact<iterator, has_begin_method_archetype, C>::value;
    static constexpr auto begin_const_method_works =
      is_detected_exact<const_iterator, has_begin_method_archetype, const C>::value;
    static constexpr auto cbegin_method_works =
      is_detected_exact<const_iterator, has_cbegin_method_archetype, C>::value;
    static constexpr auto end_method_works =
      is_detected_exact<iterator, has_end_method_archetype, C>::value;
    static constexpr auto end_const_method_works =
      is_detected_exact<const_iterator, has_end_method_archetype, const C>::value;
    static constexpr auto cend_method_works =
      is_detected_exact<const_iterator, has_cend_method_archetype, C>::value;

    // must be EqualityComparable if value_type is EqualityComparable
    // bug in tuple definition of equality causes this to not work for tuples
    //static constexpr auto compatible_equality_comparable =
    //  (not is_equality_comparable<value_type>::value) or is_equality_comparable<C>::value;

    // TODO swap, size, max_size, empty

  public:

    static constexpr auto value = \
         has_value_type
      && value_type_is_valid
      && has_reference
      && has_const_reference
      && has_iterator
      && has_const_iterator
      && iterator_is_valid
      && const_iterator_is_valid
      && has_difference_type
      && has_size_type
      && is_default_constructible
      && is_assignable
      && is_destructible
      && begin_method_works
      && begin_const_method_works
      && cbegin_method_works
      && end_method_works
      && end_const_method_works
      && cend_method_works
      //&& compatible_equality_comparable
    ;

};

} // end namespace meta
} // end namespace darma_runtime

#endif //DARMA_IMPL_META_IS_CONTAINER_H
