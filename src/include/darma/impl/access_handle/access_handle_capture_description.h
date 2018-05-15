/*
//@HEADER
// ************************************************************************
//
//                      access_handle_capture_description.h
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

#ifndef DARMAFRONTEND_ACCESS_HANDLE_CAPTURE_DESCRIPTION_H
#define DARMAFRONTEND_ACCESS_HANDLE_CAPTURE_DESCRIPTION_H

#include <darma/impl/create_work/capture_permissions.h>
#include <darma/interface/app/access_handle.h>

namespace darma {
namespace detail {

template <typename SourceAccessHandleT, typename CapturedAccessHandleT>
struct AccessHandleCaptureDescriptionBase
  : CaptureDescriptionBase
{
  protected:

    SourceAccessHandleT const* source_;
    CapturedAccessHandleT* captured_;

  public:

    AccessHandleCaptureDescriptionBase(
      SourceAccessHandleT const* source,
      CapturedAccessHandleT* captured,
      frontend::permissions_t scheduling_permissions,
      frontend::permissions_t immediate_permissions
    ) : CaptureDescriptionBase(scheduling_permissions, immediate_permissions),
        source_(source),
        captured_(captured)
    { }

    CapturedObjectBase const* get_source_pointer() override {
      return source_;
    }

    CapturedObjectBase* get_captured_pointer() override {
      return captured_;
    }

    void replace_source_pointer(CapturedObjectBase const* new_src) override {
      auto* new_src_cast = utility::safe_static_cast<SourceAccessHandleT const*>(
        new_src
      );
      source_ = new_src_cast;
    }

    void replace_captured_pointer(CapturedObjectBase* new_capt) override {
      auto* new_capt_cast = utility::safe_static_cast<CapturedAccessHandleT*>(
        new_capt
      );
      captured_ = new_capt_cast;
    }

    void
    require_ability_to_schedule(CaptureDescriptionBase const& other) override {
      // For now, assume that they have to have the same source type and a
      // generic captured type (this works with the patterns we have implemented
      // so far, but might be generalized in the future.
      auto const* other_cast = utility::safe_static_cast<AccessHandleCaptureDescriptionBase const*>(
        &other
      );

      scheduling_permissions_ = frontend::permissions_t(
        (int)other_cast->scheduling_permissions_ | (int)this->scheduling_permissions_
      );
      scheduling_permissions_ = frontend::permissions_t(
        (int)other_cast->immediate_permissions_ | (int)this->scheduling_permissions_
      );
    }
};

template <typename SourceAccessHandleT, typename CapturedAccessHandleT>
struct AccessHandleCaptureDescription
  : AccessHandleCaptureDescriptionBase<SourceAccessHandleT, CapturedAccessHandleT>
{

  using base_t = AccessHandleCaptureDescriptionBase<SourceAccessHandleT, CapturedAccessHandleT>;

  using base_t::base_t;

  void invoke_capture(TaskBase* task) override {
    // This was the old pattern; keep it for now until we get the code moved over
    assert(this->captured_);
    this->captured_->set_current_use(detail::make_captured_use_holder(
      this->captured_->var_handle_base_,
      this->scheduling_permissions_,
      this->immediate_permissions_,
      this->source_->get_current_use()
    ));
    task->add_dependency(*this->captured_->current_use_base_->use_base);
  }
};

} // end namespace detail


template <typename T, typename Traits>
std::unique_ptr<detail::CaptureDescriptionBase>
AccessHandle<T, Traits>::get_capture_description(
  detail::CapturedObjectBase& captured_in,
  detail::CapturedObjectBase::capture_op_t schedule_capture_op,
  detail::CapturedObjectBase::capture_op_t immediate_capture_op
) const {
  using desc_t = detail::AccessHandleCaptureDescription<
    detail::BasicAccessHandle,
    detail::BasicAccessHandle
  >;

  auto* captured_ptr = utility::safe_static_cast<detail::BasicAccessHandle*>(
    &captured_in
  );

  // This was the old pattern, which we'll use here for now:
  auto init_perm_pair = detail::AccessHandleBaseAttorney::get_permissions_before_downgrades(
    *this, schedule_capture_op, immediate_capture_op
  );

  auto perm_pair = detail::CapturedObjectAttorney::get_and_clear_requested_capture_permissions(
    *this, init_perm_pair.scheduling, init_perm_pair.immediate
  );

  return std::make_unique<desc_t>(
    this, captured_ptr,
    (frontend::permissions_t)perm_pair.scheduling,
    (frontend::permissions_t)perm_pair.immediate
  );

};


} // end namespace darma

#endif //DARMAFRONTEND_ACCESS_HANDLE_CAPTURE_DESCRIPTION_H
