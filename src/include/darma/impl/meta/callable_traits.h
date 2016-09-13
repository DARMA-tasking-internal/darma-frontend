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
#include <tinympl/range_c.hpp>

#include <darma/impl/meta/detection.h>
#include <darma/impl/util.h>

#include "any_convertible.h"
#include "is_callable.h"

namespace darma_runtime {

namespace meta {

namespace _callable_traits_impl {

//==============================================================================
// <editor-fold desc="get_param_N">

template <size_t N, typename Callable>
struct get_param_N
  : get_param_N<N+1, decltype(&Callable::operator())> { };

// function version
template <size_t N, typename ReturnType, typename... Args>
struct get_param_N<N, ReturnType (Args...)>
  : get_param_N<N, ReturnType (*)(Args...)> { };

// pointer-to-function version
template <size_t N, typename ReturnType, typename... Args>
struct get_param_N<N, ReturnType (*)(Args...)>
  : tinympl::identity<typename tinympl::variadic::at_t<N, Args...>> { };

// reference-to-function version
template <size_t N, typename ReturnType, typename... Args>
struct get_param_N<N, ReturnType (&)(Args...)>
  : get_param_N<N, ReturnType (*)(Args...)> { };

// pointer-to-method callables
template <size_t N, typename ClassParamType, typename... Args>
struct _get_ptm_param_N
  : std::conditional<
      N == 0, tinympl::identity<ClassParamType>,
      // delayed evaluation...
      typename tinympl::lazy<tinympl::variadic::types_only::at>::template applied_to<
        std::integral_constant<size_t, N-1>,
        Args...
      >
    >::type
{ };

// remove cv qualifiers from pointer-to-method callables
template <size_t N, typename ClassType, typename ReturnType, typename... Args>
struct get_param_N<N, ReturnType (ClassType::*)(Args...)>
  : _get_ptm_param_N<N, ClassType*, Args...> { };

template <size_t N, typename ClassType, typename ReturnType, typename... Args>
struct get_param_N<N, ReturnType (ClassType::*)(Args...) const>
  : _get_ptm_param_N<N, ClassType const*, Args...> { };

template <size_t N, typename ClassType, typename ReturnType, typename... Args>
struct get_param_N<N, ReturnType (ClassType::*)(Args...) const volatile>
  : _get_ptm_param_N<N, ClassType const volatile*, Args...> { };

template <size_t N, typename ClassType, typename ReturnType, typename... Args>
struct get_param_N<N, ReturnType (ClassType::*)(Args...) volatile>
  : _get_ptm_param_N<N, ClassType volatile*, Args...> { };

// pointer-to-member callables
template <size_t N, typename ClassType, typename ReturnType>
struct get_param_N<N, ReturnType (ClassType::*)>
  : tinympl::identity<ClassType>
{
  static_assert(N == 0, "Can't get argument other than the first one of a"
    " pointer-to-data-member callable");
};

// </editor-fold> end get_param_N
//==============================================================================


//==============================================================================
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
//==============================================================================


//==============================================================================
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
//==============================================================================


//==============================================================================
// <editor-fold desc="call_operator_is_overloaded">

template <typename Functor>
using call_operator_exists = tinympl::bool_<
  count_min_args<Functor>::value != DARMA_META_MAX_CALLABLE_ARGS+1
>;

template <typename Functor>
using fails_for_overloaded_functor_call_archetype = decltype(&Functor::operator());

// anything else...
template <typename T>
struct is_non_functor_callable : std::false_type { };
// pointer-to-member function
template <typename T, typename U, typename... Args>
struct is_non_functor_callable<U (T::*)(Args...)> : std::true_type { };
template <typename T, typename U, typename... Args>
struct is_non_functor_callable<U (T::*)(Args...) const> : std::true_type { };
template <typename T, typename U, typename... Args>
struct is_non_functor_callable<U (T::*)(Args...) const volatile> : std::true_type { };
template <typename T, typename U, typename... Args>
struct is_non_functor_callable<U (T::*)(Args...) volatile> : std::true_type { };
// function pointer
template <typename T, typename... Args>
struct is_non_functor_callable<T (*)(Args...)> : std::true_type { };
// function reference
template <typename T, typename... Args>
struct is_non_functor_callable<T (&)(Args...)> : std::true_type { };
// "just a function"
template <typename T, typename... Args>
struct is_non_functor_callable<T (Args...)> : std::true_type { };
// Pointer-to-data-member
template <typename T, typename U>
struct is_non_functor_callable<U (T::*)> : std::true_type { };


template <typename Functor>
using call_operator_is_overloaded = tinympl::bool_<
  call_operator_exists<Functor>::value
  and (not is_detected<fails_for_overloaded_functor_call_archetype, Functor>::value)
  and (not is_non_functor_callable<Functor>::value)
>;

// </editor-fold>
//==============================================================================



//==============================================================================
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
//==============================================================================

//==============================================================================
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
//==============================================================================


//==============================================================================
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
//==============================================================================


//==============================================================================
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
//==============================================================================

} // end namespace _callable_traits_impl

//==============================================================================
// <editor-fold desc="callable_traits">

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

    static_assert(
      not _callable_traits_impl::call_operator_is_overloaded<Callable>::value,
      "Can't handle functors with overloaded call operators"
    );

    using _idx_seq = tinympl::as_sequence_t<
      typename tinympl::make_range_c<size_t, 0, base_t::n_args_max>::type
    >;

    template <typename wrapped_idx>
    using _get_param_n_wrapped = _callable_traits_impl::get_param_N<
      wrapped_idx::value, Callable
    >;

    using _params = typename tinympl::transform<
      _idx_seq, _get_param_n_wrapped, tinympl::vector
    >::type;



