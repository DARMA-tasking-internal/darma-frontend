/*
//@HEADER
// ************************************************************************
//
//                      access_handle_traits.h
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

#ifndef DARMA_IMPL_ACCESS_HANDLE_ACCESS_HANDLE_TRAITS_H
#define DARMA_IMPL_ACCESS_HANDLE_ACCESS_HANDLE_TRAITS_H

#include <cstdlib> // std::size_t

#include <tinympl/logical_and.hpp>

#include <darma/impl/handle_fwd.h>
#include <darma/impl/util/optional_boolean.h>

namespace darma_runtime {
namespace detail {

//==============================================================================
// <editor-fold desc="Various categories of traits"> {{{1

template <
  access_handle_permissions_t RequiredSchedulingPermissions = NotGiven,
  access_handle_permissions_t RequiredImmediatePermissions = NotGiven,
  access_handle_permissions_t StaticSchedulingPermissions = NotGiven,
  access_handle_permissions_t StaticImmediatePermissions = NotGiven,
  optional_boolean_t IsCommutative = OptionalBoolean::Unknown
>
struct access_handle_permissions_traits
{
  static constexpr auto required_scheduling_permissions = RequiredSchedulingPermissions;
  static constexpr auto
    required_scheduling_permissions_given = RequiredSchedulingPermissions != NotGiven;
  static constexpr auto required_immediate_permissions = RequiredImmediatePermissions;
  static constexpr auto
    required_immediate_permissions_given = RequiredImmediatePermissions != NotGiven;
  static constexpr auto static_scheduling_permissions = StaticSchedulingPermissions;
  static constexpr auto
    static_scheduling_permissions_given = StaticSchedulingPermissions != NotGiven;
  static constexpr auto static_immediate_permissions = StaticImmediatePermissions;
  static constexpr auto
    static_immediate_permissions_given = StaticImmediatePermissions != NotGiven;

  static constexpr auto is_commutative = IsCommutative;
};

template <
  access_handle_task_collection_capture_mode_t CollectionCaptureMode =
    AccessHandleTaskCollectionCaptureMode::NoCollectionCapture,
  typename OwningIndexType = std::size_t
>
struct access_handle_collection_capture_traits
{
  static constexpr auto collection_capture_mode = CollectionCaptureMode;
  static constexpr auto
    collection_capture_given = CollectionCaptureMode != NoCollectionCapture;
  static constexpr auto
    collection_captured_as_shared_read = CollectionCaptureMode == SharedRead;
  static constexpr auto collection_captured_as_unique_modify =
    CollectionCaptureMode == UniqueModify;

  using owning_index_t = OwningIndexType;
};

//typedef enum AccessHandleCopyAssignabilityModeAsLHS {
//  LHSDynamic = -1,
//  LHSAlwaysFalse = 0,
//  LHSAlwaysTrue = 1,
//  LHSOnlyOnce = 2
//} access_handle_copy_assignability_mode_as_lhs;
//
//typedef enum AccessHandleCopyAssignabilityModeAsRHS {
//  RHSDynamic = -1,
//  RHSAlwaysFalse = 0,
//  RHSAlwaysTrue = 1,
//  RHSOnlyOnce = 2
//} access_handle_copy_assignability_mode_as_rhs;

template <
  optional_boolean_t IsCopyAssignable = OptionalBoolean::Unknown
>
struct access_handle_semantic_traits {
  // TODO add an "assignable once" mode for classes and stuff
  static constexpr auto is_copy_assignable = IsCopyAssignable;
};

template <
  typename T,
  typename Allocator = darma::serialization::darma_allocator<T>
>
struct access_handle_allocation_traits {
  using allocator_t = Allocator;
};

// </editor-fold> end Various categories of traits }}}1
//==============================================================================


//==============================================================================
// <editor-fold desc="access_handle_special_members and helpers"> {{{1

//------------------------------------------------------------------------------
// <editor-fold desc="access_handle_special_permissions_members"> {{{2

template <
  typename PermissionsTraits = access_handle_permissions_traits<>
>
struct access_handle_special_permissions_members
{
  // TODO move dynamic commutativity here, potentially


  mutable bool is_commutative_dynamic =
    PermissionsTraits::is_commutative == OptionalBoolean::KnownTrue;

  // There's only one for now, so...
  template <typename OtherTraits>
  using is_reinterpret_castable_from = std::true_type;

  access_handle_special_permissions_members() = default;

  template <typename PermissionsTraitsT>
  access_handle_special_permissions_members& operator=(
    access_handle_special_permissions_members<PermissionsTraitsT> const& other
  ) {
    is_commutative_dynamic = other.is_commutative_dynamic;
    return *this;
  }
  template <typename PermissionsTraitsT>
  access_handle_special_permissions_members& operator=(
    access_handle_special_permissions_members<PermissionsTraitsT>&& other
  ) {
    is_commutative_dynamic = other.is_commutative_dynamic;
    return *this;
  }
  template <typename PermissionsTraitsT>
  access_handle_special_permissions_members(
    access_handle_special_permissions_members<PermissionsTraitsT> const& other
  ) : is_commutative_dynamic(other.is_commutative_dynamic)
  { }
  template <typename PermissionsTraitsT>
  access_handle_special_permissions_members(
    access_handle_special_permissions_members<PermissionsTraitsT>&& other
  ) : is_commutative_dynamic(other.is_commutative_dynamic)
  { }

};

// </editor-fold> end access_handle_special_permissions_members }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="access_handle_special_collection_capture_members"> {{{2

template <
  typename CollectionCaptureTraits = access_handle_collection_capture_traits<>
>
struct access_handle_special_collection_capture_members
{
  // Check that the other traits are empty, just to be safe
  template <typename OtherTraits>
  using is_reinterpret_castable_from = std::is_empty<
    access_handle_special_collection_capture_members<OtherTraits>
  >;

  template <typename OtherTraits>
  using is_convertible_from = std::true_type;

  access_handle_special_collection_capture_members() = default;

  template <
    typename OtherTraits,
    typename=std::enable_if_t<is_convertible_from<OtherTraits>::value>
  >
  access_handle_special_collection_capture_members(
    access_handle_special_collection_capture_members<OtherTraits> const&
  ) { /* no members, so nothing to do */ }

  template <
    typename OtherTraits,
    typename=std::enable_if_t<is_convertible_from<OtherTraits>::value>
  >
  access_handle_special_collection_capture_members(
    access_handle_special_collection_capture_members<OtherTraits>&&
  ) { /* no members, so nothing to do */ }

  template <
    typename OtherTraits,
    typename=std::enable_if_t<is_convertible_from<OtherTraits>::value>
  >
  access_handle_special_collection_capture_members& operator=(
    access_handle_special_collection_capture_members<OtherTraits> const&
  ) {
    /* no members, so nothing to do */
    return *this;
  }

  template <
    typename OtherTraits,
    typename=std::enable_if_t<is_convertible_from<OtherTraits>::value>
  >
  access_handle_special_collection_capture_members& operator=(
    access_handle_special_collection_capture_members<OtherTraits>&&
  ) {
    /* no members, so nothing to do */
    return *this;
  }

};

