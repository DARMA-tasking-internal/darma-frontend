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

#include <darma/impl/handle_fwd.h>

namespace darma_runtime {
namespace detail {

template <
  access_handle_permissions_t MinSchedulingPermissions = NotGiven,
  access_handle_permissions_t MinImmediatePermissions = NotGiven,
  access_handle_permissions_t MaxSchedulingPermissions = NotGiven,
  access_handle_permissions_t MaxImmediatePermissions = NotGiven
>
struct access_handle_permissions_traits {
  static constexpr auto min_scheduling_permissions = MinSchedulingPermissions;
  static constexpr auto min_scheduling_permissions_given = MinSchedulingPermissions != NotGiven;
  static constexpr auto min_immediate_permissions = MinImmediatePermissions;
  static constexpr auto min_immediate_permissions_given = MinImmediatePermissions != NotGiven;
  static constexpr auto max_scheduling_permissions = MaxSchedulingPermissions;
  static constexpr auto max_scheduling_permissions_given = MaxSchedulingPermissions != NotGiven;
  static constexpr auto max_immediate_permissions = MaxImmediatePermissions;
  static constexpr auto max_immediate_permissions_given = MaxImmediatePermissions != NotGiven;
};

template <
  access_handle_task_collection_capture_mode_t CollectionCaptureMode =
    AccessHandleTaskCollectionCaptureMode::NoCollectionCapture,
  typename OwningIndexType = std::size_t
>
struct access_handle_collection_capture_traits {
  static constexpr auto collection_capture_mode = CollectionCaptureMode;
  static constexpr auto collection_capture_given = CollectionCaptureMode != NoCollectionCapture;
  static constexpr auto collection_captured_as_shared_read = CollectionCaptureMode == SharedRead;
  static constexpr auto collection_captured_as_unique_modify = CollectionCaptureMode == UniqueModify;

  using owning_index_t = OwningIndexType;
};

template <
  typename PermissionsTraits = access_handle_permissions_traits<>,
  typename CollectionCaptureTraits = access_handle_collection_capture_traits<>
>
struct access_handle_special_members { };

// (Not really true, needs more explanation): Min permissions refers to as a
//  parameter, max permissions refers to as a call argument or lvalue
// (or as a parameter for determining whether a capture is read-only).  All
// are only the known compile-time bounds; if no restrictions are given at
// compile time, all will be AccessHandlePermissions::NotGiven
// TODO more full documentation
template <
  typename PermissionsTraits = access_handle_permissions_traits<>,
  typename CollectionCaptureTraits = access_handle_collection_capture_traits<>
>
struct access_handle_traits {

  using permissions_traits = PermissionsTraits;
  using collection_capture_traits = CollectionCaptureTraits;

  using special_members_t = access_handle_special_members<
    permissions_traits, collection_capture_traits
  >;

  static constexpr auto min_scheduling_permissions =
    permissions_traits::min_scheduling_permissions;
  static constexpr auto min_scheduling_permissions_given =
    permissions_traits::min_scheduling_permissions_given;
  static constexpr auto min_immediate_permissions =
    permissions_traits::min_immediate_permissions;
  static constexpr auto min_immediate_permissions_given =
    permissions_traits::min_immediate_permissions_given;
  static constexpr auto max_scheduling_permissions =
    permissions_traits::max_scheduling_permissions;
  static constexpr auto max_scheduling_permissions_given =
    permissions_traits::max_scheduling_permissions_given;
  static constexpr auto max_immediate_permissions =
    permissions_traits::max_immediate_permissions;
  static constexpr auto max_immediate_permissions_given =
    permissions_traits::max_immediate_permissions_given;

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
  struct with_min_scheduling_permissions {
    using type = access_handle_traits<
      access_handle_permissions_traits<
        new_min_schedule_permissions,
        min_immediate_permissions,
        max_scheduling_permissions,
        max_immediate_permissions
      >,
      collection_capture_traits
    >;
  };

