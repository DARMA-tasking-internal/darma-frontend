/*
//@HEADER
// ************************************************************************
//
//                      functor_traits.h
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

#ifndef DARMA_IMPL_FUNCTOR_TRAITS_H
#define DARMA_IMPL_FUNCTOR_TRAITS_H

#include <cstdint>
#include <type_traits>

#include <darma/impl/meta/callable_traits.h>
#include "task.h"

namespace darma_runtime {

namespace detail {

////////////////////////////////////////////////////////////////////////////////

// <editor-fold desc="Functor traits">

namespace _functor_traits_impl {

template <typename T>
using decayed_is_access_handle = is_access_handle<std::decay_t<T>>;

//template <typename T>
//using _compile_time_modifiable_archetype = std::integral_constant<bool, T::is_compile_time_modifiable>;
//template <typename T>
//using decayed_is_compile_time_modifiable = meta::detected_or_t<std::false_type,
//  _compile_time_modifiable_archetype, std::decay_t<T>
//>;
//template <typename T>
//using _compile_time_readable_archetype = std::integral_constant<bool, T::is_compile_time_readable>;
//template <typename T>
//using decayed_is_compile_time_readable = meta::detected_or_t<std::false_type,
//  _compile_time_readable_archetype, std::decay_t<T>
//>;

template <typename T>
using _compile_time_immediate_readable = std::integral_constant<bool, T::is_compile_time_immediate_readable>;
template <typename T>
using _compile_time_immediate_modifiable = std::integral_constant<bool, T::is_compile_time_immediate_modifiable>;
template <typename T>
using _compile_time_schedule_readable = std::integral_constant<bool, T::is_compile_time_schedule_readable>;
template <typename T>
using _compile_time_schedule_modifiable = std::integral_constant<bool, T::is_compile_time_schedule_modifiable>;

template <typename T>
using decayed_is_compile_time_immediate_readable = typename meta::detected_or_t<std::true_type,
  _compile_time_immediate_readable, std::decay_t<T>
>::type;
template <typename T>
using decayed_is_compile_time_immediate_modifiable = typename meta::detected_or_t<std::true_type,
  _compile_time_immediate_modifiable, std::decay_t<T>
>::type;
template <typename T>
using decayed_is_compile_time_schedule_readable = typename meta::detected_or_t<std::true_type,
  _compile_time_schedule_readable, std::decay_t<T>
>::type;
template <typename T>
using decayed_is_compile_time_schedule_modifiable = typename meta::detected_or_t<std::true_type,
  _compile_time_schedule_modifiable, std::decay_t<T>
>::type;


template <typename T>
using _compile_time_read_access_analog_archetype = typename T::CompileTimeReadAccessAnalog;
template <typename T>
using _value_type_archetype = typename T::value_type;
template <typename T>
using value_type_if_defined_t = typename meta::is_detected<_value_type_archetype, T>::type;

} // end namespace _functor_traits_impl

// formal arg introspection oesn't work for rvalue references or for deduced types
template <typename Functor>
struct functor_traits {

  private:
    using traits = meta::callable_traits<Functor>;

  public:

    static constexpr auto n_args_min = meta::callable_traits<Functor>::n_args_min;
    static constexpr auto n_args_max = meta::callable_traits<Functor>::n_args_max;

    template <size_t N>
    struct formal_arg_traits {
      static constexpr auto
      is_access_handle = traits::template arg_n_matches<
        _functor_traits_impl::decayed_is_access_handle, N
      >::value;

      static constexpr auto
      is_compile_time_immediate_modifiable_handle = tinympl::logical_and<
        typename traits::template arg_n_matches<
          _functor_traits_impl::decayed_is_compile_time_immediate_modifiable, N
        >,
        std::integral_constant<bool, is_access_handle>
      >::value;

      static constexpr auto
      is_compile_time_immediate_read_only_handle = tinympl::logical_and<
        typename traits::template arg_n_matches<
          _functor_traits_impl::decayed_is_compile_time_immediate_readable, N
        >,
        std::integral_constant<bool, not is_compile_time_immediate_modifiable_handle>,
        std::integral_constant<bool, is_access_handle>
      >::value;

      static constexpr auto is_by_value = traits::template arg_n_is_by_value<N>::value;
      static constexpr auto is_by_reference = traits::template arg_n_is_by_reference<N>::value;
      static constexpr auto is_const_lvalue_reference = traits::template arg_n_is_const_lvalue_reference<N>::value;
      static constexpr auto is_nonconst_lvalue_reference = traits::template arg_n_is_nonconst_lvalue_reference<N>::value;

      template <typename T>
      using is_impliticly_convertible_from = typename traits::template arg_n_is_implicitly_convertible_from<N, T>;

    };

};

// TODO strictness specifiers
template <
  typename Functor,
  typename... CallArgs
>
struct functor_call_traits {

  public:

    template <size_t N>
    struct call_arg_traits {

      private:
        using CallArg = tinympl::variadic::at_t<N, CallArgs...>;

        using access_handle_value_type = _functor_traits_impl::value_type_if_defined_t<std::decay_t<CallArg>>;

      public:

        static constexpr auto is_access_handle =
          darma_runtime::detail::is_access_handle<std::decay_t<CallArg>>::value;

        using formal_traits = typename functor_traits<Functor>::template formal_arg_traits<N>;

        // Did the caller give a handle and the functor give an argument that was a const (reference to)
        // a type that is convertible to that handle's value_type?
        static constexpr bool is_const_conversion_capture =
          // Only if the call argument is an access handle
          is_access_handle
          // and not if the formal argument is an AccessHandle
          and (not formal_traits::is_access_handle)
          // If so, the value type has to be convertible
          and formal_traits::template is_impliticly_convertible_from<access_handle_value_type>::value
          // also, the corresponding formal argument has to be a const_lvalue_reference or passed by value
          and (formal_traits::is_const_lvalue_reference or formal_traits::is_by_value)
        ; // end is_const_conversion_capture

        static constexpr bool is_nonconst_conversion_capture =
          // Only if the call argument is an access handle
          is_access_handle
          // only if the formal argument isn't an AccessHandle
          and (not formal_traits::is_access_handle)
          // If so, the value type has to be convertible
          and formal_traits::template is_impliticly_convertible_from<access_handle_value_type>::value
          // and the formal argument must be a non-const lvalue reference
          and formal_traits::is_nonconst_lvalue_reference
        ; // end is_nonconst_conversion_capture

        static constexpr bool is_read_only_capture =
          // If we're converting from an AccessHandle<T> to a const T& or T formal argument, then it's a read capture
          is_const_conversion_capture
          // or if this argument is an access handle and the formal argument is compile-time read only
          or (is_access_handle and formal_traits::is_compile_time_immediate_read_only_handle)
        ;

        static constexpr bool is_conversion_capture = is_const_conversion_capture or is_nonconst_conversion_capture;

        typedef std::conditional<
          is_read_only_capture,
          meta::detected_t<
            _functor_traits_impl::_compile_time_read_access_analog_archetype,
            std::decay_t<CallArg>
          > const,
          std::remove_cv_t<std::remove_reference_t<CallArg>> const
        > args_tuple_entry;

        template <typename T>
        static std::enable_if_t<
          not _functor_traits_impl::decayed_is_access_handle<T>::value
            or not is_conversion_capture,
          T
        >
        get_converted_arg(T&& val) {
          return val;
        }

        template <typename T>
        static std::enable_if_t<
          _functor_traits_impl::decayed_is_access_handle<std::decay_t<T>>::value
            and is_const_conversion_capture,
          typename std::decay_t<T>::value_type const&
        >
        get_converted_arg(T&& val) {
          return val.get_value();
        }

        template <typename T>
        static std::enable_if_t<
          _functor_traits_impl::decayed_is_access_handle<std::decay_t<T>>::value
            and is_nonconst_conversion_capture,
          typename std::decay_t<T>::value_type&
        >
        get_converted_arg(T&& val) {
          return val.get_reference();
        }

    };

    template <typename IndexWrapped>
    struct call_arg_traits_types_only {
      using type = call_arg_traits<IndexWrapped::value>;
    };

  private:

    //template <typename IndexWrapped>
    //using _call_arg_traits = call_arg_traits<IndexWrapped::value>;

    template <typename IndexWrapped>
    using _arg_traits_tuple_entry = typename call_arg_traits<IndexWrapped::value>::args_tuple_entry;

  public:

    using args_tuple_t = typename tinympl::transform<
      tinympl::as_sequence_t<typename tinympl::range_c<size_t, 0, sizeof...(CallArgs)>::type>,
      _arg_traits_tuple_entry,
      std::tuple
    >::type;


};

// </editor-fold>

////////////////////////////////////////////////////////////////////////////////

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_FUNCTOR_TRAITS_H