template <typename OwningIndexT>
struct access_handle_special_collection_capture_members<
  access_handle_collection_capture_traits<
    AccessHandleTaskCollectionCaptureMode::UniqueModify, OwningIndexT
  >
>
{
  // TODO this probably also needs to be included in Unknown capture mode


  mutable OwningIndexT owning_index_;
  mutable std::size_t owning_backend_index_ = 0;

  access_handle_special_collection_capture_members() = default;
  access_handle_special_collection_capture_members(
    access_handle_special_collection_capture_members const&
  ) = default;
  access_handle_special_collection_capture_members(
    access_handle_special_collection_capture_members&&
  ) = default;
  access_handle_special_collection_capture_members& operator=(
    access_handle_special_collection_capture_members const&
  ) = default;
  access_handle_special_collection_capture_members& operator=(
    access_handle_special_collection_capture_members&&
  ) = default;

  // Note that there's no conversion ctor from other traits versions like there
  // is in the generic case

  // Type and mode must match exactly
  template <typename OtherTraits>
  using is_reinterpret_castable_from = tinympl::and_<
    tinympl::bool_<
      OtherTraits::collection_captured_as_unique_modify
    >,
    std::is_same<OwningIndexT, typename OtherTraits::owning_index_t>
  >;

};

// </editor-fold> end access_handle_special_collection_capture_members }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="access_handle_special_semantic_members"> {{{2