  template <access_handle_permissions_t new_max_schedule_permissions>
  struct with_max_scheduling_permissions {
    using type = access_handle_traits<
      access_handle_permissions_traits<
        min_scheduling_permissions,
        min_immediate_permissions,
        new_max_schedule_permissions,
        max_immediate_permissions
      >,
      collection_capture_traits
    >;
  };

  template <access_handle_permissions_t new_min_immediate_permissions>
  struct with_min_immediate_permissions {
    using type = access_handle_traits<
      access_handle_permissions_traits<
        min_scheduling_permissions,
        new_min_immediate_permissions,
        max_scheduling_permissions,
        max_immediate_permissions
      >,
      collection_capture_traits
    >;
  };

  template <access_handle_permissions_t new_max_immediate_permissions>
  struct with_max_immediate_permissions {
    using type = access_handle_traits<
      access_handle_permissions_traits<
        min_scheduling_permissions,
        min_immediate_permissions,
        max_scheduling_permissions,
        new_max_immediate_permissions
      >,
      collection_capture_traits
    >;
  };

  template <access_handle_task_collection_capture_mode_t new_capture_mode>
  struct with_collection_capture_mode {
    using type = access_handle_traits<
      permissions_traits,
      access_handle_collection_capture_traits<
        new_capture_mode,
        owning_index_t
      >
    >;
  };

  template <typename NewOwningIndexT>
  struct with_owning_index_type {
    using type = access_handle_traits<
      permissions_traits,
      access_handle_collection_capture_traits<
        collection_capture_mode,
        NewOwningIndexT
      >
    >;
  };

};


//------------------------------------------------------------
// <editor-fold desc="make_access_traits and associated 'keyword' template arguments">

template <access_handle_permissions_t permissions>
struct min_scheduling_permissions {
  static constexpr auto value = permissions;
};

template <access_handle_permissions_t permissions>
struct max_scheduling_permissions {
  static constexpr auto value = permissions;
};

template <access_handle_permissions_t permissions>
struct min_immediate_permissions {
  static constexpr auto value = permissions;
};

template <access_handle_permissions_t permissions>
struct max_immediate_permissions {
  static constexpr auto value = permissions;
};

namespace _impl {

template <typename traits, typename... modifiers>
struct _make_access_handle_traits;

template <typename traits, access_handle_permissions_t permissions, typename... modifiers>
struct _make_access_handle_traits<traits, min_scheduling_permissions<permissions>, modifiers...> {
  using type = typename _make_access_handle_traits<
    typename traits::template with_min_scheduling_permissions<permissions>::type,
    modifiers...
  >::type;
};

template <typename traits, access_handle_permissions_t permissions, typename... modifiers>
struct _make_access_handle_traits<traits, max_scheduling_permissions<permissions>, modifiers...> {
  using type = typename _make_access_handle_traits<
    typename traits::template with_max_scheduling_permissions<permissions>::type,
    modifiers...
  >::type;
};

template <typename traits, access_handle_permissions_t permissions, typename... modifiers>
struct _make_access_handle_traits<traits, min_immediate_permissions<permissions>, modifiers...> {
  using type = typename _make_access_handle_traits<
    typename traits::template with_min_immediate_permissions<permissions>::type,
    modifiers...
  >::type;
};

template <typename traits, access_handle_permissions_t permissions, typename... modifiers>
struct _make_access_handle_traits<traits, max_immediate_permissions<permissions>, modifiers...> {
  using type = typename _make_access_handle_traits<
    typename traits::template with_max_immediate_permissions<permissions>::type,
    modifiers...
  >::type;
};

template <typename traits>
struct _make_access_handle_traits<traits> {
  using type = traits;
};

} // end namespace _impl

template <typename... modifiers>
struct make_access_handle_traits {
  using type = typename
  _impl::_make_access_handle_traits<access_handle_traits<>, modifiers...>::type;
};

template <typename... modifiers>
using make_access_handle_traits_t = typename make_access_handle_traits<modifiers...>::type;

// </editor-fold>
//------------------------------------------------------------


} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_ACCESS_HANDLE_ACCESS_HANDLE_TRAITS_H
