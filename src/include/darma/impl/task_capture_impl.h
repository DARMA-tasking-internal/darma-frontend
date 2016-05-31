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

  using abstract::frontend::Use::Permissions::None;
  using abstract::frontend::Use::Permissions::Read;
  using abstract::frontend::Use::Permissions::Modify;
  using abstract::backend::Runtime::FlowPropagationPurpose::Input;
  using abstract::backend::Runtime::FlowPropagationPurpose::Output;
  using abstract::backend::Runtime::FlowPropagationPurpose::OutputFlowOfReadOperation;
  using abstract::backend::Runtime::FlowPropagationPurpose::ForwardingChanges;

  typedef AccessHandleT1 AccessHandleT;

  // Note: source_and_continuing is not functionally const, since
  // we modify it significantly (it just happens that those modifications
  // are to mutable member variables which have to be mutable because
  // of the [=] capture behavior)

  registrations_to_run.emplace_back([&]{

    auto& source = source_and_continuing;
    auto& continuing = source_and_continuing;

    typedef typename AccessHandleT::dep_handle_ptr_maker_t dep_handle_ptr_maker_t;
    typedef typename AccessHandleT::read_only_usage_holder_ptr_maker_t read_only_usage_holder_ptr_maker_t;

    //bool ignored = ignored_handles.find(source.dep_handle_.get()) != ignored_handles.end();
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
        switch (source.current_use_.schedule_permissions_) {
          case Read: {
            capture_type = AccessHandleT::ro_capture;
            break;
          }
          case Modify: {
            capture_type = AccessHandleT::mod_capture;
            break;
          }
          case None: {
            DARMA_ASSERT_MESSAGE(false, "Handle used after release");
            break;
          }
        };
      }

      ////////////////////////////////////////////////////////////////////////////////

      auto _ro_capture_non_mod_imm = [&]{
        auto* captured_in_flow = detail::backend_runtime->make_same_flow(
          source.current_use_.in_flow_, Input
        );
        auto* captured_out_flow = detail::backend_runtime->make_same_flow(
          captured_in_flow, OutputFlowOfReadOperation
        );
        captured.current_use_ = HandleUse(source.var_handle_,
          captured_in_flow, captured_out_flow, Read, Read
        );
        // Continuing use stays the same:  (as if:)
        // continuing.current_use_ = source.current_use_
      };

      auto _ro_capture_mod_imm = [&]{
        auto* captured_in_flow = detail::backend_runtime->make_forwarding_flow(
          source.current_use_.in_flow_, ForwardingChanges
        );
        auto* captured_out_flow = detail::backend_runtime->make_same_flow(
          captured_in_flow, OutputFlowOfReadOperation
        );
        auto* continuing_in_flow = detail::backend_runtime->make_same_flow(
          captured_in_flow, Input
        );
        auto* continuing_out_flow = detail::backend_runtime->make_same_flow(
          source.current_use_.out_flow_, Output
        );

        captured.current_use_ = HandleUse(source.var_handle_,
          captured_in_flow, captured_out_flow, Read, Read
        );
        continuing._switch_to_new_use(HandleUse(source.var_handle_,
          continuing_in_flow, continuing_out_flow,
          source.current_use_.schedule_permissions_, Read
        ));

      };

      switch (capture_type) {
        case AccessHandleT::ro_capture: {
          switch (source.current_use_.schedule_permissions_) {
            case None: {
              // Unreachable
              DARMA_ASSERT_MESSAGE(false, "Handle used after release"); // LCOV_EXCL_LINE
              break;
            }
            case Read: {
              switch (source.current_use_.immediate_permissions_) {
                case None:
                case Read:
                  _ro_capture_non_mod_imm();
                  break;
                case Modify:
                  _ro_capture_mod_imm();
              }
              break;
            }
            case Modify: {
              switch (source.current_use_.immediate_permissions_) {
                case None:
                case Read:
                  _ro_capture_non_mod_imm();
                case Modify:
                  _ro_capture_mod_imm();
              }
              break;
            }
          }; // end switch source.current_use_.schedule_permissions_
          break;
        }
        case AccessHandleT::mod_capture: {
          switch (source.current_use_.schedule_permissions_) {
            case None: {
              // Unreachable
              DARMA_ASSERT_MESSAGE(false, "Handle used after release"); // LCOV_EXCL_LINE
              break;
            }
            case Read: {
              DARMA_ASSERT_MESSAGE(false,
                "Tried to schedule a modifying task of a handle with read-only schedule access"); // LCOV_EXCL_LINE
              break; // unreachable
            }
            case Modify: {
              switch (source.current_use_.immediate_permissions_) {
                case None:
                case Read: {
                  // TODO implement this
                  // mod(MN) and mod(MR)
                  auto* captured_in_flow = detail::backend_runtime->make_same_flow(
                    source.current_use_.in_flow_, Input
                  );
                  auto* captured_out_flow = detail::backend_runtime->make_next_flow(
                    source.current_use_.in_flow_, Output
                  );
                  auto* continuing_in_flow = detail::backend_runtime->make_same_flow(
                    captured_out_flow, Input
                  );
                  auto* continuing_out_flow = detail::backend_runtime->make_same_flow(
                    source.current_use_.out_flow_, Output
                  );
                  captured.current_use_ = HandleUse(source.var_handle_,
                    captured_in_flow, captured_out_flow, Modify, Modify
                  );
                  continuing._switch_to_new_use(HandleUse(source.var_handle_,
                    continuing_in_flow, continuing_out_flow, Modify,
                    source.current_use_.immediate_permissions_
                  ));
                  break;
                }
                case Modify: {
                  auto* captured_in_flow = detail::backend_runtime->make_forwarding_flow(
                    source.current_use_.in_flow_, ForwardingChanges
                  );
                  auto* captured_out_flow = detail::backend_runtime->make_next_flow(
                    captured_in_flow, Output
                  );
                  auto* continuing_in_flow = detail::backend_runtime->make_same_flow(
                    captured_out_flow, Input
                  );
                  auto* continuing_out_flow = detail::backend_runtime->make_same_flow(
                    source.current_use_.out_flow_, Output
                  );
                  captured.current_use_ = HandleUse(source.var_handle_,
                    captured_in_flow, captured_out_flow, Modify, Modify
                  );
                  continuing._switch_to_new_use(HandleUse(source.var_handle_,
                    continuing_in_flow, continuing_out_flow, Modify, Read
                  ));
                  break;
                }
              } // end switch source.current_use_.immediate_permissions_
              break;
            }
          }; // end switch source.current_use_.schedule_permissions_
          break;
        } // end mod_capture case
      } // end switch(capture_type)

      // Now add the dependency
      add_dependency(captured.current_use_);

    }
    else {
      // ignored
      //captured.capturing_task = nullptr;
      captured.current_use_ = detail::HandleUse{};
    }

  });
}


} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_TASK_CAPTURE_IMPL_H_H
