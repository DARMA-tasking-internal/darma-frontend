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
#include <tinympl/delay_all.hpp>

#include <darma/impl/meta/detection.h>
#include <darma/impl/util.h>
#include <darma/impl/util/static_assertions.h>

#include "darma/impl/meta/any_convertible.h"
#include "darma/impl/meta/is_callable.h"

namespace darma_runtime {

namespace meta {

//==============================================================================
// <editor-fold desc="get_params">


// The default version just gets the parameters to the Callable::operator()
// method (meaning that we can assume it's a functor-like callable, since
// all of the other cases are handled in specializations), but we have to
// pop off the first parameter since the std::invoke (C++17) numbering for
// instance method callables assumes that the first parameter is an instance
// of the class itself (or a pointer to it, or a base class, or a
// std::reference_wrapper, but we'll ignore these for our purposes).  For more
// details, see http://en.cppreference.com/w/cpp/utility/functional/invoke
template <typename Callable>
struct get_params
  : get_params<decltype(&Callable::operator())>::type::pop_front { };

// function version
template <typename ReturnType, typename... Args>
struct get_params<ReturnType (Args...)>
  : get_params<ReturnType (*)(Args...)> { };

// pointer-to-function version
template <typename ReturnType, typename... Args>
struct get_params<ReturnType (*)(Args...)>
  : tinympl::identity<tinympl::vector<Args...>> { };

// reference-to-function version
template <typename ReturnType, typename... Args>
struct get_params<ReturnType (&)(Args...)>
  : get_params<ReturnType (*)(Args...)> { };

// Instance member functions have class type as their first argument, according
// to std::invoke (C++17) parameter ordering
template <typename ClassType, typename ReturnType, typename... Args>
struct get_params<ReturnType (ClassType::*)(Args...)>
  : tinympl::identity<tinympl::vector<ClassType, Args...>> { };

// versions that remove cv qualifiers from pointer-to-method callables
// all of them just forward to the unqualified version above
template <typename ClassType, typename ReturnType, typename... Args>
struct get_params<ReturnType (ClassType::*)(Args...) const>
  : get_params<ReturnType (ClassType::*)(Args...)> { };
template <typename ClassType, typename ReturnType, typename... Args>
struct get_params<ReturnType (ClassType::*)(Args...) const volatile>
  : get_params<ReturnType (ClassType::*)(Args...)> { };
template <typename ClassType, typename ReturnType, typename... Args>
struct get_params<ReturnType (ClassType::*)(Args...) volatile>
  : get_params<ReturnType (ClassType::*)(Args...)> { };

// pointer-to-member callable
template <typename ClassType, typename ReturnType>
struct get_params<ReturnType (ClassType::*)>
  : tinympl::identity<tinympl::vector<ClassType>> { };

// a more verbose alias
template <typename Callable>
using get_parameters = get_params<Callable>;

// the *_t versions of the above
template <typename Callable>
using get_params_t = typename get_params<Callable>::type;
template <typename Callable>
using get_parameters_t = typename get_params<Callable>::type;

// </editor-fold> end get_params
//==============================================================================


//==============================================================================
// <editor-fold desc="get_param_N">

template <size_t N, typename Callable>
struct get_param_N {
  private:
    using _params = get_params_t<Callable>;
  public:
    static_assert(N < _params::size, "parameter index out of range");
    using type = tinympl::at_t<N, _params>;
};

// a more verbose alias
template <size_t N, typename Callable>
using get_parameter_N = get_param_N<N, Callable>;

// the *_t versions of the above
template <size_t N, typename Callable>
using get_param_N_t = typename get_param_N<N, Callable>::type;
template <size_t N, typename Callable>
using get_parameter_N_t = typename get_param_N<N, Callable>::type;

// An SFINAE-friendly version, without the static_assert
template <size_t N, typename Callable, typename Enable=void>
struct get_param_N_SFINAE { }; // substitution failure case

// substitution success case:
template <size_t N, typename Callable>
struct get_param_N_SFINAE<N, Callable,
  std::enable_if_t<N+1 <= get_params_t<Callable>::size>
> : get_param_N<N, Callable> { };

// </editor-fold> end get_param_N
//==============================================================================

namespace _callable_traits_impl {

//==============================================================================
// <editor-fold desc="count_min_args">

template <typename F, size_t MaxToTryPlus1 = DARMA_META_MAX_CALLABLE_ARGS,
  // These should never be non-defaulted in calling code (only in recursion
  // from the implementation here)
  size_t I = 0, typename... Args
>
struct count_min_args
  // Double "::type" short-circuits recursion when possible
  : std::conditional_t<
      is_callable_with_args<F, Args...>::value,
      std::integral_constant<size_t, I>,
      count_min_args<F, MaxToTryPlus1, I+1, Args..., any_arg>
    >::type
{ };

template <typename F, size_t MaxToTryPlus1, typename... Args>
struct count_min_args<F, MaxToTryPlus1, MaxToTryPlus1, Args...>
  : std::integral_constant<size_t, MaxToTryPlus1+1> { };

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

} // end namespace _callable_traits_impl

//==============================================================================
// <editor-fold desc="callable_traits">

template <typename Callable>
struct callable_traits {
  public:

    using callable_t = Callable;
    using params_vector = get_params_t<Callable>;
    using args_vector = params_vector;

    static constexpr auto n_params_max = params_vector::size;
    static constexpr auto n_params_min = params_vector::size; // for now

    // Deprecated naming convention:
    static constexpr auto n_args_min = n_params_min;
    static constexpr auto n_args_max = n_params_max;

    //--------------------------------------------------------------------------