template <typename SemanticTraits>
struct access_handle_special_semantic_members {

  // For now at least, always allow the loss of information
  template <typename OtherTraits>
  using is_convertible_from = std::true_type;

  // Check that the other traits are empty, just to be safe
  template <typename OtherTraits>
  using is_reinterpret_castable_from = std::is_empty<
    access_handle_special_semantic_members<OtherTraits>
  >;

  access_handle_special_semantic_members() = default;

  template <
    typename OtherTraits,
    typename=std::enable_if_t<is_convertible_from<OtherTraits>::value>
  >
  access_handle_special_semantic_members(
    access_handle_special_semantic_members<OtherTraits> const&
  ) { /* no members, so nothing to do */ }

  template <
    typename OtherTraits,
    typename=std::enable_if_t<is_convertible_from<OtherTraits>::value>
  >
  access_handle_special_semantic_members(
    access_handle_special_semantic_members<OtherTraits>&&
  ) { /* no members, so nothing to do */ }

  template <
    typename OtherTraits,
    typename=std::enable_if_t<is_convertible_from<OtherTraits>::value>
  >
  access_handle_special_semantic_members& operator=(
    access_handle_special_semantic_members<OtherTraits> const&
  ) {
    /* no members, so nothing to do */
    return *this;
  }

  template <
    typename OtherTraits,
    typename=std::enable_if_t<is_convertible_from<OtherTraits>::value>
  >
  access_handle_special_semantic_members& operator=(
    access_handle_special_semantic_members<OtherTraits>&&
  ) {
    /* no members, so nothing to do */
    return *this;
  }

};

// </editor-fold> end access_handle_special_semantic_members }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="access_handle_special_allocation_members"> {{{2

template <typename AllocationTraits>
struct access_handle_special_allocation_members {
  // All constructibility/convertibility constraints should be implicitly
  // inherited from allocator_t (I think...)
  typename AllocationTraits::allocator_t allocator;

  template <typename OtherTraits>
  using is_convertible_from = std::is_convertible<
    typename OtherTraits::allocator_t, typename AllocationTraits::allocator_t
  >;

  template <typename OtherTraits>
  using is_reinterpret_castable_from = std::is_same<
    typename OtherTraits::allocator_t, typename AllocationTraits::allocator_t
  >;

  template <typename OtherTraits>
  using is_copy_convertible_from = std::is_convertible<
    typename OtherTraits::allocator_t const&, typename AllocationTraits::allocator_t
  >;

  template <typename OtherTraits>
  using is_move_convertible_from = std::is_convertible<
    typename OtherTraits::allocator_t&&, typename AllocationTraits::allocator_t
  >;

  template <typename OtherTraits>
  using is_copy_assignable_from = std::is_assignable<
    typename AllocationTraits::allocator_t&,
    typename OtherTraits::allocator_t const&
  >;

  template <typename OtherTraits>
  using is_move_assignable_from = std::is_assignable<
    typename AllocationTraits::allocator_t&,
    typename OtherTraits::allocator_t&&
  >;

  template <typename _Ignored_SFINAE=void,
    typename=std::enable_if_t<
      std::is_default_constructible<typename AllocationTraits::allocator_t>::value
      and std::is_void<_Ignored_SFINAE>::value
    >
  >
  access_handle_special_allocation_members()
    : allocator()
  { }

