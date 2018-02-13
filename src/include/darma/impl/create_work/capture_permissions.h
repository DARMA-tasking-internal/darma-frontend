/*
//@HEADER
// ************************************************************************
//
//                      capture_permissions.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_IMPL_CREATE_WORK_CAPTURE_PERMISSIONS_H
#define DARMAFRONTEND_IMPL_CREATE_WORK_CAPTURE_PERMISSIONS_H

#include <cstdint>
#include <darma/interface/frontend/use.h>

namespace darma_runtime {
namespace detail {

struct CapturedObjectAttorney;
class CaptureDescriptionBase;

class CapturedObjectBase {

  public:

    struct CapturedAsInfo {
      bool ReadOnly : 1;
      bool ScheduleOnly : 1;
      bool Leaf : 1;
      bool Ignored : 1; // currently unused
      bool WriteOnly : 1; // currently unused
      bool Commutative : 1;
      bool Reduce : 1;
      bool Relaxed : 1;
      bool TransfersImmediateRead : 1; // currently unused
      bool TransfersImmediateModify : 1; // currently unused
      bool TransfersSchedulingRead : 1; // currently unused
      bool TransfersSchedulingModify : 1; // currently unused
      int _unused_padding : 20;
    };

    typedef enum CaptureOp {
      read_only_capture,
      write_only_capture,
      modify_capture,
      no_capture
    } capture_op_t;

  protected:

    using captured_as_info_as_int_t = std::uint32_t;

    static_assert(
      sizeof(CapturedAsInfo) == sizeof(captured_as_info_as_int_t),
      "Size mismatch in CapturedAsInfo"
    );

    union captured_as_info_t {
      captured_as_info_as_int_t as_int;
      CapturedAsInfo info;
    };

    // Must be mutable because it might be changed during copy construction
    // or inside of a lambda where it was captured as const (by default, since
    // user lambdas won't be marked as mutable)
    mutable captured_as_info_t captured_as;

    CapturedObjectBase() {
      // Should be a no-op in most compilers, but we need to do this to avoid
      // U.B. caused by reading from captured_as.info after no changes are made
      clear_capture_state();
    }

    void clear_capture_state() const {
      captured_as.as_int = 0;
      // switch the union back to info mode (to avoid undefined behavior when
      // reading it if nothing gets set; should compile to a no-op if the
      // compiler is smart enough)
      captured_as.info.ReadOnly = false;
    }

    virtual std::unique_ptr<CaptureDescriptionBase>
    get_capture_description(
      CapturedObjectBase& captured,
      capture_op_t schedule_capture_op,
      capture_op_t immediate_capture_op
    ) const =0;

    friend struct CapturedObjectAttorney;


};

/// Return value for functions that modify permissions
struct PermissionsPair {
  int scheduling;
  int immediate;
};


struct DeferredPermissionsModificationsBase {
  virtual void do_permissions_modifications() = 0;
  virtual ~DeferredPermissionsModificationsBase() = default;
};

template <typename Callable, typename... Args>
struct DeferredPermissionsModifications : DeferredPermissionsModificationsBase
{
  public:

    template <
      typename CapturedObjectBaseT,
      typename CallableT
    >
    int
    apply_permissions_modifications(
      CapturedObjectBaseT&& obj,
      CallableT const& callable,
      std::enable_if_t<
        std::is_base_of<CapturedObjectBase, std::decay_t<CapturedObjectBaseT>>::value,
        utility::_not_a_type
      > /*unused*/ = { }
    ) {
      callable(std::forward<CapturedObjectBaseT>(obj));
      return 0;
    }

    template <
      typename DeferredPermissionsModificationsT,
      typename CallableT
    >
    int
    apply_permissions_modifications(
      DeferredPermissionsModificationsT&& obj,
      CallableT const& callable,
      std::enable_if_t<
        std::is_base_of<
          DeferredPermissionsModificationsBase,
          std::decay_t<DeferredPermissionsModificationsT>
        >::value,
        utility::_not_a_type
      > /*unused*/ = { }
    ) {
      obj.apply_permissions_modifications(callable);
      // Don't forget to invoke the nested modifications as well:
      obj.do_permissions_modifications();
      return 0;
    }


  private:

    Callable callable_;
    std::tuple<Args&&...> arg_refs_tuple_;

    template <
      typename CallableT,
      size_t... Idxs
    >
    void _apply_permissions_modifications_impl(
      std::integer_sequence<size_t, Idxs...>,
      CallableT const& callable
    ) {
      std::make_tuple( // fold emulation
        apply_permissions_modifications(
          std::get<Idxs>(arg_refs_tuple_),
          callable
        )...
      );

    }

  public:

    DeferredPermissionsModifications(DeferredPermissionsModifications const&) = delete;
    DeferredPermissionsModifications(DeferredPermissionsModifications&&) noexcept = default;
    ~DeferredPermissionsModifications() override = default;

    template <
      typename CallableT
    >
    void apply_permissions_modifications(
      CallableT const& callable
    ) {
      _apply_permissions_modifications_impl(
        std::index_sequence_for<Args...>{},
        callable
      );
    }

    template <typename CallableDeduced, typename... ArgsDeduced>
    DeferredPermissionsModifications(
      utility::variadic_constructor_tag_t /*unused*/, // prevent generation of move and copy ctors via this template
      CallableDeduced&& callable,
      ArgsDeduced&&... args_in
    ) : callable_(std::forward<CallableDeduced>(callable)),
        arg_refs_tuple_(std::forward<ArgsDeduced>(args_in)...)
    { }

    void do_permissions_modifications() override {
      apply_permissions_modifications(callable_);
    }

};

