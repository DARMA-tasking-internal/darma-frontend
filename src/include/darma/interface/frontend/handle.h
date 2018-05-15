/*
//@HEADER
// ************************************************************************
//
//                      handle.h
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMA_INTERFACE_FRONTEND_HANDLE_H
#define DARMA_INTERFACE_FRONTEND_HANDLE_H

#include <darma/interface/frontend/serialization_manager.h>
#include <darma/interface/frontend/array_concept_manager.h>
#include <darma/interface/frontend/array_movement_manager.h>
#include <darma_types.h>

namespace darma {
namespace abstract {
namespace frontend {

/** @brief Encapsulates a named, mutable chunk of data which may be accessed by one or more tasks
 *  that use that data (or the privilege to schedule permissions on that data).
 *
 *  A Handle represents an entity conceptually similar to a variable in a serial program.
 */
class Handle {
  public:

    /** @brief Indicate whether or not the backend should assign a unique key
     *  to the handle using the set_key() function
     *
     *  @return false if the backend should generate a key for this Handle and
     *  pass it to set_key(), true otherwise.
     *
     *  @sa Handle::set_key()
     *
     *  @todo rename this something like needs_backend_assigned_key() and negate it
     */
    virtual bool
    has_user_defined_key() const =0;

    /**
     * @brief get_key Returns a unique key.
     *
     * It is an error to call this before calling set_key() if
     * has_user_defined_key() returns false.
     *
     * Multiple calls to this function on the same handle object must always
     * return the same value.
     * @return A unique key identifying the tuple.
     */
    virtual types::key_t const&
    get_key() const =0;

    /** @brief Set the value to be returned by get_key() for the rest of the
     *  Handle's lifetime.
     *
     *  It is only valid to call this if has_user_defined_key() returns false.
     *  Furthermore, if h.has_user_defined_key() does return false for a given
     *  handle, h.set_key() must be called at least by the time the call to
     *  Runtime::make_initial_flow with a pointer to h as an argument returns.
     *  It is invalid to pass a pointer to a Handle that returns false for
     *  has_user_defined_key() to Runtime::make_fetching_flow() or
     *  Runtime::make_null_flow().
     *
     *  @remark This function should be called at most once in the lifetime
     *  of a Handle object
     *
     *
     *  @param generated_key The globally unique generated key that this object
     *  should return from get_key()
     */
    virtual void
    set_key(types::key_t const& generated_key) =0;

    /**
     * @brief get_serialization_manager Returns a type-specific serialization
     * manager. The object returned will be persistent as long as the Handle
     * exists
     *
     * @return A type-specific serialization manager
     */
    virtual SerializationManager const*
    get_serialization_manager() const =0;

    /** @brief TODO
     *
     */
    virtual ArrayMovementManager const*
    get_array_movement_manager() const =0;

    /** @brief TODO
     *
     */
    virtual ArrayConceptManager const*
    get_array_concept_manager() const =0;

#if _darma_has_feature(unmanaged_data)
    virtual bool
    data_is_unmanaged() const { return false; }
#endif // _darma_has_feature_mpi_interoperability

};

} // end namespace frontend
} // end namespace abstract
} // end namespace darma

#endif //DARMA_INTERFACE_FRONTEND_HANDLE_H