  template <
    typename OtherTraits,
    typename=std::enable_if_t<is_copy_convertible_from<OtherTraits>::value>
  >
  access_handle_special_allocation_members(
    access_handle_special_allocation_members<OtherTraits> const& other
  ) : allocator(other.allocator) { }

  template <
    typename OtherTraits,
    typename=std::enable_if_t<is_move_convertible_from<OtherTraits>::value>
  >
  access_handle_special_allocation_members(
    access_handle_special_allocation_members<OtherTraits>&& other
  ) : allocator(std::move(other.allocator)) { }

  template <
    typename OtherTraits,
    typename=std::enable_if_t<is_copy_assignable_from<OtherTraits>::value>
  >
  access_handle_special_allocation_members& operator=(
    access_handle_special_allocation_members<OtherTraits> const& other
  ) {
    allocator = other.allocator;
    return *this;
  }

  template <
    typename OtherTraits,
    typename=std::enable_if_t<is_move_assignable_from<OtherTraits>::value>
  >
  access_handle_special_allocation_members& operator=(
    access_handle_special_allocation_members<OtherTraits>&& other
  ) {
    allocator = std::move(other.allocator);
    return *this;
  }
};

// </editor-fold> end access_handle_special_allocation_members }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="access_handle_special_members"> {{{2

template <
  typename PermissionsTraits,
  typename CollectionCaptureTraits,
  typename SemanticTraits,
  typename AllocationTraits
