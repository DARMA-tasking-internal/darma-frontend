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
#include <darma/impl/create_if_then_fwd.h>
#include <darma/impl/create_work_while_fwd.h>

#include <darma/impl/use.h> // UseHolder
#include <darma/impl/task/task.h> // TaskBase

// TODO move this to access_handle directory when we're at a stable merge point

namespace darma_runtime {
namespace detail {

class AccessHandleBase {
  public:

    using use_holder_base_ptr = UseHolderBase*;

    typedef enum CaptureOp {
      ro_capture,
      mod_capture
    } capture_op_t;

    // TODO figure out if this as efficient as a bitfield (it's definitely more readable)
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
    template <typename, typename, bool, typename, typename, bool, typename, typename, bool>
    friend class darma_runtime::detail::IfLambdaThenLambdaTask;
    friend class ParsedCaptureOptions;
    template <typename, typename, bool, typename, typename, bool, bool, bool>
    friend class WhileDoTask;
    template <typename, typename>
    friend class darma_runtime::AccessHandle;

    // Copy the concrete object instance
    virtual std::shared_ptr<AccessHandleBase> copy(bool check_context = true) const =0;

    virtual void call_make_captured_use_holder(
      std::shared_ptr<detail::VariableHandleBase> var_handle,
      detail::HandleUse::permissions_t req_sched,
      detail::HandleUse::permissions_t req_immed,
      detail::AccessHandleBase const& source
    ) =0;
    virtual void replace_use_holder_with(AccessHandleBase const&) =0;
    // really should be something like "release_current_use_holders"...
    virtual void release_current_use() const =0;
    virtual void call_add_dependency(
      detail::TaskBase* task
    );

    virtual ~AccessHandleBase() = default;

    template <typename, typename...>
    friend struct FunctorTask;

};

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_ACCESS_HANDLE_BASE_H
