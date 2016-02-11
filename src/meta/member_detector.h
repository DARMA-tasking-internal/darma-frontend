/*
//@HEADER
// ************************************************************************
//
//                          member_detector.h
//                         darma_new
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

#ifndef SRC_META_MEMBER_DETECTOR_H_
#define SRC_META_MEMBER_DETECTOR_H_

#include <tinympl/logical_and.hpp>
#include <tinympl/logical_not.hpp>
#include <tinympl/delay.hpp>
#include <type_traits>

namespace darma_runtime { namespace meta {

template <typename FunctionSignature>
struct as_method;

template <
  typename ReturnValue,
  typename... Args
>
struct as_method<ReturnValue(Args...)>
{
  template <typename T>
  struct bind {
    typedef ReturnValue (T::*type)(Args...);
  };

  template <typename T>
  struct bind_const {
    typedef ReturnValue (T::*type)(Args...) const;
  };
};

}} // end namespace darma_runtime::meta

// Wide swaths borrowed from:
//   https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Member_Detector
#define DARMA_META_MAKE_MEMBER_DETECTORS(MEMBER_NAME)                                              \
template <typename Class>                                                                           \
struct type_of_member_named_##MEMBER_NAME {                                                         \
  typedef decltype(&Class::MEMBER_NAME) type;                                                       \
};                                                                                                  \
template <typename T>                                                                               \
class has_member_named_##MEMBER_NAME                                                                \
{                                                                                                   \
    struct Fallback { int MEMBER_NAME; };                                                           \
    struct Derived : T, Fallback { };                                                               \
    template<typename U, U> struct Check;                                                           \
    typedef char No[1]; typedef char Yes[2];                                                        \
    template<typename U> static No & func(Check<int Fallback::*, &U::MEMBER_NAME> *);               \
    template<typename U> static Yes & func(...);                                                    \
  public:                                                                                           \
    static constexpr bool value = sizeof(func<Derived>(0)) == sizeof(Yes);                          \
    typedef std::integral_constant<bool, value> type;                                               \
};                                                                                                  \
template <typename T>                                                                               \
class has_method_named_##MEMBER_NAME                                                                \
  : public tinympl::and_<                                                                           \
      has_member_named_##MEMBER_NAME<T>,                                                            \
      tinympl::delay<                                                                               \
        tinympl::not_,                                                                              \
        tinympl::delay<                                                                             \
          std::is_member_object_pointer, type_of_member_named_##MEMBER_NAME<T>                      \
        >                                                                                           \
      >                                                                                             \
    >::type                                                                                         \
{ };                                                                                                \
template <typename T>                                                                               \
class has_instance_method_named_##MEMBER_NAME                                                       \
  : public tinympl::and_<                                                                           \
      has_member_named_##MEMBER_NAME<T>,                                                            \
      tinympl::delay<                                                                               \
        std::is_member_function_pointer, type_of_member_named_##MEMBER_NAME<T>                      \
      >                                                                                             \
    >::type                                                                                         \
{ };                                                                                                \
template <typename T>                                                                               \
class has_static_member_named_##MEMBER_NAME                                                         \
  : public tinympl::and_<                                                                           \
      has_member_named_##MEMBER_NAME<T>,                                                            \
      tinympl::delay<                                                                               \
        tinympl::not_,                                                                              \
        tinympl::delay<                                                                             \
          std::is_member_pointer, type_of_member_named_##MEMBER_NAME<T>                             \
        >                                                                                           \
      >                                                                                             \
    >::type                                                                                         \
{ };                                                                                                \
template <typename T, typename function_signature>                                                  \
class has_method_named_##MEMBER_NAME##_with_signature {                                             \
  private:                                                                                          \
    template <                                                                                      \
      typename U,                                                                                   \
      typename darma_runtime::meta::as_method<function_signature>::template bind<U>::type          \
    > struct Check;                                                                                 \
    template <typename U> static char func(Check<U, &U::MEMBER_NAME>*);                             \
    template <typename U> static int func(...);                                                     \
  public:                                                                                           \
    static constexpr const bool value = sizeof(func<T>(0)) == sizeof(char);                         \
    typedef std::integral_constant<bool, value> type;                                               \
};                                                                                                  \
template <typename T, typename function_signature>                                                  \
class has_const_method_named_##MEMBER_NAME##_with_signature {                                       \
  private:                                                                                          \
    template <                                                                                      \
      typename U,                                                                                   \
      typename darma_runtime::meta::as_method<function_signature>::template bind_const<U>::type    \
    > struct Check;                                                                                 \
    template <typename U> static char func(Check<U, &U::MEMBER_NAME>*);                             \
    template <typename U> static int func(...);                                                     \
  public:                                                                                           \
    static constexpr bool value = sizeof(func<T>(0)) == sizeof(char);                               \
    typedef std::integral_constant<bool, value> type;                                               \
};                                                                                                  \
template <typename T>                                                                               \
class has_member_type_named_##MEMBER_NAME                                                           \
{                                                                                                   \
    struct Fallback { struct MEMBER_NAME { }; };                                                    \
    struct Derived : T, Fallback { };                                                               \
    typedef char No[1]; typedef char Yes[2];                                                        \
    template<typename U> static No& func( typename U::MEMBER_NAME* );                               \
    template<typename U> static Yes& func( U* );                                                    \
  public:                                                                                           \
    static constexpr bool value = sizeof(func<Derived>(nullptr)) == sizeof(Yes);                    \
    typedef std::integral_constant<bool, value> type;                                               \
};


#endif /* SRC_META_MEMBER_DETECTOR_H_ */