>
struct access_handle_special_members
  : access_handle_special_permissions_members<PermissionsTraits>,
    access_handle_special_collection_capture_members<CollectionCaptureTraits>,
    access_handle_special_semantic_members<SemanticTraits>,
    access_handle_special_allocation_members<AllocationTraits>
{

  using special_permissions_members =
    access_handle_special_permissions_members<PermissionsTraits>;
  using special_collection_capture_members =
    access_handle_special_collection_capture_members<CollectionCaptureTraits>;
  using special_semantic_members =
    access_handle_special_semantic_members<SemanticTraits>;
  using special_allocation_members =
    access_handle_special_allocation_members<AllocationTraits>;

  template <typename OtherTraits>
  using is_reinterpret_castable_from = tinympl::and_<
    typename special_permissions_members
      ::template is_reinterpret_castable_from<typename OtherTraits::permissions_traits>,
    typename special_collection_capture_members
      ::template is_reinterpret_castable_from<typename OtherTraits::collection_capture_traits>,
    typename special_semantic_members
      ::template is_reinterpret_castable_from<typename OtherTraits::semantic_traits>,
    typename special_allocation_members
      ::template is_reinterpret_castable_from<typename OtherTraits::allocation_traits>
  >;


  access_handle_special_members() = default;

  template <
    typename PermissionsTraitsT,
    typename CollectionCaptureTraitsT,
    typename SemanticTraitsT,
    typename AllocationTraitsT,
    typename=std::enable_if_t<tinympl::and_<
      std::is_convertible<
        access_handle_special_permissions_members<PermissionsTraitsT>&&,
        special_permissions_members
      >,
      std::is_convertible<
        access_handle_special_collection_capture_members<CollectionCaptureTraitsT>&&,
        special_collection_capture_members
      >,
      std::is_convertible<
        access_handle_special_semantic_members<SemanticTraitsT>&&,
        special_semantic_members
      >,
      std::is_convertible<
        access_handle_special_allocation_members<AllocationTraitsT>&&,
        special_allocation_members
      >
    >::type::value>
  >
  access_handle_special_members(
    access_handle_special_members<
      PermissionsTraitsT,
      CollectionCaptureTraitsT,
      SemanticTraitsT,
      AllocationTraitsT
    >&& other
  ) : special_permissions_members(std::move(other)),
      special_collection_capture_members(std::move(other)),
      special_semantic_members(std::move(other)),
      special_allocation_members(std::move(other))
  { }

  template <
    typename PermissionsTraitsT,
    typename CollectionCaptureTraitsT,
    typename SemanticTraitsT,
    typename AllocationTraitsT,
    typename=std::enable_if_t<tinympl::and_<
      std::is_convertible<
        access_handle_special_permissions_members<PermissionsTraitsT> const&,
        special_permissions_members
      >,
      std::is_convertible<
        access_handle_special_collection_capture_members<CollectionCaptureTraitsT> const&,
        special_collection_capture_members
      >,
      std::is_convertible<
        access_handle_special_semantic_members<SemanticTraitsT> const&,
        special_semantic_members
      >,
      std::is_convertible<
        access_handle_special_allocation_members<AllocationTraitsT> const&,
        special_allocation_members
      >
    >::type::value>
  >
  access_handle_special_members(
    access_handle_special_members<
      PermissionsTraitsT,
      CollectionCaptureTraitsT,
      SemanticTraitsT,
      AllocationTraitsT
    > const& other
  ) : special_permissions_members(other),
      special_collection_capture_members(other),
      special_semantic_members(other),
      special_allocation_members(other)
  { }

  template <
    typename PermissionsTraitsT,
    typename CollectionCaptureTraitsT,
    typename SemanticTraitsT,
    typename AllocationTraitsT,
    typename=std::enable_if_t<tinympl::and_<
      std::is_assignable<
        special_permissions_members&,
        access_handle_special_permissions_members<PermissionsTraitsT> const&
      >,
      std::is_assignable<
        special_collection_capture_members&,
        access_handle_special_collection_capture_members<CollectionCaptureTraitsT> const&
      >,
      std::is_assignable<
        special_semantic_members&,
        access_handle_special_semantic_members<SemanticTraitsT> const&
      >,
      std::is_assignable<
        special_allocation_members&,
        access_handle_special_allocation_members<AllocationTraitsT> const&
      >
    >::type::value>
  >
  access_handle_special_members& operator=(
    access_handle_special_members<
      PermissionsTraitsT,
      CollectionCaptureTraitsT,
      SemanticTraitsT,
      AllocationTraitsT
    > const& other
  ) {
    this->special_permissions_members::operator=(other);
    this->special_collection_capture_members::operator=(other);
    this->special_semantic_members::operator=(other);
    this->special_allocation_members::operator=(other);
    return *this;
  };

  template <
    typename PermissionsTraitsT,
    typename CollectionCaptureTraitsT,
    typename SemanticTraitsT,
    typename AllocationTraitsT,
    typename=std::enable_if_t<tinympl::and_<
      std::is_assignable<
        special_permissions_members&,
        access_handle_special_permissions_members<PermissionsTraitsT>&&
      >,
      std::is_assignable<
        special_collection_capture_members&,
        access_handle_special_collection_capture_members<CollectionCaptureTraitsT>&&
      >,
      std::is_assignable<
        special_semantic_members&,
        access_handle_special_semantic_members<SemanticTraitsT>&&
      >,
      std::is_assignable<
        special_allocation_members&,
        access_handle_special_allocation_members<AllocationTraitsT>&&
      >
    >::type::value>
  >
  access_handle_special_members& operator=(
    access_handle_special_members<
      PermissionsTraitsT,
      CollectionCaptureTraitsT,
      SemanticTraitsT,
      AllocationTraitsT
    >&& other
  ) {
    this->special_permissions_members::operator=(std::move(other));
    this->special_collection_capture_members::operator=(std::move(other));
    this->special_semantic_members::operator=(std::move(other));
    this->special_allocation_members::operator=(std::move(other));
    return *this;
  };
};

// </editor-fold> end access_handle_special_members }}}2
//------------------------------------------------------------------------------

// </editor-fold> end access_handle_special_members and helpers }}}1
//==============================================================================


