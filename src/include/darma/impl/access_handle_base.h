/*
//@HEADER
// ************************************************************************
//
//                      access_handle_base.h
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

#ifndef DARMA_IMPL_ACCESS_HANDLE_BASE_H
#define DARMA_IMPL_ACCESS_HANDLE_BASE_H

#include <cstdlib> // std::size_t

#include <darma/impl/task/task_fwd.h> // TaskBase forward declaration
#include <darma/impl/create_work/create_if_then_fwd.h>
#include <darma/impl/create_work/create_work_while_fwd.h>

#include <darma/impl/use.h> // UseHolder
#include <darma/impl/task/task.h> // TaskBase
#include <darma/impl/create_work/capture_permissions.h>

// TODO move this to access_handle directory when we're at a stable merge point

namespace darma_runtime {
namespace detail {

struct AccessHandleBaseAttorney;

class AccessHandleBase : public CapturedObjectBase {
  public:

    using use_holder_base_ptr = UseHolderBase*;

    typedef enum CaptureOp {
      read_only_capture,
      write_only_capture,
      modify_capture,
      no_capture
    } capture_op_t;

    static constexpr HandleUse::permissions_t get_captured_permissions_for(
      capture_op_t op,
      frontend::permissions_t permissions
    ) {
      using darma_runtime::frontend::Permissions;
      // This used to be a lot more complicated when Commutative/Relaxed/etc
      // were permissions rather than coherence modes.  I've left it here
      // for backwards compatibility, but it may be moved at some point
      switch(op) {
        case CaptureOp::read_only_capture : {
          switch(permissions) {
            case Permissions::Modify :
            case Permissions::Read : {
              return Permissions::Read;
            }
            case Permissions::Write :
            case Permissions::Invalid : // to avoid compiler warnings...
            case Permissions::None : {
              // can't do a read_only_capture if you have the above scheduling
              // permissions
              return Permissions::Invalid;
            }
          }
        }
        case CaptureOp::write_only_capture : {
          switch(permissions) {
            case Permissions::Modify :
            case Permissions::Write : {
              return Permissions::Write;
            }
            case Permissions::None :
            case Permissions::Invalid : // to avoid compiler warnings...
            case Permissions::Read : {
              // can't do a write_only_capture if you have the above scheduling
              // permissions
              return Permissions::Invalid;
            }
          }
        }
        case CaptureOp::modify_capture : {
          switch(permissions) {
            case Permissions::Modify :
            case Permissions::Write :
            case Permissions::Read : {
              // capture the greatest permissions allowed
              return permissions;
            }
            case Permissions::Invalid : // to avoid compiler warnings...
            case Permissions::None : {
              // can't capture with no scheduling permissions
              return Permissions::Invalid;
            }
          }
        }
        case CaptureOp::no_capture : {
          // for completeness
          return Permissions::None;
        }
      }
    }

    // TODO remove this
    typedef enum CapturedAsInfo {
      Normal = 0,
      Ignored = 1,
      ReadOnly = 2,
      ScheduleOnly = 4,
      Leaf = 8
    } captured_as_info_t;


  protected:

    using task_t = detail::TaskBase;

    mutable unsigned captured_as_ = CapturedAsInfo::Normal;
    task_t* capturing_task = nullptr;
    std::size_t lambda_capture_unpack_index = 0;
    mutable use_holder_base_ptr current_use_base_ = nullptr;

    // This is ugly, but it works for now:
    std::shared_ptr<VariableHandleBase> var_handle_base_;

    friend class TaskBase;
    template <typename, typename, bool, typename, typename, bool, typename, typename, bool, bool>
    friend class darma_runtime::detail::IfThenElseCaptureManager;
    friend class ParsedCaptureOptions;
    template <typename, typename, bool, typename, typename, bool>
    friend struct WhileDoCaptureManager;
    template <typename, typename>
    friend class darma_runtime::AccessHandle;

    // Copy the concrete object instance
    virtual std::shared_ptr<AccessHandleBase> copy(bool check_context = true) const =0;

    virtual void call_make_captured_use_holder(
      std::shared_ptr<detail::VariableHandleBase> var_handle,
      detail::HandleUse::permissions_t req_sched,
      detail::HandleUse::permissions_t req_immed,
      detail::AccessHandleBase const& source,
      bool register_continuation_use
    ) =0;
    virtual void replace_use_holder_with(AccessHandleBase const&) =0;
    // really should be something like "release_current_use_holders"...
    virtual void release_current_use() const =0;
    virtual void call_add_dependency(
      detail::TaskBase* task
    );
    friend struct AccessHandleBaseAttorney;


    virtual ~AccessHandleBase() = default;

    template <typename, typename...>
    friend struct FunctorTask;

};


struct AccessHandleBaseAttorney {
  static PermissionsPair
  get_permissions_before_downgrades(
    AccessHandleBase const& ah,
    AccessHandleBase::capture_op_t schedule_capture_op,
    AccessHandleBase::capture_op_t immediate_capture_op
  ) {
    using frontend::Permissions;

    auto rv = PermissionsPair{
      (int)AccessHandleBase::get_captured_permissions_for(
        schedule_capture_op,
        ah.current_use_base_->use_base->scheduling_permissions_
      ),
      (int)AccessHandleBase::get_captured_permissions_for(
        immediate_capture_op,
        // Immediate also references the source's scheduling permissions,
        // since that's the maximum we're allowed to schedule
        ah.current_use_base_->use_base->scheduling_permissions_
      )
    };
    assert(rv.scheduling != (int)Permissions::Invalid);
    assert(rv.immediate != (int)Permissions::Invalid);
    return rv;
  }

};


} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_ACCESS_HANDLE_BASE_H
