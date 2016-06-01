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

/** @brief A backend-allocated object representing the input/output state of a Handle at the beginning/end of a task.
 *  When executing tasks, data "flows" from one task to the next.
 *  A precursor task produces data that will be consumed by a successor task.
 *  Each task carries a unique Use variable. Each Use has an input flow and output flow (or input-only, for read-only tasks).
 *  An output Flow variable represents the "producer" half and an input Flow variable represents the "consumer" half
 *  of a producer-consumer connection between tasks.
 *  An equivalence relationship between output and input Flows is indicated by allocating the input Flow
 *  with calls to make_same().  Equivalence must be defined within the backend.
 *  The translation layer will never make an equivalence test itself.
 *
 *  + Creation -- &flw is a pointer returned by any of make_initial_flow(),
 *    make_fetching_flow(), Flow::make_same(), Flow::make_next().  Upon return
 *  + Exactly 1 Operation use *or* publication use:
 *    - Operation use: &flw is either a return value of Use::get_in_flow() or Use::get_out_flow() for some
 *      Use object usage that is an argument to register_use() at
 *      some time after usage was created but before it is released.  To ensure this
 *      strict ordering of the Flow life-cycle, the runtime must enforce
 *      atomicity among register_use(u), runtime.make_next(flw), and
 *      release_use(&u) for any Flow that could be returned by usage.get_in_flow() or
 *      usage.get_out_flow()
 *    - Publication use: &flw is the first argument to publish_flow(). To
 *      ensure this strict ordering of the Flow life-cycle, the runtime must
 *      enforce atomicity among publish_flow(&flw, ...), runtime.make_next(flw), and
 *      release_published_flow(&flw) for any given flw
 *  + At most one call to runtime.make_next(flw), which should happen after the one
 *    Operation/publication use but before release
 *  + release -- Depending on whether the Flow was used for an Operation or Publication:
 *     - If flw is the return value of usage.get_in_flow() or usage.get_out_flow() for some
 *       Use passed to register_use(), then flw is released
 *       when release_use(&u) is called.
 *     - If flw was the first argument to a call to publish_flow() in its lifetime, flw is released
 *       by a call to release_published_flow(&flw)
 *    Upon return from the release sequence (whether by release_use() or
 *    release_published_flow()), it is no longer valid to call make_same() or make_next() on flw
 *    or to dereference a pointer to flw.
 *
 *  Two Flow objects, a and b, are considered to consume or produce the same dat
 *  if a was constructed using make_same(b) or if b was constructed using make_same(a).
 *  The flow returned by make_same(a), however, is a different object and is therefore
 *  independently modifiable by the backend for backend transformations of the task graph
 */
class Flow {
  public:

};


} // end namespace backend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_INTERFACE_BACKEND_FLOW_H
