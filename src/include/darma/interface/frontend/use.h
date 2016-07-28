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

#include <darma/interface/frontend/frontend_fwd.h>

#include <darma/interface/backend/flow.h>
#include <darma/interface/backend/allocation_policy.h>

#include "handle.h"

namespace darma_runtime {
namespace abstract {
namespace frontend {

/** @brief Encapsulates the state, permissions, and data reference for a Handle at a given point in logical time as required by an operation.
 *
 *  Use objects have a life cycle with 3 strictly ordered phases.  For some Use instance u,
 *    + Creation/registration -- `&u` is passed as the argument to
 *      Runtime::register_use().  At this time, u.get_in_flow() and
 *      u.get_out_flow() must return unique, valid Flow objects.
 *    + Task or Publish use (up to once in lifetime):
 *      - Task use: For tasks, `&u` can be accessed through the iterable
 *        returned by t.get_dependencies() for some Task object `t` passed
 *        to Runtime::register_task() after `u` is created and before `u` is released.
 *        At task execution time, u.immediate_permissions(), u.scheduling_permissions(), 
 *        and u.get_data_pointer_reference() must
 *        return valid values, and these values must remain valid until Runtime::release_use(u) is
 *        called (note that migration may change this time frame in future versions
 *        of the spec).
 *      - Publish use: A single call to Runtime::publish_use() may be made for any Use.
 *        The frontend may immediately call Runtime::release_use() after publish_use().
 *        If the publish is deferred and has not completed by the time release_use() is called,
 *        the backend runtime must extract the necessary Flow and key fields from the Use.
 *    + Release -- Following a task use or a publish use, the translation layer will make
 *        a single call to Runtime::release_use(). The Use instance may no longer be valid on return.
 *        The destructor of Use will NOT delete its input and output flow.
 *        The backend runtime is responsible for deleting Flow allocations, which may occur during release.
 *        
 */
class Use {
  public:

    /** @brief An enumeration of the allowed values that immediate_permissions() and scheduling_permissions() can return
     */
    typedef enum Permissions {
      None=0,   /*!< A Use may not perform any operations (read or write). Usually only immediate_permissions will be None */
      Read=1,   /*!< An immediate (scheduling) Use may only perform read operations (create read-only tasks) */
      Write=2,  /*!< An immediate (scheduling) Use may perform write operations (create write tasks) */
      Modify=3,  /*!< Read|Write. An immediate (scheduling) Use may perform any operations (create any tasks) */
      Reduce=4  /*!< An immediate (scheduling) Use may perform reduce operations (create reduce tasks).
                     This is not a strict subset of Read/Write privileges */
    } permissions_t;

    /** @brief Return a pointer to the handle that this object encapsulates a use of.
     */
    virtual Handle const*
    get_handle() const =0;

    /** @brief Get the Flow that must be ready for use as a precondition for the
     *  Task t that depends on this Use
     */
    virtual backend::Flow*&
    get_in_flow() =0;

    /** @brief Get the Flow that is produced or made available when this Use is
     *  released
     */
    virtual backend::Flow*&
    get_out_flow() =0;

    /** @brief Get the immediate permissions needed for the Flow returned by
     *  get_in_flow() to be ready as a precondition for this Use
     *
     *  @return
     */
    virtual permissions_t
    immediate_permissions() const =0;

    /** @brief Get the scheduling permissions needed for the Flow returned by
     *  get_in_flow() to be ready as a precondition for this Use
     *
     *  @return
     */
    virtual permissions_t
    scheduling_permissions() const =0;

    /** @brief Get a reference to the data pointer on which the requested
     *  immediate permissions have been granted.
     *
     *  For a Use requesting immediate permissions, the runtime will set the
     *  value of the reference returned by this function to the beginning of the
     *  data requested at least by the time the backend calls Task::run() on the
     *  task requesting this Use
     *
     */
    virtual void*&
    get_data_pointer_reference() =0;

    /** @todo document this
     *
     *  @param alloc_policy
     */
    virtual void
    set_allocation_policy(
      backend::AllocationPolicy* alloc_policy
    ) =0;

};


} // end namespace frontend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_IMPLEMENTATION_FRONTEND_USE_H
