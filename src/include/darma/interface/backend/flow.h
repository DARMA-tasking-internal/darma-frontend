/*
//@HEADER
// ************************************************************************
//
//                      flow.h
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

#ifndef DARMA_INTERFACE_BACKEND_FLOW_H
#define DARMA_INTERFACE_BACKEND_FLOW_H

namespace darma_runtime {
namespace abstract {
namespace backend {

/** @brief A backend-allocated object representing the input/output state of a
 *  Handle at the beginning/end of an operation.
 *
 *  A Flow represents either the input or output state of a Handle at the
 *  beginning or end, respectively, of an operation.  A Flow belongs to a single
 *  Use object (containing both an input and output Flow) carried by a single
 *  operation/task.  An input Flow indicates the value consumed by an operation.
 *  All input flows for a task's initial set of Uses must become available for
 *  the task to begin executing.  For a Modify Use only, the output Flow
 *  indicates the value produced by the operation.  Interpretation of the output
 *  Flow of a Read Use is described later.  Tasks may register additional Use
 *  objects (with unique Flows) when access to a Handle is required at a later
 *  point in logical time (e.g., after modification by a child task).
 *
 *  When executing tasks, data "flows" from the output Flow in a producer task's
 *  Use to an equivalent input Flow in a consumer task's Use.  Pairing of an
 *  input and output Flow within a Modify Use implies that, when
 *  `Runtime::release_use()` is called, the input Flow should be used to satisfy
 *  other Read Uses with input flows that are equivalent to the output Flow of
 *  the Use being released.  See below for the definition of equivalence.
 *
 *  The output Flow of a Read Use indicates the release of data and clearing of
 *  an anti-dependence.  Modify Uses should only be satisfied after all
 *  anti-dependencies have been cleared and it is no longer possible for
 *  additional anti-dependencies to be created (all other Uses with Read or
 *  greater scheduling or immediate permissions on equivalent Flows have been
 *  released).
 *
 *  An equivalence relationship between two Flows `a` and `b` is indicated by
 *  allocating the Flow `a` with a call to `Runtime::make_same_flow(b)` or vice
 *  versa.  Equivalence must be defined within the backend; the translation
 *  layer will never make an equivalence test itself.
 *
 *  The life-cycle of a Flow consists of 4 strictly ordered phases.  For some
 *  Flow instance flw:
 *
 *  + Creation -- `&flw` is a pointer returned by any of
 *    + `Runtime::make_initial_flow()`,
 *    + `Runtime::make_fetching_flow()`,
 *    + `Runtime::make_null_flow()`,
 *    + `Runtime::make_same_flow()`,
 *    + `Runtime::make_forwarding_flow()`, or
 *    + `Runtime::make_next_flow()`.
 *  + Registration -- Each Flow is owned by a Use as either input or output.
 *      Each Use will be registered through `Runtime::register_use()` before
 *      being used in a task or publication, indirectly registering its flows.
 *      All flows have exactly one Use association in their lifetime; that is,
 *      `&flw` is either a return value of `Use::get_in_flow()` or
 *      `Use::get_out_flow()` for some Use object that is an argument to
 *      `Runtime::register_use()` at some time after `flw` was created but
 *      before that Use is released.
 *  + Propagation -- After registration but before release, each instance can
 *    have
 *    + any number of calls to `Runtime::make_same_flow(&flw, ...)`,
 *    + at most one call to `Runtime::make_next_flow(&flw, ...)`, and
 *    + at most one call to `Runtime::make_forwarding_flow(&flw, ...)`.
 *  + Release -- Flows are indirectly released through a call to
 *       `Runtime::release_use()` on the owning Use.  The Flow will never be
 *       used directly (or indirectly) by the translation layer after this call.
 *       The backend is responsible for deleting the memory allocated to a Flow
 *       at this time.
 *
 *  To ensure this strict ordering of the Flow life-cycle, the runtime must
 *  enforce atomicity among `Runtime::register_use(&u)`,
 *  `Runtime::make_next_flow(&flw, ...)`/ `make_forwarding_flow(&flw, ...)`/
 *  `make_same_flow(&flw, ...)`, `Runtime::release_use(&u)` for any Flow `flw`
 *  that could be returned by `u.get_in_flow()` or `u.get_out_flow()` for some
 *  Use `u`.
 *
 *  Although two Flow objects, `a` and `b`, are considered to consume or produce
 *  the same version of the same data if `a` was constructed using
 *  `Runtime::make_same(b)` or if `b` was constructed using
 *  `Runtime::make_same(a)`, the two flows are distinct objects and, therefore,
 *  have independent life cycles and can be independently modifiable by the
 *  backend.
 */
class Flow {
  public:

};


} // end namespace backend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_INTERFACE_BACKEND_FLOW_H