template <typename Callable, typename... PassThroughArgs>
auto
make_deferred_permissions_modifications(
  Callable&& callable, PassThroughArgs&&... pass_through
) {
  return DeferredPermissionsModifications<
    std::decay_t<Callable>, PassThroughArgs...
  >(
    utility::variadic_constructor_tag,
    std::forward<Callable>(callable),
    std::forward<PassThroughArgs>(pass_through)...
  );
}

struct CapturedObjectAttorney {


  static CapturedObjectBase::CapturedAsInfo&
  captured_as_info(CapturedObjectBase const& obj) {
    return obj.captured_as.info;
  }

  static void
  clear_capture_state(CapturedObjectBase const& obj) {
    obj.clear_capture_state();
  }

  static PermissionsPair
  get_and_clear_requested_capture_permissions(
    CapturedObjectBase const& obj,
    int default_scheduling,
    int default_immediate
  ) {

    using frontend::Permissions;

    PermissionsPair rv { default_scheduling, default_immediate };
    if(obj.captured_as.info.ReadOnly) {
      // Turn off the Write bit, if it's on
      rv.scheduling &= ~(int)(Permissions::Write);
      rv.immediate &= ~(int)(Permissions::Write);
    }

    if(obj.captured_as.info.WriteOnly) {
      // Turn off the Read bit, if it's on
      rv.scheduling &= ~(int)(Permissions::Read);
      rv.immediate &= ~(int)(Permissions::Read);
    }

    if(obj.captured_as.info.ScheduleOnly) {
      rv.immediate = (int)Permissions::None;
    }

    if(obj.captured_as.info.Leaf) {
      rv.scheduling = (int)Permissions::None;
    }

    clear_capture_state(obj);

    return rv;
  }

  static
  std::unique_ptr<CaptureDescriptionBase>
  get_capture_description(
    CapturedObjectBase const& obj,
    CapturedObjectBase& captured,
    CapturedObjectBase::capture_op_t schedule_capture_op,
    CapturedObjectBase::capture_op_t immediate_capture_op
  ) {
    return obj.get_capture_description(
      captured, schedule_capture_op, immediate_capture_op
    );
  }



};

class CaptureDescriptionBase {
  protected:

    frontend::permissions_t scheduling_permissions_;
    frontend::permissions_t immediate_permissions_;

    CaptureDescriptionBase(
      frontend::permissions_t scheduling_permissions,
      frontend::permissions_t immediate_permissions
    ) : scheduling_permissions_(scheduling_permissions),
        immediate_permissions_(immediate_permissions)
    { }

  public:
    virtual ~CaptureDescriptionBase(){}

    virtual void
    replace_source_pointer(CapturedObjectBase const*) =0;

    virtual void
    replace_captured_pointer(CapturedObjectBase*) =0;

    virtual void
    require_ability_to_schedule(CaptureDescriptionBase const& other) =0;

    virtual void
    invoke_capture(TaskBase*) =0;

    virtual CapturedObjectBase const* get_source_pointer() =0;

    virtual CapturedObjectBase* get_captured_pointer() =0;

};

} // end namespace detail

template <typename... Args>
auto
reads(Args&&... args) {
  return darma_runtime::detail::make_deferred_permissions_modifications(
    [&](auto const& arg){
      detail::CapturedObjectAttorney::captured_as_info(
        std::forward<decltype(arg)>(arg)
      ).ReadOnly = true;
    },
    std::forward<Args>(args)...
  );
}

template <typename... Args>
auto
schedule_only(Args&&... args) {
  return darma_runtime::detail::make_deferred_permissions_modifications(
    [&](auto const& arg){
      detail::CapturedObjectAttorney::captured_as_info(
        std::forward<decltype(arg)>(arg)
      ).ScheduleOnly = true;
    },
    std::forward<Args>(args)...
  );
}

} // end namespace darma_runtime

#endif //DARMAFRONTEND_IMPL_CREATE_WORK_CAPTURE_PERMISSIONS_H