    template <size_t N>
    struct param_n
      : tinympl::at<N, params_vector>
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
      typename _Param = tinympl::at_t<N, params_vector>
    >
    struct param_n_is_nonconst_rvalue_reference
      : tinympl::bool_<
          std::is_rvalue_reference<_Param>::value
          and not std::is_const<std::remove_reference_t<_Param>>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, params_vector>>::value,
        "Can't give value for convenience positional default template parameter"
      );
    };
    // Deprecated naming convention:
    template <size_t N>
    using arg_n_is_nonconst_rvalue_reference = param_n_is_nonconst_rvalue_reference<N>;

    //--------------------------------------------------------------------------

    template <size_t N,
      // convenience; should never be given as a template parameter
      typename _Param = tinympl::at_t<N, params_vector>
    >
    struct param_n_is_by_reference
      : tinympl::bool_<
          std::is_reference<_Param>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, params_vector>>::value,
        "Can't give value for convenience positional default template parameter"
      );
    };
    // Deprecated naming convention:
    template <size_t N>
    using arg_n_is_by_reference = param_n_is_by_reference<N>;

    //--------------------------------------------------------------------------

    template <size_t N,
      // convenience; should never be given as a template parameter
      typename _Param = tinympl::at_t<N, params_vector>
    >
    struct param_n_is_by_value
      : tinympl::bool_<
          not std::is_reference<_Param>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, params_vector>>::value,
        "Can't give value for convenience positional default template parameter"
      );
    };
    // Deprecated naming convention:
    template <size_t N>
    using arg_n_is_by_value = param_n_is_by_value<N>;

    //--------------------------------------------------------------------------

    template <size_t N,
      // convenience; should never be given as a template parameter
      typename _Param = tinympl::at_t<N, params_vector>
    >
    struct param_n_is_const_lvalue_reference
      : tinympl::bool_<
          std::is_lvalue_reference<_Param>::value
          and std::is_const<std::remove_reference_t<_Param>>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, params_vector>>::value,
        "Can't give value for convenience positional default template parameter"
      );
    };
    // Deprecated naming convention:
    template <size_t N>
    using arg_n_is_const_lvalue_reference = param_n_is_const_lvalue_reference<N>;

    //--------------------------------------------------------------------------

    template <size_t N,
      // convenience; should never be given as a template parameter
      typename _Param = tinympl::at_t<N, params_vector>
    >
    struct param_n_accepts_const_reference
      : tinympl::bool_<
          param_n_is_by_value<N>::value
            or param_n_is_const_lvalue_reference<N>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, params_vector>>::value,
        "Can't give value for convenience positional default template parameter"
      );
    };
    // Deprecated naming convention:
    template <size_t N>
    using arg_n_accepts_const_reference = param_n_accepts_const_reference<N>;

    //--------------------------------------------------------------------------

    template <size_t N,
      // convenience; should never be given as a template parameter
      typename _Param = tinympl::at_t<N, params_vector>
    >
    struct param_n_is_nonconst_lvalue_reference
      : tinympl::bool_<
          std::is_lvalue_reference<_Param>::value
            and not std::is_const<std::remove_reference_t<_Param>>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, params_vector>>::value,
        "Can't give value for convenience positional default template parameter"
      );
    };
    // Deprecated naming convention:
    template <size_t N>
    using arg_n_is_nonconst_lvalue_reference = param_n_is_nonconst_lvalue_reference<N>;

    //--------------------------------------------------------------------------

    template <size_t N, typename U,
      // convenience; should never be given as a template parameter
      typename _Param = tinympl::at_t<N, params_vector>
    >
    struct param_n_is_implicitly_convertible_from
      : tinympl::bool_<
          std::is_convertible<U, _Param>::value
        >
    {
      static_assert(std::is_same<_Param, tinympl::at_t<N, params_vector>>::value,
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
          tinympl::bool_< N+1 <= n_params_max>,
          // Lazily evaluate the UnaryMetafunction, in case it's not
          // SFINAE-friendly
          tinympl::extract_value_potentially_lazy<
            // TODO make this part SFINAE friendly also (i.e., evaluation of UnaryMetafunction itself)?!?
            typename tinympl::lazy<UnaryMetafunction>::template instantiated_with<
              // Evaluate the at with a default in case it's not valid (checked
              // by the first condition
              tinympl::at_or_t<meta::nonesuch, N, params_vector>
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
    struct allparams_vector_match
      : tinympl::all_of<params_vector, UnaryMetafunction>
    { };
    // Deprecated naming convention:
    template < template <class...> class UnaryMetafunction >
    using all_args_match = allparams_vector_match<UnaryMetafunction>;

    //--------------------------------------------------------------------------

    template <size_t N>
    struct param_n_traits {
      static constexpr auto is_by_reference = param_n_is_by_reference<N>::value;
      static constexpr auto is_by_value = param_n_is_by_value<N>::value;
      static constexpr auto is_const_lvalue_reference = param_n_is_const_lvalue_reference<N>::value;
      static constexpr auto is_nonconst_lvalue_reference = param_n_is_nonconst_lvalue_reference<N>::value;
      static constexpr auto is_nonconst_rvalue_reference = param_n_is_nonconst_rvalue_reference<N>::value;

      template <template <class...> class UnaryMetafunction>
      struct matches {
        static constexpr auto value = param_n_matches<UnaryMetafunction, N>::value;
        using type = std::integral_constant<bool, value>;
      };

      using type = param_n_t<N>;

      using callable = callable_t;
    };

};

// </editor-fold>
//==============================================================================

} // end namespace meta

} // end namespace darma_runtime


#endif /* DARMA_IMPL_META_CALLABLE_TRAITS_H_ */
