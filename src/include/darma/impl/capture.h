/*
//@HEADER
// ************************************************************************
//
//                      capture.h
//                         DARMA
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

#ifndef DARMA_IMPL_CAPTURE_H
#define DARMA_IMPL_CAPTURE_H

#include <darma/impl/flow_handling.h>
#include "handle.h"
#include "use.h"

namespace darma_runtime {
namespace detail {


// returns the captured UseHolder in a properly registered state
// and with all of the proper flags set
inline auto
make_captured_use_holder(
  std::shared_ptr<VariableHandleBase> const& var_handle,
  HandleUse::permissions_t requested_scheduling_permissions,
  HandleUse::permissions_t requested_immediate_permissions,
  std::shared_ptr<UseHolder>& source_and_continuing_holder
) {

  // source scheduling permissions shouldn't be None at this point
  DARMA_ASSERT_MESSAGE(
    source_and_continuing_holder->use.scheduling_permissions_ != HandleUse::None,
    "Can't schedule a task on a handle with leaf permissions"
  );

  auto* backend_runtime = abstract::backend::get_backend_runtime();

  std::shared_ptr<UseHolder> captured_use_holder;

  switch(requested_immediate_permissions) {
    //==========================================================================
    case HandleUse::None: {

      // schedule-only cases

      switch(source_and_continuing_holder->use.immediate_permissions_) {

        //----------------------------------------------------------------------

        case HandleUse::None: {

          // This feels like it should be the dominant use case...
          // switch on the requested scheduling permissions...
          switch(requested_scheduling_permissions) {
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            case HandleUse::None: {
              // Well, what the heck are you doing with it, then...?
              // Shouldn't be reachable from the current front end, anyway
              DARMA_ASSERT_NOT_IMPLEMENTED();                                   // LCOV_EXCL_LINE
              break;                                                            // LCOV_EXCL_LINE
            } // end case None requested scheduling permissions
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            case HandleUse::Read: {

              // We still need to create a new use, because it's a separate task
              captured_use_holder = detail::make_shared<UseHolder>(HandleUse(
                var_handle,
                source_and_continuing_holder->use.in_flow_,
                source_and_continuing_holder->use.in_flow_,
                /* Scheduling permissions */
                HandleUse::Read,
                /* Immediate permissions */
                HandleUse::None
              ));
              captured_use_holder->do_register();

              break;
            } // end case Read requested scheduling permissions
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            case HandleUse::Modify: {

              // Modify schedule-only permissions on a handle with no immediate
              // permissions.

              DARMA_ASSERT_MESSAGE(
                source_and_continuing_holder->use.scheduling_permissions_ == HandleUse::Modify,
                "Can't schedule a (schedule-only) Modify task on a handle without"
                  " modify schedule permissions"
              );

              // We still need to make a next flow...
              // ...actually, this is basically just a regular modify capture with
              // some small exceptions
              auto captured_out_flow = make_next_flow_ptr(
                source_and_continuing_holder->use.in_flow_, backend_runtime
              );

              captured_use_holder = detail::make_shared<UseHolder>(HandleUse(
                var_handle,
                source_and_continuing_holder->use.in_flow_,
                captured_out_flow,
                /* Scheduling permissions */
                HandleUse::Modify,
                /* Immediate permissions */
                HandleUse::None
              ));

              captured_use_holder->do_register();

              // Note that this next part is almost exactly the same as the logic for
              // the immediate modify case

              // if the source use was registered, we need to release it now
              if(source_and_continuing_holder->is_registered) {

                // TODO creating a new use here might be inconsistent with how we don't create continuation uses in the immediate modify case

                // We need to register a new use here, though, since the
                // continuing context will have different flows from the
                // already-registered capturing context

                // copy the use holder so that it can be released after we register
                // the new use in the continuing context
                auto source_use_holder = source_and_continuing_holder;

                // Make a new use for the continuing context (since, for some reason
                // or another, we needed one in the source context)
                source_and_continuing_holder = detail::make_shared<UseHolder>(HandleUse(
                  var_handle,
                  captured_out_flow,
                  source_use_holder->use.out_flow_,
                  /* Scheduling permissions */
                  HandleUse::Modify,
                  /* Immediate permissions */
                  HandleUse::None // This is a schedule-only use
                ));
                // Register our new use
                source_and_continuing_holder->do_register();

                // This new use could establish an alias if no additional tasks are
                // scheduled on it...
                source_and_continuing_holder->could_be_alias = true;

                // now that we've created a continuing context use holder, the source
                // should NOT establish an alias (the continuing use holder will do
                // it instead), so flip the flag here
                source_use_holder->could_be_alias = false;
                // Now we can release the source use
                source_use_holder->do_release();

              }
              else {
                // otherwise, we can just reuse the existing use holder

                // ...but the in_flow needs to be set to the out_flow of the captured
                // Use for the purposes of the next task
                source_and_continuing_holder->use.in_flow_ = captured_out_flow;
                // Out flow should be unchanged, but it will still establish an alias
                // when it goes away if no other tasks are asigned to it
                source_and_continuing_holder->could_be_alias = true;
              }

              break;
            } // end case Modify requested scheduling permissions
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            default: {
              DARMA_ASSERT_NOT_IMPLEMENTED();                                   // LCOV_EXCL_LINE
            } // end default
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
          } // end switch on requested scheduling permissions

          break;
        } // end case None source immediate permissions

        //----------------------------------------------------------------------

        case HandleUse::Read: {
          DARMA_ASSERT_NOT_IMPLEMENTED(
            "Schedule-only permissions on handles that already have immediate"
              " Read permissions."
          );                                                                    // LCOV_EXCL_LINE
          break;                                                                // LCOV_EXCL_LINE
        }

        //----------------------------------------------------------------------

        case HandleUse::Modify: {
          DARMA_ASSERT_NOT_IMPLEMENTED(
            "Schedule-only permissions on handles that already have immediate"
              " Modify permissions."
          );                                                                    // LCOV_EXCL_LINE
          break;                                                                // LCOV_EXCL_LINE
        }

        //----------------------------------------------------------------------

        default: {
          DARMA_ASSERT_NOT_IMPLEMENTED();                                       // LCOV_EXCL_LINE
        } // end default

        //----------------------------------------------------------------------
      } // end switch on source immediate permissions

      break;

    } // end case None requested immediate permissions
    //==========================================================================
    case HandleUse::Read: {

      DARMA_ASSERT_MESSAGE(
        (source_and_continuing_holder->use.scheduling_permissions_ & HandleUse::Read) != 0,
        "Can't schedule a read on a handle without at least Read schedule permissions"
      );
      DARMA_ASSERT_MESSAGE(
        requested_scheduling_permissions != HandleUse::Modify,
        "Modify scheduling permissions on with Read immediate permissions is not implemented"
      );
      DARMA_ASSERT_MESSAGE(
        requested_scheduling_permissions != HandleUse::Write,
        "Write scheduling permissions on with Read immediate permissions is not implemented"
      );

      switch(source_and_continuing_holder->use.immediate_permissions_) {
        //----------------------------------------------------------------------
        case HandleUse::None:
        case HandleUse::Read: {

          captured_use_holder = detail::make_shared<UseHolder>(HandleUse(
            var_handle,
            source_and_continuing_holder->use.in_flow_,
            source_and_continuing_holder->use.in_flow_,
            /* Scheduling permissions */
            requested_scheduling_permissions,
            /* Immediate permissions */
            HandleUse::Read
          ));
          captured_use_holder->do_register();

          break;

        } // end case Read or None immediate permissions on source
        //----------------------------------------------------------------------
        case HandleUse::Modify: {

          // TODO if the handle doesn't have modify schedule permissions, this logic could be completely different
          // (i.e., instead of making a forwarding flow, we could just use
          // the output flow)

          // This is a ro capture of a handle with modify immediate permissions

          // we need to make a forwarded flow and register a new use in the
          // continuing context
          auto forwarded_flow = make_forwarding_flow_ptr(
            source_and_continuing_holder->use.in_flow_, backend_runtime
          );

          captured_use_holder = detail::make_shared<UseHolder>(HandleUse(
            var_handle,
            forwarded_flow,
            forwarded_flow,
            /* Scheduling permissions */
            requested_scheduling_permissions,
            /* Immediate permissions */
            HandleUse::Read
          ));
          captured_use_holder->do_register();

          // The continuing context actually needs to have a Use as well,
          // since it has access to the underlying data...

          // Also, this continuing context Use has to be registered before the
          // source use is released
          // make another shared ptr to the source use holder so that we can
          // release it
          auto source_use_holder = source_and_continuing_holder;

          // now make the use for the continuing context
          source_and_continuing_holder = detail::make_shared<UseHolder>(HandleUse(
            var_handle,
            forwarded_flow,
            // It still carries the out flow of the task, though, and should
            // establish an alias on release if there are no more modifies
            source_use_holder->use.out_flow_,
            /* Scheduling permissions: (should be unchanged) */
            source_use_holder->use.scheduling_permissions_,
            /* Immediate permissions: */
            HandleUse::Read
          ));
          // But this *can* still establish an alias (if it has Modify
          // scheduling permissions) because it could be the one that detects
          // that the forwarding flow aliases the out flow (i.e., that there
          // are no more modifies)
          source_and_continuing_holder->could_be_alias = true;
          // Now register the continuing use
          source_and_continuing_holder->do_register();

          // We can go ahead and pass on the underlying pointer, though, since
          // the Use is associated with a handle in a context that's uninterruptible
          void*& ptr = source_and_continuing_holder->use.get_data_pointer_reference();
          // (But only if the backend didn't somehow set it to something
          // else during registration...)
          if(ptr == nullptr) {
            ptr = source_use_holder->use.get_data_pointer_reference();
          }

          // now release the source
          source_use_holder->do_release();

          break;

        } // end case Modify immediate permissions on source
        //----------------------------------------------------------------------
        default: {
          DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
        } // end default
        //----------------------------------------------------------------------
      } // end switch on source immediate permissions

      break;

    } // end case Read requested immediate permissions
    //==========================================================================
    case HandleUse::Modify: {

      DARMA_ASSERT_MESSAGE(
        source_and_continuing_holder->use.scheduling_permissions_ == HandleUse::Modify,
        "Can't schedule a read on a handle without Modify schedule permissions"
      );

      switch(source_and_continuing_holder->use.immediate_permissions_) {
        //----------------------------------------------------------------------
        case HandleUse::None: {

          // Modify capture of handle without modify immediate permissions

          // Create the out flow
          auto captured_out_flow = make_next_flow_ptr(
            source_and_continuing_holder->use.in_flow_, backend_runtime
          );

          // make the captured use holder
          captured_use_holder = detail::make_shared<UseHolder>(HandleUse(
            var_handle,
            source_and_continuing_holder->use.in_flow_,
            captured_out_flow,
            /* Scheduling permissions */
            requested_scheduling_permissions,
            /* Immediate permissions */
            HandleUse::Modify
          ));

          // register the captured use
          captured_use_holder->do_register();

          // if the source use was registered, we need to release it now
          if(source_and_continuing_holder->is_registered) {
            // We need to register a new use here, though
            // TODO creating a new use here might be inconsistent with how we don't create continuation uses in the modify immediate source case

            // copy the use holder so that it can be released after we register
            // the new use in the continuing context
            auto source_use_holder = source_and_continuing_holder;

            // Make a new use for the continuing context (since, for some reason
            // or another, we needed one in the source context)
            source_and_continuing_holder = detail::make_shared<UseHolder>(HandleUse(
              var_handle,
              captured_out_flow,
              source_use_holder->use.out_flow_,
              /* Scheduling permissions */
              HandleUse::Modify,
              /* Immediate permissions */
              HandleUse::None // This is a schedule-only use
            ));
            // and register then new use
            source_and_continuing_holder->do_register();

            // This new use could establish an alias if no additional tasks are
            // scheduled on it...
            source_and_continuing_holder->could_be_alias = true;

            // now that we've created a continuing context use holder, the source
            // should NOT establish an alias (the continuing use holder will do
            // it instead), so flip the flag here
            source_use_holder->could_be_alias = false;
            // Now we can release the source use
            source_use_holder->do_release();
          }
          else {
            // Otherwise, we can reuse the old use holder (since it was never
            // registered).  For the same reason, there should not be a
            // registered use in the continuing context...

            // ...but the in_flow needs to be set to the out_flow of the captured
            // Use for the purposes of the next task
            source_and_continuing_holder->use.in_flow_ = captured_out_flow;
            // Out flow should be unchanged, but it will still establish an alias
            // when it goes away if no other tasks are asigned to it
            source_and_continuing_holder->could_be_alias = true;
          }

          break;
        } // end case None source immediate permissions
        //----------------------------------------------------------------------
        case HandleUse::Read: {
          // This was getting to complicated to keep in the same block with the
          // None case.  I don't even think it's reachable in the current
          // interface, so it's unimplemented for now
          DARMA_ASSERT_NOT_IMPLEMENTED();                                       // LCOV_EXCL_LINE
          break;                                                                // LCOV_EXCL_LINE
        } // end case Read source immediate permissions
        //----------------------------------------------------------------------
        case HandleUse::Modify: {

          // Modify capture of handle with modify immediate permissions

          // we need to forward the input flow, since it could have changed
          // (because the source has modify immediate permissions)
          auto captured_in_flow = make_forwarding_flow_ptr(
            source_and_continuing_holder->use.in_flow_, backend_runtime
          );
          auto captured_out_flow = make_next_flow_ptr(
            captured_in_flow, backend_runtime
          );

          // make the captured use holder
          captured_use_holder = detail::make_shared<UseHolder>(HandleUse(
            var_handle,
            captured_in_flow, captured_out_flow,
            /* Scheduling permissions */
            requested_scheduling_permissions,
            /* Immediate permissions */
            HandleUse::Modify
          ));

          // register the captured use
          captured_use_holder->do_register();

          // and release the source use
          // (there should not be a registered use in the continuing context)
          source_and_continuing_holder->do_release();

          // ...but we do need to set up some stuff for the next time the
          // UseHolder is encountered (whether it's establishing an alias or
          // another capture)
          // (note that the out flow and scheduling permissions remain unchanged)
          source_and_continuing_holder->use.immediate_permissions_ = HandleUse::None;
          source_and_continuing_holder->use.in_flow_ = captured_out_flow;
          source_and_continuing_holder->could_be_alias = true;

          // TODO decide if this continuing context use needs to be registered as schedule only (for consistency)

          break;
        } // end case Modify source immediate permissions
        //----------------------------------------------------------------------
        default: {
          DARMA_ASSERT_NOT_IMPLEMENTED();                                       // LCOV_EXCL_LINE
        } // end default
        //----------------------------------------------------------------------
      } // end switch on source immediate permissions

      break;
    } // end case Modify requested immediate permissions

    //==========================================================================

    default: {
      DARMA_ASSERT_NOT_IMPLEMENTED();                                           // LCOV_EXCL_LINE
    } // end default

    //==========================================================================
  } // end switch requested immediate permissions

  return captured_use_holder;
}



} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_CAPTURE_H
