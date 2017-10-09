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
#include <darma/impl/capture/semantics.h>
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
  typename register_continuation_useIntegralConstantType=std::true_type
>
inline auto
make_captured_use_holder(
  std::shared_ptr<VariableHandleBase> const& var_handle,
  frontend::Permissions requested_scheduling_permissions,
  frontend::Permissions requested_immediate_permissions,
  UseHolderT* source_and_continuing_holder,
  UseMaker&& use_holder_maker,
  ContinuingUseMaker&& continuing_use_holder_maker,
  bool register_continuation_use = true
) {

  using namespace darma_runtime::detail::flow_relationships;

  using namespace darma_runtime::abstract::frontend;
  using darma_runtime::frontend::Permissions;
  using darma_runtime::frontend::CoherenceMode;

  // source scheduling permissions shouldn't be None at this point
  DARMA_ASSERT_MESSAGE(
    source_and_continuing_holder->use->scheduling_permissions_ != frontend::Permissions::None,
    "Can't schedule a task on a handle with leaf permissions"
  );

  auto* backend_runtime = abstract::backend::get_backend_runtime();
  std::size_t saved_collection_owner = source_and_continuing_holder->use->collection_owner_;

  std::shared_ptr<UseHolderT> captured_use_holder;

  capture_semantics::apply_with_capture_case(
    source_and_continuing_holder->use->scheduling_permissions_,
    source_and_continuing_holder->use->immediate_permissions_,
    requested_scheduling_permissions,
    requested_immediate_permissions,
    source_and_continuing_holder->use->coherence_mode_,
    [&](auto capture_case) {
      auto* source_in = &source_and_continuing_holder->use->in_flow_;
      auto* source_out = &source_and_continuing_holder->use->out_flow_;
      auto* source_anti_in = &source_and_continuing_holder->use->anti_in_flow_;
      auto* source_anti_out = &source_and_continuing_holder->use->anti_out_flow_;

      // make the captured use
      captured_use_holder = use_holder_maker(
        var_handle,
        capture_case.captured_scheduling,
        capture_case.captured_immediate,
        capture_case.captured_in_flow_relationship(
          source_in, source_out
        ),
        capture_case.captured_out_flow_relationship(
          source_in, source_out
        ),
        capture_case.captured_anti_in_flow_relationship(
          source_in, source_out, source_anti_in, source_anti_out
        ),
        capture_case.captured_anti_out_flow_relationship(
          source_in, source_out, source_anti_in, source_anti_out
        )
      );

      captured_use_holder->could_be_alias = capture_case.could_be_alias;

      // TODO this branch could/should be done statically
      if(capture_case.needs_new_continuation_use) {

        auto* captured_in = &captured_use_holder->use->in_flow_;
        auto* captured_out = &captured_use_holder->use->out_flow_;
        auto* captured_anti_in = &captured_use_holder->use->anti_in_flow_;
        auto* captured_anti_out = &captured_use_holder->use->anti_out_flow_;

        // If a new use will be made and immediate data will be accessible
        // in the continuation, we need to get the pointer here before the old
        // use goes away so that we can give it to the new use.
        void* old_ptr = nullptr;
        if(capture_case.continuation_immediate != Permissions::None) {
          assert(capture_case.source_immediate != Permissions::None);
          old_ptr = source_and_continuing_holder->use->data_;
        }

        // Make sure an alias isn't established on the source
        source_and_continuing_holder->could_be_alias = false;

        // Make the continuation use
        source_and_continuing_holder->replace_use(
          continuing_use_holder_maker(
            var_handle,
            capture_case.continuation_scheduling,
            capture_case.continuation_immediate,
            capture_case.continuation_in_flow_relationship(
              source_in, source_out,
              captured_in, captured_out
            ),
            capture_case.continuation_out_flow_relationship(
              source_in, source_out,
              captured_in, captured_out
            ),
            capture_case.continuation_anti_in_flow_relationship(
              source_in, source_out, captured_in, captured_out,
              source_anti_in, source_anti_out, captured_anti_in, captured_anti_out
            ),
            capture_case.continuation_anti_out_flow_relationship(
              source_in, source_out, captured_in, captured_out,
              source_anti_in, source_anti_out, captured_anti_in, captured_anti_out
            )
          ),
          register_continuation_use
        );

        // If a new use is made but it has access to the same data, pass the pointer
        // on to the new use
        if(capture_case.continuation_immediate != Permissions::None) {
          assert(old_ptr);
          source_and_continuing_holder->use->data_ = old_ptr;
        }

        // continuation can always be an alias if it's new
        source_and_continuing_holder->could_be_alias = true;

      }


    }
  );

  // Propagate the collection owner information
  captured_use_holder->use->collection_owner_ = saved_collection_owner;
  // Also for the continuing, in case there's been a change
  source_and_continuing_holder->use->collection_owner_ = saved_collection_owner;

  return captured_use_holder;
}


inline auto
make_captured_use_holder(
  std::shared_ptr<VariableHandleBase> const& var_handle,
  frontend::Permissions requested_scheduling_permissions,
  frontend::Permissions requested_immediate_permissions,
  UseHolder* source_and_continuing_holder,
  bool register_continuation_use = true
) {
  // TODO is_dependency_ pass through here, for use with allreduce and publish, etc?
  return make_captured_use_holder(
    var_handle, requested_scheduling_permissions, requested_immediate_permissions,
    source_and_continuing_holder,
    /* Use holder maker */
    [](auto&& varhandle, auto sched, auto immed, auto&&... args) {
      using namespace darma_runtime::detail;
      auto rv = std::make_shared<GenericUseHolder<HandleUse>>(HandleUse(
        std::forward<decltype(varhandle)>(varhandle),
        sched, immed,
        std::forward<decltype(args)>(args)...
      ), true, true);
      return rv;
    },
    /* continuing use maker */
    [](auto&& varhandle, auto sched, auto immed, auto&&... args) {
      using namespace darma_runtime::detail;
      HandleUse rv(
        std::forward<decltype(varhandle)>(varhandle),
        sched, immed,
        std::forward<decltype(args)>(args)...
      );
      rv.is_dependency_ = false;
      return rv;
    },
    register_continuation_use
  );
}

} // end namespace detail
} // end namespace darma_runtime

