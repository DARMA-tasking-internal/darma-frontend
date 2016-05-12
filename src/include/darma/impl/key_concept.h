/*
//@HEADER
// ************************************************************************
//
//                          key_concept.h
//                         dharma_new
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

#ifndef FRONTEND_INCLUDE_DARMA_KEY_CONCEPT_H_
#define FRONTEND_INCLUDE_DARMA_KEY_CONCEPT_H_

#include <type_traits> // std::integral_constant

#include <darma/impl/meta/detection.h>

namespace darma_runtime {
namespace detail {

template <typename key_type>
struct key_traits;

template <typename T>
struct meets_key_concept {
  private:

  ////////////////////////////////////////////////////////////////////////////////
  // must have a valid traits specialization

    // Must specialize the key_traits class
    template <typename Key>
    //using specializes_key_traits_archetype = decltype( std::declval<key_traits<Key>>() );
    using specializes_key_traits_archetype = key_traits<Key>;

    // This will be the type meta::nonesuch if the specialization doesn't exist
    // and key_traits<Key> if it does (see meta::detected_t)
    typedef meta::detected_t<specializes_key_traits_archetype, T> traits_t;

  public:
    static constexpr auto has_traits_specialization =
        meta::is_detected<specializes_key_traits_archetype, T>::value;

  ////////////////////////////////////////////////////////////////////////////////
  // must have a valid key_equal in traits

  private:
    // Traits class must define key_equal type
    template <typename Traits>
    using traits_key_equal_archetype = typename Traits::key_equal;
    typedef meta::detected_t<traits_key_equal_archetype, traits_t> key_equal_t;

  public:
    static constexpr auto has_key_equal =
        meta::is_detected<traits_key_equal_archetype, traits_t>::value;

  private:
    // The key equal functor must be able to compare two keys and must be default constructible
    template <typename KeyEqual>
    using key_equal_archetype = decltype( KeyEqual()(std::declval<const T>(), std::declval<const T>()) );

  public:
    static constexpr auto key_equal_works =
        meta::is_detected_convertible<bool, key_equal_archetype, key_equal_t>::value;

  ////////////////////////////////////////////////////////////////////////////////
  // must have a valid hasher in traits

  private:
    // Traits class must define hasher type
    template <typename Traits>
    using traits_hasher_archetype = typename Traits::hasher;
    typedef meta::detected_t<traits_hasher_archetype, traits_t> hasher_t;

  public:
    static constexpr auto has_hasher =
        meta::is_detected<traits_hasher_archetype, traits_t>::value;

  private:
    // The hasher functor must be default constructible and must be callable with a const T
    template <typename Hasher>
    using hasher_archetype = decltype( Hasher()( std::declval<const T>() ) );

  public:
    static constexpr auto hasher_works =
        meta::is_detected_convertible<size_t, hasher_archetype, hasher_t>::value;

  ////////////////////////////////////////////////////////////////////////////////
  // must have a valid maker and maker_from_tuple in traits

  private:
    // Traits class must define maker and maker_from_tuple member types
    template <typename Traits>
    using traits_maker_archetype = typename Traits::maker;
    typedef meta::detected_t<traits_maker_archetype, traits_t> maker_t;
    template <typename Traits>
    using traits_maker_from_tuple_archetype = typename Traits::maker_from_tuple;
    typedef meta::detected_t<traits_maker_from_tuple_archetype, traits_t> maker_from_tuple_t;

  public:
    static constexpr auto has_maker =
        meta::is_detected<traits_maker_archetype, traits_t>::value;
    static constexpr auto has_maker_from_tuple =
        meta::is_detected<traits_maker_from_tuple_archetype, traits_t>::value;

  // This probably needs stronger concept support to do formally, since we need a KeyPart concept
  //public:
  //  static constexpr auto maker_works =
  //      meta::is_detected_convertible<size_t, hasher_archetype, hasher_t>::value;

  ////////////////////////////////////////////////////////////////////////////////
  // must have a valid component<N>() member function

  private:
    template <int N>
    struct component_method_archetype {
      template <typename Key>
      using apply = decltype( std::declval<Key>().template component<N>() );
    };

    // Test the first few integers...
    typedef meta::detected_t<component_method_archetype<0>::template apply, T> key_part_0_t;
    static constexpr auto has_key_part_0 =
        meta::is_detected<component_method_archetype<0>::template apply, T>::value;
    typedef meta::detected_t<component_method_archetype<1>::template apply, T> key_part_1_t;
    static constexpr auto has_key_part_1 =
        meta::is_detected<component_method_archetype<1>::template apply, T>::value;
    typedef meta::detected_t<component_method_archetype<2>::template apply, T> key_part_2_t;
    static constexpr auto has_key_part_2 =
        meta::is_detected<component_method_archetype<2>::template apply, T>::value;
    typedef meta::detected_t<component_method_archetype<3>::template apply, T> key_part_3_t;
    static constexpr auto has_key_part_3 =
        meta::is_detected<component_method_archetype<3>::template apply, T>::value;
    // And a random larger integer...
    typedef meta::detected_t<component_method_archetype<17>::template apply, T> key_part_17_t;
    static constexpr auto has_key_part_17 =
        meta::is_detected<component_method_archetype<17>::template apply, T>::value;

    // Also test that the key parts have an as<> function (just using int for now)
    typedef int component_as_test_t;
    template <typename KeyPart>
    using key_part_as_archetype = decltype( std::declval<KeyPart>().template as<component_as_test_t>() );

    static constexpr auto key_part_0_has_as =
        meta::is_detected_convertible<component_as_test_t, key_part_as_archetype, key_part_0_t>::value;
    static constexpr auto key_part_1_has_as =
        meta::is_detected_convertible<component_as_test_t, key_part_as_archetype, key_part_1_t>::value;
    static constexpr auto key_part_2_has_as =
        meta::is_detected_convertible<component_as_test_t, key_part_as_archetype, key_part_2_t>::value;
    static constexpr auto key_part_3_has_as =
        meta::is_detected_convertible<component_as_test_t, key_part_as_archetype, key_part_3_t>::value;
    static constexpr auto key_part_17_has_as =
        meta::is_detected_convertible<component_as_test_t, key_part_as_archetype, key_part_17_t>::value;

  public:

    static constexpr auto has_component_method =
         has_key_part_0
      && has_key_part_1
      && has_key_part_2
      && has_key_part_3
      && has_key_part_17
    ;

    static constexpr auto key_component_method_works =
         key_part_0_has_as
      && key_part_1_has_as
      && key_part_2_has_as
      && key_part_3_has_as
      && key_part_17_has_as
    ;

  public:

    static constexpr auto value =
         has_traits_specialization
      && has_key_equal
      && key_equal_works
      && has_hasher
      && hasher_works
      && has_maker
      && has_maker_from_tuple
      && has_component_method
      && key_component_method_works
    ;

    typedef std::integral_constant<bool, value> type;
};

template <typename Key>
std::enable_if_t<detail::meets_key_concept<Key>::value, bool>
operator==(const Key& a, const Key& b) {
  return typename detail::key_traits<Key>::key_equal()(a, b);
}

template <typename Key>
std::enable_if_t<detail::meets_key_concept<Key>::value, std::ostream&>
operator<<(std::ostream& o, Key const& k) {
  k.print_human_readable(", ", o);
  return o;
};

} // end namespace detail



} // end namespace darma_runtime


#define DARMA_STATIC_ASSERT_VALID_KEY_TYPE(K) \
  static_assert(darma_runtime::detail::meets_key_concept<K>::has_traits_specialization, \
    "key_traits not specialized for type" #K); \
  static_assert(darma_runtime::detail::meets_key_concept<K>::has_key_equal, \
    "key_traits<" #K "> specialization missing key_equal member type"); \
  static_assert(darma_runtime::detail::meets_key_concept<K>::key_equal_works, \
    "key_traits<" #K ">::key_equal does work as expected"); \
  static_assert(darma_runtime::detail::meets_key_concept<K>::has_hasher, \
    "key_traits<" #K "> specialization missing hasher member type"); \
  static_assert(darma_runtime::detail::meets_key_concept<K>::hasher_works, \
    "key_traits<" #K ">::hasher does work as expected"); \
  static_assert(darma_runtime::detail::meets_key_concept<K>::has_maker, \
    "key_traits<" #K "> specialization missing maker member type"); \
  static_assert(darma_runtime::detail::meets_key_concept<K>::has_maker_from_tuple, \
    "key_traits<" #K "> specialization missing maker_from_tuple member type"); \
  static_assert(darma_runtime::detail::meets_key_concept<K>::has_component_method, \
    #K " is missing template method component<N>()"); \
  static_assert(darma_runtime::detail::meets_key_concept<K>::key_component_method_works, \
    "return value of component<N>() for " #K " does not have an as<>() method that returns the component as a type"); \

static_assert(not darma_runtime::detail::meets_key_concept<int>::value,
  "oops, int shouldn't be a valid key");

#endif /* FRONTEND_INCLUDE_DARMA_KEY_CONCEPT_H_ */
