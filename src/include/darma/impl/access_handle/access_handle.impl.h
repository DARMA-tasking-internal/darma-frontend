/*
//@HEADER
// ************************************************************************
//
//                      access_handle.impl.h
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

#ifndef DARMAFRONTEND_ACCESS_HANDLE_IMPL_H
#define DARMAFRONTEND_ACCESS_HANDLE_IMPL_H

#include <darma/interface/app/access_handle.h>

namespace darma_runtime {

template <typename T, typename Traits>
AccessHandle<T, Traits>::AccessHandle(AccessHandle<T, Traits> const& copied_from)
  : current_use_(current_use_base_)
{

  auto result = this->copy_capture_handler_t::handle_copy_construct(
    copied_from
  );

  if(not result.did_capture and not result.argument_is_garbage) {
    // then we need to propagate stuff here, since no capture handler was invoked
    var_handle_ = copied_from.var_handle_;
    var_handle_base_ = var_handle_;
    other_private_members_ = copied_from.other_private_members_;
    current_use_ = copied_from.current_use_;
  }


//  auto* running_task = static_cast<detail::TaskBase* const>(
//    abstract::backend::get_backend_context()->get_running_task()
//  );
//
//  if (running_task != nullptr) {
//    capturing_task = running_task->current_create_work_context;
//  } else {
//    capturing_task = nullptr;
//  }
//
//  // TODO mark this unlikely?
//  if(capturing_task != nullptr
//    && capturing_task->lambda_serdes_mode != serialization::detail::SerializerMode::None
//    ) {
//    serialization::SimplePackUnpackArchive ar;
//    if(capturing_task->lambda_serdes_mode == serialization::detail::SerializerMode::Sizing) {
//      darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess
//      ::start_sizing(ar);
//      serialization::detail::serializability_traits<AccessHandle>::compute_size(copied_from, ar);
//      capturing_task->lambda_serdes_computed_size +=
//        darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess::get_size(ar);
//    }
//    else if(capturing_task->lambda_serdes_mode == serialization::detail::SerializerMode::Packing) {
//      darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess
//      ::start_packing_with_buffer(ar, capturing_task->lambda_serdes_buffer);
//      serialization::detail::serializability_traits<AccessHandle>::pack(copied_from, ar);
//    }
//    else {
//      // NOTE THAT IN THIS CASE, copied_from IS GARBAGE!!!
//      assert(capturing_task->lambda_serdes_mode == serialization::detail::SerializerMode::Unpacking);
//      darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess
//      ::start_unpacking_with_buffer(ar, capturing_task->lambda_serdes_buffer);
//      unpack_from_archive(ar);
//      capturing_task->add_dependency(*current_use_base_->use_base);
//    }
//    // now advance the buffer for the next user
//    if(capturing_task->lambda_serdes_mode != serialization::detail::SerializerMode::Sizing) {
//      capturing_task->lambda_serdes_buffer = static_cast<char*>(
//        darma_runtime::serialization::detail
//        ::DependencyHandle_attorneys::ArchiveAccess::get_spot(ar)
//      );
//    }
//  }
//  else {
//    other_private_members_ = copied_from.other_private_members_;
//    var_handle_ = copied_from.var_handle_;
//    var_handle_base_ = var_handle_;
//
//    // Now check if we're in a capturing context:
//    if (capturing_task != nullptr) {
//      AccessHandle const* source = &copied_from;
//      if (capturing_task->is_double_copy_capture) {
//        assert(copied_from.prev_copied_from() != nullptr);
//        source = copied_from.prev_copied_from();
//        copied_from.current_use_ = nullptr;
//        copied_from.current_use_base_ = nullptr;
//      }
//
//      capturing_task->do_capture(*this, *source);
//
//      if (source->current_use_) {
//        source->current_use_->use_base->already_captured = true;
//        // TODO this flag should be on the AccessHandleBase itself
//        capturing_task->uses_to_unmark_already_captured.insert(
//          source->current_use_->use_base
//        );
//      }
//    } // end if capturing_task != nullptr
//    else {
//      current_use_ = copied_from.current_use_;
//      // Also, save prev copied from in case this is a double capture, like in
//      // create_condition.  This is the only time that the prev_copied_from ptr
//      // should be valid (i.e., when task->is_double_copy_capture is set to true)
//      prev_copied_from() = &copied_from;
//    }
//  }
}

} // end namespace darma_runtime


#endif //DARMAFRONTEND_ACCESS_HANDLE_IMPL_H
