/*
//@HEADER
// ************************************************************************
//
//                      macros.h
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

#ifndef DARMA_IMPL_OO_MACROS_H
#define DARMA_IMPL_OO_MACROS_H

#include <type_traits> // std::declval
#include <utility> // std::forward

#include <tinympl/logical_not.hpp>

#include <darma/impl/oo/util.h> // detail::empty_base
#include <darma/impl/meta/detection.h> // meta::is_detected
#include <darma/impl/handle.h> // detail::is_access_handle

#define _DARMA__OO_PASSTHROUGH_CONSTRUCTORS(name, ext) \
/* Explicitly default the copy, move, and default constructors */ \
constexpr inline name##ext() = default; \
constexpr inline name##ext(name##ext const&) = default; \
constexpr inline name##ext(name##ext &&) = default; \
\
/* for types in a chained base hierarchy that have a member of the correct name */ \
/* and type, extract that value and forward the object on to the base */ \
template <typename T, \
  typename = std::enable_if_t< \
    not std::is_same<std::decay_t<T>, name##ext>::value \
    and _darma__has_##name##_member_access<T, std::decay_t<ValueType>, ValueType>::value \
    and darma_runtime::oo::detail::is_chained_base_class<std::decay_t<T>>::value \
    /* and std::is_convertible< \
      decltype( \
        _darma__##name##_oo_access_friend_t<std::decay_t<ValueType>>::template name<ValueType>( \
          std::declval<T>() \
        ) \
      ), \
      ValueType \
    >::value */ \
  > \
> \
constexpr inline explicit \
name##ext(T&& other) \
  : Base(std::forward<T>(other)), name( \
       _darma__##name##_oo_access_friend_t<std::decay_t<ValueType>>::template name<ValueType>( \
         std::forward<T>(other) \
       ) \
    ) \
{ };

#define DARMA_OO_DEFINE_TAG(name) \
struct name; \
using _darma__##name##_oo_tag_class = name; \
\
template <typename ValueType, typename IsAccessHandle> \
struct _darma__##name##__oo_access {  \
  static_assert(not std::is_reference<ValueType>::value, "ValueType can't be a reference"); \
  static_assert(not std::is_const<ValueType>::value, "ValueType can't be a const qualified"); \
  template <typename ExpectedType, typename T> \
  static std::enable_if_t< \
    ( \
      not darma_runtime::detail::is_access_handle<std::decay_t<decltype(std::declval<T>().name)>>::value \
      or darma_runtime::detail::is_access_handle<std::decay_t<ExpectedType>>::value \
    ) \
    and std::is_lvalue_reference<T&&>::value,  \
    std::remove_reference_t<decltype(std::declval<T>().name)>& \
  > \
  name(T&& from) { return from.name; } \
  \
  template <typename ExpectedType, typename T> \
  static std::enable_if_t< \
    ( \
      not darma_runtime::detail::is_access_handle<std::decay_t<decltype(std::declval<T>().name)>>::value \
      or darma_runtime::detail::is_access_handle<std::decay_t<ExpectedType>>::value \
    ) \
    and std::is_rvalue_reference<T&&>::value,  \
    std::remove_reference_t<decltype(std::declval<T>().name)>&& \
  > \
  name(T&& from) { return std::forward<T>(from).name; } \
  template <typename ExpectedType, typename T> \
  static std::enable_if_t< \
    ( \
      darma_runtime::detail::is_access_handle<std::decay_t<decltype(std::declval<T>().name)>>::value \
      and not darma_runtime::detail::is_access_handle<std::decay_t<ExpectedType>>::value \
    ) \
    /* TODO check that the value_type is convertible */ \
    and std::is_const<std::remove_reference_t<ExpectedType>>::value, \
    ExpectedType \
  > \
  name(T&& from) { return from.name.get_value(); } \
  template <typename ExpectedType, typename T> \
  static std::enable_if_t< \
    ( \
      darma_runtime::detail::is_access_handle<std::decay_t<decltype(std::declval<T>().name)>>::value \
      and not darma_runtime::detail::is_access_handle<std::decay_t<ExpectedType>>::value \
    ) \
    /* TODO check that the value_type is convertible */ \
    and std::is_lvalue_reference<ExpectedType>::value \
    and not std::is_const<std::remove_reference_t<ExpectedType>>::value, \
    ExpectedType \
  > \
  name(T&& from) { return from.name.get_reference(); } \
}; \
\
template <typename T> \
using _darma__##name##_oo_access_friend = tinympl::identity<_darma__##name##__oo_access< \
  std::conditional_t<::darma_runtime::detail::is_access_handle<std::decay_t<T>>::value, \
    ::darma_runtime::detail::value_type_if_access_handle_t<std::decay_t<T>>, \
    std::decay_t<T> \
  >, \
  typename darma_runtime::detail::is_access_handle<std::decay_t<T>>::type \
