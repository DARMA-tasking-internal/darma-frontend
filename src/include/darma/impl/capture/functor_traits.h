/*
//@HEADER
// ************************************************************************
//
//                      functor_traits.h
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMA_IMPL_FUNCTOR_TRAITS_H
#define DARMA_IMPL_FUNCTOR_TRAITS_H

#include <cstdint>
#include <type_traits>

#include <darma/impl/capture/callable_traits.h>
#include <tinympl/select.hpp>
#include <darma/impl/task/task_base.h>
#include <darma/impl/async_accessible/async_access_traits.h>
#include "darma/impl/task/task.h"
#include "darma/impl/handle.h"

namespace darma_runtime {

namespace detail {

////////////////////////////////////////////////////////////////////////////////

// <editor-fold desc="FunctorWrapper traits">

namespace _functor_traits_impl {

template <typename T>
using decayed_is_access_handle = is_access_handle<std::decay_t<T>>;

template <typename T>
using _compile_time_immediate_readable = std::integral_constant<bool, T::is_compile_time_immediate_readable>;

template <typename T>
using _compile_time_immediate_modifiable = std::integral_constant<bool, T::is_compile_time_immediate_modifiable>;

template <typename T>
using _compile_time_scheduling_readable = std::integral_constant<bool, T::is_compile_time_scheduling_readable>;

template <typename T>
using _compile_time_scheduling_modifiable = std::integral_constant<bool, T::is_compile_time_scheduling_modifiable>;

template <typename T>
using decayed_is_compile_time_immediate_readable = typename tinympl::detected_or_t<std::true_type,
  _compile_time_immediate_readable, std::decay_t<T>
>::type;

template <typename T>
using decayed_is_compile_time_immediate_modifiable = typename tinympl::detected_or_t<std::true_type,
  _compile_time_immediate_modifiable, std::decay_t<T>
>::type;

template <typename T>
using decayed_is_compile_time_scheduling_readable = typename tinympl::detected_or_t<std::true_type,
  _compile_time_scheduling_readable, std::decay_t<T>
>::type;

template <typename T>
using decayed_is_compile_time_scheduling_modifiable = typename tinympl::detected_or_t<std::true_type,
  _compile_time_scheduling_modifiable, std::decay_t<T>
>::type;


template <typename T>
using _compile_time_read_access_analog_archetype = typename T::CompileTimeReadAccessAnalog;

template <typename T>
using _value_type_archetype = typename T::value_type;

template <typename T>
using value_type_if_defined_t = typename meta::is_detected<_value_type_archetype, T>::type;

//int _compile_time_read_access_analog_archetype;
} // end namespace _functor_traits_impl

// formal arg introspection doesn't work for rvalue references or for deduced types
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

      using param_t = typename traits::template param_n_t<N>;

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
              ::template with_static_immediate_permissions<detail::AccessHandlePermissions::Read>::type
              ::template with_static_scheduling_permissions<detail::AccessHandlePermissions::None>::type
              // Also set the min schedule and immediate permissions, in case they were higher before
              ::template with_required_immediate_permissions<detail::AccessHandlePermissions::Read>::type
              ::template with_required_scheduling_permissions<detail::AccessHandlePermissions::None>::type
          >;

        using _const_conversion_capture_arg_tuple_entry = tinympl::detected_t<
          _const_conversion_capture_arg_tuple_entry_archetype, CallArg
        >;
        static_assert(
          not is_const_conversion_capture or not
            std::is_same<_const_conversion_capture_arg_tuple_entry, tinympl::nonesuch>::value,
          "internal error: const conversion capture detected, but arg_tuple type not detected correctly"
        );

        // If it's a nonconst conversion capture, make sure it's a leaf and has minimum
        // immediate permissions of modify
        template <typename AccessHandleT>
        using _nonconst_conversion_capture_arg_tuple_entry_archetype =
          typename std::decay_t<AccessHandleT>::template with_traits<
            typename std::decay_t<AccessHandleT>::traits
              ::template with_required_immediate_permissions<detail::AccessHandlePermissions::Modify>::type
              ::template with_static_scheduling_permissions<detail::AccessHandlePermissions::None>::type
              // Also set the min schedule permissions, in case they were higher before
              ::template with_required_scheduling_permissions<detail::AccessHandlePermissions::None>::type
              // but the max immediate permissions should stay the same.  If they are given and less
              // than Modify, we should get a compile-time error
          >;

        using _nonconst_conversion_capture_arg_tuple_entry = tinympl::detected_t<
          _nonconst_conversion_capture_arg_tuple_entry_archetype, CallArg
        >;
        static_assert(
          (not is_nonconst_conversion_capture)
            or (not std::is_same<_nonconst_conversion_capture_arg_tuple_entry, tinympl::nonesuch>::value),
          "internal error: non-const conversion capture detected, but arg_tuple type not detected correctly"
        );

        // Ignore scheduling permissions for now
        template <typename AccessHandleT>
        using _read_only_handle_capture_arg_tuple_entry_archetype =
          typename std::decay_t<AccessHandleT>::template with_traits<
            typename std::decay_t<AccessHandleT>::traits
              ::template with_static_immediate_permissions<detail::AccessHandlePermissions::Read>::type
              // Also set the min schedule permissions, in case they were higher before
              ::template with_required_immediate_permissions<detail::AccessHandlePermissions::Read>::type
              ::template with_required_scheduling_permissions<detail::AccessHandlePermissions::Read>::type
          >;

        using _read_only_handle_capture_arg_tuple_entry = tinympl::detected_t<
          _read_only_handle_capture_arg_tuple_entry_archetype, CallArg
        >;

        using _access_handle_like_arg_tuple_entry = std::decay_t<
          typename formal_traits::param_t
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
          // TODO this should be just the parameter itself if both it and the arg are access handles
          // For the AccessHandle<T> => ReadAccessHandle<T>& case:
          std::integral_constant<bool, is_access_handle>,
            /* => */ _access_handle_like_arg_tuple_entry,
          //------------------------------------------------------------
          // For the AccessHandle<T> => ReadAccessHandle<T>& case:
          //std::integral_constant<bool, is_read_only_handle_capture>,
          //  /* => */ _read_only_handle_capture_arg_tuple_entry,
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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef DARMA_USE_NEW_FUNCTOR_CAPTURE
namespace detail {

template <typename CaptureManagerT>
struct captured_aao_storage_ctor_helper
  : CaptureManager
{
  CaptureManagerT* passthrough_to_manager;

  captured_aao_storage_ctor_helper()
    : passthrough_to_manager(
    get_running_task_impl()->current_create_work_context
  )
  {
    assert(passthrough_to_manager != nullptr);
    get_running_task_impl()->current_create_work_context = this;
    passthrough_to_manager = nullptr;
  }
};

template <typename AAO, typename ParamType, typename CaptureManagerT>
struct captured_aao_storage
  : captured_aao_storage_ctor_helper<CaptureManagerT>
{
  AAO stored_value;
  explicit
  captured_aao_storage(
    AAO const& source
  ) : stored_value(source)
  {
    get_running_task_impl()->current_create_work_context = this->passthrough_to_manager;
  }

  using aao_traits =  async_accessible_object_traits<AAO>;
  using aao_param_traits =  async_accessible_object_traits<std::decay_t<ParamType>>;
  static constexpr auto required_scheduling = aao_traits
    ::template implied_scheduling_permissions_as_argument_for_parameter_c<ParamType>::value;
  static constexpr auto required_immediate = aao_traits
    ::template implied_immediate_permissions_as_argument_for_parameter_c<ParamType>::value;

  void do_capture(
    AccessHandleBase& captured,
    AccessHandleBase const& source_and_continuing
  ) override {

    auto pair_before_downgrades = AccessHandleBaseAttorney::get_permissions_before_downgrades(
      source_and_continuing,
      scheduling_capture_op,
      immediate_capture_op
    );

    auto req_scheduling = required_scheduling;
    auto req_immediate = required_immediate;

    if(req_scheduling == frontend::Permissions::_notGiven) {
      req_scheduling = pair_before_downgrades.scheduling;
    }
    if(req_immediate == frontend::Permissions::_notGiven) {
      req_immediate = pair_before_downgrades.immediate;
    }

    this->passthrough_to_manager->do_capture(
      captured, source_and_continuing,
      req_scheduling, req_immediate,
      aao_param_traits::implies_move_as_parameter_for_argument_c<AAO>::value,
      aao_param_traits::implies_copy_as_parameter_for_argument_c<AAO>::value
    );

  }
};


namespace _impl {

template <typename Functor>
using _is_generated_explicit_functor_archetype =
  typename Functor::_darma_INTERNAL_functor_traits;

template <typename Functor>
using _is_user_explicit_functor_archetype =
  typename Functor::_darma_INTERNAL_functor_traits;


template <typename Functor>
using _is_deducible_functor_archetype = decltype(&Functor::operator());

template <typename Functor>
using _is_deducible_functor = tinympl::bool_<
  tinympl::is_detected<_is_deducible_functor_archetype, Functor>::value
>;
template <typename Functor>
using _is_generated_explicit_functor = tinympl::bool_<
  tinympl::is_detected<_is_deducible_functor_archetype, Functor>::value
>;
template <typename Functor>
using _is_user_explicit_functor = tinympl::bool_<
  tinympl::is_detected<_is_deducible_functor_archetype, Functor>::value
>;

} // end namespace _impl

template <typename Functor, typename ParamsVector>
struct functor_closure_overload_description;

template <typename Functor, typename... Params>
struct functor_closure_overload_description<
  Functor, tinympl::vector<Params...>
> {

  template <typename Arg, typename Param>
  using arg_storage_t = std::conditional_t<
    async_accessible_object_traits<std::decay_t<Arg>>::is_async_accessible_object,
    // We should eventually be able to propagate the functor task type to avoid an extra vtable lookup
    captured_aao_storage<std::decay_t<Arg>, Param, detail::CaptureManager>,
    std::decay_t<Param>
  >;

  template <typename... Args>
  using stored_args_tuple_t = typename tinympl::zip<
    std::tuple,
    arg_storage_t,
    tinympl::vector<Args...>,
    tinympl::vector<Params...>
  >::type;

  //template <typename Arg, typename Param>
  //struct call_traits
  //
  //template <typename... Args>
  //using call_traits_vector =

};

template <typename>
struct functor_closure_traits_impl;

template <typename... OverloadDescs>
struct functor_closure_traits_impl<
  tinympl::vector<OverloadDescs...>
> {
  using overloads_vector = tinympl::vector<OverloadDescs...>;

  // For now, we only support one overload, so it has to be the first one
  template <typename... ArgTypes>
  using best_overload = typename overloads_vector::front;
};

} // end namespace detail

template <typename Functor, typename Enable=void>
struct functor_closure_traits;

template <
  typename Functor
>
struct functor_closure_traits<
  Functor, std::enable_if_t<
    not detail::_impl::_is_generated_explicit_functor<Functor>::value
    and not detail::_impl::_is_user_explicit_functor<Functor>::value
    and detail::_impl::_is_deducible_functor<Functor>::value
  >
> : detail::functor_closure_traits_impl<
      tinympl::vector<
        detail::functor_closure_overload_description<
          Functor, typename meta::callable_traits<Functor>::params_vector
        >
      >
    >
{

};

#endif



} // end namespace darma_runtime

#endif //DARMA_IMPL_FUNCTOR_TRAITS_H