// (Not really true, needs more explanation): Min permissions refers to as a
//  parameter, max permissions refers to as a call argument or lvalue
// (or as a parameter for determining whether a capture is read-only).  All
// are only the known compile-time bounds; if no restrictions are given at
// compile time, all will be AccessHandlePermissions::NotGiven
// TODO more full documentation
// Min ~= "required" or "requested"
// Max ~= "statically available (e.g., unless permissions are explicitly reduced or released)"
// TODO refactor this?
template <
  typename T,
  typename PermissionsTraits = access_handle_permissions_traits<>,
  typename CollectionCaptureTraits = access_handle_collection_capture_traits<>,
  typename SemanticTraits = access_handle_semantic_traits<>,
  typename AllocationTraits = access_handle_allocation_traits<T>
>
struct access_handle_traits {

  using value_type = T;

  using permissions_traits = PermissionsTraits;
  using collection_capture_traits = CollectionCaptureTraits;
  using semantic_traits = SemanticTraits;
  using allocation_traits = AllocationTraits;

  using special_members_t = access_handle_special_members<
    permissions_traits, collection_capture_traits, semantic_traits, allocation_traits
  >;

  template <typename OtherTraits>
  using is_reinterpret_castable_from =
    typename special_members_t::template is_reinterpret_castable_from<OtherTraits>;

  static constexpr auto required_scheduling_permissions =
    permissions_traits::required_scheduling_permissions;
  static constexpr auto required_scheduling_permissions_given =
    permissions_traits::required_scheduling_permissions_given;
  static constexpr auto required_immediate_permissions =
    permissions_traits::required_immediate_permissions;
  static constexpr auto required_immediate_permissions_given =
    permissions_traits::required_immediate_permissions_given;
  static constexpr auto static_scheduling_permissions =
    permissions_traits::static_scheduling_permissions;
  static constexpr auto static_scheduling_permissions_given =
    permissions_traits::static_scheduling_permissions_given;
  static constexpr auto static_immediate_permissions =
    permissions_traits::static_immediate_permissions;
  static constexpr auto static_immediate_permissions_given =
    permissions_traits::static_immediate_permissions_given;

  static constexpr auto collection_capture_mode =
    collection_capture_traits::collection_capture_mode;
  static constexpr auto collection_capture_given =
    collection_capture_traits::collection_capture_given;
  static constexpr auto collection_captured_as_shared_read =
    collection_capture_traits::collection_captured_as_shared_read;
  static constexpr auto collection_captured_as_unique_modify =
    collection_capture_traits::collection_captured_as_unique_modify;

  using owning_index_t = typename collection_capture_traits::owning_index_t;

  template <access_handle_permissions_t new_min_schedule_permissions>
  struct with_required_scheduling_permissions {
    using type = access_handle_traits<T,
      access_handle_permissions_traits<
        new_min_schedule_permissions,
        required_immediate_permissions,
        static_scheduling_permissions,
        static_immediate_permissions
      >,
      collection_capture_traits,
      semantic_traits
    >;
  };

  template <access_handle_permissions_t new_max_schedule_permissions>
  struct with_static_scheduling_permissions {
    using type = access_handle_traits<T,
      access_handle_permissions_traits<
        required_scheduling_permissions,
        required_immediate_permissions,
        new_max_schedule_permissions,
        static_immediate_permissions
      >,
      collection_capture_traits,
      semantic_traits
    >;
  };

  template <access_handle_permissions_t new_required_immediate_permissions>
  struct with_required_immediate_permissions {
    using type = access_handle_traits<T,
      access_handle_permissions_traits<
        required_scheduling_permissions,
        new_required_immediate_permissions,
        static_scheduling_permissions,
        static_immediate_permissions
      >,
      collection_capture_traits,
      semantic_traits
    >;
  };

  template <access_handle_permissions_t new_static_immediate_permissions>
  struct with_static_immediate_permissions {
    using type = access_handle_traits<T,
      access_handle_permissions_traits<
        required_scheduling_permissions,
        required_immediate_permissions,
        static_scheduling_permissions,
        new_static_immediate_permissions
      >,
      collection_capture_traits,
      semantic_traits
    >;
  };

  template <access_handle_task_collection_capture_mode_t new_capture_mode>
  struct with_collection_capture_mode {
    using type = access_handle_traits<T,
      permissions_traits,
      access_handle_collection_capture_traits<
        new_capture_mode,
        owning_index_t
      >,
      semantic_traits
    >;
  };

