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

namespace darma_runtime {

namespace detail {

template <typename AccessHandleT>
void
TaskBase::do_capture(
  AccessHandleT const& source,
  AccessHandleT& captured,
  AccessHandleT& continuing,
  bool is_read_only
) {

  registrations_to_run.emplace_back([&]{

    typedef typename AccessHandleT::dep_handle_ptr_maker_t dep_handle_ptr_maker_t;
    typedef typename AccessHandleT::read_only_usage_holder_ptr_maker_t read_only_usage_holder_ptr_maker_t;

    bool ignored = ignored_handles.find(source.dep_handle_.get()) != ignored_handles.end();

    if (not ignored) {

      ////////////////////////////////////////////////////////////////////////////////

      // Determine the capture type
      // TODO check that any explicit permissions are obeyed

      typename AccessHandleT::capture_op_t capture_type;

      // first check for any explicit permissions
      auto found = read_only_handles.find(source.dep_handle_);
      if (found != read_only_handles.end()) {
        capture_type = AccessHandleT::ro_capture;
        read_only_handles.erase(found);
      }
      else {
        // Deduce capture type from state
        switch (source.state_) {
          case AccessHandleT::Read_None:
          case AccessHandleT::Read_Read: {
            capture_type = AccessHandleT::ro_capture;
            break;
          }
          case AccessHandleT::Modify_None:
          case AccessHandleT::Modify_Read:
          case AccessHandleT::Modify_Modify: {
            capture_type = AccessHandleT::mod_capture;
            break;
          }
          case AccessHandleT::None_None: {
            DARMA_ASSERT_MESSAGE(false, "Handle used after release");
            break;
          }
        };
      }

      ////////////////////////////////////////////////////////////////////////////////

      switch (capture_type) {
        case AccessHandleT::ro_capture: {
          switch (source.state_) {
            case AccessHandleT::None_None: {
              // Unreachable
              DARMA_ASSERT_MESSAGE(false, "Handle used after release"); // LCOV_EXCL_LINE
              break;
            }
            case AccessHandleT::Read_None:
            case AccessHandleT::Read_Read:
            case AccessHandleT::Modify_None:
            case AccessHandleT::Modify_Read: {
              captured.dep_handle_ = source.dep_handle_;
              captured.state_ = AccessHandleT::Read_Read;
              captured.read_only_holder_ = source.read_only_holder_;

              // Outer dep handle, state, read_only_holder stays the same

              break;
            }
            case AccessHandleT::Modify_Modify: {
              // We're creating a subsequent at the same version depth
              continuing.dep_handle_->has_subsequent_at_version_depth = true;

              version_t next_version = source.dep_handle_->get_version();
              ++next_version;
              captured.dep_handle_ = dep_handle_ptr_maker_t()(
                source.dep_handle_->get_key(),
                next_version
              );
              captured.read_only_holder_ = read_only_usage_holder_ptr_maker_t()(
                captured.dep_handle_
              );
              captured.state_ = AccessHandleT::Read_Read;


              continuing.dep_handle_ = captured.dep_handle_;
              continuing.read_only_holder_ = captured.read_only_holder_;
              continuing.state_ = AccessHandleT::Modify_Read;

              break;
            }
          }; // end switch outer.state_
          add_dependency(
            captured.dep_handle_,
            /*needs_read_data = */ true,
            /*needs_write_data = */ false
          );
          break;
        }
        case AccessHandleT::mod_capture: {
          switch (source.state_) {
            case AccessHandleT::None_None: {
              // Unreachable in 0.2
              DARMA_ASSERT_MESSAGE(false, "Handle used after release"); // LCOV_EXCL_LINE
              break;
            }
            case AccessHandleT::Read_None:
            case AccessHandleT::Read_Read: {
              // Unreachable in 0.2
              DARMA_ASSERT_MESSAGE(false,
                "Tried to schedule a modifying task of a handle with read-only schedule access"); // LCOV_EXCL_LINE
              break; // unreachable
            }
            case AccessHandleT::Modify_None:
            case AccessHandleT::Modify_Read: {
              version_t outer_version = source.dep_handle_->get_version();
              ++outer_version;
              captured.dep_handle_ = source.dep_handle_;
              captured.read_only_holder_ = 0;
              captured.state_ = AccessHandleT::Modify_Modify;
              captured.dep_handle_->push_subversion();

              continuing.dep_handle_ = dep_handle_ptr_maker_t()(
                source.dep_handle_->get_key(),
                outer_version
              );
              continuing.read_only_holder_ = read_only_usage_holder_ptr_maker_t()(
                continuing.dep_handle_
              );
              continuing.state_ = AccessHandleT::Modify_None;

              break;
            }
            case AccessHandleT::Modify_Modify: {
              version_t outer_version = source.dep_handle_->get_version();
              ++outer_version;
              version_t captured_version = source.dep_handle_->get_version();
              captured_version.push_subversion();
              ++captured_version;

              // We're creating a subsequent at the same version depth
              continuing.dep_handle_->has_subsequent_at_version_depth = true;

              // avoid releasing the old until these two are made
              auto tmp = source.dep_handle_;
              auto tmp_ro = source.read_only_holder_;

              captured.dep_handle_ = dep_handle_ptr_maker_t()(
                source.dep_handle_->get_key(), captured_version
              );
              // No read only uses of this new handle
              captured.read_only_holder_ = read_only_usage_holder_ptr_maker_t()(
                captured.dep_handle_
              );
              captured.read_only_holder_.reset();
              captured.state_ = AccessHandleT::Modify_Modify;

              continuing.dep_handle_ = dep_handle_ptr_maker_t()(
                source.dep_handle_->get_key(), outer_version
              );
              continuing.read_only_holder_ = read_only_usage_holder_ptr_maker_t()(
                continuing.dep_handle_
              );
              continuing.state_ = AccessHandleT::Modify_None;

              break;
            }
          } // end switch outer.state
          add_dependency(
            captured.dep_handle_,
            /*needs_read_data = */ captured.dep_handle_->get_version() != version_t(),
            /*needs_write_data = */ true
          );
          break;
        } // end mod_capture case
      } // end switch(capture_type)
    } else {
      // ignored
      captured.capturing_task = nullptr;
      captured.dep_handle_.reset();
      captured.read_only_holder_.reset();
      captured.state_ = AccessHandleT::None_None;
    }
  });
}


} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_TASK_CAPTURE_IMPL_H_H
