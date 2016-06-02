/*
//@HEADER
// ************************************************************************
//
//                          task_capture_impl.h
//                         darma_new
//              Copyright (C) 2016 Sandia Corporation
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

#ifndef DARMA_TASK_CAPTURE_IMPL_H_H
#define DARMA_TASK_CAPTURE_IMPL_H_H

#include <darma/impl/task.h>
#include <darma/impl/handle.h>
#include "use.h"

namespace darma_runtime {

namespace detail {

template <
  typename AccessHandleT1,
  typename AccessHandleT2
>
void
TaskBase::do_capture(
  AccessHandleT1& captured,
  AccessHandleT2 const& source_and_continuing
) {

  using purpose_t = abstract::backend::Runtime::FlowPropagationPurpose;

  typedef AccessHandleT1 AccessHandleT;

  // Note: source_and_continuing is not functionally const, since
  // we modify it significantly (it just happens that those modifications
  // are to mutable member variables which have to be mutable because
  // of the [=] capture behavior)

  registrations_to_run.emplace_back([&]{

    auto& source = source_and_continuing;
    auto& continuing = source_and_continuing;

    bool ignored = (source.captured_as_ & AccessHandleBase::Ignored) != 0;

    if (not ignored) {

      ////////////////////////////////////////////////////////////////////////////////

      // Determine the capture type

      // Unset the uncaptured bit
      source.captured_as_ &= ~AccessHandleBase::Uncaptured;

      typename AccessHandleT::capture_op_t capture_type;

      // first check for any explicit permissions
      bool is_marked_read_capture = (source.captured_as_ & AccessHandleBase::ReadOnly) != 0;
      if (is_marked_read_capture) {
        capture_type = AccessHandleT::ro_capture;
      }
      else {
        // Deduce capture type from state
        assert(source.captured_as_ == AccessHandleBase::CapturedAsInfo::Normal);
        switch (source.current_use_->scheduling_permissions_) {
          case HandleUse::Read: {
            capture_type = AccessHandleT::ro_capture;
            break;
          }
          case HandleUse::Modify: {
            capture_type = AccessHandleT::mod_capture;
            break;
          }
          case HandleUse::None: {
            DARMA_ASSERT_MESSAGE(false, "Handle used after release");
            break;
          }
        };
      }

      ////////////////////////////////////////////////////////////////////////////////

      auto _ro_capture_non_mod_imm = [&]{
        auto captured_in_flow = detail::backend_runtime->make_same_flow(
          source.current_use_->in_flow_, purpose_t::Input
        );
        auto captured_out_flow = detail::backend_runtime->make_same_flow(
          captured_in_flow, purpose_t::OutputFlowOfReadOperation
        );
        captured.current_use_ = detail::make_unique<HandleUse>(source.var_handle_.get(),
          captured_in_flow, captured_out_flow, HandleUse::Read, HandleUse::Read
        );
        detail::backend_runtime->register_use(captured.current_use_.get());
        // Continuing use stays the same:  (as if:)
        // continuing.current_use_ = source.current_use_
      };

      auto _ro_capture_mod_imm = [&]{
        auto captured_in_flow = detail::backend_runtime->make_forwarding_flow(
          source.current_use_->in_flow_, purpose_t::ForwardingChanges
        );
        auto captured_out_flow = detail::backend_runtime->make_same_flow(
          captured_in_flow, purpose_t::OutputFlowOfReadOperation
        );
        auto continuing_in_flow = detail::backend_runtime->make_same_flow(
          captured_in_flow, purpose_t::Input
        );
        auto continuing_out_flow = detail::backend_runtime->make_same_flow(
          source.current_use_->out_flow_, purpose_t::Output
        );

        captured.current_use_ = detail::make_unique<HandleUse>(source.var_handle_.get(),
          captured_in_flow, captured_out_flow, HandleUse::Read, HandleUse::Read
        );
        detail::backend_runtime->register_use(captured.current_use_.get());
        continuing._switch_to_new_use(detail::make_unique<HandleUse>(source.var_handle_.get(),
          continuing_in_flow, continuing_out_flow,
          source.current_use_->scheduling_permissions_, HandleUse::Read
        ));

      };

      switch (capture_type) {
        case AccessHandleT::ro_capture: {
          switch (source.current_use_->scheduling_permissions_) {
            case HandleUse::None: {
              // Unreachable
              DARMA_ASSERT_MESSAGE(false, "Handle used after release"); // LCOV_EXCL_LINE
              break;
            }
            case HandleUse::Read: {
              switch (source.current_use_->immediate_permissions_) {
                case HandleUse::None:
                case HandleUse::Read:
                  _ro_capture_non_mod_imm();
                  break;
                case HandleUse::Modify:
                  _ro_capture_mod_imm();
              }
              break;
            }
            case HandleUse::Modify: {
              switch (source.current_use_->immediate_permissions_) {
                case HandleUse::None:
                case HandleUse::Read:
                  _ro_capture_non_mod_imm();
                case HandleUse::Modify:
                  _ro_capture_mod_imm();
              }
              break;
            }
          }; // end switch source.current_use_->scheduling_permissions_
          break;
        }
        case AccessHandleT::mod_capture: {
          switch (source.current_use_->scheduling_permissions_) {
            case HandleUse::None: {
              // Unreachable
              DARMA_ASSERT_MESSAGE(false, "Handle used after release"); // LCOV_EXCL_LINE
              break;
            }
            case HandleUse::Read: {
              DARMA_ASSERT_MESSAGE(false,
                "Tried to schedule a modifying task of a handle with read-only schedule access"); // LCOV_EXCL_LINE
              break; // unreachable
            }
            case HandleUse::Modify: {
              switch (source.current_use_->immediate_permissions_) {
                case HandleUse::None:
                case HandleUse::Read: {
                  // mod(MN) and mod(MR)
                  auto captured_in_flow = detail::backend_runtime->make_same_flow(
                    source.current_use_->in_flow_, purpose_t::Input
                  );
                  auto captured_out_flow = detail::backend_runtime->make_next_flow(
                    captured_in_flow, purpose_t::Output
                  );
                  auto continuing_in_flow = detail::backend_runtime->make_same_flow(
                    captured_out_flow, purpose_t::Input
                  );
                  auto continuing_out_flow = detail::backend_runtime->make_same_flow(
                    source.current_use_->out_flow_, purpose_t::Output
                  );
                  captured.current_use_ = detail::make_unique<HandleUse>(source.var_handle_.get(),
                    captured_in_flow, captured_out_flow, HandleUse::Modify, HandleUse::Modify
                  );
                  detail::backend_runtime->register_use(captured.current_use_.get());
                  continuing._switch_to_new_use(detail::make_unique<HandleUse>(source.var_handle_.get(),
                    continuing_in_flow, continuing_out_flow, HandleUse::Modify,
                    source.current_use_->immediate_permissions_
                  ));
                  break;
                }
                case HandleUse::Modify: {
                  auto captured_in_flow = detail::backend_runtime->make_forwarding_flow(
                    source.current_use_->in_flow_, purpose_t::ForwardingChanges
                  );
                  auto captured_out_flow = detail::backend_runtime->make_next_flow(
                    captured_in_flow, purpose_t::Output
                  );
                  auto continuing_in_flow = detail::backend_runtime->make_same_flow(
                    captured_out_flow, purpose_t::Input
                  );
                  auto continuing_out_flow = detail::backend_runtime->make_same_flow(
                    source.current_use_->out_flow_, purpose_t::Output
                  );
                  captured.current_use_ = detail::make_unique<HandleUse>(source.var_handle_.get(),
                    captured_in_flow, captured_out_flow, HandleUse::Modify, HandleUse::Modify
                  );
                  detail::backend_runtime->register_use(captured.current_use_.get());
                  continuing._switch_to_new_use(detail::make_unique<HandleUse>(source.var_handle_.get(),
                    continuing_in_flow, continuing_out_flow, HandleUse::Modify, HandleUse::Read
                  ));
                  break;
                }
              } // end switch source.current_use_->immediate_permissions_
              break;
            }
          }; // end switch source.current_use_->scheduling_permissions_
          break;
        } // end mod_capture case
      } // end switch(capture_type)

      // Now add the dependency
      add_dependency(captured.current_use_);

      captured.var_handle_ = source.var_handle_;

    }
    else {
      // ignored
      //captured.capturing_task = nullptr;
      captured.current_use_ = nullptr;
    }

  });
}


} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_TASK_CAPTURE_IMPL_H_H
