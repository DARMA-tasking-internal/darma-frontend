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
#include <tinympl/select.hpp>
#include "task.h"
#include "handle.h"

namespace darma_runtime {

namespace detail {

////////////////////////////////////////////////////////////////////////////////

// <editor-fold desc="FunctorWrapper traits">

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

//int _compile_time_read_access_analog_archetype;
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
      using is_implicitly_convertible_from =
        typename traits::template arg_n_is_implicitly_convertible_from<N, T>;

    };

};

// TODO user supplied strictness specifier typedefs
template <
  typename Functor,
  typename... CallArgs
>
struct functor_call_traits {

  public:

    template <size_t N>
    struct call_arg_traits {

        using CallArg = tinympl::variadic::at_t<N, CallArgs...>;
      private:

        using access_handle_value_type = _functor_traits_impl::value_type_if_defined_t<std::decay_t<CallArg>>;

      public:

        static constexpr auto is_access_handle =
          darma_runtime::detail::is_access_handle<std::decay_t<CallArg>>::value;

        using formal_traits = typename functor_traits<Functor>::template formal_arg_traits<N>;

        // Did the caller give a handle and the functor give an argument that was a const (reference to)
        // a type that is convertible to that handle's value_type?
        // Indicates AccessHandle<T> => T const& or T
        static constexpr bool is_const_conversion_capture =
          // Only if the call argument is an access handle
          is_access_handle
          // and not if the formal argument is an AccessHandle
          and (not formal_traits::is_access_handle)
          // If so, the value type has to be convertible
          and formal_traits::template is_implicitly_convertible_from<access_handle_value_type const&>::value
          // also, the corresponding formal argument has to be a const_lvalue_reference or passed by value
          and (formal_traits::is_const_lvalue_reference or formal_traits::is_by_value)
        ; // end is_const_conversion_capture

        // Indicates AccessHandle<T> => T&
        static constexpr bool is_nonconst_conversion_capture =
          // Only if the call argument is an access handle
          is_access_handle
          // only if the formal argument isn't an AccessHandle
          and (not formal_traits::is_access_handle)
          // If so, the value type has to be convertible
          and formal_traits::template is_implicitly_convertible_from<access_handle_value_type&>::value
          // and the formal argument must be a non-const lvalue reference
          and formal_traits::is_nonconst_lvalue_reference
        ; // end is_nonconst_conversion_capture

        // TODO handle static scheduling permissions in formal args as well

        // Indicates AccessHandle<T> => ReadOnlyAccessHandle<T>
        static constexpr bool is_read_only_handle_capture =
          (is_access_handle and formal_traits::is_compile_time_immediate_read_only_handle);

        //static constexpr bool is_read_only_capture =
        //  // If we're converting from an AccessHandle<T> to a const T& or T formal argument, then it's a read capture
        //  is_const_conversion_capture
        //  // or if this argument is an access handle and the formal argument is compile-time read only
        //  or (is_access_handle and formal_traits::is_compile_time_immediate_read_only_handle)
        //;

        static constexpr bool is_conversion_capture = is_const_conversion_capture or is_nonconst_conversion_capture;

        static_assert(not is_conversion_capture or (
          is_const_conversion_capture xor is_nonconst_conversion_capture
        ), "internal error: Can't be both const conversion capture and nonconst conversion capture");

        // Was the calling argument a T&&?  (excluding AccessHandle<T>&&)
        static constexpr bool is_nonhandle_rvalue_reference =
          std::is_rvalue_reference<CallArg>::value and not is_access_handle;

        // is the formal argument a T rather than a T& or T const&?  If so, we can std::move into it
        // from the args tuple
        static constexpr bool formal_arg_accepts_move = formal_traits::is_by_value;

        // Specifically disallow AccessHandle<T>&& => * case for now
        static_assert(
          not (std::is_rvalue_reference<CallArg>::value and is_access_handle),
          "Calling functors with AccessHandle<T> rvalues is not yet supported"
        );

