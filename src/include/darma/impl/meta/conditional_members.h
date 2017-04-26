/*
//@HEADER
// ************************************************************************
//
//                      conditional_members.h
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

#ifndef DARMAFRONTEND_IMPL_META_CONDITIONAL_MEMBERS_H
#define DARMAFRONTEND_IMPL_META_CONDITIONAL_MEMBERS_H

#include <utility> // std::index_sequence_for
#include <tinympl/vector.hpp>
#include <tinympl/variadic/at.hpp>

namespace darma_runtime {
namespace meta {

namespace impl {

template <size_t Idx, typename ConditionsAndTypesVector, typename Enable=void>
struct with_conditional_member_entry_impl;

template <size_t Idx, typename... ConditionsAndTypes>
struct with_conditional_member_entry_impl<Idx,
  tinympl::vector<ConditionsAndTypes...>,
  std::enable_if_t<
    tinympl::variadic::at_t<Idx, ConditionsAndTypes...>::condition_value
      and not tinympl::variadic::at_t<Idx, ConditionsAndTypes...>::has_initializer
  >
> {
  using type = typename tinympl::variadic::at_t<Idx, ConditionsAndTypes...>::type;
  type value;
};

template <size_t Idx, typename... ConditionsAndTypes>
struct with_conditional_member_entry_impl<Idx,
  tinympl::vector<ConditionsAndTypes...>,
  std::enable_if_t<
    tinympl::variadic::at_t<Idx, ConditionsAndTypes...>::condition_value
      and tinympl::variadic::at_t<Idx, ConditionsAndTypes...>::has_initializer
  >
> {
  using member_description = tinympl::variadic::at_t<Idx, ConditionsAndTypes...>;
  using type = typename member_description::type;
  using initializer = typename member_description::initializer;
  type value = initializer{}();
};

template <size_t Idx, typename... ConditionsAndTypes>
struct with_conditional_member_entry_impl<Idx,
  tinympl::vector<ConditionsAndTypes...>,
  std::enable_if_t<
    not tinympl::variadic::at_t<Idx, ConditionsAndTypes...>::condition_value
      and not tinympl::variadic::at_t<Idx, ConditionsAndTypes...>::has_static_default
  >
> {
  using member_description = tinympl::variadic::at_t<Idx, ConditionsAndTypes...>;
  using type = typename member_description::type;
};

template <size_t Idx, typename... ConditionsAndTypes>
struct with_conditional_member_entry_impl<Idx,
  tinympl::vector<ConditionsAndTypes...>,
  std::enable_if_t<
    not tinympl::variadic::at_t<Idx, ConditionsAndTypes...>::condition_value
      and tinympl::variadic::at_t<Idx, ConditionsAndTypes...>::has_static_default
  >
> {
  using member_description = tinympl::variadic::at_t<Idx, ConditionsAndTypes...>;
  using type = typename member_description::type;
  static constexpr auto value = member_description::static_default_value;
};


template <typename...> struct with_conditional_members_impl;

template <typename... ConditionsAndTypes, size_t... Idxs>
struct with_conditional_members_impl<
  tinympl::vector<ConditionsAndTypes...>, std::integer_sequence<size_t, Idxs...>
> : with_conditional_member_entry_impl<
      Idxs, tinympl::vector<ConditionsAndTypes...>
    >...
{ };

struct conditional_member_access {
  template <size_t Idx, typename ClassWithConditionalMembers>
  static auto&
  get_conditional_member_impl(ClassWithConditionalMembers& obj) {
    // this is an upcast, so we don't really need static_cast, but for
    // the sake of formality:
    return static_cast<
      darma_runtime::meta::impl::with_conditional_member_entry_impl<
        Idx, typename ClassWithConditionalMembers::conditions_and_types_vector_t
      >&
    >(obj).value;
  }

  template <size_t Idx, typename ClassWithConditionalMembers>
  static auto const&
  get_conditional_member_impl(ClassWithConditionalMembers const& obj) {
    // this is an upcast, so we don't really need static_cast, but for
    // the sake of formality:
    return static_cast<
      darma_runtime::meta::impl::with_conditional_member_entry_impl<
        Idx, typename ClassWithConditionalMembers::conditions_and_types_vector_t
      > const&
    >(obj).value;
  }

};

} // end namespace impl

template <
  bool Condition, typename ValueType
>
struct conditional_member_description {
  static constexpr auto condition_value = Condition;
  static constexpr auto has_initializer = false;
  static constexpr auto has_static_default = false;
  using type = ValueType;
};

template <
  bool Condition, typename ValueType, typename Initializer
>
struct conditional_member_with_initializer_description {
  static constexpr auto condition_value = Condition;
  static constexpr auto has_initializer = true;
  static constexpr auto has_static_default = false;
  using type = ValueType;
  using initializer = Initializer;
};

template <
  bool Condition, typename ValueType, ValueType Default
>
struct conditional_member_with_static_default_description {
  static constexpr auto condition_value = Condition;
  static constexpr auto has_initializer = false;
  static constexpr auto has_static_default = true;
  static constexpr auto static_default_value = Default;
  using type = ValueType;
};


template <typename... ConditionsAndTypes>
struct with_conditional_members
  : private impl::with_conditional_members_impl<
      tinympl::vector<ConditionsAndTypes...>,
      std::make_index_sequence<sizeof...(ConditionsAndTypes)>
    >
{
  //static_assert(
  //  tinympl::and_<
  //    tinympl::is_instantiation_of<
  //      conditional_member_description, ConditionsAndTypes
  //    >...
  //  >::value,
  //  "Template parameters for with_conditional_members<...> should be of the form"
  //    " conditional_member_*_description<Condition1, Type1>,"
  //    " conditional_member_*_description<Condition2, Type2>, ..."
  //);

  using conditions_and_types_vector_t = tinympl::vector<
    ConditionsAndTypes...
  >;


  template <typename, size_t Idx>
  friend struct has_conditional_member;

  friend struct impl::conditional_member_access;

};

template <size_t Idx, typename ClassWithConditionalMembers>
auto&
get_conditional_member(ClassWithConditionalMembers& obj) {
  return impl::conditional_member_access::get_conditional_member_impl<Idx>(obj);
};

template <size_t Idx, typename ClassWithConditionalMembers>
auto const&
get_conditional_member(ClassWithConditionalMembers const& obj) {
  return impl::conditional_member_access::get_conditional_member_impl<Idx>(obj);
};

template <typename ClassWithConditionalMembers, size_t Idx>
struct has_conditional_member {
  using type = tinympl::bool_<
    tinympl::at_t<Idx,
      typename ClassWithConditionalMembers::conditions_and_types_vector_t
    >::condition_value
  >;
  static constexpr auto value = type::value;
};


} // end namespace meta
} // end namespace darma_runtime

#endif //DARMAFRONTEND_IMPL_META_CONDITIONAL_MEMBERS_H