  template <typename NewOwningIndexT>
  struct with_owning_index_type {
    using type = access_handle_traits<T,
      permissions_traits,
      access_handle_collection_capture_traits<
        collection_capture_mode,
        NewOwningIndexT
      >,
      semantic_traits
    >;
  };

  template <OptionalBoolean new_copy_assignable>
  struct with_copy_assignable {
    using type = access_handle_traits<T,
      permissions_traits,
      collection_capture_traits,
      access_handle_semantic_traits<
        new_copy_assignable
      >
    >;
  };

};


//------------------------------------------------------------------------------
// <editor-fold desc="make_access_traits and associated 'keyword' template arguments"> {{{2

template <access_handle_permissions_t permissions>
struct required_scheduling_permissions {
  static constexpr auto value = permissions;
  template <typename Traits>
  using modified_traits = typename Traits
    ::template with_required_scheduling_permissions<permissions>;
};

template <access_handle_permissions_t permissions>
struct static_scheduling_permissions {
  static constexpr auto value = permissions;
  template <typename Traits>
  using modified_traits = typename Traits
    ::template with_static_scheduling_permissions<permissions>;
};

template <access_handle_permissions_t permissions>
struct required_immediate_permissions {
  static constexpr auto value = permissions;
  template <typename Traits>
  using modified_traits = typename Traits
    ::template with_required_immediate_permissions<permissions>;
};

template <access_handle_permissions_t permissions>
struct static_immediate_permissions {
  static constexpr auto value = permissions;
  template <typename Traits>
  using modified_traits = typename Traits
    ::template with_static_immediate_permissions<permissions>;
};

template <access_handle_task_collection_capture_mode_t capture_mode>
struct collection_capture_mode {
  static constexpr auto value = capture_mode;
  template <typename Traits>
  using modified_traits = typename Traits
    ::template with_collection_capture_mode<capture_mode>;
};

template <typename OwningIndex>
struct owning_index_type {
  using type = OwningIndex;
  template <typename Traits>
  using modified_traits = typename Traits
    ::template with_owning_index_type<OwningIndex>;
};

template <bool copy_assignable>
struct copy_assignable_handle {
  static constexpr auto value = copy_assignable;
  template <typename Traits>
  using modified_traits = typename Traits
    ::template with_copy_assignable<
      copy_assignable ? OptionalBoolean::KnownTrue : OptionalBoolean::KnownFalse
    >;
};
template <bool copy_assignable>
using copy_assignability = copy_assignable_handle<copy_assignable>;

struct unknown_copy_assignability {
  static constexpr auto value = OptionalBoolean::Unknown;
  template <typename Traits>
  using modified_traits = typename Traits
    ::template with_copy_assignable<OptionalBoolean::Unknown>;
};

template <typename T, typename... modifiers>
struct make_access_handle_traits {
  template <typename Traits, typename Modifier>
  using _add_trait = typename Modifier::template modified_traits<Traits>;
  using type = typename tinympl::variadic::left_fold<
    _add_trait, access_handle_traits<T>, modifiers...
  >::type;
  template <typename Traits>
  struct from_traits {
    using type = typename tinympl::variadic::left_fold<
      _add_trait, Traits, modifiers...
    >::type;
  };
};

template <typename... modifiers>
using make_access_handle_traits_t = typename make_access_handle_traits<modifiers...>::type;

// </editor-fold> }}}2
//------------------------------------------------------------------------------

} // end namespace detail

namespace advanced {
namespace access_handle_traits {

template <bool copy_assignable_bool=true>
using copy_assignable = darma_runtime::detail::copy_assignable_handle<copy_assignable_bool>;

} // end namespace access_handle_traits
} // end namespace advanced

} // end namespace darma_runtime

#endif //DARMA_IMPL_ACCESS_HANDLE_ACCESS_HANDLE_TRAITS_H