      private:
        // If it's a const conversion capture, make sure it's a leaf and has maximum
        // immediate permissions of read-only
        template <typename AccessHandleT>
        using _const_conversion_capture_arg_tuple_entry_archetype =
          typename std::decay_t<AccessHandleT>::template with_traits<
            typename std::decay_t<AccessHandleT>::traits
              ::template with_max_immediate_permissions<detail::AccessHandlePermissions::Read>::type
              ::template with_max_schedule_permissions<detail::AccessHandlePermissions::None>::type
              // Also set the min schedule and immediate permissions, in case they were higher before
              ::template with_min_immediate_permissions<detail::AccessHandlePermissions::Read>::type
              ::template with_min_schedule_permissions<detail::AccessHandlePermissions::None>::type
          >;

        using _const_conversion_capture_arg_tuple_entry = meta::detected_t<
          _const_conversion_capture_arg_tuple_entry_archetype, CallArg
        >;
        static_assert(
          not is_const_conversion_capture or not
            std::is_same<_const_conversion_capture_arg_tuple_entry, meta::nonesuch>::value,
          "internal error: const conversion capture detected, but arg_tuple type not detected correctly"
        );

        // If it's a nonconst conversion capture, make sure it's a leaf and has minimum
        // immediate permissions of modify
        template <typename AccessHandleT>
        using _nonconst_conversion_capture_arg_tuple_entry_archetype =
          typename std::decay_t<AccessHandleT>::template with_traits<
            typename std::decay_t<AccessHandleT>::traits
              ::template with_min_immediate_permissions<detail::AccessHandlePermissions::Modify>::type
              ::template with_max_schedule_permissions<detail::AccessHandlePermissions::None>::type
              // Also set the min schedule permissions, in case they were higher before
              ::template with_min_schedule_permissions<detail::AccessHandlePermissions::None>::type
              // but the max immediate permissions should stay the same.  If they are given and less
              // than Modify, we should get a compile-time error
          >;

        using _nonconst_conversion_capture_arg_tuple_entry = meta::detected_t<
          _nonconst_conversion_capture_arg_tuple_entry_archetype, CallArg
        >;
        static_assert(
          (not is_nonconst_conversion_capture)
            or (not std::is_same<_nonconst_conversion_capture_arg_tuple_entry, meta::nonesuch>::value),
          "internal error: non-const conversion capture detected, but arg_tuple type not detected correctly"
        );

        // Ignore scheduling permissions for now
        template <typename AccessHandleT>
        using _read_only_handle_capture_arg_tuple_entry_archetype =
          typename std::decay_t<AccessHandleT>::template with_traits<
            typename std::decay_t<AccessHandleT>::traits
              ::template with_max_immediate_permissions<detail::AccessHandlePermissions::Read>::type
              // Also set the min schedule permissions, in case they were higher before
              ::template with_min_immediate_permissions<detail::AccessHandlePermissions::Read>::type
              ::template with_min_schedule_permissions<detail::AccessHandlePermissions::Read>::type
          >;

        using _read_only_handle_capture_arg_tuple_entry = meta::detected_t<
          _read_only_handle_capture_arg_tuple_entry_archetype, CallArg
        >;

      public:

        // disallow the implicit {T&, T const&} => {T&, T const&} cases (all 4)
        static_assert(
          is_access_handle or
            (formal_traits::is_by_value or not std::is_lvalue_reference<CallArg>::value),
          "implicit copy of lvalue not allowed.  Either use darma_copy(), wrap the variable in"
            " an AccessHandle<T>, or change the functor-style task to take the argument by value"
        );

        // disallow the T&& => T& case
        static_assert(
          is_access_handle or
            not (is_nonhandle_rvalue_reference and formal_traits::is_nonconst_lvalue_reference),
          "cannot bind rvalue to non-const lvalue reference in functor-style task invocation"
        );

        // disallow {T&, T const&, T&&} => {AccessHandle<T>, AccessHandle<U>} cases, at least for now
        static_assert(
          is_access_handle or not formal_traits::is_access_handle,
          "implicit conversion of non-AccessHandle<T> argument to AccessHandle<T> is not yet supported."
            "  You must create an AccessHandle<T> and set its value to the argument before invoking this"
            " functor-style task."
        );

        // TODO disallow unconvertible with helpful error?

        static_assert(
          not (is_nonconst_conversion_capture and is_const_conversion_capture),
          "internal error deducing type conversion for functor-style task call"
        );