  public:

    template <size_t N>
    struct param_n
      : tinympl::at<N, _params>
    { };

    // Deprecated naming convention:
    template <size_t N>
    using arg_n = param_n<N>;

    // *_t aliases
    template <size_t N>
    using param_n_t = typename param_n<N>::type;
    template <size_t N>
    using arg_n_t = typename param_n<N>::type;

    //--------------------------------------------------------------------------

    template <size_t N,
      // convenience; should never be given as a template parameter
      typename _Param = tinympl::at_t<N, _params>
    >
    struct param_n_is_nonconst_rvalue_reference
      : tinympl::bool_<
          std::is_rvalue_reference<_Param>::value
          and not std::is_const<std::remove_reference_t<_Param>>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, _params>>::value,
        "Can't give value for convenience positional default template parameter"
      );
    };
    // Deprecated naming convention:
    template <size_t N>
    using arg_n_is_nonconst_rvalue_reference = param_n_is_nonconst_rvalue_reference<N>;

    //--------------------------------------------------------------------------

    template <size_t N,
      // convenience; should never be given as a template parameter
      typename _Param = tinympl::at_t<N, _params>
    >
    struct param_n_is_by_reference
      : tinympl::bool_<
          std::is_reference<_Param>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, _params>>::value,
        "Can't give value for convenience positional default template parameter"
      );
    };
    // Deprecated naming convention:
    template <size_t N>
    using arg_n_is_by_reference = param_n_is_by_reference<N>;

    //--------------------------------------------------------------------------

    template <size_t N,
      // convenience; should never be given as a template parameter
      typename _Param = tinympl::at_t<N, _params>
    >
    struct param_n_is_by_value
      : tinympl::bool_<
          not std::is_reference<_Param>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, _params>>::value,
        "Can't give value for convenience positional default template parameter"
      );
    };
    // Deprecated naming convention:
    template <size_t N>
    using arg_n_is_by_value = param_n_is_by_value<N>;

    //--------------------------------------------------------------------------

    template <size_t N,
      // convenience; should never be given as a template parameter
      typename _Param = tinympl::at_t<N, _params>
    >
    struct param_n_is_const_lvalue_reference
      : tinympl::bool_<
          std::is_lvalue_reference<_Param>::value
          and std::is_const<std::remove_reference_t<_Param>>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, _params>>::value,
        "Can't give value for convenience positional default template parameter"
      );
    };
    // Deprecated naming convention:
    template <size_t N>
    using arg_n_is_const_lvalue_reference = param_n_is_const_lvalue_reference<N>;

    //--------------------------------------------------------------------------

    template <size_t N,
      // convenience; should never be given as a template parameter
      typename _Param = tinympl::at_t<N, _params>
    >
    struct param_n_accepts_const_reference
      : tinympl::bool_<
          param_n_is_by_value<N>::value
            or param_n_is_const_lvalue_reference<N>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, _params>>::value,
        "Can't give value for convenience positional default template parameter"
      );
    };
    // Deprecated naming convention:
    template <size_t N>
    using arg_n_accepts_const_reference = param_n_accepts_const_reference<N>;

    //--------------------------------------------------------------------------

    template <size_t N,
      // convenience; should never be given as a template parameter
      typename _Param = tinympl::at_t<N, _params>
    >
    struct param_n_is_nonconst_lvalue_reference
      : tinympl::bool_<
          std::is_lvalue_reference<_Param>::value
            and not std::is_const<std::remove_reference_t<_Param>>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, _params>>::value,
        "Can't give value for convenience positional default template parameter"
      );
    };
    // Deprecated naming convention:
    template <size_t N>
    using arg_n_is_nonconst_lvalue_reference = param_n_is_nonconst_lvalue_reference<N>;

    //--------------------------------------------------------------------------

    template <size_t N, typename U,
      // convenience; should never be given as a template parameter
      typename _Param = tinympl::at_t<N, _params>
    >
    struct param_n_is_implicitly_convertible_from
      : tinympl::bool_<
          std::is_convertible<U, _Param>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, _params>>::value,
        "Can't give value for convenience positional default template parameter"
      );
    };
    // Deprecated naming convention:
    template <size_t N, typename U>
    using arg_n_is_implicitly_convertible_from =
      param_n_is_implicitly_convertible_from<N, U>;

    //--------------------------------------------------------------------------

    template <
      template <class...> class UnaryMetafunction,
      size_t N
    >
    struct param_n_matches
      : tinympl::and_<
          tinympl::bool_< N+1 <= base_t::n_args_max>,
          tinympl::extract_value_potentially_lazy<
            typename tinympl::lazy<UnaryMetafunction>::template instantiated_with<
              tinympl::lazy<tinympl::types_only::at>::template applied_to<
                std::integral_constant<size_t, N>,
                _params
              > // end lazy<at>
            > // end lazy<UnaryMetafunction>
          > // end extract_value_potentially_lazy
        > // end and_ (that short circuits if necessary)
    { };
    // Deprecated naming convention:
    template <template <class...> class UnaryMetafunction, size_t N>
    using arg_n_matches = param_n_matches<UnaryMetafunction, N>;

    //--------------------------------------------------------------------------

    template <
      template <class...> class UnaryMetafunction
    >
    struct all_params_match
      : tinympl::all_of<_params, UnaryMetafunction>
    { };
    // Deprecated naming convention:
    template < template <class...> class UnaryMetafunction >
    using all_args_match = all_params_match<UnaryMetafunction>;

};

// </editor-fold>
//==============================================================================

} // end namespace meta

} // end namespace darma_runtime


#endif /* DARMA_IMPL_META_CALLABLE_TRAITS_H_ */
