/*
//@HEADER
// ************************************************************************
//
//                      semantics.h
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


/*******************************************************************************
 *  @internal
 *  @file
 *  @brief Contains the semantic transformations for capturing versioned
 *  asynchronous smart pointers
 *
 *  All of the permissions and flow information for the allowed transitions from
 *  a source Use to a new captured Use and (most of the time) a new continuation
 *  Use.  The best way to express this information uses pattern matching, since
 *  many different inputs often share an output implementation.  Unfortunately,
 *  static pattern matching in C++ is awkward (at best) and has to use SFINAE.
 *  We use the macros `_darma_CAPTURE_CASE()` and
 *  `_darma_CAPTURE_CASE_NO_NEW_CONTINUATION_USE()` to express the pattern cases
 *  (and the helper macro `_darma_ANY_OF()` expressing cases that match
 *  multiple potential inputs), with the first 5 parameters expressing the
 *  pattern inputs and the rest expressing the output of the pattern matching
 *  case.  (These macros are, in turn, implemented with a partial specialization
 *  of the class template
 *  `darma_runtime::detail::capture_semantics::CaptureCase` that uses SFINAE
 *  for pattern matching.)
 *
 *  Cases are also denoted with comments so that they can quickly be searched
 *  for.  For instance, `MN -> { RR } -> MN` indicates that the implementation
 *  of the `Read` scheduling, `Read` immediate capture of a source use with
 *  `Modify` scheduling, `Modify` immediate permissions is implemented by the
 *  subsequent case.
 *
 ******************************************************************************/

#ifndef DARMAFRONTEND_SEMANTICS_H
#define DARMAFRONTEND_SEMANTICS_H

#include <darma/interface/frontend.h>
#include <darma/interface/frontend/use.h>

#include <tinympl/find.hpp>
#include <darma/impl/handle_use_base.h>

#include "semantics_helpers.h"

namespace darma_runtime {
namespace detail {
namespace capture_semantics {

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %   RN -> { RN } -> RN     %
// %   MN -> { RN } -> MN     %
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
_darma_CAPTURE_CASE_NO_NEW_CONTINUATION_USE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  _darma_ANY_OF(Read, Modify), None,
  /* captured permissions */
  Read, None,
  /* ---------- output for cases matching this pattern ----------*/
  /* captured flows */
  same_flow(source_in), insignificant_flow(),
  /* captured anti-flows */
  insignificant_flow(), same_anti_flow(source_anti_out),
  /* could be alias: */
  false
);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %   MN -> { RM } -> MN     % // our implementation of this case may be non-minimal
// %   MN -> { MM } -> MN     %
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
_darma_CAPTURE_CASE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  Modify, None,
  /* captured permissions */
  _darma_ANY_OF(Read, Modify), Modify,
  /* ---------- output for cases matching this pattern ----------*/
  /* continuing permissions */
  Modify, None,
  /* captured flows */
  same_flow(source_in), next_of_in_flow(),
  /* captured anti-flows */
  same_anti_flow(source_anti_out), next_anti_flow_of_anti_in(),
  /* continuing flows */
  same_flow(captured_out), same_flow(source_out),
  /* continuing anti-flows */
  same_anti_flow(source_anti_in), same_anti_flow(captured_anti_out),
  /* could be alias: */
  false
);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %   MN -> { NM } -> MN     %
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
_darma_CAPTURE_CASE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  Modify, None,
  /* captured permissions */
  None, Modify,
  /* ---------- output for cases matching this pattern ----------*/
  /* continuing permissions */
  Modify, None,
  /* captured flows */
  same_flow(source_in), next_of_in_flow(),
  /* captured anti-flows */
  same_anti_flow(source_anti_out), insignificant_flow(),
  /* continuing flows */
  same_flow(captured_out), same_flow(source_out),
  /* continuing anti-flows */
  same_anti_flow(source_anti_in), next_anti_flow(source_anti_out),
  /* could be alias: */
  false
);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %   RR -> { RN } -> RR     %
// %   MR -> { RN } -> MR     %
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// TODO test and/or use this!
_darma_CAPTURE_CASE_NO_NEW_CONTINUATION_USE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  _darma_ANY_OF(Read, Modify), Read,
  /* captured permissions */
  Read, None,
  /* ---------- output for cases matching this pattern ----------*/
  /* captured flows */
  same_flow(source_in), insignificant_flow(),
  /* captured anti-flows */
  insignificant_flow(), same_anti_flow(source_anti_out),
  /* could be alias: */
  false
);

