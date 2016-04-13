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

#include <darma/impl/meta/detection.h>
#include <src/include/tinympl/always_true.hpp>

namespace darma_runtime {

namespace meta {

namespace _callable_traits_impl {

//struct any_arg { template <typename T> operator T() { } };

// Much credit is owed to help from:
//   http://stackoverflow.com/questions/36581303/counting-arguments-of-an-arbitrary-callable-with-the-c-detection-idiom
// for this solution

template <
  template <class...> class UnaryMetafunction
>
struct any_arg_conditional {
  template <typename T,
    typename = std::enable_if_t<
      not std::is_same<T, std::remove_reference_t<T>>::value
      and UnaryMetafunction<T>::value
    >
  >
  operator T();

  template <typename T,
    typename = std::enable_if_t<
      std::is_same<T, std::remove_reference_t<T>>::value
      and UnaryMetafunction<T>::value
    >
  >
  operator T&();

  template <typename T,
    typename = std::enable_if_t<
      std::is_same<T, std::remove_reference_t<T>>::value
      and UnaryMetafunction<T>::value
    >
  >
  operator T&&();
};

using any_arg = any_arg_conditional<tinympl::always_true>;

template <typename F, typename... Args>
using callable_archetype = decltype( std::declval<F>()(std::declval<Args>()...) );
template <typename F, typename... Args>
using is_callable_with_args = is_detected<callable_archetype, F, Args...>;

template <typename F, size_t I = 0,  typename... Args>
struct count_min_args
  // Double "::type" short-circuits recursion when possible
  : std::conditional_t<
    is_callable_with_args<F, Args...>::value,
    std::integral_constant<size_t, I>,
    count_min_args<F, I+1, Args..., any_arg>
    >::type
{ };

// Base case just returns max; error message elsewhere.
template <typename F, typename... Args>
struct count_min_args<F, DARMA_META_MAX_CALLABLE_ARGS, Args...>
  : std::integral_constant<size_t, DARMA_META_MAX_CALLABLE_ARGS+1> { };

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
    > { };

// Base case just returns max; error message elsewhere.
template <typename F, bool min_found, typename... Args>
struct count_max_args<F, DARMA_META_MAX_CALLABLE_ARGS, min_found, Args...>
  : std::integral_constant<size_t, DARMA_META_MAX_CALLABLE_ARGS+1> { };

template <
  template <class...> class UnaryMetafunction,
  typename F, size_t N, size_t I, size_t NTotal,
  typename... Args
>
struct arg_n_is_impl
  : std::conditional_t<
      N >= NTotal,
      std::false_type,
      arg_n_is_impl<
        UnaryMetafunction, F, N, I+1, NTotal, Args..., std::conditional_t<
          I == N,
          any_arg_conditional<UnaryMetafunction>,
          any_arg
        >
      >
    >
{ };

template <
  template <class...> class UnaryMetafunction,
  typename F, size_t N, size_t NTotal,
  typename... Args
>
struct arg_n_is_impl<
  UnaryMetafunction, F, N, NTotal, NTotal, Args...
> : is_callable_with_args<F, Args...>
{ };


template <
  template <class...> class UnaryMetafunction,
  typename F, size_t N
>
struct arg_n_is
  : arg_n_is_impl<UnaryMetafunction, F, N, 0, count_max_args<F>::value>
{ };

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
    "callable_traits<> used with callable having an invalid parameter (e.g., deleted"
      " copy constructor for a value parameter)"
      " (or, much less likely, you may need to increase DARMA_META_MAX_CALLABLE_ARGS, but "
      "this is very unlikely to be the problem)"
  );
};



} // end namespace _callable_traits_impl


// Note:: Not valid for lvalue references
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
