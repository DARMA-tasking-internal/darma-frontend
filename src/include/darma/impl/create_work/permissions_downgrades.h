/*
//@HEADER
// ************************************************************************
//
//                      permissions_downgrades.h
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

#ifndef DARMAFRONTEND_PERMISSIONS_DOWNGRADES_H
#define DARMAFRONTEND_PERMISSIONS_DOWNGRADES_H

#include <darma/impl/handle_fwd.h>
#include <darma/impl/handle_attorneys.h>

#include <darma/interface/app/access_handle.h> // is_access_handle

namespace darma_runtime {

namespace detail {

template <AccessHandlePermissions SchedulingDowngrade>
struct _scheduling_permissions_downgrade_handler;
template <AccessHandlePermissions ImmediateDowngrade>
struct _immediate_permissions_downgrade_handler;

// Handle the case where no known scheduling downgrade flag exists
template <>
struct _scheduling_permissions_downgrade_handler<AccessHandlePermissions::NotGiven> {
  template <typename AccessHandleT>
  void apply_flag(AccessHandleT const&) const { }
  HandleUse::permissions_t get_permissions(
    HandleUse::permissions_t default_perms
  ) const {
    return default_perms;
  }
};

// Handle the case where no known immediate downgrade flag exists
template <>
struct _immediate_permissions_downgrade_handler<AccessHandlePermissions::NotGiven> {
  template <typename AccessHandleT>
  void apply_flag(AccessHandleT const&) const { }
  HandleUse::permissions_t get_permissions(
    HandleUse::permissions_t default_perms
  ) const {
    return default_perms;
  }
};

// Apply the "ScheduleOnly" flag
template<>
struct _immediate_permissions_downgrade_handler<AccessHandlePermissions::None> {
  template <typename AccessHandleT>
  void apply_flag(AccessHandleT const& ah) const {
    detail::create_work_attorneys::for_AccessHandle::captured_as_info(ah) |=
      detail::AccessHandleBase::ScheduleOnly;
  }
  HandleUse::permissions_t get_permissions(
    HandleUse::permissions_t
  ) const {
    return frontend::Permissions::None;
  }
};

// Apply the "ReadOnly" flag
template<>
struct _immediate_permissions_downgrade_handler<AccessHandlePermissions::Read>{
  template <typename AccessHandleT>
  void apply_flag(AccessHandleT const& ah) const {
    detail::create_work_attorneys::for_AccessHandle::captured_as_info(ah) |=
      detail::AccessHandleBase::ReadOnly;
  }
  HandleUse::permissions_t get_permissions(
    HandleUse::permissions_t
  ) const {
    return frontend::Permissions::Read;
  }
};

// Apply the "ReadOnly" flag (scheduling downgrade version)
template<>
struct _scheduling_permissions_downgrade_handler<AccessHandlePermissions::Read>{
  template <typename AccessHandleT>
  void apply_flag(AccessHandleT const& ah) const {
    detail::create_work_attorneys::for_AccessHandle::captured_as_info(ah) |=
      detail::AccessHandleBase::ReadOnly;
  }
  HandleUse::permissions_t get_permissions(
    HandleUse::permissions_t
  ) const {
    return frontend::Permissions::Read;
  }
};

template <
  typename AccessHandleT,
  AccessHandlePermissions SchedulingDowngrade /*= AccessHandlePermissions::NotGiven*/,
  AccessHandlePermissions ImmediateDowngrade /*= AccessHandlePermissions::NotGiven*/
>
struct PermissionsDowngradeDescription {
  AccessHandleT& handle;
  static constexpr auto scheduling_downgrade = SchedulingDowngrade;
  static constexpr auto immediate_downgrade = ImmediateDowngrade;
  explicit PermissionsDowngradeDescription(AccessHandleT& handle_in)
    : handle(handle_in)
  {
    _scheduling_permissions_downgrade_handler<SchedulingDowngrade>{}.apply_flag(
      handle
    );
    _immediate_permissions_downgrade_handler<ImmediateDowngrade>{}.apply_flag(
      handle
    );
  }

  HandleUse::permissions_t
  get_requested_scheduling_permissions(
    HandleUse::permissions_t default_permissions
  ) const {
    return _scheduling_permissions_downgrade_handler<
      SchedulingDowngrade
    >{}.get_permissions(
      default_permissions
    );
  }

  HandleUse::permissions_t
  get_requested_immediate_permissions(
    HandleUse::permissions_t default_permissions
  ) const {
    return _immediate_permissions_downgrade_handler<
      ImmediateDowngrade
    >{}.get_permissions(
      default_permissions
    );
  }
};

template <
  AccessHandlePermissions SchedulingDowngrade,
  AccessHandlePermissions ImmediateDowngrade,
  typename AccessHandleT,
  typename = std::enable_if_t<
    is_access_handle<std::decay_t<AccessHandleT>>::value
  >
>
auto
make_permissions_downgrade_description(AccessHandleT& handle) {
  return PermissionsDowngradeDescription<
    AccessHandleT, SchedulingDowngrade, ImmediateDowngrade
  >(handle);
};

template <
  AccessHandlePermissions SchedulingDowngrade,
  AccessHandlePermissions ImmediateDowngrade,
  typename AccessHandleT,
  AccessHandlePermissions OldSchedDowngrade,
  AccessHandlePermissions OldImmedDowngrade
>
auto
make_permissions_downgrade_description(
  PermissionsDowngradeDescription<
    AccessHandleT, OldSchedDowngrade, OldImmedDowngrade
  >&& downgrade_holder
) {
  return PermissionsDowngradeDescription<
    AccessHandleT,
    OldSchedDowngrade == AccessHandlePermissions::NotGiven ?
      SchedulingDowngrade : (
      SchedulingDowngrade == AccessHandlePermissions::NotGiven ?
        OldSchedDowngrade : (
        OldSchedDowngrade < SchedulingDowngrade ?
          OldSchedDowngrade : SchedulingDowngrade
      )
    ),
    OldImmedDowngrade == AccessHandlePermissions::NotGiven ?
      ImmediateDowngrade : (
      ImmediateDowngrade == AccessHandlePermissions::NotGiven ?
        OldImmedDowngrade : (
        OldImmedDowngrade < ImmediateDowngrade ?
          OldImmedDowngrade : ImmediateDowngrade
      )
    )
  >(std::move(downgrade_holder).handle);
};

template <
  AccessHandlePermissions SchedulingDowngrade,
  AccessHandlePermissions ImmediateDowngrade,
  typename... Args, size_t... Idxs
>
auto
_make_permissions_downgrade_description_tuple_helper(
  std::tuple<Args...>&& tup,
  std::integer_sequence<size_t, Idxs...>
) {
  return std::make_tuple(
    make_permissions_downgrade_description<
      SchedulingDowngrade, ImmediateDowngrade
    >(
      std::get<Idxs>(std::move(tup))
    )...
  );
};

template <
  AccessHandlePermissions SchedulingDowngrade,
  AccessHandlePermissions ImmediateDowngrade,
  typename... Args
>
auto
make_permissions_downgrade_description(
  std::tuple<Args...>&& tup
) {
  return darma_runtime::detail::_make_permissions_downgrade_description_tuple_helper<
    SchedulingDowngrade, ImmediateDowngrade
  >(
    std::move(tup), std::index_sequence_for<Args...>{}
  );
};

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMAFRONTEND_PERMISSIONS_DOWNGRADES_H
