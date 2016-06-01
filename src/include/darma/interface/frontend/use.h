/*
//@HEADER
// ************************************************************************
//
//                      use.h
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

#ifndef DARMA_IMPLEMENTATION_FRONTEND_USE_H
#define DARMA_IMPLEMENTATION_FRONTEND_USE_H

#include <darma/interface/backend/flow.h>
#include "handle.h"

namespace darma_runtime {
namespace abstract {
namespace frontend {

/** @brief Encapsulates the state, permissions, and data reference for a given use of a Handle at a given time.
 *
 *  @todo update this to include publish_use
 *
 *  Use objects have a life cycle with 3 strictly ordered phases.  For some Use instance ha,
 *    + Creation/registration -- &ha is passed as argument to
 *      register_handle_access().  At this time, ha.get_in_flow() and
 *      ha.get_out_flow() must return unique, valid Flow objects (or a special
 *      null_flow object or something).
 *    + Task use (up to once in lifetime) -- ha is the dereference of the
 *      iterator to the iterable returned by t.get_dependencies() for some Task
 *      object t passed to register_task() after ha is created and before ha is
 *      released.  At this time, ha.immediate_permissions(),
 *      ha.scheduling_permissions(), and ha.get_data_pointer_reference() must
 *      return valid values, and these values must remain valid until release() is
 *      called (note that migration may change this time frame in future versions
 *      of the spec).
 *    + release -- release_handle(&ha) called.  TODO atomicity constraints
 *
 */
class Use {
  public:

    /** @brief An enumeration of the allowed values that immediate_permissions() and scheduling_permissions() can return
     */
    typedef enum Permissions {
      None=0, Read=1, Write=2,
      Reduce=4, Modify=Read|Write
    } permissions_t;

    /** @brief Return a pointer to the handle that this object encapsulates the use of.
     */
    virtual Handle const* get_handle() const =0;

    /** @brief Get the usage object that this Use needs to be available with the appropriate permissions
     */
    virtual backend::Flow* get_in_flow() =0;

    /** @brief Get the usage object that this Use *makes* available when it is released
     */
    virtual backend::Flow* get_out_flow() =0;

    /** Get the immediate permissions needed for the Flow returned by get_in_flow()
     */
    virtual permissions_t immediate_permissions() const =0;

    /** Get the scheduling permissions needed for the Flow returned by get_in_flow()
     */
    virtual permissions_t scheduling_permissions() const =0;

    /** Get a reference to the data pointer on which the requested immediate permissions have been granted.
     *
     *  For a Use requesting immediate permissions, the runtime will set the value of the reference
     *  returned by this function to the beginning of the data requested at least by the time the backend
     *  calls Task::run() on the task requesting this Use
     *
     */
    virtual void*& get_data_pointer_reference() =0;

};

} // end namespace frontend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_IMPLEMENTATION_FRONTEND_USE_H
