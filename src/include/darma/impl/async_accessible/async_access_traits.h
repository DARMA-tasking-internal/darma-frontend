/*
//@HEADER
// ************************************************************************
//
//                      async_access_traits.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMA_IMPL_ASYNC_ACCESSIBLE_ASYNC_ACCESS_TRAITS_H
#define DARMA_IMPL_ASYNC_ACCESSIBLE_ASYNC_ACCESS_TRAITS_H

#include <type_traits>

#include <tinympl/detection.hpp>
#include <tinympl/bool.hpp>

#include <darma/impl/handle_fwd.h>
#include <darma/impl/index_range/mapping.h>
#include <tinympl/logical_not.hpp>
#include <tinympl/select.hpp>
#include <darma/interface/frontend.h>
// TODO change this to use a forward declaration
#include <darma/interface/app/access_handle.h>

#define _darma_ASYNC_ACCESS_DETECTED_TYPE_IMPL( \
  assign_to, detected_name, default_type, access_specifier \
) \
  private: \
    template <typename _##detected_name##_arch_param> \
    using _##assign_to##_detect_##detected_name##_archetype = \
      typename _##detected_name##_arch_param::detected_name; \
  access_specifier: \
    using assign_to = _aao_detect<default_type, \
      _##assign_to##_detect_##detected_name##_archetype \
    >

#define _darma_AAO_DETECTED_TYPE( \
  detected_name, default_type \
) \
  _darma_ASYNC_ACCESS_DETECTED_TYPE_IMPL( \
    detected_name, detected_name, default_type, public \
  )

#define _darma_ASYNC_ACCESS_DETECTED_VALUE_IMPL( \
  assign_to, detected_name, default_value, access_specifier \
) \
  private: \
    template <typename _##detected_name##_arch_param> \
    using _##assign_to##_detect_value_##detected_name##_archetype = \
      ::std::integral_constant< \
        decltype(_##detected_name##_arch_param::detected_name), \
        _##detected_name##_arch_param::detected_name \
      >; \
  access_specifier: \
    static constexpr auto assign_to = _aao_detect< \
      ::std::integral_constant<decltype(default_value), default_value>, \
      _##assign_to##_detect_value_##detected_name##_archetype \
    >::value

#define _darma_AAO_DETECTED_VALUE( \
  detected_name, default_value \
) \
  _darma_ASYNC_ACCESS_DETECTED_VALUE_IMPL( \
    detected_name, detected_name, default_value, public \
  )

namespace darma_runtime {

template <typename T, typename Enable=void>
struct async_accessible_object_adaptor_enabled_if
{ };

template <typename T>
struct async_accessible_object_adaptor
  : async_accessible_object_adaptor_enabled_if<T>
{ };

template <typename T>
struct async_accessible_object_traits {

  public:

    using object_type = T;

    using adaptor_type = async_accessible_object_adaptor<T>;

    //TINYMPL_PUBLIC_DETECTED_TYPE(value_type, value_type, T);

  private:

    template <typename ObjectOrAdaptor>
    using _is_async_accessible_object_archetype = tinympl::bool_<
      ObjectOrAdaptor::is_async_accessible_object
    >;

    static constexpr auto _object_type_defines_is_async_accessible_object
      = tinympl::is_detected<
          _is_async_accessible_object_archetype, object_type
        >::value;

    // if only the adaptor defines the object as asynchronously accessible,
    // then the object might know nothing about the AAO interface (e.g., it might
    // have a member type named `value_type` that means something else), and we
    // should only ask the adaptor for AAO traits
    using _object_type_for_detection = std::conditional_t<
      _object_type_defines_is_async_accessible_object,
      object_type,
      meta::nonesuch
    >;



  //==============================================================================
  // <editor-fold desc="private helpers"> {{{1

  private:

    struct _not_detected_type { };

    template <
      typename default_type,
      template <typename...> class object_detector_archetype,
      template <typename...> class adaptor_detector_archetype,
      typename... ArgsForBoth
    >
    using _aao_detect_different_archs = tinympl::detected_or_t<
      /* ask the object_type only if the adaptor fails */ \
      tinympl::detected_or_t<
        /* fall back to the default as a last resort */ \
        default_type,
        object_detector_archetype,
        _object_type_for_detection, ArgsForBoth...
      >,
      /* first ask the adaptor */ \
      adaptor_detector_archetype,
      adaptor_type, ArgsForBoth...
    >;

    template <
      template <typename...> class object_detector_archetype,
      template <typename...> class adaptor_detector_archetype,
      typename... Args
    >
    using _aao_is_detected_different_archs_c = tinympl::not_<std::is_same<
      _not_detected_type,
      _aao_detect_different_archs<_not_detected_type,
        object_detector_archetype, adaptor_detector_archetype, Args...
      >
    >>;

    template <
      typename default_type,
      template <typename...> class detector_archetype,
      typename... Args
    >
    using _aao_detect = _aao_detect_different_archs<
      default_type, detector_archetype, detector_archetype, Args...
    >;

    template <
      template <typename...> class detector_archetype,
      typename... Args
    >
    using _aao_is_detected_c = tinympl::bool_<
      _aao_is_detected_different_archs_c<
        detector_archetype, detector_archetype, Args...
      >::value
    >;

  // </editor-fold> end private helpers }}}1
  //==============================================================================

  public:

    // If this evaluates to false, everything else in this class template should
    // be ignored
    static constexpr auto is_async_accessible_object = _aao_detect<
      std::false_type,
      _is_async_accessible_object_archetype
    >::value;

    _darma_AAO_DETECTED_TYPE(value_type, meta::nonesuch);


  //============================================================================
  // <editor-fold desc="permissions"> {{{1

  public:

    _darma_AAO_DETECTED_VALUE(
      required_immediate_permissions,
      detail::AccessHandlePermissions::NotGiven
    );

    _darma_AAO_DETECTED_VALUE(
      required_scheduling_permissions,
      detail::AccessHandlePermissions::NotGiven
    );

    _darma_AAO_DETECTED_VALUE(
      static_immediate_permissions,
      detail::AccessHandlePermissions::NotGiven
    );

    _darma_AAO_DETECTED_VALUE(
      static_scheduling_permissions,
      detail::AccessHandlePermissions::NotGiven
    );

  // </editor-fold> end permissions }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="index_range"> {{{1

  private:

    template <typename Object, typename _Unused>
    using _get_index_range_object_archetype = std::decay_t<
      decltype(std::declval<Object>().get_index_range())
    >;

    template <typename Adaptor, typename Object>
    using _get_index_range_adaptor_archetype = std::decay_t<
      decltype(Adaptor::get_index_range(std::declval<Object const&>()))
    >;

  public:

    static constexpr auto has_index_range = _aao_is_detected_different_archs_c<
      _get_index_range_object_archetype,
      _get_index_range_adaptor_archetype,
      object_type // works around an ICC bug that can't deal with names from context in detectors
    >::value;

    using index_range = std::decay_t<
      _aao_detect_different_archs<
        meta::nonesuch,
        _get_index_range_object_archetype,
        _get_index_range_adaptor_archetype,
        object_type // works around an ICC bug that can't deal with names from context in detectors
      >
    >;

  // </editor-fold> end index_range }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="mapped_with and mapped_type"> {{{1

  private:

    template <typename U, typename MappingT>
    using _mapped_with_archetype = decltype(
      std::declval<U>().mapped_with(std::declval<MappingT>())
    );

  public:

    // This should eventually be a variable template, once we're allowed to have
    // them in DARMA.  For now, the *_c extension (c is for "constant") indicates
    // something that will be a variable template named without the _c once that
    // feature is supported
    template <typename MappingT>
    using can_be_mapped_with_c = tinympl::bool_<
      tinympl::is_detected<_mapped_with_archetype, T, MappingT>::value
    >;

    template <typename MappingT>
    using mapped_type = tinympl::detected_t<
      _mapped_with_archetype, T, MappingT
    >;

  // </editor-fold> end mapped_with and mapped_type }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="implied capture permissions as an argument"> {{{1

  private:

    // reserving the name `scheduling_permissions_for` for the eventual value template
    template <typename ObjectOrAdaptor, typename U>
    using _implied_scheduling_permissions_archetype =
      typename ObjectOrAdaptor::template implied_scheduling_permissions_as_argument_for_parameter<U>;

    // reserving the name `immediate_permissions_for` for the eventual value template
    template <typename ObjectOrAdaptor, typename U>
    using _implied_immediate_permissions_archetype =
      typename ObjectOrAdaptor::template implied_immediate_permissions_as_argument_for_parameter<U>;

  public:

    template <typename ParamType>
    using default_scheduling_permissions_as_argument_for_parameter_c = tinympl::select_first_t<
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // First, ask the parameter type (if it's an AAO also)
      tinympl::bool_<
        async_accessible_object_traits<std::decay_t<ParamType>>::is_async_accessible_object
        and (OptionalBoolean)async_accessible_object_traits<std::decay_t<ParamType>>
          ::template can_be_parameter_for_argument_c<object_type>::value == OptionalBoolean::KnownTrue
      >, /* => */
        typename async_accessible_object_traits<std::decay_t<ParamType>>
          ::template scheduling_permissions_as_parameter_for_argument_c<object_type>
      ,
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Non-const lvalue reference convertible from value_type, default to no
      // scheduling permissions (only for non-AAO params)
      tinympl::bool_<
        not async_accessible_object_traits<std::decay_t<ParamType>>::is_async_accessible_object
        and std::is_convertible<value_type, std::decay_t<ParamType>>::value
        and std::is_lvalue_reference<ParamType>::value
        and not std::is_const<std::remove_reference_t<ParamType>>::value
      >, /* => */ std::integral_constant<frontend::permissions_t,
        frontend::Permissions::None
      >,
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Const lvalue reference convertible from value_type, default to no
      // scheduling permissions (only for non-AAO params)
      tinympl::bool_<
        not async_accessible_object_traits<std::decay_t<ParamType>>::is_async_accessible_object
        and std::is_convertible<value_type, std::decay_t<ParamType>>::value
        and std::is_lvalue_reference<ParamType>::value
        and not std::is_const<std::remove_reference_t<ParamType>>::value
      >, /* => */ std::integral_constant<frontend::permissions_t,
        frontend::Permissions::None
      >,
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // By-value parameter convertible from value_type, default to no
      // scheduling permissions (only for non-AAO params)
      tinympl::bool_<
        not async_accessible_object_traits<std::decay_t<ParamType>>::is_async_accessible_object
        and std::is_convertible<value_type, std::decay_t<ParamType>>::value
        and not std::is_reference<ParamType>::value
      >, /* => */ std::integral_constant<frontend::permissions_t,
        frontend::Permissions::None
      >,
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // TODO Rvalue reference case?
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Don't know what to do if we got here
      // TODO better error message generation
      std::true_type
      , /* => */ std::integral_constant<frontend::permissions_t,
        frontend::Permissions::_invalid
      >
    >;

    template <typename ParamType>
    using default_immediate_permissions_as_argument_for_parameter_c = tinympl::select_first_t<
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // First, ask the parameter type (if it's an AAO also)
      tinympl::bool_<
        async_accessible_object_traits<std::decay_t<ParamType>>::is_async_accessible_object
        and (OptionalBoolean)async_accessible_object_traits<std::decay_t<ParamType>>
          ::template can_be_parameter_for_argument_c<object_type>::value == OptionalBoolean::KnownTrue
      >, /* => */
        typename async_accessible_object_traits<std::decay_t<ParamType>>
          ::template immediate_permissions_as_parameter_for_argument_c<object_type>
      ,
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Non-const lvalue reference convertible from value_type, default to Modify
      // immediate permissions (only for non-AAO params)
      tinympl::bool_<
        not async_accessible_object_traits<std::decay_t<ParamType>>::is_async_accessible_object
        and std::is_base_of<std::decay_t<ParamType>, value_type>::value
        and std::is_lvalue_reference<ParamType>::value
        and not std::is_const<std::remove_reference_t<ParamType>>::value
      >, /* => */ std::integral_constant<frontend::permissions_t,
        frontend::Permissions::Modify
      >,
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Const lvalue reference convertible from value_type, default to Read
      // immediate permissions (only for non-AAO params)
      tinympl::bool_<
        not async_accessible_object_traits<std::decay_t<ParamType>>::is_async_accessible_object
        // allow binding of a const reference to an implicitly converted temporary
        and std::is_convertible<value_type, std::decay_t<ParamType>>::value
        and std::is_lvalue_reference<ParamType>::value
        and not std::is_const<std::remove_reference_t<ParamType>>::value
      >, /* => */ std::integral_constant<frontend::permissions_t,
        frontend::Permissions::Read
      >,
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // By-value parameter convertible from value_type, default to read
      // immediate permissions (only for non-AAO params)
      tinympl::bool_<
        not async_accessible_object_traits<std::decay_t<ParamType>>::is_async_accessible_object
        and std::is_convertible<value_type, std::decay_t<ParamType>>::value
        and not std::is_reference<ParamType>::value
      >, /* => */ std::integral_constant<frontend::permissions_t,
        frontend::Permissions::Read
      >,
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // TODO Rvalue reference case?
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Don't know what to do if we got here
      // TODO better error message generation
      std::true_type
      , /* => */ std::integral_constant<frontend::permissions_t,
        frontend::Permissions::_invalid
      >
    >;

    template <typename ParamType>
    using implied_scheduling_permissions_as_argument_for_parameter_c = _aao_detect<
      default_scheduling_permissions_as_argument_for_parameter_c<ParamType>,
      _implied_scheduling_permissions_archetype, ParamType
    >;

    template <typename ParamType>
    using implied_immediate_permissions_as_argument_for_parameter_c = _aao_detect<
      default_immediate_permissions_as_argument_for_parameter_c<ParamType>,
      _implied_immediate_permissions_archetype, ParamType
    >;

  // </editor-fold> end implied capture permissions as an argument }}}1
  //============================================================================

  //==============================================================================
  // <editor-fold desc="implied capture permissions as a parameter"> {{{1

  private:

    template <typename ObjectOrAdaptor, typename ArgType>
    using _implied_scheduling_permissions_as_parameter_archetype =
      typename ObjectOrAdaptor::template implied_scheduling_permissions_as_parameter_for_argument<ArgType>;

    template <typename ObjectOrAdaptor, typename ArgType>
    using _implied_immediate_permissions_as_parameter_archetype =
      typename ObjectOrAdaptor::template implied_immediate_permissions_as_parameter_for_argument<ArgType>;

    template <typename ObjectOrAdaptor, typename ArgType>
    using _can_be_parameter_for_argument_archetype =
      typename ObjectOrAdaptor::template can_be_parameter_for_argument<ArgType>;


  public:

    template <typename ArgumentType>
    using can_be_parameter_for_argument_c = _aao_detect<
      std::integral_constant<OptionalBoolean, OptionalBoolean::Unknown>,
      _can_be_parameter_for_argument_archetype,
      ArgumentType
    >;

    template <typename ArgumentType>
    using scheduling_permissions_as_parameter_for_argument_c = _aao_detect<
      // default to the required_scheduling_permissions static member
      std::integral_constant<frontend::permissions_t,
        frontend::permissions_t((int)required_scheduling_permissions)
      >,
      // Ask the object or the adaptor first
      _implied_scheduling_permissions_as_parameter_archetype
    >;

    template <typename ArgumentType>
    using immediate_permissions_as_parameter_for_argument_c = _aao_detect<
      // default to the required_immediate_permissions static member
      std::integral_constant<frontend::permissions_t,
        frontend::permissions_t((int)required_immediate_permissions)
      >,
      // Ask the object or the adaptor first
      _implied_immediate_permissions_as_parameter_archetype
    >;

    // For now, nothing other than value_type as the parameter implies copy
    template <typename ArgumentType>
    using implies_copy_as_parameter_for_argument_c = std::integral_constant<
      OptionalBoolean, OptionalBoolean::Unknown
    >;

    template <typename ArgumentType>
    using implies_move_as_parameter_for_argument_c = std::integral_constant<
      OptionalBoolean, OptionalBoolean::Unknown
    >;

  // </editor-fold> end implied capture permissions as a parameter }}}1
  //==============================================================================

  private:

    template <typename ObjectType, typename _Ignored=void>
    using _has_get_value_archetype = decltype(std::declval<ObjectType const&>().get_value());

    template <typename ObjectType, typename _Ignored=void>
    using _has_get_reference_archetype = decltype(std::declval<ObjectType const&>().get_reference());

    template <typename AdaptorType, typename ObjectType>
    using _adaptor_has_get_value_archetype = decltype(AdaptorType::get_value(std::declval<ObjectType const&>()));

    template <typename AdaptorType, typename ObjectType>
    using _adaptor_has_get_reference_archetype = decltype(AdaptorType::get_reference(std::declval<ObjectType const&>()));


  public:

    template <typename _Ignored=void>
    static decltype(auto)
    get_value(
      object_type const& obj,
      std::enable_if_t<
        std::is_void<_Ignored>::value // always true
          and tinympl::is_detected<_has_get_value_archetype, _object_type_for_detection>::value,
        utility::_not_a_type
      > = { }
    ) {
      return obj.get_value();
    }

    template <typename _Ignored=void>
    static decltype(auto)
    get_value(
      object_type const& obj,
      std::enable_if_t<
        std::is_void<_Ignored>::value // always true
          and tinympl::is_detected<_adaptor_has_get_value_archetype, adaptor_type>::value,
        utility::_not_a_type
      > = { }
    ) {
      return adaptor_type::get_value(obj);
    }

    template <typename _Ignored=void>
    static decltype(auto)
    get_reference(
      object_type const& obj,
      std::enable_if_t<
        std::is_void<_Ignored>::value // always true
          and tinympl::is_detected<_has_get_reference_archetype, _object_type_for_detection>::value,
        utility::_not_a_type
      > = { }
    ) {
      return obj.get_reference();
    }

    template <typename _Ignored=void>
    static decltype(auto)
    get_reference(
      object_type const& obj,
      std::enable_if_t<
        std::is_void<_Ignored>::value // always true
          and tinympl::is_detected<_adaptor_has_get_reference_archetype, adaptor_type>::value,
        utility::_not_a_type
      > = { }
    ) {
      return adaptor_type::get_reference(obj);
    }

};


// TODO move this to access handle definition file
template <typename T, typename Traits>
struct async_accessible_object_adaptor<
  AccessHandle<T, Traits>
>
{
  private:

    template <typename Argument>
    struct _can_be_param_for_arg_impl {
      static constexpr auto value = false;
    };

    template <typename U, typename OtherTraits>
    struct _can_be_param_for_arg_impl<
      AccessHandle<U, OtherTraits>
    > {
      static constexpr auto value = std::is_base_of<U, T>::value
        and Traits::template is_convertible_from<OtherTraits>::value;
    };


  public:

    template <typename Argument>
    using can_be_parameter_for_argument = tinympl::bool_<
      _can_be_param_for_arg_impl<Argument>::value
    >;

};

} // end namespace darma_runtime

#endif //DARMA_IMPL_ASYNC_ACCESSIBLE_ASYNC_ACCESS_TRAITS_H
