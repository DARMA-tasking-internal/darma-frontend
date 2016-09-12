/*
//@HEADER
// ************************************************************************
//
//                       callable_traits.h
//                           dharma
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

#ifndef DARMA_IMPL_META_CALLABLE_TRAITS_H_
#define DARMA_IMPL_META_CALLABLE_TRAITS_H_

#ifndef DARMA_META_MAX_CALLABLE_ARGS
#  define DARMA_META_MAX_CALLABLE_ARGS 126
#endif

#include <functional>
#include <type_traits>
#include <utility> // declval

#include <tinympl/always_true.hpp>
#include <tinympl/logical_or.hpp>
#include <tinympl/logical_not.hpp>

#include <darma/impl/meta/detection.h>
#include <darma/impl/util.h>

#include "any_convertible.h"
#include "is_callable.h"

namespace darma_runtime {

namespace meta {

namespace _callable_traits_impl {

template <size_t N, typename Callable>
struct get_param_N
  : get_param_N<N, decltype(&Callable::operator())>
{ };

template <size_t N, typename ReturnType, typename... Args>
struct get_param_N<N, ReturnType (*)(Args...)>
  : tinympl::identity<typename tinympl::variadic::at_t<N, Args...>>
{ };

template <size_t N, typename ClassType, typename ReturnType, typename... Args>
struct get_param_N<N, ReturnType (ClassType::*)(Args...)>
  : tinympl::identity<typename tinympl::variadic::at_t<N, Args...>>
{ };

// remove cv qualifiers from instance method callables
template <size_t N, typename ClassType, typename ReturnType, typename... Args>
struct get_param_N<N, ReturnType (ClassType::*)(Args...) const>
  : get_param_N<N, ReturnType (ClassType::*)(Args...)> { };
template <size_t N, typename ClassType, typename ReturnType, typename... Args>
struct get_param_N<N, ReturnType (ClassType::*)(Args...) const volatile>
  : get_param_N<N, ReturnType (ClassType::*)(Args...)> { };
template <size_t N, typename ClassType, typename ReturnType, typename... Args>
struct get_param_N<N, ReturnType (ClassType::*)(Args...) volatile>
  : get_param_N<N, ReturnType (ClassType::*)(Args...)> { };



////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="count_min_args">

template <typename F, size_t I = 0,  typename... Args>
struct count_min_args
  // Double "::type" short-circuits recursion when possible
  : std::conditional_t<
    is_callable_with_args<F, Args...>::value,
    std::integral_constant<size_t, I>,
    count_min_args<F, I+1, Args..., any_arg>
    >::type
{ };

template <typename F, typename... Args>
struct count_min_args<F, DARMA_META_MAX_CALLABLE_ARGS, Args...>
  : std::integral_constant<size_t, DARMA_META_MAX_CALLABLE_ARGS+1> { };

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="count_max_args">

template <typename F, size_t I = 0, bool min_found = false, typename... Args>
struct count_max_args
  : std::conditional_t<
      is_callable_with_args<F, Args...>::value,
      count_max_args<F, I+1, true, Args..., any_arg>,
      std::conditional_t<
        min_found,
        std::integral_constant<size_t, I-1>,
        count_max_args<F, I+1, false, Args..., any_arg>
      >
    >
{ };

// Base case just returns max; error message elsewhere.
template <typename F, bool min_found, typename... Args>
struct count_max_args<F, DARMA_META_MAX_CALLABLE_ARGS, min_found, Args...>
  : std::integral_constant<size_t, DARMA_META_MAX_CALLABLE_ARGS+1> { };

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="is_callable_replace_arg_n">

template <typename F,
  typename ArgNReplacement, typename OtherArgsType,
  size_t N, size_t I, size_t NTotal,
  typename... Args
>
struct is_callable_replace_arg_n
  : std::conditional_t<
      N >= NTotal,
      std::false_type,
      is_callable_replace_arg_n<F,
        ArgNReplacement, OtherArgsType,
        N, I+1, NTotal, Args...,
        std::conditional_t< I == N, ArgNReplacement, OtherArgsType >
      >
    >
{ };

template <typename F,
  typename ArgNReplacement, typename OtherArgsType,
  size_t N, size_t NTotal,
  typename... Args
>
struct is_callable_replace_arg_n<F,
  ArgNReplacement, OtherArgsType, N, NTotal, NTotal, Args...
> : is_callable_with_args<F, Args...>
{ };

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="arg_n_is">

// Be careful!  Things like is_const and is_reference don't work here because
// they can choose another cast operator
template <
  template <class...> class UnaryMetafunction,
  typename F, size_t N
>
struct arg_n_is
  : is_callable_replace_arg_n<F,
      any_arg_conditional<UnaryMetafunction>, any_arg,
      N, 0, count_max_args<F>::value
    >
{ };

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="all_args_match">

template <
  template <class...> class UnaryMetafunction,
  typename F, size_t I, size_t NTotal,
  typename... Args
>
struct all_args_match_impl
  : all_args_match_impl<
      UnaryMetafunction, F, I+1, NTotal, Args..., any_arg_conditional<UnaryMetafunction>
  >
{ };

template <
  template <class...> class UnaryMetafunction,
  typename F, size_t NTotal,
  typename... Args
>
struct all_args_match_impl<
  UnaryMetafunction, F, NTotal, NTotal, Args...
> : is_callable_with_args<F, Args...>
{ };

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="_callable_traits_maybe_min_eq_max">


template <typename Callable, size_t NArgsMin, size_t NArgsMax>
struct _callable_traits_maybe_min_eq_max {
  static constexpr auto n_args_min = NArgsMin;
  static constexpr auto n_args_max = NArgsMax;
  static_assert(n_args_min != DARMA_META_MAX_CALLABLE_ARGS+1
    and n_args_max != DARMA_META_MAX_CALLABLE_ARGS+1,
    "callable_traits<> used with callable having an invalid parameter (e.g., deleted"
      " copy constructor for a value parameter)"
      " (or, much less likely, you may need to increase DARMA_META_MAX_CALLABLE_ARGS, but "
      "this is very unlikely to be the problem)"
  );
};

template <typename Callable, size_t NArgs>
struct _callable_traits_maybe_min_eq_max<Callable, NArgs, NArgs> {
  static constexpr auto n_args_min = NArgs;
  static constexpr auto n_args_max = NArgs;
  static constexpr auto n_args = NArgs;
  static_assert(n_args != DARMA_META_MAX_CALLABLE_ARGS+1,
    "callable_traits<> used with callable having an invalid parameter (e.g.,"
      " deleted copy and move constructor for a value parameter)"
      " (or, much less likely, you may need to increase"
      " DARMA_META_MAX_CALLABLE_ARGS, but this is very unlikely to be the"
      " problem)"
  );
};

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

} // end namespace _callable_traits_impl

// TODO make these work (or at least fail reasonably) for templated Callables and universal references

template <typename Callable>
struct callable_traits
  : _callable_traits_impl::_callable_traits_maybe_min_eq_max<Callable,
       _callable_traits_impl::count_min_args<Callable>::value,
       _callable_traits_impl::count_max_args<Callable>::value
    >
{
  private:
    typedef _callable_traits_impl::_callable_traits_maybe_min_eq_max<Callable,
       _callable_traits_impl::count_min_args<Callable>::value,
       _callable_traits_impl::count_max_args<Callable>::value
    > base_t;

  public:

    template <size_t N>
    struct arg_n_is_nonconst_rvalue_reference
      : _callable_traits_impl::is_callable_replace_arg_n<Callable,
          any_nonconst_rvalue_reference,
          any_arg,
          N, 0, base_t::n_args_max
        >
    { };


    template <size_t N>
    struct arg_n_is_by_reference
      : _callable_traits_impl::is_callable_replace_arg_n<Callable,
          ambiguous_if_by_value,
          any_arg,
          N, 0, base_t::n_args_max
        >
    {
      static_assert(
        N < base_t::n_args_max,
        "N given to arg_n_is_by_reference is out of range for number of"
        " arguments to F"
      );
    };

    template <size_t N>
    struct arg_n_is_by_value
      : std::integral_constant< bool, not arg_n_is_by_reference<N>::value >
    {
      static_assert(
        N < base_t::n_args_max,
        "N given to arg_n_is_by_value is out of range for number of"
        " arguments to F"
      );
    };

    template <size_t N>
    struct arg_n_accepts_const_reference
      // The logical_or here is necessary to resolve an ambiguity between g++
      // and clang++. With clang++, only the part is necessary, but g++ doesn't
      // allow the first cast to resolve for by-value arguments, so the second
      // one is necessary in that case.  Specifically, this works around bug
      // #63217 in gcc.
      : tinympl::logical_or<
          _callable_traits_impl::is_callable_replace_arg_n<Callable,
            any_const_reference,
            any_arg,
            N, 0, base_t::n_args_max
          >,
          arg_n_is_by_value<N>
        >::type
    {
      static_assert(
        N < base_t::n_args_max,
        "N given to arg_accepts_const_reference is out of range for number of"
        " arguments to F"
      );
    };

    template <size_t N>
    struct arg_n_is_const_lvalue_reference
      : tinympl::logical_and<
          arg_n_accepts_const_reference<N>,
          tinympl::not_<arg_n_is_by_value<N>>
        >::type
    { };

    template <size_t N>
    struct arg_n_is_nonconst_lvalue_reference
      // Process of elimination: it's a reference but it doesn't take a const
      // reference
      : std::integral_constant<bool,
          arg_n_is_by_reference<N>::value
            and not arg_n_accepts_const_reference<N>::value
            and not arg_n_is_nonconst_rvalue_reference<N>::value
        >
    { };

    template <size_t N, typename U>
    struct arg_n_is_implicitly_convertible_from
      : _callable_traits_impl::is_callable_replace_arg_n<Callable,
          U, any_arg, N, 0, base_t::n_args_max
        >
    { };

    template <
      template <class...> class UnaryMetafunction,
      size_t N
    >
    struct arg_n_matches
      : _callable_traits_impl::arg_n_is<UnaryMetafunction, Callable, N>
    { };

    template <
      template <class...> class UnaryMetafunction
    >
    struct all_args_match
      : _callable_traits_impl::all_args_match_impl<
        UnaryMetafunction, Callable, 0, base_t::n_args_max
      >
    { };

};

} // end namespace meta

} // end namespace darma_runtime


#endif /* DARMA_IMPL_META_CALLABLE_TRAITS_H_ */