>>; \
\
/* special access friend allows friendship between classes containing wrapping members */ \
/* of AccessHandle types with different traits  */ \
template <typename T> \
using _darma__##name##_oo_access_friend_t = typename _darma__##name##_oo_access_friend<T>::type; \
template <typename T, typename ValueType, typename ExpectedType> \
using _darma__##name##__member_access_archetype = \
  decltype( \
    _darma__##name##_oo_access_friend_t<ValueType>::template name<ExpectedType>( \
      std::declval<T>() \
    )  \
  );\
template <typename T, typename ValueType, typename ExpectedType> \
using _darma__has_##name##_member_access = \
  ::darma_runtime::meta::is_detected< \
    _darma__##name##__member_access_archetype, T, ValueType, ExpectedType \
  >; \
\
template <typename ValueType, typename Base> \
struct name##__as_private_field : Base { \
    _DARMA__OO_PASSTHROUGH_CONSTRUCTORS(name, __as_private_field) \
  protected: \
    ValueType name; \
    /* Should be friends with both the access handle and non-access handle attorneys */ \
    friend struct _darma__##name##__oo_access< \
      std::conditional_t<::darma_runtime::detail::is_access_handle<std::decay_t<ValueType>>::value, \
        ::darma_runtime::detail::value_type_if_access_handle_t<std::decay_t<ValueType>>, \
        std::decay_t<ValueType> \
      >, \
      typename darma_runtime::detail::is_access_handle<std::decay_t<ValueType>>::type \
    >; \
    friend struct _darma__##name##__oo_access< \
      std::conditional_t<::darma_runtime::detail::is_access_handle<std::decay_t<ValueType>>::value, \
        ::darma_runtime::detail::value_type_if_access_handle_t<std::decay_t<ValueType>>, \
        std::decay_t<ValueType> \
      >, \
      typename tinympl::not_< \
         typename darma_runtime::detail::is_access_handle<std::decay_t<ValueType>>::type \
       >::type \
    >; \
}; \
\
template <typename ValueType, typename Base> \
struct name##__as_public_field : Base { \
  _DARMA__OO_PASSTHROUGH_CONSTRUCTORS(name, __as_public_field) \
  ValueType name; \
  /* Should be friends with both the access handle and non-access handle attorneys */ \
  friend struct _darma__##name##__oo_access< \
    std::conditional_t<::darma_runtime::detail::is_access_handle<std::decay_t<ValueType>>::value, \
      ::darma_runtime::detail::value_type_if_access_handle_t<std::decay_t<ValueType>>, \
      std::decay_t<ValueType> \
    >, \
    typename darma_runtime::detail::is_access_handle<std::decay_t<ValueType>>::type \
  >; \
  friend struct _darma__##name##__oo_access< \
    std::conditional_t<::darma_runtime::detail::is_access_handle<std::decay_t<ValueType>>::value, \
      ::darma_runtime::detail::value_type_if_access_handle_t<std::decay_t<ValueType>>, \
      std::decay_t<ValueType> \
    >, \
    typename tinympl::not_< \
       typename darma_runtime::detail::is_access_handle<std::decay_t<ValueType>>::type \
     >::type \
  >; \
}; \
\
template <typename OfClass, typename Base, typename CastThisTo> \
struct name##__as_public_method : Base { \
  using deferred = name##__as_public_method; \
  /* Explicitly default the copy, move, and default constructors */ \
  constexpr inline name##__as_public_method() = default; \
  constexpr inline name##__as_public_method(name##__as_public_method const& val) = default; \
  constexpr inline name##__as_public_method(name##__as_public_method && val) = default; \
  \
  /* Forward to base class if it's not a copy or move constructor */ \
  template <typename T, \
    typename = std::enable_if_t< \
      not std::is_same<std::decay_t<T>, name##__as_public_method>::value \
        and ::darma_runtime::oo::detail::is_chained_base_class<std::decay_t<T>>::value \
    > \
  > \
  constexpr inline explicit \
  name##__as_public_method(T&& val) \
    : Base(std::forward<T>(val)) \
  { } \
  \
  template <typename... Args> \
  void name(Args&&... args) { \
    using method_struct_t = decltype(  \
        (_darma__get_associated_method_template_specialization( \
           std::declval<OfClass&>(), \
           std::declval< _darma__##name##_oo_tag_class & >()  \
        )) \
    ); \
    /* TODO forward arguments as well */ \
    darma_runtime::oo::detail::_create_deferred_method_call<method_struct_t>( \
      *static_cast<CastThisTo*>(this) \
    ); \
  } \
}; \
\
template <typename OfClass, typename Base, typename CastThisTo> \
struct name##__as_immediate_public_method : Base { \
  using immediate = name##__as_immediate_public_method; \
  /* Explicitly default the copy, move, and default constructors */ \
  constexpr inline name##__as_immediate_public_method() = default; \
  constexpr inline name##__as_immediate_public_method(name##__as_immediate_public_method const& val) = default; \
  constexpr inline name##__as_immediate_public_method(name##__as_immediate_public_method && val) = default; \
  \
  /* Forward to base class if it's not a copy or move constructor */ \
  template <typename T, \
    typename = std::enable_if_t< \
      not std::is_same<std::decay_t<T>, name##__as_immediate_public_method>::value \
        and ::darma_runtime::oo::detail::is_chained_base_class<std::decay_t<T>>::value \
    > \
  > \
  constexpr inline explicit \
  name##__as_immediate_public_method(T&& val) \
    : Base(std::forward<T>(val)) \
  { } \
  \
  template <typename... Args> \
  void name(Args&&... args) { \
    using method_struct_t = decltype(  \
        (_darma__get_associated_method_template_specialization( \
           std::declval<OfClass&>(), \
           std::declval< _darma__##name##_oo_tag_class & >()  \
        )) \
    ); \
    method_struct_t(*static_cast<CastThisTo*>(this))(std::forward<Args>(args)...); \
  } \
}; \
template <typename ReturnType> \
struct name##__run_method_invoker { \
  template <typename T, typename... Args> \
  static ReturnType run(T&& to_call, Args&&... args) { \
    return std::forward<T>(to_call).name(std::forward<Args>(args)...); \
  } \
}; \
\
struct name { \
  template <typename T, typename Base> \
  using as_private_field_in_chain = name##__as_private_field<T, Base>; \
  template <typename T> \
  using as_private_field = name##__as_private_field<T, darma_runtime::oo::detail::empty_base>; \
  template <typename T, typename Base> \
  using as_public_field_in_chain = name##__as_public_field<T, Base>; \
  template <typename T> \
  using as_public_field = name##__as_public_field<T, darma_runtime::oo::detail::empty_base>; \
  template <typename OfClass, typename Base, typename CastThisTo=OfClass> \
  using as_public_method_in_chain = name##__as_public_method<OfClass, Base, CastThisTo>; \
  template <typename OfClass, typename Base, typename CastThisTo=OfClass> \
  using as_immediate_public_method_in_chain = name##__as_immediate_public_method<OfClass, Base, CastThisTo>; \
  template <typename OfClass> \
  using as_public_method = name##__as_public_method<OfClass, darma_runtime::oo::detail::empty_base, OfClass>; \
  template <typename OfClass> \
  using as_immediate_public_method = name##__as_immediate_public_method<OfClass, darma_runtime::oo::detail::empty_base, OfClass>; \
  template <typename ReturnType> \
  using run_method_invoker = name##__run_method_invoker<ReturnType>; \
}

#define DARMA_OO_DECLARE_CLASS(name) \
  struct name; \
  template <typename Tag> struct name##_method; \
  /* A function to use with ADL to associate the name##_method template with name */ \
  /* Note that this method should always have no definition and should never be called */ \
  /* in an evaluated context. */ \
  template <typename Tag> \
  name##_method<std::remove_reference_t<Tag>> _darma__get_associated_method_template_specialization(name&, Tag&); \
  template <typename Tag> \
  Tag _darma__get_associated_method_template_tag(name##_method<Tag>&);

//template <typename T, typename Base>
//struct name##__as_private_method : Base {
//  protected:
//    template <typename... Args>
//    typename T::return_type
//    name(Args&&... args) {
//      typename T::implementation()
//    }
//};
//template <typename T, typename Base>
//struct name##__as_public_field : Base {
//  T name;
//};
//template <typename T, typename Base>
//using as_public_method_in_chain = name##__as_public_method<T, Base>;
//template <typename T>
//using as_public_method = name##__as_public_method<T, darma_runtime::detail::empty_base>;

#endif //DARMA_IMPL_OO_MACROS_H
