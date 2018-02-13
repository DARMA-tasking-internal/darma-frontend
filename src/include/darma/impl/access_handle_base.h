/*
//@HEADER
// ************************************************************************
//
//                      access_handle_base.h
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

#ifndef DARMA_IMPL_ACCESS_HANDLE_BASE_H
#define DARMA_IMPL_ACCESS_HANDLE_BASE_H

#include <cstdlib> // std::size_t

#include <darma/impl/task/task_fwd.h> // TaskBase forward declaration
#include <darma/impl/create_work/create_if_then_fwd.h>
#include <darma/impl/create_work/create_work_while_fwd.h>

#include <darma/impl/use.h> // UseHolder
#include <darma/impl/task/task.h> // TaskBase
#include <darma/impl/create_work/capture_permissions.h> // CapturedObjectBase
#include <darma/impl/use/use_holder.h>

// TODO move this to access_handle directory when we're at a stable merge point

namespace darma_runtime {
namespace detail {

struct AccessHandleBaseAttorney;

class AccessHandleBase
  : public CapturedObjectBase
{
  protected:

    using task_t = detail::TaskBase;


    // TODO this should be a free function
    static constexpr
    HandleUse::permissions_t
    get_captured_permissions_for(
      capture_op_t op,
      frontend::permissions_t permissions
    );

  protected:

    //------------------------------------------------------------------------------
    // <editor-fold desc="protected ctors"> {{{2

    AccessHandleBase()
    { }

    AccessHandleBase(AccessHandleBase const& other)
      : CapturedObjectBase(other),
        var_handle_base_(other.var_handle_base_),
        current_use_base_(other.current_use_base_)
    { }

    AccessHandleBase(AccessHandleBase&& other) noexcept
      : CapturedObjectBase(std::move(other)),
        var_handle_base_(std::move(other.var_handle_base_)),
        current_use_base_(std::move(other.current_use_base_))
    { }

    AccessHandleBase(
      std::shared_ptr<VariableHandleBase> const& var_handle,
      std::shared_ptr<UseHolderBase> const& use_holder
    ) : CapturedObjectBase(),
        var_handle_base_(var_handle),
        current_use_base_(use_holder)
    { }

    // </editor-fold> end protected ctors }}}2
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    // <editor-fold desc="assignment operator implementation"> {{{2

    void _do_assignment(AccessHandleBase&& other) {
      var_handle_base_ = std::move(other.var_handle_base_);
      current_use_base_ = std::move(other.current_use_base_);
    }

    void _do_assignment(AccessHandleBase const& other) {
      var_handle_base_ = other.var_handle_base_;
      current_use_base_ = other.current_use_base_;
    }


    // </editor-fold> end assignment operator implementation }}}2
    //------------------------------------------------------------------------------


    //------------------------------------------------------------------------------
    // <editor-fold desc="protected data members"> {{{2

    task_t* capturing_task = nullptr;
    std::size_t lambda_capture_unpack_index = 0;
    mutable std::shared_ptr<UseHolderBase> current_use_base_ = nullptr;
    std::shared_ptr<VariableHandleBase> var_handle_base_ = nullptr;

    // </editor-fold> end protected data members }}}2
    //------------------------------------------------------------------------------


    //------------------------------------------------------------------------------
    // <editor-fold desc="friends"> {{{2

    friend class TaskBase;

    template <typename, typename, bool, typename, typename, bool, typename, typename, bool, bool>
    friend class darma_runtime::detail::IfThenElseCaptureManager;

    template <typename, typename, bool, typename, typename, bool>
    friend struct WhileDoCaptureManager;

    template <typename, typename>
    friend class darma_runtime::AccessHandle;

    friend struct AccessHandleBaseAttorney;

    template <typename, typename>
    friend struct AccessHandleCaptureDescription;

    template <typename, typename...>
    friend struct FunctorTask;

    // </editor-fold> end friends }}}2
    //------------------------------------------------------------------------------


    //------------------------------------------------------------------------------
    // <editor-fold desc="virtual methods"> {{{2

    // Copy the concrete object instance
    virtual std::shared_ptr<AccessHandleBase>
    copy() const =0;

//    virtual void
//    call_make_captured_use_holder(
//      std::shared_ptr<detail::VariableHandleBase> var_handle,
//      detail::HandleUse::permissions_t req_sched,
//      detail::HandleUse::permissions_t req_immed,
//      detail::AccessHandleBase const& source,
//      bool register_continuation_use
//    ) =0;

    // really should be something like "release_current_use_holders"...
    virtual void release_current_use() const =0;

//    virtual void call_add_dependency(
//      detail::TaskBase* task
//    );

    // </editor-fold> end virtual methods }}}2
    //------------------------------------------------------------------------------


    virtual ~AccessHandleBase() = default;

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
    assert(rv.scheduling != (int)Permissions::_invalid);
    assert(rv.immediate != (int)Permissions::_invalid);
    return rv;
  }

};


} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_ACCESS_HANDLE_BASE_H
