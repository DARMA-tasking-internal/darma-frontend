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
#include <darma/impl/util/smart_pointers.h>

#include <darma/impl/flow_handling.h>

#include <thread>

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

  typedef AccessHandleT1 AccessHandleT;

  DARMA_ASSERT_MESSAGE(
    source_and_continuing.current_use_.get() != nullptr,
    "Can't capture handle after it was released or before it was initialized"
  );

  DARMA_ASSERT_MESSAGE(
    source_and_continuing.current_use_->use.scheduling_permissions_ != HandleUse::Permissions::None,
    "Can't do a capture of an AccessHandle with scheduling permissions of None"
  );

  source_and_continuing.captured_as_ |= default_capture_as_info;

  // Note: source_and_continuing is not functionally const, since
  // we modify it significantly (it just happens that those modifications
  // are to mutable member variables which have to be mutable because
  // of the [=] capture behavior)
  registrations_to_run.emplace_back([&]{

    auto* backend_runtime = abstract::backend::get_backend_runtime();

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
        assert((source.captured_as_ & AccessHandleBase::ReadOnly) == 0);
        switch (source.current_use_->use.scheduling_permissions_) {
          case HandleUse::Read: {
            capture_type = AccessHandleT::ro_capture;
            break;
          }
          case HandleUse::Modify: {
            capture_type = AccessHandleT::mod_capture;
            break;
          }
          case HandleUse::None: {
            DARMA_ASSERT_UNREACHABLE_FAILURE(); // LCOV_EXCL_LINE
            break;
          }
          default: {
            DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
            break;
          }
        };
      }

      ////////////////////////////////////////////////////////////////////////////////

      auto _ro_capture_non_mod_imm = [&]{
        captured.current_use_ = detail::make_shared<UseHolder>(HandleUse(
          source.var_handle_.get(),
          source.current_use_->use.in_flow_,
          source.current_use_->use.in_flow_,
          source.captured_as_ & AccessHandleBase::Leaf ?
            HandleUse::None : HandleUse::Read,
          HandleUse::Read
        ));
        captured.current_use_->do_register();
        // Continuing use stays the same
      };

      auto _ro_capture_mod_imm = [&]{
        auto forwarded_flow = make_forwarding_flow_ptr(
          source.current_use_->use.in_flow_, backend_runtime
        );
        captured.current_use_ = detail::make_shared<UseHolder>(HandleUse(
          source.var_handle_.get(),
          forwarded_flow, forwarded_flow,
          source.captured_as_ & AccessHandleBase::Leaf ?
            HandleUse::None : HandleUse::Read,
          HandleUse::Read
        ));
        captured.current_use_->do_register();
        source.current_use_->do_release();
        continuing.current_use_->use.immediate_permissions_ = HandleUse::Read;
        continuing.current_use_->use.in_flow_ = forwarded_flow;
        // out flow and scheduling permissions are unchanged
        continuing.current_use_->could_be_alias = true;

      };

      switch (capture_type) {
        ////////////////////////////////////////////////////////////////////////////////
        case AccessHandleT::ro_capture: {
          switch (source.current_use_->use.scheduling_permissions_) {
            case HandleUse::Read: {
              switch (source.current_use_->use.immediate_permissions_) {
                case HandleUse::None:
                case HandleUse::Read:
                  _ro_capture_non_mod_imm();
                  break;
                case HandleUse::Modify:
                  _ro_capture_mod_imm();
                  break;
                default: {
                  DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
                  break;
                }
              }
              break;
            }
            case HandleUse::Modify: {
              switch (source.current_use_->use.immediate_permissions_) {
                case HandleUse::None:
                case HandleUse::Read:
                  _ro_capture_non_mod_imm();
                  break;
                case HandleUse::Modify:
                  _ro_capture_mod_imm();
                  break;
                default: {
                  DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
                  break;
                }
              }
              break;
            }
            case HandleUse::None: {
              // Unreachable, should already be handled above
              DARMA_ASSERT_UNREACHABLE_FAILURE(); // LCOV_EXCL_LINE
              break;
            }
            default: {
              DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
              break;
            }
          } // end switch source.current_use_->use.scheduling_permissions_
          break;
        }
        ////////////////////////////////////////////////////////////////////////////////
        case AccessHandleT::mod_capture: {
          DARMA_ASSERT_MESSAGE(source.current_use_->use.scheduling_permissions_ == HandleUse::Permissions::Modify,
            "Can't do a Modify capture of a handle without Modify scheduling permissions"
          );
          switch (source.current_use_->use.immediate_permissions_) {
            case HandleUse::None:
            case HandleUse::Read: {
              // mod(MN) and mod(MR)
              auto captured_out_flow = make_next_flow_ptr(
                  source.current_use_->use.in_flow_, backend_runtime
              );
              captured.current_use_ = detail::make_shared<UseHolder>(HandleUse(
                source.var_handle_.get(),
                source.current_use_->use.in_flow_,
                captured_out_flow,
                source.captured_as_ & AccessHandleBase::Leaf ?
                  HandleUse::None : HandleUse::Modify,
                HandleUse::Modify
              ));
              captured.current_use_->do_register();
              continuing.current_use_->use.in_flow_ = captured_out_flow;
              break;
            }
            case HandleUse::Modify: {
              auto captured_in_flow = make_forwarding_flow_ptr(
                source.current_use_->use.in_flow_, backend_runtime
              );
              auto captured_out_flow = make_next_flow_ptr(
                captured_in_flow, backend_runtime
              );
              captured.current_use_ = detail::make_shared<UseHolder>(HandleUse(
                source.var_handle_.get(),
                captured_in_flow, captured_out_flow,
                source.captured_as_ & AccessHandleBase::Leaf ?
                  HandleUse::None : HandleUse::Modify,
                HandleUse::Modify
              ));
              captured.current_use_->do_register();

              // Release the current use (from the source)
              source.current_use_->do_release();
              // And make the continuing context state correct
              continuing.current_use_->use.scheduling_permissions_ = HandleUse::Modify;
              continuing.current_use_->use.immediate_permissions_ = HandleUse::None;
              continuing.current_use_->use.in_flow_ = captured_out_flow;
              continuing.current_use_->could_be_alias = true;
              // continuing out flow is unchanged
              break;
            }
            default: {
              DARMA_ASSERT_NOT_IMPLEMENTED();
              break;
            }
          } // end switch source.current_use_->use.scheduling_permissions_
          break;
        } // end mod_capture case
        ////////////////////////////////////////////////////////////////////////////////
      } // end switch(capture_type)

      // Now add the dependency
      add_dependency(captured.current_use_->use);

      // Indicate that we've processed the "leaf" information by resetting the flag
      source.captured_as_ &= ~AccessHandleBase::Leaf;

      captured.var_handle_ = source.var_handle_;

    }
    else {
      // ignored
      captured.current_use_ = nullptr;
    }

  });
}


} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_TASK_CAPTURE_IMPL_H_H