        using args_tuple_entry = tinympl::select_first_t<
          //------------------------------------------------------------
          // For the AccessHandle<T> => T or T const& cases:
          std::integral_constant<bool, is_const_conversion_capture>,
            /* => */ _const_conversion_capture_arg_tuple_entry,
          //------------------------------------------------------------
          // For the AccessHandle<T> => T& case:
          std::integral_constant<bool, is_nonconst_conversion_capture>,
            /* => */ _nonconst_conversion_capture_arg_tuple_entry,
          //------------------------------------------------------------
          // For the AccessHandle<T> => ReadAccessHandle<T>& case:
          std::integral_constant<bool, is_read_only_handle_capture>,
            /* => */ _read_only_handle_capture_arg_tuple_entry,
          //------------------------------------------------------------
          // Something is wrong with my logic for this case, so we'll leave it out for now
          // (only results in one extra copy)
          //// If the formal argument is by value, make it non-const since we're
          //// going to move into it from the args tuple (also check for not handle to be safe)
          //std::integral_constant<bool, formal_arg_accepts_move and not is_access_handle>,
          //  /* => */ std::remove_cv_t<std::remove_reference_t<CallArg>>,
          //------------------------------------------------------------
          // For other cases, require non-reference
          std::true_type,
            /* => */ //std::remove_reference_t<CallArg>
            /* => */ std::decay_t<CallArg>
        >;

        // Normal case, just pass through
        template <typename T>
        static std::enable_if_t<
          not _functor_traits_impl::decayed_is_access_handle<T>::value
          or (
            // Some AccessHandle cases need to be handled here
            _functor_traits_impl::decayed_is_access_handle<T>::value
            and (
              // these are handled by the conversion capture versions below
              not is_conversion_capture
              // This case needs to be handled by moving in to the argument, below
              and not formal_traits::is_by_value
            )
          )
          , T
        >
        get_converted_arg(T&& val) {
          return val;
        }

        // Something is wrong with my logic for this case, so we'll leave it out for now
        // (only results in one extra copy)
        //// if it's non-const, we did that because we want to move out of it
        //// and into the formal argument
        //template <typename T>
        //static std::enable_if_t<
        //  (not _functor_traits_impl::decayed_is_access_handle<T>::value
        //    or not is_conversion_capture)
        //  and formal_arg_accepts_move
        //  , std::remove_reference_t<T>
        //>
        //get_converted_arg(T&& val) {
        //  // Yes, I know it looks like I'm moving out of an lvalue reference, but
        //  // I'm basically just pusing the expiration of the lvalue inside this function,
        //  // since its a mess to try and expire some elements of a tuple and not others
        //  return std::move(val);
        //}

        template <typename T>
        static std::enable_if_t<
          _functor_traits_impl::decayed_is_access_handle<T>::value
            and formal_traits::is_by_value
            and formal_traits::is_access_handle,
          typename std::remove_reference_t<T>&
        >
        get_converted_arg(T&& val) {
          // Note that this should work this way even if T&& is an lvalue reference!
          // TODO turn the move on/off from outside for different use in the normal case/while case
          return val; //std::move(val);
        }

        template <typename T>
        static std::enable_if_t<
          _functor_traits_impl::decayed_is_access_handle<T>::value
            and is_const_conversion_capture,
          typename std::decay_t<T>::value_type const&
        >
        get_converted_arg(T&& val) {
          return val.get_value();
        }

        template <typename T>
        static std::enable_if_t<
          _functor_traits_impl::decayed_is_access_handle<T>::value
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

    template <typename IndexWrapped>
    using _arg_traits_tuple_entry_wrapped = tinympl::identity<
      typename call_arg_traits<IndexWrapped::value>::args_tuple_entry
    >;

  public:

    using args_tuple_t = typename tinympl::transform<
      tinympl::as_sequence_t<typename tinympl::make_range_c<size_t, 0, sizeof...(CallArgs)>::type>,
      _arg_traits_tuple_entry_wrapped,
      std::tuple
    >::type;


};

// </editor-fold>

////////////////////////////////////////////////////////////////////////////////

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_FUNCTOR_TRAITS_H
