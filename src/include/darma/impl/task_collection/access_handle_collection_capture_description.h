/*
//@HEADER
// ************************************************************************
//
//                      access_handle_collection_capture_description.h
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

#ifndef DARMAFRONTEND_ACCESS_HANDLE_COLLECTION_CAPTURE_DESCRIPTION_H
#define DARMAFRONTEND_ACCESS_HANDLE_COLLECTION_CAPTURE_DESCRIPTION_H

#include <darma/impl/create_work/capture_permissions.h>

#include <darma/impl/task_collection/access_handle_collection.h>


namespace darma_runtime {
namespace detail {

template <typename SourceAccessHandleCollectionT, typename CapturedAccessHandleCollectionT>
struct AccessHandleCollectionCaptureDescription
  : AccessHandleCaptureDescriptionBase<SourceAccessHandleCollectionT, CapturedAccessHandleCollectionT>
{
  protected:

    using base_t = AccessHandleCaptureDescriptionBase<
      SourceAccessHandleCollectionT, CapturedAccessHandleCollectionT
    >;

  public:

    using base_t::base_t;

    void invoke_capture(TaskBase* task) override {
      if(this->source_->mapped_backend_index_ == this->source_->unknown_backend_index) {
        //captured_->current_use_ = detail::make_captured_use_holder(
        //  captured_->var_handle_base_,>
        //  scheduling_permissions_,
        //  immediate_permissions_,
        //  *source_,
        //  true
        //  /* TODO !!! propagate IndexRange */
        //);
        // For now...
        this->captured_->current_use_ = this->captured_->_call_make_captured_use_holder_impl(
          this->captured_->var_handle_base_,
          (frontend::permissions_t)std::max(
            (int)this->scheduling_permissions_, (int)this->immediate_permissions_
          ),
          frontend::Permissions::None,
          *this->source_,
          true // TODO don't always register continuation use
        );
        task->add_dependency(*this->captured_->current_use_base_->use_base);
      }
      else {
        for(auto&& local_holder_pair : this->source_->local_use_holders_) {
          this->captured_->local_use_holders_[local_holder_pair.first] = detail::make_captured_use_holder(
            this->captured_->var_handle_base_,
            this->scheduling_permissions_,
            this->immediate_permissions_,
            local_holder_pair.second.get(),
            true // TODO don't always register continuation use
          );
          task->add_dependency(*this->captured_->local_use_holders_[local_holder_pair.first]->use.get());
        }
        // Now create a captured version of the collection use
        this->captured_->current_use_ = this->captured_->_call_make_captured_use_holder_impl(
          this->captured_->var_handle_base_,
          this->scheduling_permissions_,
          this->immediate_permissions_,
          *this->source_,
          true // TODO don't always register continuation use
        );
        task->add_dependency(*this->captured_->current_use_->use.get());
      }
    }
};


} // end namespace detail


template <typename T, typename IndexRangeT, typename Traits>
std::unique_ptr<detail::CaptureDescriptionBase>
AccessHandleCollection<T, IndexRangeT, Traits>::get_capture_description(
  detail::CapturedObjectBase& captured_in,
  capture_op_t schedule_capture_op,
  capture_op_t immediate_capture_op
) const {

  auto* captured_ptr = detail::safe_static_cast<AccessHandleCollection<T, IndexRangeT, Traits>*>(
    &captured_in
  );

  // This was the old pattern, which we'll use here for now:
  auto init_perm_pair = detail::AccessHandleBaseAttorney::get_permissions_before_downgrades(
    *this, schedule_capture_op, immediate_capture_op
  );

  auto perm_pair = detail::CapturedObjectAttorney::get_and_clear_requested_capture_permissions(
    *this, init_perm_pair.scheduling, init_perm_pair.immediate
  );
  return std::make_unique<detail::AccessHandleCollectionCaptureDescription<
    AccessHandleCollection<T, IndexRangeT, Traits>,
    AccessHandleCollection<T, IndexRangeT, Traits>
  > >(
    this, captured_ptr,
    (frontend::permissions_t)perm_pair.scheduling,
    (frontend::permissions_t)perm_pair.immediate
  );


}


} // end namespace darma_runtime


#endif //DARMAFRONTEND_ACCESS_HANDLE_COLLECTION_CAPTURE_DESCRIPTION_H
