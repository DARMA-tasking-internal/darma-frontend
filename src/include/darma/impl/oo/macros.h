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

#include <darma/impl/oo/util.h> // detail::empty_base
#include <darma/impl/meta/detection.h> // meta::is_detected

#define _DARMA__OO_PASSTHROUGH_CONSTRUCTORS(name, ext) \
name##ext() = default; \
name##ext(name##ext const&) = default; \
name##ext(name##ext &&) = default; \
template <typename T, \
  typename = std::enable_if_t< \
    not std::is_same<std::decay_t<T>, name##ext>::value \
    and _darma__has_##name##_member_access<T, ValueType>::value \
  > \
> \
name##ext(T&& other) \
  : Base(std::forward<T>(other)), name( \
       _darma__##name##__oo_access<ValueType>::name(std::forward<T>(other)) \
    ) \
{ };

#define DARMA_OO_DEFINE_TAG(name) \
template <typename ValueType> \
struct _darma__##name##__oo_access {  \
  template <typename T> \
  static std::enable_if_t< \
    std::is_lvalue_reference<T&&>::value,  \
    ValueType>& \
  name(T&& from) { return from.name; } \
  template <typename T> \
  static std::enable_if_t< \
    std::is_rvalue_reference<T&&>::value,  \
    ValueType>&& \
  name(T&& from) { return std::forward<T>(from).name; } \
}; \
template <typename T, typename ValueType> using _darma__##name##__member_access_archetype = \
  decltype( _darma__##name##__oo_access<ValueType>::template name(std::declval<T>()) );\
template <typename T, typename ValueType> using _darma__has_##name##_member_access = \
  ::darma_runtime::meta::is_detected<ValueType, _darma__##name##__member_access_archetype, T, ValueType>; \
template <typename ValueType, typename Base> \
struct name##__as_private_field : Base { \
  _DARMA__OO_PASSTHROUGH_CONSTRUCTORS(name, __as_private_field) \
  protected: \
    ValueType name; \
    friend struct _darma__##name##__oo_access<ValueType>; \
}; \
template <typename ValueType, typename Base> \
struct name##__as_public_field : Base { \
  _DARMA__OO_PASSTHROUGH_CONSTRUCTORS(name, __as_public_field) \
  ValueType name; \
  friend struct _darma__##name##__oo_access<ValueType>; \
}; \
struct name { \
  template <typename T, typename Base> \
  using as_private_field_in_chain = name##__as_private_field<T, Base>; \
  template <typename T> \
  using as_private_field = name##__as_private_field<T, darma_runtime::oo::detail::empty_base>; \
  template <typename T, typename Base> \
  using as_public_field_in_chain = name##__as_public_field<T, Base>; \
  template <typename T> \
  using as_public_field = name##__as_public_field<T, darma_runtime::oo::detail::empty_base>; \
};

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
