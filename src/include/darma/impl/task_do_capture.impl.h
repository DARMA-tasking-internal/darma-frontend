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

  bool check_aliasing = source_and_continuing.current_use_->captured_but_not_handled;

  source_and_continuing.captured_as_ |= default_capture_as_info;

  // Note: source_and_continuing is not functionally const, since
  // we modify it significantly (it just happens that those modifications
  // are to mutable member variables which have to be mutable because
  // of the [=] capture behavior)
  registrations_to_run.emplace_back([&,check_aliasing=check_aliasing]{

    auto* backend_runtime = abstract::backend::get_backend_runtime();
    //std::cout << backend_runtime->get_execution_resource_count(0) << std::endl;

    auto& source = source_and_continuing;
    auto& continuing = source_and_continuing;

    if(check_aliasing) {
      if(allowed_aliasing and allowed_aliasing->aliasing_is_allowed_for(source, this)) {
        // Short-circuit rather than capturing twice...
        // TODO get the maximum permissions and pass those on
        return;
      }
      else {
        // Unallowed alias...
        DARMA_ASSERT_FAILURE(
          "Captured the same handle (with key = " << source_and_continuing.get_key()
          << ") via different variables without explicitly allowing aliasing in"
            " create_work call (using the keyword_arguments_for_task_creation::allow_aliasing"
            " keyword argument)."
        );
      }
    }

    // note the fact that we're handling the capture now
    source.current_use_->captured_but_not_handled = false;

    bool ignored = (source.captured_as_ & AccessHandleBase::Ignored) != 0;

    if (not ignored) {

      ////////////////////////////////////////////////////////////////////////////////

      // Determine the capture type


      typename AccessHandleT::capture_op_t capture_type;

      // first check for any explicit permissions
      bool is_marked_read_capture = (source.captured_as_ & AccessHandleBase::ReadOnly) != 0;
      // Indicate that we've processed the ReadOnly bit by resetting it
      source.captured_as_ &= ~AccessHandleBase::ReadOnly;

      if (is_marked_read_capture) {
        capture_type = AccessHandleT::ro_capture;
      }
      else {
        // Deduce capture type from state
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

      auto _ro_capture_mod_imm = [&]{

      };

      switch (capture_type) {
        ////////////////////////////////////////////////////////////////////////////////
        case AccessHandleT::ro_capture: {
          // We don't need to worry about scheduling permissions for now, until
          // we introduce modes like write or reduce, since we already check for
          // None above, and the behavior is the same for Modify scheduling permissions
          // and Read scheduling permissions (for now, anyway.  There may be
          // a way to pass on Modify scheduling permissions while requesting
          // Read immediate permissions at some point in the future.
          switch (source.current_use_->use.immediate_permissions_) {
            case HandleUse::None:
            case HandleUse::Read: {
              captured.current_use_ = detail::make_shared<UseHolder>(
                HandleUse(
                  source.var_handle_,
                  source.current_use_->use.in_flow_,
                  source.current_use_->use.in_flow_,
                  /* Scheduling permissions: */
                  source.captured_as_ & AccessHandleBase::Leaf ?
                    HandleUse::None : HandleUse::Read,
                  /* Immediate permissions: */
                  HandleUse::Read
                )
              );
              captured.current_use_->do_register();
              // Continuing use stays the same
              break;
            }
            case HandleUse::Modify: {
              auto forwarded_flow = make_forwarding_flow_ptr(
                source.current_use_->use.in_flow_, backend_runtime
              );
              captured.current_use_ = detail::make_shared<UseHolder>(HandleUse(
                source.var_handle_,
                forwarded_flow, forwarded_flow,
                /* Scheduling permissions: */
                source.captured_as_ & AccessHandleBase::Leaf ?
                  HandleUse::None : HandleUse::Read,
                /* Immediate permissions: */
                HandleUse::Read
              ));
              captured.current_use_->do_register();

              auto source_use_to_release_after = source.current_use_;

              // The continuing context actually needs to have a Use as well,
              // since it has access to the underlying data...
              continuing.current_use_ = detail::make_shared<UseHolder>(HandleUse(
                source.var_handle_,
                forwarded_flow,
                // It still carries the out flow of the task, though, and should
                // establish an alias on release if there are no more modifies
                source_use_to_release_after->use.out_flow_,
                /* Scheduling permissions: (unchanged from source) */
                source_use_to_release_after->use.scheduling_permissions_,
                /* Immediate permissions: */
                HandleUse::Read
              ));
              // But this *can* still establish an alias (if it has Modify
              // scheduling permissions) because it could be the one that detects
              // that the forwarding flow aliases the out flow (i.e., that there
              // are no more modifies)
              continuing.current_use_->could_be_alias =
                source_use_to_release_after->use.scheduling_permissions_ == HandleUse::Modify;
              continuing.current_use_->do_register();
              // We can go ahead and pass on the underlying pointer, though, since
              // the Use is associated with a handle in a context that's uninterruptible
              void*& ptr = continuing.current_use_->use.get_data_pointer_reference();
              // (But only if the backend didn't somehow set it to something
              // else during registration...)
              if(ptr == nullptr) {
                ptr = source_use_to_release_after->use.get_data_pointer_reference();
              }

              // Now we can release the source, finally
              source_use_to_release_after->do_release();
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
                source.var_handle_,
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
                source.var_handle_,
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

    // Assert that all of the captured_as info has been handled:
    assert(source.captured_as_ == AccessHandleBase::Normal);
    // And, just for good measure, that there aren't any flags set on captured
    assert(captured.captured_as_ == AccessHandleBase::Normal);

  });
}


} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_TASK_CAPTURE_IMPL_H_H