// %%%%%%%%%%%%%%%%%%%%%%%%%
// %   MN -> { MN } -> MN  %
// %%%%%%%%%%%%%%%%%%%%%%%%%
_darma_CAPTURE_CASE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  Modify, None,
  /* captured permissions */
  Modify, None,
  /* ---------- output for cases matching this pattern ----------*/
  /* continuing permissions */
  Modify, None,
  /* captured flows */
  same_flow(source_in), next_of_in_flow(),
  /* captured anti-flows */
  next_anti_flow(source_anti_out), same_anti_flow(source_anti_out),
  /* continuing flows */
  same_flow(captured_out), same_flow(source_out),
  /* continuing anti-flows */
  same_anti_flow(source_anti_in), same_anti_flow(captured_anti_in),
  /* could be alias: */
  true
);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %   MR -> { MN } -> MN     %
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
_darma_CAPTURE_CASE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  Modify, Read,
  /* captured permissions */
  Modify, None,
  /* ---------- output for cases matching this pattern ----------*/
  /* continuing permissions */
  Modify, None,
  /* captured flows */
  forwarding_flow(source_in), next_of_in_flow(),
  /* captured anti-flows */
  next_anti_flow(source_anti_out), same_anti_flow(source_anti_out),
  /* continuing flows */
  same_flow(captured_out), same_flow(source_out),
  /* continuing anti-flows */
  same_anti_flow(source_anti_in), same_anti_flow(captured_anti_in),
  /* could be alias: */
  true
);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %   MM -> { MN } -> MN     %
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
_darma_CAPTURE_CASE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  Modify, Modify,
  /* captured permissions */
  Modify, None,
  /* ---------- output for cases matching this pattern ----------*/
  /* continuing permissions */
  Modify, None,
  /* captured flows */
  forwarding_flow(source_in), next_of_in_flow(),
  /* captured anti-flows */
  next_anti_flow(source_anti_out), next_anti_flow_of_anti_in(),
  /* continuing flows */
  same_flow(captured_out), same_flow(source_out),
  /* continuing anti-flows */
  same_anti_flow(source_anti_out), same_anti_flow(captured_anti_in),
  /* could be alias: */
  true
);

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
_darma_CAPTURE_CASE_NO_NEW_CONTINUATION_USE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  _darma_ANY_OF(Read, Modify), _darma_ANY_OF(None, Read),
  /* captured permissions */
  _darma_ANY_OF(None, Read), Read,
  /* ---------- output for cases matching this pattern ----------*/
  /* captured flows */
  same_flow(source_in), insignificant_flow(),
  /* captured anti-flows */
  insignificant_flow(), same_anti_flow(source_anti_out),
  /* could be alias: */
  false
);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// %    MM -> { NR } -> MR   %
// %    MM -> { RR } -> MR   %
// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// TODO transfer pointer to continuation use? or should we just make the backend do that...
_darma_CAPTURE_CASE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  Modify, Modify,
  /* captured permissions */
  _darma_ANY_OF(None, Read), Read,
  /* ---------- output for cases matching this pattern ----------*/
  /* continuing permissions */
  Modify, Read,
  /* captured flows */
  forwarding_flow(source_in), insignificant_flow(),
  /* captured anti-flows */
  insignificant_flow(), same_anti_flow(source_anti_out),
  /* continuing flows */
  same_flow(captured_in), same_flow(source_out),
  /* continuing anti-flows */ /* Could be wrong about the anti-in flow?!?? */
  insignificant_flow(), same_anti_flow(captured_anti_out),
  /* could be alias: */
  false
);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %   MN -> { MR } -> MN     %
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
_darma_CAPTURE_CASE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  Modify, None,
  /* captured permissions */
  Modify, Read,
  /* ---------- output for cases matching this pattern ----------*/
  /* continuing permissions */
  Modify, None,
  /* captured flows */
  same_flow(source_in), next_of_in_flow(),
  /* captured anti-flows */
  next_anti_flow(source_anti_out), same_anti_flow(source_anti_out),
  /* continuing flows */
  same_flow(captured_out), same_flow(source_out),
  /* continuing anti-flows */
  same_anti_flow(source_anti_in), same_anti_flow(captured_anti_in),
  /* could be alias: */
  true
);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %   MR -> { MR } -> MN     %
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
_darma_CAPTURE_CASE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  Modify, Read,
  /* captured permissions */
  Modify, Read,
  /* ---------- output for cases matching this pattern ----------*/
  /* continuing permissions */
  Modify, None,
  /* captured flows */
  same_flow(source_in), next_of_in_flow(),
  /* captured anti-flows */
  next_anti_flow(source_anti_out), same_anti_flow(source_anti_out),
  /* continuing flows */
  same_flow(captured_out), same_flow(source_out),
  /* continuing anti-flows */
  same_anti_flow(source_anti_in), same_anti_flow(captured_anti_in),
  /* could be alias: */
  true
);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %   MM -> { MR } -> MN     %
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
_darma_CAPTURE_CASE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  Modify, Modify,
  /* captured permissions */
  Modify, Read,
  /* ---------- output for cases matching this pattern ----------*/
  /* continuing permissions */
  Modify, None,
  /* captured flows */
  forwarding_flow(source_in), next_of_in_flow(),
  /* captured anti-flows */
  next_anti_flow(source_anti_out), same_anti_flow(source_anti_out),
  /* continuing flows */
  same_flow(captured_out), same_flow(source_out),
  /* continuing anti-flows */ /* TODO check the in flow here */
  insignificant_flow(), same_anti_flow(captured_anti_in),
  /* could be alias: */
  true
);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %   MR -> { NM } -> MN     %
// %   MR -> { RM } -> MN     %
// %   MR -> { MM } -> MN     %
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
_darma_CAPTURE_CASE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  Modify, Read,
  /* captured permissions */
  _darma_ANY_OF(None, Read, Modify), Modify,
  /* ---------- output for cases matching this pattern ----------*/
  /* continuing permissions */
  Modify, None,
  /* captured flows */
  same_flow(source_in), next_of_in_flow(),
  /* captured anti-flows */
  same_anti_flow(source_anti_out), next_anti_flow_of_anti_in(),
  /* continuing flows */
  same_flow(captured_out), same_flow(source_out),
  /* continuing anti-flows */
  same_anti_flow(source_anti_in), same_anti_flow(captured_anti_out),
  /* could be alias: */
  false
);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %   MM -> { NM } -> MN     % this case is untested for anti-flows
// %   MM -> { RM } -> MN     % our implementation of this case may be non-minimal
// %   MM -> { MM } -> MN     %
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
_darma_CAPTURE_CASE(
  /* ---------- pattern parameters ---------- */
  Sequential,
  /* Source Permissions */
  Modify, Modify,
  /* captured permissions */
  _darma_ANY_OF(None, Read, Modify), Modify,
  /* ---------- output for cases matching this pattern ----------*/
  /* continuing permissions */
  Modify, None,
  /* captured flows */
  forwarding_flow(source_in), next_of_in_flow(),
  /* captured anti-flows */
  forwarding_anti_flow(source_anti_out), next_anti_flow_of_anti_in(),
  /* continuing flows */
  same_flow(captured_out), same_flow(source_out),
  /* continuing anti-flows */
  same_anti_flow(source_anti_out), same_anti_flow(captured_anti_out),
  /* could be alias: */
  false
);


// Instantiate the registry-creating static variable now that all partial specializations are defined
template <
  frontend::permissions_t SourceSchedulingIn, frontend::permissions_t SourceImmediateIn,
  frontend::permissions_t CapturedSchedulingIn, frontend::permissions_t CapturedImmediateIn,
  frontend::coherence_mode_t CoherenceModeIn,
  typename Enable
>
const size_t CaptureCase<
  SourceSchedulingIn, SourceImmediateIn,
  CapturedSchedulingIn, CapturedImmediateIn,
  CoherenceModeIn, Enable
>::_index = register_capture_case<CaptureCase<
  SourceSchedulingIn, SourceImmediateIn,
  CapturedSchedulingIn, CapturedImmediateIn,
  CoherenceModeIn, Enable
>>();

} // end namespace capture_semantics
} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_SEMANTICS_H