// OLD CODE!!!
//  switch(source_and_continuing_holder->use->coherence_mode_) {
//    //==========================================================================
//    // <editor-fold desc="Sequential coherence mode"> {{{1
//    case CoherenceMode::Sequential: {
//      switch(requested_immediate_permissions) {
//        //==========================================================================
//        // <editor-fold desc="requested_immediate_permissions = None"> {{{1
//        case Permissions::None: {
//
//          // schedule-only cases
//
//          switch(source_and_continuing_holder->use->immediate_permissions_) {
//
//            //----------------------------------------------------------------------
//            // <editor-fold desc="None source immediate permissions"> {{{3
//            case Permissions::None: { // Source immediate permissions
//
//              // This feels like it should be the dominant use case...
//              // switch on the requested scheduling permissions...
//              switch(requested_scheduling_permissions) {
//                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                case Permissions::None: { // requested scheduling permissions
//                  // Well, what the heck are you doing with it, then...?
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // %   RN -> { NN } -> RN     %
//                  // %   MN -> { NN } -> MN     %
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // Shouldn't be reachable from the current front end, anyway
//                  DARMA_ASSERT_NOT_IMPLEMENTED();                                   // LCOV_EXCL_LINE
//                  break;                                                            // LCOV_EXCL_LINE
//                } // end case None requested scheduling permissions
//                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                // <editor-fold desc="Read requested scheduling permissions"> {{{3
//                  case Permissions::Read: { // requested scheduling permissions
//
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // %   RN -> { RN } -> RN     %
//                  // %   MN -> { RN } -> MN     %
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // TODO @antiflows implement and test antiflows for this case
////                  #if _darma_has_feature(anti_flows)
////                  DARMA_ASSERT_NOT_IMPLEMENTED("Anti-flows for XN -> { RN } capture case");
////                  #endif
////
////                  // We still need to create a new use, because it's a separate task
////                  captured_use_holder = use_holder_maker(
////                  var_handle,
////                  /* Scheduling permissions */
////                  Permissions::Read,
////                  /* Immediate permissions */
////                  Permissions::None,
////                  same_flow(&source_and_continuing_holder->use->in_flow_),
////                  //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
////                  same_flow_as_in()
////                  //FlowRelationship::Same, nullptr, true
////                  #if _darma_has_feature(anti_flows)
////                  , insignificant_flow(),
////                  same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
////                  #endif // _darma_has_feature(anti_flows)
////                  );
//
//                  // Continuing use same as source use
//
//                  break;
//                  }
//                  // </editor-fold> end Read requested scheduling permissions }}}3
//                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                // <editor-fold desc="Modify requested scheduling permissions"> {{{3
//                  case Permissions::Modify: { // requested scheduling permissions
//
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // %   MN -> { MN } -> MN                      %
//                  // %   XN -> { MN } -> XN (disallowed here)    %
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//                  // Modify schedule-only permissions on a handle with no immediate
//                  // permissions.
//
////                  DARMA_ASSERT_MESSAGE(
////                  source_and_continuing_holder->use->scheduling_permissions_ == Permissions::Modify,
////                  "Can't schedule a (schedule-only) Modify task on a handle without"
////                  " modify schedule permissions"
////                  );
////
////                  // We still need to make a next flow...
////                  // ...actually, this is basically just a regular modify capture with
////                  // some small exceptions
////
////                  captured_use_holder = use_holder_maker(
////                  var_handle,
////                  /* Scheduling permissions */
////                  Permissions::Modify,
////                  /* Immediate permissions */
////                  Permissions::None,
////                  same_flow(&source_and_continuing_holder->use->in_flow_),
////                  //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
////                  next_of_in_flow()
////                  //FlowRelationship::Next, nullptr, true
////                  #if _darma_has_feature(anti_flows)
////                  , next_anti_flow(&source_and_continuing_holder->use->anti_out_flow_),
////                  same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
////                  #endif // _darma_has_feature(anti_flows)
////                  );
////                  // The in and out are different, but we don't know if any tasks
////                  // will be scheduled that will make them actually different, so
////                  // the captured holder could establish an alias
////                  captured_use_holder->could_be_alias = true;
////
////
////                  // Note that this next part is almost exactly the same as the logic for
////                  // the immediate modify case
////
////                  // if the source use was registered, we need to release it now
////
////                  // We need to register a new use here, though, since the
////                  // continuing context will have different flows from the
////                  // already-registered capturing context
////
////                  // Make a new use for the continuing context (since, for some reason
////                  // or another, we needed one in the source context)
////
////                  // now that we've created a continuing context use holder, the source
////                  // should NOT establish an alias (the continuing use holder will do
////                  // it instead), so flip the flag here
////                  source_and_continuing_holder->could_be_alias = false;
////                  source_and_continuing_holder->replace_use(
////                  continuing_use_holder_maker(
////                  var_handle,
////                  /* Scheduling permissions */
////                  Permissions::Modify,
////                  /* Immediate permissions */
////                  Permissions::None, // This is a schedule-only use
////                  same_flow(&captured_use_holder->use->out_flow_),
////                  //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
////                  same_flow(&source_and_continuing_holder->use->out_flow_)
////                  //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
////                  #if _darma_has_feature(anti_flows)
////                  , same_anti_flow(&source_and_continuing_holder->use->anti_in_flow_),
////                  same_anti_flow(&captured_use_holder->use->anti_in_flow_)
////                  #endif // _darma_has_feature(anti_flows)
////                  ),
////                  register_continuation_use
////                  );
////
////                  // This new use could establish an alias if no additional tasks are
////                  // scheduled on it...
////                  source_and_continuing_holder->could_be_alias = true;
//
//                  break;
//                  } // end case Modify requested scheduling permissions
//                  // </editor-fold> end Modify requested scheduling permissions }}}3
//                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                default: {
//                  DARMA_ASSERT_NOT_IMPLEMENTED();                                   // LCOV_EXCL_LINE
//                } // end default
//                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//              } // end switch on requested scheduling permissions
//
//              break;
//            } // end case None source immediate permissions
//            // </editor-fold> end None source immediate permissions }}}3
//            //----------------------------------------------------------------------
//            // <editor-fold desc="Read source immediate permissions"> {{{3
//            case Permissions::Read: { // source immediate permissions
//
//              // Mostly untested/unimplemented because these are corner cases...
//
//              switch(requested_scheduling_permissions) {
//                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                // <editor-fold desc="Read requested scheduling permissions"> {{{3
//                case Permissions::Read : { // requested scheduling permissions
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // %   RR -> { RN } -> RR     %
//                  // %   MR -> { RN } -> MR     %
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//                  // UNTESTED CODE !!!!!!!!!!!!!!!!!!
//                  // TODO @antiflows test antiflows for this case
//
////                  captured_use_holder = use_holder_maker(
////                    var_handle,
////                    /* Scheduling permissions */
////                    Permissions::Read,
////                    /* Immediate permissions */
////                    Permissions::None,
////                    same_flow(&source_and_continuing_holder->use->in_flow_),
////                    //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
////                    // FlowRelationship::Same, nullptr, true
////    #if _darma_has_feature(anti_flows)
////                    insignificant_flow()
////                      , insignificant_flow(),
////                      same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
////    #else
////                    same_flow_as_in()
////    #endif // _darma_has_feature(anti_flows)
////                  );
//
//                  break;
//                }
//                // </editor-fold> end Read requested scheduling permissions }}}3
//                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                // <editor-fold desc="Modify requested scheduling permissions"> {{{3
//                case Permissions::Modify : { // requested scheduling permissions
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // %   MR -> { MN } -> MN     %
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
////                  // TODO @antiflows test antiflows for this case
////
////                  // Assert that, e.g., RR -> { MN } -> ... is not allowed
////                  DARMA_ASSERT_MESSAGE(
////                    source_and_continuing_holder->use->scheduling_permissions_ == Permissions::Modify,
////                    "Can't make a Modify schedule-only capture on a handle without"
////                      " at Modify schedule permissions"
////                  );
////
////                  captured_use_holder = use_holder_maker(
////                    var_handle,
////                    /* Scheduling permissions */
////                    Permissions::Modify,
////                    /* Immediate permissions */
////                    Permissions::None,
////                    forwarding_flow(&source_and_continuing_holder->use->in_flow_),
////                    //FlowRelationship::Forwarding, &source_and_continuing_holder->use->in_flow_,
////                    next_of_in_flow()
////                    //FlowRelationship::Next /* "scheduling next" */, nullptr, true
////    #if _darma_has_feature(anti_flows)
////                    , next_anti_flow(&source_and_continuing_holder->use->anti_out_flow_),
////                      same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
////    #endif // _darma_has_feature(anti_flows)
////                  );
////
////                  source_and_continuing_holder->replace_use(
////                    continuing_use_holder_maker(
////                      var_handle,
////                      Permissions::Modify,
////                      Permissions::None,
////                      same_flow(&captured_use_holder->use->out_flow_),
////                      //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
////                      same_flow(&source_and_continuing_holder->use->out_flow_)
////                      //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
////    #if _darma_has_feature(anti_flows)
////                      , same_anti_flow(&source_and_continuing_holder->use->anti_in_flow_),
////                        same_anti_flow(&captured_use_holder->use->anti_in_flow_)
////    #endif // _darma_has_feature(anti_flows)
////                    ),
////                    register_continuation_use
////                  );
//
//                  break;
//                }
//                // </editor-fold> end Modify requested scheduling permissions }}}3
//                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                default: {
//                  DARMA_ASSERT_NOT_IMPLEMENTED();                                   // LCOV_EXCL_LINE
//                } // end default
//                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//              } // end switch requested_scheduling_permissions
//
//
//              break;                                                                // LCOV_EXCL_LINE
//            }
//            // </editor-fold> end Read source immediate permissions }}}3
//            //----------------------------------------------------------------------
//            // <editor-fold desc="Modify source immediate permissions"> {{{3
//            case Permissions::Modify: { // source immediate permissions
//
//              switch(requested_scheduling_permissions) {
//                case Permissions::Read : { // requested scheduling permissions
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // %   MM -> { RN } -> MR     %
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  DARMA_ASSERT_NOT_IMPLEMENTED(
//                    "Schedule-only Read permissions on handles that already have immediate"
//                      " Modify permissions."
//                  );                                                                    // LCOV_EXCL_LINE
//                  break;
//                } // end case Read requested scheduling permissions
//                //------------------------------------------------------------------
//                case Permissions::Modify : { // requested scheduling permissions
//
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // %   MM -> { MN } -> MN     %
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//                  // Tested in, e.g., TestAntiFlows.high_level_example_3
//
////                  captured_use_holder = use_holder_maker(
////                    var_handle,
////                    /* Scheduling permissions */
////                    Permissions::Modify,
////                    /* Immediate permissions */
////                    Permissions::None,
////                    forwarding_flow(&source_and_continuing_holder->use->in_flow_),
////                    //FlowRelationship::Forwarding, &source_and_continuing_holder->use->in_flow_,
////                    next_of_in_flow()
////                    //FlowRelationship::Next /* "scheduling next" */, nullptr, true
////    #if _darma_has_feature(anti_flows)
////                    , next_anti_flow(&source_and_continuing_holder->use->anti_out_flow_),
////                      next_anti_flow_of_anti_in()
////    #endif // _darma_has_feature(anti_flows)
////                  );
////                  captured_use_holder->could_be_alias = true;
////
////                  source_and_continuing_holder->could_be_alias = false;
////                  source_and_continuing_holder->replace_use(
////                    continuing_use_holder_maker(
////                      var_handle,
////                      Permissions::Modify,
////                      Permissions::None,
////                      same_flow(&captured_use_holder->use->out_flow_),
////                      //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
////                      same_flow(&source_and_continuing_holder->use->out_flow_)
////                      //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
////    #if _darma_has_feature(anti_flows)
////                      , same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_),
////                        same_anti_flow(&captured_use_holder->use->anti_in_flow_)
////    #endif // _darma_has_feature(anti_flows)
////                    ),
////                    register_continuation_use
////                  );
////                  source_and_continuing_holder->could_be_alias = true;
//
//                  break;
//
//                } // end case Modify requested scheduling permissions
//                //------------------------------------------------------------------
//                default: {
//                  DARMA_ASSERT_NOT_IMPLEMENTED();                                   // LCOV_EXCL_LINE
//                } // end default
//                //------------------------------------------------------------------
//              } // end switch requested scheduling permissions
//
//              break;                                                                // LCOV_EXCL_LINE
//            }
//            // </editor-fold> end Modify source immediate permissions }}}3
//            //----------------------------------------------------------------------
//            default: {
//              DARMA_ASSERT_NOT_IMPLEMENTED();                                       // LCOV_EXCL_LINE
//            } // end default
//            //----------------------------------------------------------------------
//          } // end switch on source immediate permissions
//
//          break;
//
//        } // end case None requested immediate permissions
//        // </editor-fold> end requested_immediate_permissions = None }}}1
//        //==========================================================================
//        // <editor-fold desc="requested_immediate_permissions = Read"> {{{1
//        case Permissions::Read: { // requested immediate permissions
//
//          DARMA_ASSERT_MESSAGE(
//            ((int)source_and_continuing_holder->use->scheduling_permissions_ & (int)Permissions::Read) != 0,
//            "Can't schedule a read on a handle without at least Read schedule permissions"
//          );
//
//          switch(requested_scheduling_permissions) {
//
//            //----------------------------------------------------------------------
//            // <editor-fold desc="Requesting NR or RR case"> {{{3
//            case Permissions::None:
//            case Permissions::Read: { // requested scheduling permissions
//              // This covers the vast majority of uses
//
//              switch(source_and_continuing_holder->use->immediate_permissions_) {
//                //----------------------------------------------------------------------
//                // <editor-fold desc="None/Read source immediate permissions"> {{{2
//                case Permissions::None:
//                case Permissions::Read: { // source immediate permissions
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // %    RN -> { NR } -> RN   %
//                  // %    RN -> { RR } -> RN   %
//                  // %    MN -> { NR } -> MN   %
//                  // %    MN -> { RR } -> MN   %
//                  // %    RR -> { NR } -> RR   %
//                  // %    RR -> { RR } -> RR   %
//                  // %    MR -> { NR } -> MR   %
//                  // %    MR -> { RR } -> MR   %
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//                  // Tested in too many places to count
//
////    #if _darma_has_feature(anti_flows)
////                  captured_use_holder = use_holder_maker(
////                    var_handle,
////                    /* Scheduling permissions */
////                    requested_scheduling_permissions,
////                    /* Immediate permissions */
////                    Permissions::Read,
////                    same_flow(&source_and_continuing_holder->use->in_flow_),
////                    insignificant_flow(),
////                    insignificant_flow(),
////                    same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
////                    );
////    #else
////                  captured_use_holder = use_holder_maker(
////                    var_handle,
////                    /* Scheduling permissions */
////                    requested_scheduling_permissions,
////                    /* Immediate permissions */
////                    Permissions::Read,
////                    same_flow(&source_and_continuing_holder->use->in_flow_),
////                    //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
////                    same_flow_as_in()
////                    // FlowRelationship::Same, nullptr, true
////                  );
////
////                  // The continuation use is the source use here
////
////    #endif // _darma_has_feature(anti_flows)
//
//
//                  break;
//
//                } // end case Read or None immediate permissions on source
//                // </editor-fold> end None/Read source immediate permissions }}}2
//                //----------------------------------------------------------------------
//                // <editor-fold desc="Modify source immediate permissions"> {{{2
//                case Permissions::Modify: { // source immediate permissions
//                  switch (source_and_continuing_holder->use->scheduling_permissions_) {
//                    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                    // <editor-fold desc="Read source scheduling permissions"> {{{3
//                    case Permissions::Read : {
//                      // %%%%%%%%%%%%%%%%%%%%%%%%%%%
//                      // %    RM -> { NR } -> RR   % ?????
//                      // %    RM -> { RR } -> RR   % ?????
//                      // %%%%%%%%%%%%%%%%%%%%%%%%%%%
//                      DARMA_ASSERT_NOT_IMPLEMENTED();                               // LCOV_EXCL_LINE
//                      break;
//                    }
//                    // </editor-fold> end Read source scheduling permissions }}}3
//                    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                    // <editor-fold desc="Modify source scheduling permissions"> {{{3
//                    case Permissions::Modify : {
//                      // %%%%%%%%%%%%%%%%%%%%%%%%%%%
//                      // %    MM -> { NR } -> MR   %
//                      // %    MM -> { RR } -> MR   %
//                      // %%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//                      // Not yet tested for anti-flows
//                      // TODO @antiflows test antiflows for this case
//
//                      // This is a ro capture of a handle with modify immediate permissions
//
////                      // we need to make a forwarded flow and register a new use in the
////                      // continuing context
////                      captured_use_holder = use_holder_maker(
////                        var_handle,
////                        /* Scheduling permissions */
////                        requested_scheduling_permissions,
////                        /* Immediate permissions */
////                        Permissions::Read,
////                        forwarding_flow(&source_and_continuing_holder->use->in_flow_),
////                        //FlowRelationship::Forwarding, &source_and_continuing_holder->use->in_flow_,
////    #if _darma_has_feature(anti_flows)
////                        // Out flow
////                          insignificant_flow(),
////                          // Anti-in flow
////                          insignificant_flow(),
////                          // Anti-out flow
////                          same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
////    #else
////                        same_flow_as_in()
////                        // FlowRelationship::Same, nullptr, true
////    #endif // _darma_has_feature(anti_flows)
////                      );
////
////                      // The continuing context actually needs to have a Use as well,
////                      // since it has access to the underlying data...
////
////                      // Also, this continuing context Use has to be registered before the
////                      // source use is released
////                      // make another shared ptr to the source use holder so that we can
////                      // release it
////                      void* old_ptr = source_and_continuing_holder->use->get_data_pointer_reference();
////
////                      // now make the use for the continuing context
////                      source_and_continuing_holder->replace_use(
////                        continuing_use_holder_maker(
////                          var_handle,
////                          /* Scheduling permissions: (should be unchanged) */
////                          source_and_continuing_holder->use->scheduling_permissions_,
////                          /* Immediate permissions: */
////                          Permissions::Read,
////                          same_flow(&captured_use_holder->use->in_flow_),
////                          //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
////                          // It still carries the out flow of the task, though, and should
////                          // establish an alias on release if there are no more modifies
////                          same_flow(&source_and_continuing_holder->use->out_flow_)
////                          //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
////        #if _darma_has_feature(anti_flows)
////                          , insignificant_flow(), // TODO test this!!
////                            same_anti_flow(&captured_use_holder->use->anti_out_flow_)
////        #endif // _darma_has_feature(anti_flows)
////                        ),
////                        register_continuation_use
////                      );
////                      // But this *can* still establish an alias (if it has Modify
////                      // scheduling permissions) because it could be the one that detects
////                      // that the forwarding flow aliases the out flow (i.e., that there
////                      // are no more modifies)
////                      source_and_continuing_holder->could_be_alias = true;
////
////                      // We can go ahead and pass on the underlying pointer, though, since
////                      // the Use is associated with a handle in a context that's uninterruptible
////                      void*& new_ptr = source_and_continuing_holder->use->get_data_pointer_reference();
////                      // The backend isn't allowed to change the pointer at this stage,
////                      // since it's in the middle of a task
////                      assert(new_ptr == old_ptr || new_ptr == nullptr);
////                      new_ptr = old_ptr;
//
//                      break;
//                    } // end case Modify source scheduling permissions
//                    // </editor-fold> end Modify source scheduling permissions }}}3
//                    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                    default: {
//                      DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
//                    } // end default
//                    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                  } // end switch source scheduling permissions
//
//                  break;
//
//                } // end case Modify immediate permissions on source
//                // </editor-fold> end Modify source immediate permissions }}}2
//                //----------------------------------------------------------------------
//                default: {
//                  DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
//                } // end default
//                //----------------------------------------------------------------------
//              } // end switch on source immediate permissions
//
//              break;
//            } // end case None or Read requested scheduling permissions (requested NR or RR)
//            // </editor-fold> None/Read requested scheduling permissions }}}3
//            //----------------------------------------------------------------------
//            // <editor-fold desc="requesting MR case"> {{{1
//            case Permissions::Modify: { // requested scheduling permissions
//              // We're requesting MR, which is unusual but not impossible
//
//              DARMA_ASSERT_MESSAGE(
//                (source_and_continuing_holder->use->scheduling_permissions_ == Permissions::Modify),
//                "Can't capture a handle with modify schedule permissions without"
//                  " Modify schedule permissions in enclosing scope"
//              );
//
//              switch(source_and_continuing_holder->use->immediate_permissions_) {
//                //----------------------------------------------------------------------
//                // <editor-fold desc="None immediate permissions"> {{{1
//                case Permissions::None: { // source immediate permissions
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // %   MN -> { MR } -> MN     %
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//                  // TODO @antiflows test antiflows for this case
//
////                  captured_use_holder = use_holder_maker(
////                    var_handle,
////                    /* Scheduling permissions */
////                    Permissions::Modify,
////                    /* Immediate permissions */
////                    Permissions::Read,
////                    same_flow(&source_and_continuing_holder->use->in_flow_),
////                    //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
////                    next_of_in_flow()
////                    //FlowRelationship::Next, nullptr, true
////    #if _darma_has_feature(anti_flows)
////                    // Anti-in
////                      // this has to be an "already ready" anti-flow.  Forwarding allows us to do that
////                      // BUT THIS CAN'T BE ALREADY READY!!!
////                      //, forwarding_anti_flow(&source_and_continuing_holder->use->anti_out_flow_),
////                      // For now:
////                      , next_anti_flow(&source_and_continuing_holder->use->anti_out_flow_),
////                      // Anti-out
////                      same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
////    #endif // _darma_has_feature(anti_flows)
////                  );
////                  captured_use_holder->could_be_alias = true;
////
////                  // the source should *not* establish an alias when potentially
////                  // released, in the replace below, since the continuation will
////                  // handle that
////                  source_and_continuing_holder->could_be_alias = false;
////
////                  source_and_continuing_holder->replace_use(
////                    continuing_use_holder_maker(
////                      var_handle,
////                      Permissions::Modify,
////                      Permissions::None,
////                      same_flow(&captured_use_holder->use->out_flow_),
////                      //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
////                      same_flow(&source_and_continuing_holder->use->out_flow_)
////                      //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
////    #if _darma_has_feature(anti_flows)
////                      // Anti-in
////                        , same_anti_flow(&source_and_continuing_holder->use->anti_in_flow_),
////                        // Anti-out
////                        same_anti_flow(&captured_use_holder->use->anti_in_flow_)
////    #endif // _darma_has_feature(anti_flows)
////                    ),
////                    register_continuation_use
////                  );
////                  source_and_continuing_holder->could_be_alias = true;
//
//                  break;
//                } // end None source immediate permissions
//                // </editor-fold> end None immediate permissions }}}1
//                //------------------------------------------------------------------
//                // <editor-fold desc="Read immediate permissions"> {{{1
//                case Permissions::Read: { // source immediate permissions
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // %   MR -> { MR } -> MN     %
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // this case is an MR capture of MR
//
//                  // TODO @antiflows test antiflows for this case
//
////                  captured_use_holder = use_holder_maker(
////                    var_handle,
////                    /* Scheduling permissions */
////                    Permissions::Modify,
////                    /* Immediate permissions */
////                    Permissions::Read,
////                    same_flow(&source_and_continuing_holder->use->in_flow_),
////                    //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
////                    next_of_in_flow()
////                    //FlowRelationship::Next, nullptr, true
////    #if _darma_has_feature(anti_flows)
////                    // Anti-in
////                      , next_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
////                      // Anti-out
////                      , same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
////                      //same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
////    #endif // _darma_has_feature(anti_flows)
////                  );
////                  captured_use_holder->could_be_alias = true;
////
////                  // the source should *not* establish an alias when potentially
////                  // released, in the replace below, since the continuation will
////                  // handle that
////                  source_and_continuing_holder->could_be_alias = false;
////
////                  source_and_continuing_holder->replace_use(
////                    continuing_use_holder_maker(
////                      var_handle,
////                      Permissions::Modify,
////                      Permissions::None,
////                      same_flow(&captured_use_holder->use->out_flow_),
////                      //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
////                      same_flow(&source_and_continuing_holder->use->out_flow_)
////                      //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
////    #if _darma_has_feature(anti_flows)
////                      // Anti-in
////                        , same_anti_flow(&source_and_continuing_holder->use->anti_in_flow_),
////                        // Anti-out
////                        same_anti_flow(&captured_use_holder->use->anti_in_flow_)
////    #endif // _darma_has_feature(anti_flows)
////                    ),
////                    register_continuation_use
////                  );
////                  source_and_continuing_holder->could_be_alias = true;
//
//                  break;
//                } // end case source immediate permissions None or Read
//                // </editor-fold> end None or Read immediate permissions }}}1
//                //------------------------------------------------------------------
//                // <editor-fold desc="Modify source immediate permissions"> {{{1
//                case Permissions::Modify: { // source immediate permissions
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                  // %   MM -> { MR } -> MN     %
//                  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//                  // TODO @antiflows test antiflows for this case
//
////                  captured_use_holder = use_holder_maker(
////                    var_handle,
////                    /* Scheduling permissions */
////                    Permissions::Modify,
////                    /* Immediate permissions */
////                    Permissions::Read,
////                    forwarding_flow(&source_and_continuing_holder->use->in_flow_),
////                    //FlowRelationship::Forwarding, &source_and_continuing_holder->use->in_flow_,
////                    next_of_in_flow()
////                    //FlowRelationship::Next, nullptr, true
////    #if _darma_has_feature(anti_flows)
////                    // Anti-in
////                      , next_anti_flow(&source_and_continuing_holder->use->anti_out_flow_),
////                      // Anti-out
////                      same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
////    #endif // _darma_has_feature(anti_flows)
////                  );
////                  captured_use_holder->could_be_alias = true;
////
////                  source_and_continuing_holder->replace_use(
////                    continuing_use_holder_maker(
////                      var_handle,
////                      Permissions::Modify, Permissions::None,
////                      same_flow(&captured_use_holder->use->out_flow_),
////                      //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
////                      same_flow(&source_and_continuing_holder->use->out_flow_)
////                      //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
////    #if _darma_has_feature(anti_flows)
////                      // Anti-in
////                        , insignificant_flow(),
////                        // Anti-out
////                        same_anti_flow(&captured_use_holder->use->anti_in_flow_)
////    #endif // _darma_has_feature(anti_flows)
////                    ),
////                    register_continuation_use
////                  );
////                  source_and_continuing_holder->could_be_alias = true;
//
//                  break;
//
//                } // end case source immediate permissions Modify
//                // </editor-fold> end Modify source immediate permissions }}}1
//                //------------------------------------------------------------------
//                default: {
//                  DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
//                } // end default
//                //------------------------------------------------------------------
//              } // end switch on source immediate permissions
//
//              break;
//            } // end case requested MR
//            // </editor-fold> end requesting MR case }}}1
//            //----------------------------------------------------------------------
//            default: {
//              DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
//            } // end default
//            //----------------------------------------------------------------------
//          } // end switch on requested immediate permissions
//
//          break;
//
//        } // end case Read requested immediate permissions
//        // </editor-fold> end requested_immediate_permissions = Read }}}1
//        //==========================================================================
//        // <editor-fold desc="requested_immediate_permissions = Modify"> {{{1
//        case Permissions::Modify: { // requested immediate permissions
//
//          DARMA_ASSERT_MESSAGE(
//            source_and_continuing_holder->use->scheduling_permissions_ == Permissions::Modify,
//            "Can't schedule a modify on a handle without Modify schedule permissions"
//          );
//
//          switch(source_and_continuing_holder->use->immediate_permissions_) {
//            //----------------------------------------------------------------------
//            // <editor-fold desc="None source immediate permissions"> {{{2
//            case Permissions::None: {
//              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//              // %   MN -> { NM } -> MN     %
//              // %   MN -> { RM } -> MN     % // our implementation of this case may be non-minimal
//              // %   MN -> { MM } -> MN     %
//              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//              // Tested in, e.g. TestAntiFlows.high_level_example_1 and TestAntiFlows.high_level_example_3
//
//              // Modify capture of handle without modify immediate permissions
//
////              capture_semantics::apply_with_capture_case(
////                source_and_continuing_holder->use->scheduling_permissions_,
////                source_and_continuing_holder->use->immediate_permissions_,
////                requested_scheduling_permissions,
////                requested_immediate_permissions,
////                coherencemode::sequential,
////                [&](auto capture_case) {
////                  auto* source_in = &source_and_continuing_holder->use->in_flow_;
////                  auto* source_out = &source_and_continuing_holder->use->out_flow_;
////                  auto* source_anti_in = &source_and_continuing_holder->use->anti_in_flow_;
////                  auto* source_anti_out = &source_and_continuing_holder->use->anti_out_flow_;
////
////                  captured_use_holder = use_holder_maker(
////                    var_handle,
////                    capture_case.captured_scheduling,
////                    capture_case.captured_immediate,
////                    capture_case.captured_in_flow_relationship(
////                      source_in, source_out
////                    ),
////                    capture_case.captured_out_flow_relationship(
////                      source_in, source_out
////                    ),
////                    capture_case.captured_anti_in_flow_relationship(
////                      source_in, source_out, source_anti_in, source_anti_out
////                    ),
////                    capture_case.captured_anti_out_flow_relationship(
////                      source_in, source_out, source_anti_in, source_anti_out
////                    )
////                  );
////
////                  auto* captured_in = &captured_use_holder->use->in_flow_;
////                  auto* captured_out = &captured_use_holder->use->out_flow_;
////                  auto* captured_anti_in = &captured_use_holder->use->anti_in_flow_;
////                  auto* captured_anti_out = &captured_use_holder->use->anti_out_flow_;
////
////                  // make sure an alias isn't established on the source
////                  source_and_continuing_holder->could_be_alias = false;
////
////                  source_and_continuing_holder->replace_use(
////                    continuing_use_holder_maker(
////                      var_handle,
////                      capture_case.continuation_scheduling,
////                      capture_case.continuation_immediate,
////                      capture_case.continuation_in_flow_relationship(
////                        source_in, source_out,
////                        captured_in, captured_out
////                      ),
////                      capture_case.continuation_out_flow_relationship(
////                        source_in, source_out,
////                        captured_in, captured_out
////                      ),
////                      capture_case.continuation_anti_in_flow_relationship(
////                        source_in, source_out, captured_in, captured_out,
////                        source_anti_in, source_anti_out, captured_anti_in, captured_anti_out
////                      ),
////                      capture_case.continuation_anti_out_flow_relationship(
////                        source_in, source_out, captured_in, captured_out,
////                        source_anti_in, source_anti_out, captured_anti_in, captured_anti_out
////                      )
////                    ),
////                    register_continuation_use
////                  );
////
////                  source_and_continuing_holder->could_be_alias = capture_case.could_be_alias;
////
////                }
////              );
//
////              // make the captured use holder
////              captured_use_holder = use_holder_maker(
////                var_handle,
////                /* Scheduling permissions */
////                requested_scheduling_permissions,
////                /* Immediate permissions */
////                Permissions::Modify,
////                same_flow(&source_and_continuing_holder->use->in_flow_),
////                //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
////                next_of_in_flow()
////                //FlowRelationship::Next, nullptr, true
////    #if _darma_has_feature(anti_flows)
////                // The whole point of having anti-out flows is to allow it to be the
////                  // anti-in flow of a captured use with XM permissions
////                  , same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_),
////                  // since this could schedule read-only uses (unless the scheduling
////                  // permissions are None), it also creates an anti-dependency to future
////                  // Modify tasks
////                  requested_scheduling_permissions == Permissions::None ?
////                  insignificant_flow() :
////                  next_anti_flow_of_anti_in()
////    #endif // _darma_has_feature(anti_flows)
////              );
////
////              // now that we're created a continuing context use holder, the source
////              // should NOT establish an alias (the continuing use holder will do
////              // it instead), so flip the flag here
////              source_and_continuing_holder->could_be_alias = false;
////
////              source_and_continuing_holder->replace_use(
////                continuing_use_holder_maker(
////                  var_handle,
////                  /* Scheduling permissions */
////                  Permissions::Modify,
////                  /* Immediate permissions */
////                  Permissions::None, // This is a schedule-only use
////                  same_flow(&captured_use_holder->use->out_flow_),
////                  //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
////                  same_flow(&source_and_continuing_holder->use->out_flow_)
////                  //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
////    #if _darma_has_feature(anti_flows)
////                  // anti-in =
////                    , same_anti_flow(&source_and_continuing_holder->use->anti_in_flow_),
////                    // anti-out =
////                    requested_scheduling_permissions == Permissions::None ?
////                    next_anti_flow(&source_and_continuing_holder->use->anti_out_flow_)
////                    : same_anti_flow(&captured_use_holder->use->anti_out_flow_)
////    #endif // _darma_has_feature(anti_flows)
////                ),
////                register_continuation_use
////              );
//
////              // This new use could establish an alias if no additional tasks are
////              // scheduled on it...
////              source_and_continuing_holder->could_be_alias = true;
//
//              break;
//            } // end case None source immediate permissions
//            // </editor-fold> end None source immediate permissions }}}2
//            //----------------------------------------------------------------------
//            // <editor-fold desc="Read source immediate permissions"> {{{1
//            case Permissions::Read: { // source immediate permissions
//              // Requesting XM capture of MR
//              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//              // %   MR -> { NM } -> MN     %
//              // %   MR -> { RM } -> MN     %
//              // %   MR -> { MM } -> MN     %
//              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//              // TODO @antiflows test antiflows for this case
//
//              // Create the out flow  (should this be make_forwarding_flow?)
//              // (probably not, since RR -> { RR } -> RR doesn't do forwarding?)
//
//
////              // make the captured use holder
////              captured_use_holder = use_holder_maker(
////                var_handle,
////                /* Scheduling permissions */
////                requested_scheduling_permissions,
////                /* Immediate permissions */
////                Permissions::Modify,
////                same_flow(&source_and_continuing_holder->use->in_flow_),
////                //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
////                next_of_in_flow()
////                //FlowRelationship::Next, nullptr, true
////    #if _darma_has_feature(anti_flows)
////                // Anti-in flow
////                  , same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_),
////                  // Anti-out flow
////                  // since this could schedule read-only uses (unless the scheduling
////                  // permissions are None), it also creates an anti-dependency to future
////                  // Modify tasks
////                  requested_scheduling_permissions == Permissions::None ?
////                  insignificant_flow() :
////                  next_anti_flow_of_anti_in()
////    #endif // _darma_has_feature(anti_flows)
////              );
////
////              // register the captured use
////
////              source_and_continuing_holder->replace_use(
////                continuing_use_holder_maker(
////                  var_handle,
////                  source_and_continuing_holder->use->scheduling_permissions_,
////                  Permissions::None,
////                  same_flow(&captured_use_holder->use->out_flow_),
////                  //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
////                  same_flow(&source_and_continuing_holder->use->out_flow_)
////                  //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
////    #if _darma_has_feature(anti_flows)
////                  // anti-in =
////                    // this should usually be insignificant unless the source is somehow a dependency use
////                    , same_anti_flow(&source_and_continuing_holder->use->anti_in_flow_),
////                    // anti-out =
////                    requested_scheduling_permissions == Permissions::None ?
////                    // TODO check the None case...
////                    next_anti_flow(&captured_use_holder->use->anti_in_flow_)
////                    : same_anti_flow(&captured_use_holder->use->anti_out_flow_)
////    #endif // _darma_has_feature(anti_flows)
////                ),
////                register_continuation_use
////              );
////
////              source_and_continuing_holder->could_be_alias = true;
////              // out flow and scheduling permissions unchanged
//
//              break;
//            }
//            // </editor-fold> end Read source immediate permissions  }}}1
//            //----------------------------------------------------------------------
//            // <editor-fold desc="modify source immediate permissions"> {{{1
//            case Permissions::Modify: {
//              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//              // %   MM -> { NM } -> MN     % this case is untested for anti-flows
//              // %   MM -> { RM } -> MN     % our implementation of this case may be non-minimal
//              // %   MM -> { MM } -> MN     %
//              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//              // The MM -> { MM } case is tested in, e.g., TestAntiFlows.high_level_example_1
//
//              // Modify capture of handle with modify immediate permissions
//
//              // we need to forward the input flow, since it could have changed
//              // (because the source has modify immediate permissions)
//
//              // make the captured use holder
////              captured_use_holder = use_holder_maker(
////                var_handle,
////                /* Scheduling permissions */
////                requested_scheduling_permissions,
////                /* Immediate permissions */
////                Permissions::Modify,
////                forwarding_flow(&source_and_continuing_holder->use->in_flow_),
////                //FlowRelationship::Forwarding, &source_and_continuing_holder->use->in_flow_,
////                next_of_in_flow()
////                //FlowRelationship::Next, nullptr, true
////    #if _darma_has_feature(anti_flows)
////                // Anti-in flow
////                  // because of the way forwarding works, this can never carry anti-dependencies
////                  , forwarding_anti_flow(&source_and_continuing_holder->use->anti_out_flow_),
////                  // anti-out =
////                  requested_scheduling_permissions == Permissions::None ?
////                  insignificant_flow()
////                  : next_anti_flow_of_anti_in()
////    #endif // _darma_has_feature(anti_flows)
////              );
////
////              source_and_continuing_holder->replace_use(
////                continuing_use_holder_maker(
////                  var_handle,
////                  source_and_continuing_holder->use->scheduling_permissions_,
////                  Permissions::None,
////                  same_flow(&captured_use_holder->use->out_flow_),
////                  //FlowRelationship::Same, &captured_use_holder->use->out_flow_,
////                  same_flow(&source_and_continuing_holder->use->out_flow_)
////                  //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
////    #if _darma_has_feature(anti_flows)
////                  // Anti-in flow
////                    , same_anti_flow(&source_and_continuing_holder->use->anti_out_flow_),
////                    // Anti-out flow
////                    requested_scheduling_permissions == Permissions::None ?
////                    next_anti_flow(&captured_use_holder->use->anti_in_flow_)
////                    : same_anti_flow(&captured_use_holder->use->anti_out_flow_)
////    #endif // _darma_has_feature(anti_flows)
////                ),
////                register_continuation_use
////              );
////              source_and_continuing_holder->could_be_alias = true;
//
//              break;
//            }
//            // </editor-fold> end modify source immediate permissions  }}}1
//            //----------------------------------------------------------------------
//            default: {
//              DARMA_ASSERT_NOT_IMPLEMENTED();                                       // LCOV_EXCL_LINE
//            } // end default
//            //----------------------------------------------------------------------
//          } // end switch on source immediate permissions
//
//          break;
//        } // end case Modify requested immediate permissions
//        // </editor-fold> end requested_immediate_permissions = Modify }}}1
//
//        default: {
//          DARMA_ASSERT_NOT_IMPLEMENTED();                                           // LCOV_EXCL_LINE
//        } // end default
//
//        //==========================================================================
//      } // end switch requested immediate permissions
//      break;
//    } // end case Sequential coherence mode
//    // </editor-fold> end Sequential coherence mode }}}1
//    //==========================================================================
//    default: {
//      DARMA_ASSERT_NOT_IMPLEMENTED();                                           // LCOV_EXCL_LINE
//    } // end default
//
//  } // end switch source coherence mode

//#if 0
////==========================================================================
//        // <editor-fold desc="requested_immediate_permissions = Commutative"> {{{1
//    #if _darma_has_feature(commutative_access_handles)
//        case Permissions::Commutative: { // requested immediate permissions
//
//          DARMA_ASSERT_MESSAGE(
//            source_and_continuing_holder->use->scheduling_permissions_ == Permissions::Commutative,
//            "Can't schedule a commutative use without commutative scheduling permissions"
//          );
//          DARMA_ASSERT_MESSAGE(
//            source_and_continuing_holder->use->immediate_permissions_ == Permissions::None
//              or source_and_continuing_holder->use->immediate_permissions_ == Permissions::Commutative,
//            "Can't create commutative task on handle without None or Commutative immediate permissions"
//          );
//
//
//          // TODO @antiflows update commutative for new anti-flow semantics
//    #if _darma_has_feature(anti_flows)
//          DARMA_ASSERT_NOT_IMPLEMENTED("Anti-flows for CX -> { XC } capture cases");
//    #endif
//
//          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//          // %   CN -> { CC } -> CN     %
//          // %   CC -> { CC } -> CN     %
//          // %   CN -> { NC } -> CN     %
//          // %   CC -> { NC } -> CN     %
//          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//          captured_use_holder = use_holder_maker(
//            var_handle,
//            /* Scheduling permissions */
//            requested_scheduling_permissions,
//            /* Immediate permissions */
//            Permissions::Commutative,
//            same_flow(&source_and_continuing_holder->use->in_flow_),
//            //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
//            same_flow(&source_and_continuing_holder->use->out_flow_)
//            //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_
//    #if _darma_has_feature(anti_flows)
//            , same_anti_flow(&source_and_continuing_holder->use->anti_in_flow_)
//              , insignificant_flow()
//    #endif
//          );
//
//          // TODO register this and make the continuation use have none immediate permissions
//    //#if _darma_has_feature(register_commutative_continuation_uses)
//    //
//    //      source_and_continuing_holder->replace_use(
//    //        continuing_use_holder_maker(
//    //          var_handle,
//    //          /* Scheduling permissions */
//    //          source_and_continuing_holder->use->scheduling_permissions_,
//    //          /* Immediate permissions */
//    //          source_and_continuing_holder->use->immediate_permissions_,
//    //          FlowRelationship::Same, &captured_use_holder->use->in_flow_,
//    //          FlowRelationship::Same, &captured_use_holder->use->out_flow_
//    //        ),
//    //        register_continuation_use
//    //      );
//    //
//    //#endif // _darma_has_feature(register_commutative_continuation_uses)
//
//
//          // Note that permissions/use/etc of source_and_continuing are unchanged
//
//          break;
//        } // end requested immediate permissions commutative
//    #endif // _darma_has_feature(commutative_access_handles)
//        // </editor-fold> end requested_immediate_permissions = Commutative }}}1
//        //==========================================================================
//  // <editor-fold desc="Commutative requested scheduling permissions"> {{{3
//#if _darma_has_feature(commutative_access_handles)
//  case Permissions::Commutative: { // requested scheduling permissions
//    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    // %   CN -> { CN } -> CN                      %
//    // %   XN -> { CN } -> XN (disallowed here)    %
//    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//    // TODO @antiflows implement and test antiflows for this case
//#if _darma_has_feature(anti_flows)
//    DARMA_ASSERT_NOT_IMPLEMENTED("Anti-flows for CN -> { CN } capture case");
//#endif
//
//    DARMA_ASSERT_MESSAGE(
//      source_and_continuing_holder->use->scheduling_permissions_
//        == Permissions::Commutative,
//      "Can't make a schedule-only use of handle not in schedule"
//        " commutative mode.  Need to transition handle to commutative"
//        " mode first in outer scope."
//    );
//
//    captured_use_holder = use_holder_maker(
//      var_handle,
//      /* Scheduling permissions */
//      Permissions::Commutative,
//      /* Immediate permissions */
//      Permissions::None,
//      same_flow(&source_and_continuing_holder->use->in_flow_),
//      //FlowRelationship::Same, &source_and_continuing_holder->use->in_flow_,
//      same_flow(&source_and_continuing_holder->use->out_flow_)
//      //FlowRelationship::Same, &source_and_continuing_holder->use->out_flow_, false
//#if _darma_has_feature(anti_flows)
//    , same_anti_flow(&source_and_continuing_holder->use->anti_in_flow_),
//                insignificant_flow()
//#endif // _darma_has_feature(anti_flows)
//    );
//
//    // No continuation use needed here, since source was schedule-only
//
//    break;
//  }
//#endif // _darma_has_feature(commutative_access_handles)
//  // </editor-fold> end Commutative requested scheduling permissions }}}3
//  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//            // <editor-fold desc="Commutative source immediate permissions"> {{{3
//    #if _darma_has_feature(commutative_access_handles)
//            case Permissions::Commutative : { // source immediate permissions
//              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//              // %   CC -> { CN } -> CN     %
//              // %%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//              DARMA_ASSERT_NOT_IMPLEMENTED(
//                "Schedule-only permissions on handles that already have immediate"
//                  " Commutative permissions."
//              );                                                                    // LCOV_EXCL_LINE
//
//            }
//    #endif // _darma_has_feature(commutative_access_handles)
//              // </editor-fold> end Commutative source immediate permissions }}}3
//            //----------------------------------------------------------------------
//#endif


#endif //DARMA_IMPL_CAPTURE_H
