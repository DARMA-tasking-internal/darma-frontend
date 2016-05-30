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

/** @brief A backend-allocated object representing the half (input or output) of the state at a
 *  given point in time of a use of a Handle object.
 *
 *  Put differently, a Flow represents half of the connection between the Use instance that
 *  writes a given version of a given piece of data and one of the Use objects that reads that
 *  data (with the other half being generated via a call to make_same()).
 *
 *  The life-cycle of a Flow consists of 4 strictly ordered phases.  For some Flow instance u,
 *
 *  + Creation -- &u is a pointer returned by any of make_initial_flow(),
 *    make_fetching_flow(), Flow::make_same(), Flow::make_next().  Upon return
 *  + Exactly 1 Use *or* publication use:
 *    - Use use: &u is either a return value of
 *      Use::get_in_flow() or Use::get_out_flow() for some
 *      Use object ha that is an argument to register_handle_access() at
 *      some time after u was created but before it is released.  To ensure this
 *      strict ordering of the Flow life-cycle, the runtime must enforce
 *      atomicity among register_handle_access(ha), u.make_next(), and
 *      release_handle_access(&ha) for any u that could be returned ha.get_in_flow() or
 *      ha.get_out_flow()
 *    - publication use: &u is the first argument to publish_flow(). To
 *      ensure this strict ordering of the Flow life-cycle, the runtime must
 *      enforce atomicity among publish_flow(&u, ...), u.make_next(), and
 *      release_published_flow(&u) for any given u
 *  + At most one call to u.make_next(), which should happen after the one
 *    Use/publication use but before release
 *  + release -- Depending on whether the Flow was used for Use or publication:
 *     - If u was the return value of ha.get_in_flow() or ha.get_out_flow() for some
 *       ha passed to register_handle_access() in the lifetime of u, then u is released
 *       when release_handle_access(&ha) is called.
 *     - If u was the first argument to a call to publish_flow() in its lifetime, u is released
 *       by a call to release_published_flow(&u)
 *    Upon return from the release sequence (whether by release_handle_access() or
 *    release_published_flow()), it is no longer valid to call make_same() or make_next() on u
 *    or to dereference a pointer to u.  See atomicity constraints for release phase in Use/publication
 *    use description.
 *
 *  Two Flow objects, a and b, are considered to be the same by Flow sameness rules, as used elsewhere in this
 *  documentation, if a was constructed using b.make_same() or if b was constructed using a.make_same().
 */
class Flow {
  public:

};


} // end namespace backend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_INTERFACE_BACKEND_FLOW_H
