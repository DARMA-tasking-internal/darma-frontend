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
template <
  typename UseHolderT,
  typename UseMaker,
  typename ContinuingUseMaker,
  typename AllowRegisterContinuationIntegralConstantType=std::true_type
>
auto make_captured_use_holder(
  std::shared_ptr<VariableHandleBase> const& var_handle,
  HandleUse::permissions_t requested_scheduling_permissions,
  HandleUse::permissions_t requested_immediate_permissions,
  UseHolderT* source_and_continuing_holder,
  UseMaker&& use_holder_maker,
  ContinuingUseMaker&& continuing_use_holder_maker,
  AllowRegisterContinuationIntegralConstantType = std::true_type{}
) {

  using namespace darma_runtime::detail::flow_relationships;

  static constexpr auto AllowRegisterContinuation =
    AllowRegisterContinuationIntegralConstantType::value;
  using namespace darma_runtime::abstract::frontend;

  // source scheduling permissions shouldn't be None at this point
  DARMA_ASSERT_MESSAGE(
    source_and_continuing_holder->use->scheduling_permissions_ != HandleUse::None,
    "Can't schedule a task on a handle with leaf permissions"
  );

  auto* backend_runtime = abstract::backend::get_backend_runtime();
  std::size_t saved_collection_owner = source_and_continuing_holder->use->collection_owner_;

  std::shared_ptr<UseHolderT> captured_use_holder;

  switch(requested_immediate_permissions) {
    //==========================================================================
    // <editor-fold desc="requested_immediate_permissions = None"> {{{1
    case HandleUse::None: {

      // schedule-only cases

      switch(source_and_continuing_holder->use->immediate_permissions_) {

        //----------------------------------------------------------------------

        case HandleUse::None: { // Source immediate permissions

          // This feels like it should be the dominant use case...
          // switch on the requested scheduling permissions...
          switch(requested_scheduling_permissions) {
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            case HandleUse::None: { // requested scheduling permissions
              // Well, what the heck are you doing with it, then...?
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
              // %   RN -> { NN } -> RN     %
              // %   MN -> { NN } -> MN     %
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
              // Shouldn't be reachable from the current front end, anyway
              DARMA_ASSERT_NOT_IMPLEMENTED();                                   // LCOV_EXCL_LINE
              break;                                                            // LCOV_EXCL_LINE
            } // end case None requested scheduling permissions
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            // <editor-fold desc="Read requested scheduling permissions"> {{{3
            case HandleUse::Read: { // requested scheduling permissions

              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
              // %   RN -> { RN } -> RN     %
              // %   MN -> { RN } -> MN     %
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%

              // We still need to create a new use, because it's a separate task
              captured_use_holder = use_holder_maker(
                var_handle,
                /* Scheduling permissions */
                HandleUse::Read,
                /* Immediate permissions */
                HandleUse::None,
                same_flow(&source_and_continuing_holder->use->in_flow_),
                //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
                same_flow_as_in()
                //FlowRelationship::Same, nullptr, true
#if _darma_has_feature(anti_flows)
                , insignificant_flow(),
                same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
#endif // _darma_has_feature(anti_flows)
              );

              // Continuing use same as source use

              break;
            }
            // </editor-fold> end Read requested scheduling permissions }}}3
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            // <editor-fold desc="Modify requested scheduling permissions"> {{{3
            case HandleUse::Modify: { // requested scheduling permissions

              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
              // %   MN -> { MN } -> MN                      %
              // %   XN -> { MN } -> XN (disallowed here)    %
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

              // Modify schedule-only permissions on a handle with no immediate
              // permissions.

              DARMA_ASSERT_MESSAGE(
                source_and_continuing_holder->use->scheduling_permissions_ == HandleUse::Modify,
                "Can't schedule a (schedule-only) Modify task on a handle without"
                  " modify schedule permissions"
              );

              // We still need to make a next flow...
              // ...actually, this is basically just a regular modify capture with
              // some small exceptions

              captured_use_holder = use_holder_maker(
                var_handle,
                /* Scheduling permissions */
                HandleUse::Modify,
                /* Immediate permissions */
                HandleUse::None,
                same_flow(&source_and_continuing_holder->use->in_flow_),
                //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
                next_of_in_flow()
                //FlowRelationship::Next, nullptr, true
#if _darma_has_feature(anti_flows)
                , insignificant_flow(),
                same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
#endif // _darma_has_feature(anti_flows)
              );


              // Note that this next part is almost exactly the same as the logic for
              // the immediate modify case

              // if the source use was registered, we need to release it now

              // We need to register a new use here, though, since the
              // continuing context will have different flows from the
              // already-registered capturing context

              // Make a new use for the continuing context (since, for some reason
              // or another, we needed one in the source context)

              // now that we've created a continuing context use holder, the source
              // should NOT establish an alias (the continuing use holder will do
              // it instead), so flip the flag here
              source_and_continuing_holder->could_be_alias = false;
              source_and_continuing_holder->replace_use(
                continuing_use_holder_maker(
                  var_handle,
                  /* Scheduling permissions */
                  HandleUse::Modify,
                  /* Immediate permissions */
                  HandleUse::None, // This is a schedule-only use
                  same_flow(&captured_use_holder->use->out_flow_),
                  //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
                  same_flow(&source_and_continuing_holder->use->out_flow_)
                  //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
#if _darma_has_feature(anti_flows)
                  , insignificant_flow(),
                  anti_next_of_in_flow()
#endif // _darma_has_feature(anti_flows)
                ),
                AllowRegisterContinuation
              );

              // This new use could establish an alias if no additional tasks are
              // scheduled on it...
              source_and_continuing_holder->could_be_alias = true;

              break;
            } // end case Modify requested scheduling permissions
            // </editor-fold> end Modify requested scheduling permissions }}}3
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            // <editor-fold desc="Commutative requested scheduling permissions"> {{{3
            case HandleUse::Commutative: { // requested scheduling permissions
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
              // %   CN -> { CN } -> CN                      %
              // %   XN -> { CN } -> XN (disallowed here)    %
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

              DARMA_ASSERT_MESSAGE(
                source_and_continuing_holder->use->scheduling_permissions_
                  == HandleUse::Commutative,
                "Can't make a schedule-only use of handle not in schedule"
                  " commutative mode.  Need to transition handle to commutative"
                  " mode first in outer scope."
              );

              captured_use_holder = use_holder_maker(
                var_handle,
                /* Scheduling permissions */
                HandleUse::Commutative,
                /* Immediate permissions */
                HandleUse::None,
                same_flow(&source_and_continuing_holder->use->in_flow_),
                //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
                same_flow(&source_and_continuing_holder->use->out_flow_)
                //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_, false
#if _darma_has_feature(anti_flows)
                , same_anti_flow(&source_and_continuing_holder->use->anti_in_flow_),
                insignificant_flow()
#endif // _darma_has_feature(anti_flows)
              );

              // No continuation use needed here, since source was schedule-only

              break;
            }
            // </editor-fold> end Commutative requested scheduling permissions }}}3
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            default: {
              DARMA_ASSERT_NOT_IMPLEMENTED();                                   // LCOV_EXCL_LINE
            } // end default
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
          } // end switch on requested scheduling permissions

          break;
        } // end case None source immediate permissions

        //----------------------------------------------------------------------

        case HandleUse::Read: { // source immediate permissions

          switch(requested_scheduling_permissions) {
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            // <editor-fold desc="Read requested scheduling permissions"> {{{3
            case HandleUse::Read : { // requested scheduling permissions
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
              // %   RR -> { RN } -> RR     %
              // %   MR -> { RN } -> MR     %
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%

              // UNTESTED CODE !!!!!!!!!!!!!!!!!!

              captured_use_holder = use_holder_maker(
                var_handle,
                /* Scheduling permissions */
                HandleUse::Read,
                /* Immediate permissions */
                HandleUse::None,
                same_flow(&source_and_continuing_holder->use->in_flow_),
                //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
                same_flow_as_in()
                // FlowRelationship::Same, nullptr, true
#if _darma_has_feature(anti_flows)
                , insignificant_flow(),
                same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
#endif // _darma_has_feature(anti_flows)
              );

              break;
            }
            // </editor-fold> end Read requested scheduling permissions }}}3
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            // <editor-fold desc="Modify requested scheduling permissions"> {{{3
            case HandleUse::Modify : { // requested scheduling permissions
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
              // %   MR -> { MN } -> MN     %
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%

              // UNTESTED CODE !!!!!!!!!!!!!!!!!!

              // Assert that, e.g., RR -> { MN } -> ... is not allowed
              DARMA_ASSERT_MESSAGE(
                source_and_continuing_holder->use->scheduling_permissions_ == HandleUse::Modify,
                "Can't make a Modify schedule-only capture on a handle without"
                  " at Modify schedule permissions"
              );

              captured_use_holder = use_holder_maker(
                var_handle,
                /* Scheduling permissions */
                HandleUse::Modify,
                /* Immediate permissions */
                HandleUse::None,
                forwarding_flow(&source_and_continuing_holder->use->in_flow_),
                //FlowRelationship::Forwarding, &source_and_continuing_holder->use->in_flow_,
                next_of_in_flow()
                //FlowRelationship::Next /* "scheduling next" */, nullptr, true
#if _darma_has_feature(anti_flows)
                , insignificant_flow(),
                same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
#endif // _darma_has_feature(anti_flows)
              );

              source_and_continuing_holder->replace_use(
                continuing_use_holder_maker(
                  var_handle,
                  HandleUse::Modify,
                  HandleUse::None,
                  same_flow(&captured_use_holder->use->out_flow_),
                  //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
                  same_flow(&source_and_continuing_holder->use->out_flow_)
                  //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
#if _darma_has_feature(anti_flows)
                  , insignificant_flow(),
                  anti_next_of_in_flow()
#endif // _darma_has_feature(anti_flows)
                ),
                AllowRegisterContinuation
              );

              break;
            }
            // </editor-fold> end Modify requested scheduling permissions }}}3
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            default: {
              DARMA_ASSERT_NOT_IMPLEMENTED();                                   // LCOV_EXCL_LINE
            } // end default
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
          } // end switch requested_scheduling_permissions


          break;                                                                // LCOV_EXCL_LINE
        }

        //----------------------------------------------------------------------

        case HandleUse::Modify: { // source immediate permissions
          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
          // %   MM -> { RN } -> MR     %
          // %   MM -> { MN } -> MN     %
          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
          DARMA_ASSERT_NOT_IMPLEMENTED(
            "Schedule-only permissions on handles that already have immediate"
              " Modify permissions."
          );                                                                    // LCOV_EXCL_LINE
          break;                                                                // LCOV_EXCL_LINE
        }

        //----------------------------------------------------------------------

        case HandleUse::Commutative : { // source immediate permissions
          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
          // %   CC -> { CN } -> CN     %
          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%

          DARMA_ASSERT_NOT_IMPLEMENTED(
            "Schedule-only permissions on handles that already have immediate"
              " Commutative permissions."
          );                                                                    // LCOV_EXCL_LINE

        }

        //----------------------------------------------------------------------

        default: {
          DARMA_ASSERT_NOT_IMPLEMENTED();                                       // LCOV_EXCL_LINE
        } // end default

        //----------------------------------------------------------------------
      } // end switch on source immediate permissions

      break;

    } // end case None requested immediate permissions
    // </editor-fold> end requested_immediate_permissions = None }}}1
    //==========================================================================
    // <editor-fold desc="requested_immediate_permissions = Read"> {{{1
    case HandleUse::Read: { // requested immediate permissions

      DARMA_ASSERT_MESSAGE(
        (source_and_continuing_holder->use->scheduling_permissions_ & HandleUse::Read) != 0,
        "Can't schedule a read on a handle without at least Read schedule permissions"
      );

      switch(requested_scheduling_permissions) {

        case HandleUse::None:
        case HandleUse::Read: { // requested scheduling permissions
          // This covers the vast majority of uses

          switch(source_and_continuing_holder->use->immediate_permissions_) {
            //----------------------------------------------------------------------
            // <editor-fold desc="None/Read source immediate permissions"> {{{2
            case HandleUse::None:
            case HandleUse::Read: { // source immediate permissions
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%
              // %    RN -> { NR } -> RN   %
              // %    RN -> { RR } -> RN   %
              // %    MN -> { NR } -> MN   %
              // %    MN -> { RR } -> MN   %
              // %    RR -> { NR } -> RR   %
              // %    RR -> { RR } -> RR   %
              // %    MR -> { NR } -> MR   %
              // %    MR -> { RR } -> MR   %
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%

#if _darma_has_feature(anti_flows)
              captured_use_holder = use_holder_maker(
                var_handle,
                /* Scheduling permissions */
                requested_scheduling_permissions,
                /* Immediate permissions */
                HandleUse::Read,
                same_flow(&source_and_continuing_holder->use->in_flow_),
                insignificant_flow(),
                insignificant_flow(),
                same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
              );
#else
              captured_use_holder = use_holder_maker(
                var_handle,
                /* Scheduling permissions */
                requested_scheduling_permissions,
                /* Immediate permissions */
                HandleUse::Read,
                same_flow(&source_and_continuing_holder->use->in_flow_),
                //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
                same_flow_as_in()
                // FlowRelationship::Same, nullptr, true
              );

              // The continuation use is the source use here

#endif // _darma_has_feature(anti_flows)


              break;

            } // end case Read or None immediate permissions on source
            // </editor-fold> end None/Read source immediate permissions }}}2
            //----------------------------------------------------------------------
            // <editor-fold desc="Modify source immediate permissions"> {{{2
            case HandleUse::Modify: { // source immediate permissions
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%
              // %    RM -> { NR } -> RR   %
              // %    RM -> { RR } -> RR   %
              // %    MM -> { NR } -> MR   %
              // %    MM -> { RR } -> MR   %
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%

              // TODO if the handle doesn't have modify schedule permissions, this logic could be completely different
              // (i.e., instead of making a forwarding flow, we could just use
              // the output flow)

              // This is a ro capture of a handle with modify immediate permissions

              // we need to make a forwarded flow and register a new use in the
              // continuing context
              captured_use_holder = use_holder_maker(
                var_handle,
                /* Scheduling permissions */
                requested_scheduling_permissions,
                /* Immediate permissions */
                HandleUse::Read,
                forwarding_flow(&source_and_continuing_holder->use->in_flow_),
                //FlowRelationship::Forwarding, &source_and_continuing_holder->use->in_flow_,
#if _darma_has_feature(anti_flows)
                // Out flow
                insignificant_flow(),
                // Anti-in flow
                insignificant_flow(),
                // Anti-out flow
                next_of_in_flow()
#else
                same_flow_as_in()
                // FlowRelationship::Same, nullptr, true
#endif // _darma_has_feature(anti_flows)
              );

              // The continuing context actually needs to have a Use as well,
              // since it has access to the underlying data...

              // Also, this continuing context Use has to be registered before the
              // source use is released
              // make another shared ptr to the source use holder so that we can
              // release it
              void* old_ptr = source_and_continuing_holder->use->get_data_pointer_reference();

              // now make the use for the continuing context
              source_and_continuing_holder->replace_use(
                continuing_use_holder_maker(
                  var_handle,
                  /* Scheduling permissions: (should be unchanged) */
                  source_and_continuing_holder->use->scheduling_permissions_,
                  /* Immediate permissions: */
                  HandleUse::Read,
                  same_flow(&captured_use_holder->use->in_flow_),
                  //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
                  // It still carries the out flow of the task, though, and should
                  // establish an alias on release if there are no more modifies
                  same_flow(&source_and_continuing_holder->use->out_flow_)
                  //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
#if _darma_has_feature(anti_flows)
                  , insignificant_flow(),
                  same_anti_flow(&captured_use_holder->use->anti_out_flow_)
#endif // _darma_has_feature(anti_flows)
                ),
                AllowRegisterContinuation
              );
              // But this *can* still establish an alias (if it has Modify
              // scheduling permissions) because it could be the one that detects
              // that the forwarding flow aliases the out flow (i.e., that there
              // are no more modifies)
              source_and_continuing_holder->could_be_alias = true;

              // We can go ahead and pass on the underlying pointer, though, since
              // the Use is associated with a handle in a context that's uninterruptible
              void*& new_ptr = source_and_continuing_holder->use->get_data_pointer_reference();
              // The backend isn't allowed to change the pointer at this stage,
              // since it's in the middle of a task
              assert(new_ptr == nullptr);
              new_ptr = old_ptr;

              break;

            } // end case Modify immediate permissions on source
            // </editor-fold> end Modify source immediate permissions }}}2
            //----------------------------------------------------------------------
            default: {
              DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
            } // end default
            //----------------------------------------------------------------------
          } // end switch on source immediate permissions

          break;
        } // end case None or Read requested scheduling permissions (requested NR or RR)
        //----------------------------------------------------------------------
        case HandleUse::Modify: { // requested scheduling permissions
          // We're requesting MR, which is unusual but not impossible

          DARMA_ASSERT_MESSAGE(
            (source_and_continuing_holder->use->scheduling_permissions_ == HandleUse::Modify),
            "Can't capture a handle with modify schedule permissions without"
              " Modify schedule permissions in enclosing scope"
          );

          switch(source_and_continuing_holder->use->immediate_permissions_) {
            //----------------------------------------------------------------------
            case HandleUse::None: { // source immediate permissions
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
              // %   MN -> { MR } -> MN     %
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
              // this case is an MR capture of MN

              captured_use_holder = use_holder_maker(
                var_handle,
                /* Scheduling permissions */
                HandleUse::Modify,
                /* Immediate permissions */
                HandleUse::Read,
                same_flow(&source_and_continuing_holder->use->in_flow_),
                //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
                next_of_in_flow()
                //FlowRelationship::Next, nullptr, true
#if _darma_has_feature(anti_flows)
                // Anti-in
                , insignificant_flow(),
                // Anti-out
                same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
#endif // _darma_has_feature(anti_flows)
              );
              captured_use_holder->could_be_alias = true;

              // the source should *not* establish an alias when potentially
              // released, in the replace below, since the continuation will
              // handle that
              source_and_continuing_holder->could_be_alias = false;

              source_and_continuing_holder->replace_use(
                continuing_use_holder_maker(
                  var_handle,
                  HandleUse::Modify,
                  HandleUse::None,
                  same_flow(&captured_use_holder->use->out_flow_),
                  //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
                  same_flow(&source_and_continuing_holder->use->out_flow_)
                  //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
#if _darma_has_feature(anti_flows)
                  // Anti-in
                  , insignificant_flow(),
                  // Anti-out
                  same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
#endif // _darma_has_feature(anti_flows)
                ),
                AllowRegisterContinuation
              );
              source_and_continuing_holder->could_be_alias = true;

              break;
            } // end case source immediate permissions None or Read
            //------------------------------------------------------------------
            case HandleUse::Read: { // source immediate permissions
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
              // %   MR -> { MR } -> MN     %
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
              // TODO Read case
              DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
              break;
            } // end source immediate permissions Read
            //------------------------------------------------------------------
            case HandleUse::Modify: { // source immediate permissions
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
              // %   MM -> { MR } -> MN     %
              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%

              captured_use_holder = use_holder_maker(
                var_handle,
                /* Scheduling permissions */
                HandleUse::Modify,
                /* Immediate permissions */
                HandleUse::Read,
                forwarding_flow(&source_and_continuing_holder->use->in_flow_),
                //FlowRelationship::Forwarding, &source_and_continuing_holder->use->in_flow_,
                next_of_in_flow()
                //FlowRelationship::Next, nullptr, true
#if _darma_has_feature(anti_flows)
                // Anti-in
                , insignificant_flow(),
                // Anti-out
                anti_next_of_in_flow()
#endif // _darma_has_feature(anti_flows)
              );
              captured_use_holder->could_be_alias = true;

              source_and_continuing_holder->replace_use(
                continuing_use_holder_maker(
                  var_handle,
                  HandleUse::Modify, HandleUse::None,
                  same_flow(&captured_use_holder->use->out_flow_),
                  //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
                  same_flow(&source_and_continuing_holder->use->out_flow_)
                  //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
#if _darma_has_feature(anti_flows)
                  // Anti-in
                  , insignificant_flow(),
                  // Anti-out
                  same_anti_flow(&captured_use_holder->use->anti_out_flow_)
#endif // _darma_has_feature(anti_flows)
                ),
                AllowRegisterContinuation
              );
              source_and_continuing_holder->could_be_alias = true;

              break;

            } // end case source immediate permissions Modify
            //------------------------------------------------------------------
            default: {
              DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
            } // end default
            //------------------------------------------------------------------
          } // end switch on source immediate permissions

          break;
        } // end case requested MR
        //----------------------------------------------------------------------
        default: {
          DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
        } // end default
        //----------------------------------------------------------------------
      } // end switch on requested immediate permissions

      break;

    } // end case Read requested immediate permissions
    // </editor-fold> end requested_immediate_permissions = Read }}}1
    //==========================================================================
    // <editor-fold desc="requested_immediate_permissions = Modify"> {{{1
    case HandleUse::Modify: { // requested immediate permissions

      DARMA_ASSERT_MESSAGE(
        source_and_continuing_holder->use->scheduling_permissions_ == HandleUse::Modify,
        "Can't schedule a modify on a handle without Modify schedule permissions"
      );

      switch(source_and_continuing_holder->use->immediate_permissions_) {
        //----------------------------------------------------------------------
        // <editor-fold desc="None source immediate permissions"> {{{2
        case HandleUse::None: {
          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
          // %   MN -> { NM } -> MN     %
          // %   MN -> { RM } -> MN     %
          // %   MN -> { MM } -> MN     %
          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%

          // Modify capture of handle without modify immediate permissions

          // make the captured use holder
          captured_use_holder = use_holder_maker(
            var_handle,
            /* Scheduling permissions */
            requested_scheduling_permissions,
            /* Immediate permissions */
            HandleUse::Modify,
            same_flow(&source_and_continuing_holder->use->in_flow_),
            //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
            next_of_in_flow()
            //FlowRelationship::Next, nullptr, true
#if _darma_has_feature(anti_flows)
            // The whole point of having anti-out flows is to allow it to be the
            // anti-in flow of a captured use with XM permissions
            , same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_),
            // XM permissions never carry an anti-out, since nothing follows them
            // except (by definition) a different generation of the data
            insignificant_flow()
#endif // _darma_has_feature(anti_flows)
          );

          // now that we're created a continuing context use holder, the source
          // should NOT establish an alias (the continuing use holder will do
          // it instead), so flip the flag here
          source_and_continuing_holder->could_be_alias = false;

          source_and_continuing_holder->replace_use(
            continuing_use_holder_maker(
              var_handle,
              /* Scheduling permissions */
              HandleUse::Modify,
              /* Immediate permissions */
              HandleUse::None, // This is a schedule-only use
              same_flow(&captured_use_holder->use->out_flow_),
              //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
              same_flow(&source_and_continuing_holder->use->out_flow_)
              //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
#if _darma_has_feature(anti_flows)
              // anti-in =
              , insignificant_flow(),
              // anti-out =
              anti_next_of_in_flow()
#endif // _darma_has_feature(anti_flows)
            ),
            AllowRegisterContinuation
          );

          // This new use could establish an alias if no additional tasks are
          // scheduled on it...
          source_and_continuing_holder->could_be_alias = true;

          break;
        } // end case None source immediate permissions
        // </editor-fold> end None source immediate permissions }}}2
        //----------------------------------------------------------------------
        // <editor-fold desc="Read source immediate permissions"> {{{1
        case HandleUse::Read: { // source immediate permissions
          // Requesting XM capture of MR
          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
          // %   MR -> { NM } -> MN     %
          // %   MR -> { RM } -> MN     %
          // %   MR -> { MM } -> MN     %
          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%

          // Create the out flow  (should this be make_forwarding_flow?)
          // (probably not, since RR -> { RR } -> RR doesn't do forwarding?)

          // make the captured use holder
          captured_use_holder = use_holder_maker(
            var_handle,
            /* Scheduling permissions */
            requested_scheduling_permissions,
            /* Immediate permissions */
            HandleUse::Modify,
            same_flow(&source_and_continuing_holder->use->in_flow_),
            //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
            next_of_in_flow()
            //FlowRelationship::Next, nullptr, true
#if _darma_has_feature(anti_flows)
            // Anti-in flow
            , same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
            // Anti-out flow
            , insignificant_flow()
#endif // _darma_has_feature(anti_flows)
          );

          // register the captured use

          source_and_continuing_holder->replace_use(
            continuing_use_holder_maker(
              var_handle,
              source_and_continuing_holder->use->scheduling_permissions_,
              HandleUse::None,
              same_flow(&captured_use_holder->use->out_flow_),
              //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
              same_flow(&source_and_continuing_holder->use->out_flow_)
              //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
#if _darma_has_feature(anti_flows)
              // Anti-in flow
              , insignificant_flow(),
              // Anti-out flow
              anti_next_of_in_flow()
#endif // _darma_has_feature(anti_flows)
            ),
            AllowRegisterContinuation
          );

          source_and_continuing_holder->could_be_alias = true;
          // out flow and scheduling permissions unchanged

          break;
        }
        // </editor-fold> end Read source immediate permissions  }}}1
        //----------------------------------------------------------------------
        // <editor-fold desc="modify source immediate permissions"> {{{1
        case HandleUse::Modify: {
          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
          // %   MM -> { NM } -> MN     %
          // %   MM -> { RM } -> MN     %
          // %   MM -> { MM } -> MN     %
          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%

          // Modify capture of handle with modify immediate permissions

          // we need to forward the input flow, since it could have changed
          // (because the source has modify immediate permissions)

          // make the captured use holder
          captured_use_holder = use_holder_maker(
            var_handle,
            /* Scheduling permissions */
            requested_scheduling_permissions,
            /* Immediate permissions */
            HandleUse::Modify,
            forwarding_flow(&source_and_continuing_holder->use->in_flow_),
            //FlowRelationship::Forwarding, &source_and_continuing_holder->use->in_flow_,
            next_of_in_flow()
            //FlowRelationship::Next, nullptr, true
#if _darma_has_feature(anti_flows)
            // Anti-in flow
            // because of the way forwarding works, this can never carry anti-dependencies
            , insignificant_flow(),
            // Anti-out flow
            insignificant_flow()
#endif // _darma_has_feature(anti_flows)
          );

          source_and_continuing_holder->replace_use(
            continuing_use_holder_maker(
              var_handle,
              source_and_continuing_holder->use->scheduling_permissions_,
              HandleUse::None,
              same_flow(&captured_use_holder->use->out_flow_),
              //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
              same_flow(&source_and_continuing_holder->use->out_flow_)
              //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
#if _darma_has_feature(anti_flows)
              // Anti-in flow
              , insignificant_flow(),
              // Anti-out flow
              anti_next_of_in_flow()
#endif // _darma_has_feature(anti_flows)
            ),
            AllowRegisterContinuation
          );
          source_and_continuing_holder->could_be_alias = true;

          break;
        }
        // </editor-fold> end modify source immediate permissions  }}}1
        //----------------------------------------------------------------------
        default: {
          DARMA_ASSERT_NOT_IMPLEMENTED();                                       // LCOV_EXCL_LINE
        } // end default
        //----------------------------------------------------------------------
      } // end switch on source immediate permissions

      break;
    } // end case Modify requested immediate permissions
    // </editor-fold> end requested_immediate_permissions = Modify }}}1
    //==========================================================================
    // <editor-fold desc="requested_immediate_permissions = Commutative"> {{{1
    case HandleUse::Commutative: { // requested immediate permissions

      DARMA_ASSERT_MESSAGE(
        source_and_continuing_holder->use->scheduling_permissions_ == HandleUse::Commutative,
        "Can't schedule a commutative use without commutative scheduling permissions"
      );
      DARMA_ASSERT_MESSAGE(
        source_and_continuing_holder->use->immediate_permissions_ == HandleUse::None
          or source_and_continuing_holder->use->immediate_permissions_ == HandleUse::Commutative,
        "Can't create commutative task on handle without None or Commutative immediate permissions"
      );

      // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
      // %   CN -> { CC } -> CN     %
      // %   CC -> { CC } -> CN     %
      // %   CN -> { NC } -> CN     %
      // %   CC -> { NC } -> CN     %
      // %%%%%%%%%%%%%%%%%%%%%%%%%%%%

      captured_use_holder = use_holder_maker(
        var_handle,
        /* Scheduling permissions */
        requested_scheduling_permissions,
        /* Immediate permissions */
        HandleUse::Commutative,
        same_flow(&source_and_continuing_holder->use->in_flow_),
        //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
        same_flow(&source_and_continuing_holder->use->out_flow_)
        //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
#if _darma_has_feature(anti_flows)
        , same_anti_flow(&source_and_continuing_holder->use->anti_in_flow_)
        , insignificant_flow()
#endif
      );

      // TODO register this and make the continuation use have none immediate permissions
//#if _darma_has_feature(register_commutative_continuation_uses)
//
//      source_and_continuing_holder->replace_use(
//        continuing_use_holder_maker(
//          var_handle,
//          /* Scheduling permissions */
//          source_and_continuing_holder->use->scheduling_permissions_,
//          /* Immediate permissions */
//          source_and_continuing_holder->use->immediate_permissions_,
//          FlowRelationship::Same, &captured_use_holder->use->in_flow_,
//          FlowRelationship::Same, &captured_use_holder->use->out_flow_
//        ),
//        AllowRegisterContinuation
//      );
//
//#endif // _darma_has_feature(register_commutative_continuation_uses)


      // Note that permissions/use/etc of source_and_continuing are unchanged

      break;
    } // end requested immediate permissions commutative
    // </editor-fold> end requested_immediate_permissions = Commutative }}}1
    //==========================================================================

    default: {
      DARMA_ASSERT_NOT_IMPLEMENTED();                                           // LCOV_EXCL_LINE
    } // end default

    //==========================================================================
  } // end switch requested immediate permissions

  // Propagate the collection owner information
  captured_use_holder->use->collection_owner_ = saved_collection_owner;
  // Also for the continuing, in case there's been a change
  source_and_continuing_holder->use->collection_owner_ = saved_collection_owner;

  return captured_use_holder;
}


template <bool AllowRegisterContinuation=true>
auto
make_captured_use_holder(
  std::shared_ptr<VariableHandleBase> const& var_handle,
  HandleUse::permissions_t requested_scheduling_permissions,
  HandleUse::permissions_t requested_immediate_permissions,
  UseHolder* source_and_continuing_holder
) {
  // TODO is_dependency_ pass through here, for use with allreduce and publish, etc?
  return make_captured_use_holder(
    var_handle, requested_scheduling_permissions, requested_immediate_permissions,
    source_and_continuing_holder,
    /* Use holder maker */
    [](auto&&... args) {
      using namespace darma_runtime::detail;
      auto rv = std::make_shared<GenericUseHolder<HandleUse>>(HandleUse(
        std::forward<decltype(args)>(args)...
      ), true, true);
      return rv;
    },
    /* continuing use maker */
    [](auto&&... args) {
      using namespace darma_runtime::detail;
      HandleUse rv(
        std::forward<decltype(args)>(args)...
      );
      rv.is_dependency_ = false;
      return rv;
    },
    std::integral_constant<bool, AllowRegisterContinuation>{}
  );
}


} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_CAPTURE_H
