/*
//@HEADER
// ************************************************************************
//
//                          task_capture_impl.h
//                         darma_new
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

#ifndef DARMA_TASK_CAPTURE_IMPL_H_H
#define DARMA_TASK_CAPTURE_IMPL_H_H

#include <darma/impl/task/task.h>
#include <darma/impl/handle.h>
#include <darma/impl/util/smart_pointers.h>

#include <darma/impl/flow_handling.h>

#include <thread>

#include "darma/impl/use.h"
#include "darma/impl/capture.h"

namespace darma_runtime {

namespace detail {

inline bool
TaskBase::do_capture_checks(
  AccessHandleBase const& source_and_continuing
) {

  DARMA_ASSERT_MESSAGE(
    source_and_continuing.current_use_base_ != nullptr,
    "Can't capture handle after it was released or before it was initialized"
  );

  DARMA_ASSERT_MESSAGE(
    source_and_continuing.current_use_base_->use_base->scheduling_permissions_ != frontend::Permissions::None,
    "Can't do a capture of an AccessHandle with scheduling permissions of None"
  );

  bool check_aliasing = source_and_continuing.current_use_base_->use_base->already_captured;

  if(check_aliasing) {
    if(allowed_aliasing and allowed_aliasing->aliasing_is_allowed_for(
      source_and_continuing, this
    )) {
      // Short-circuit rather than capturing twice...
      // TODO get the maximum permissions and pass those on
      return true; // trigger the short circuit
    }
    else {
      // Unallowed alias...
      //DARMA_ASSERT_FAILURE(
      //  "Captured the same handle (with key = " << source_and_continuing.get_key()
      //  << ") via different variables without explicitly allowing aliasing in"
      //    " create_work call (using the keyword_arguments_for_task_creation::allow_aliasing"
      //    " keyword argument)."
      //);
      // TODO (@minor) reinstate retrieval of key in error message?
      DARMA_ASSERT_FAILURE(
        "Captured the same handle via different variables without explicitly"
          " allowing aliasing in create_work call (using the"
          " keyword_arguments_for_task_creation::allow_aliasing keyword argument)."
      );
    }
  }


  return false;

}

inline void
TaskBase::do_capture(
  AccessHandleBase& captured,
  AccessHandleBase const& source_and_continuing
) {

  if(do_capture_checks(source_and_continuing)) return;

  CapturedObjectAttorney::get_capture_description(
    source_and_continuing, captured,
    scheduling_capture_op, immediate_capture_op
  )->invoke_capture(this);

}

} // end namespace detail

} // end namespace darma_runtime


#include <darma/impl/create_work/create_work_argument_parser.h>

namespace darma_runtime {
namespace detail {

template <
  typename... RemainingArgs
>
TaskBase::TaskBase(
  TaskBase* parent_task, variadic_arguments_begin_tag, RemainingArgs&& ... args
) {
  setup_create_work_argument_parser()
    .parse_args(std::forward<RemainingArgs>(args)...)
    .invoke([&](
        darma_runtime::types::key_t name_key,
        auto&& allow_aliasing_desc,
        bool data_parallel,
        darma_runtime::detail::variadic_arguments_begin_tag,
        auto&&... deferred_permissions_modifications
      ) {
        this->allowed_aliasing = std::forward<decltype(allow_aliasing_desc)>(allow_aliasing_desc);
        this->is_data_parallel_task_ = data_parallel;
        this->name_ = name_key;
        std::make_tuple( // only for fold emulation
          (deferred_permissions_modifications.do_permissions_modifications()
            , 0)... // fold expression emulation for void return using comma operator
        );
      }
    );

}

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_TASK_CAPTURE_IMPL_H_H
