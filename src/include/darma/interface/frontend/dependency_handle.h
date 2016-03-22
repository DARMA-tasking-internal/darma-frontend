/*
//@HEADER
// ************************************************************************
//
//                          dependency.h
//                         darma_new
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

#ifndef SRC_ABSTRACT_FRONTEND_DEPENDENCY_HANDLE_H_
#define SRC_ABSTRACT_FRONTEND_DEPENDENCY_HANDLE_H_


namespace darma_runtime {

namespace abstract {

// Forward declaration of DataBlock
namespace backend {
  class DataBlock;
} // end namespace backend

// Forward declaration of SerializationManager
namespace frontend {
  class SerializationManager;
} // end namespace backend

namespace frontend {



/**
 *  @ingroup abstract
 *
 *  @class DependencyHandle
 *
 *  @brief The fundamental abstraction for the frontend to communicate units of
 *  data dependence to the backend.
 *
 *  @tparam Key must meet the Key concept
 *  @tparam Version must meet the Version concept
 */
template <
  typename Key,
  typename Version
>
class DependencyHandle {
  public:

    /** @brief The key associated with the dependency.
     *
     *  Other dependencies that share a key with this dependency denote a requirement of the same
     *  data block (though differing versions indicate that the requirement refers to a different
     *  snapshot in time of that data block).
     *
     *  @remark The frontend must ensure that the return value for get_key() is the same from the
     *  time that Runtime::register_handle() or register_fetching_handle() is called with a given
     *  handle until Runtime::release_handle() is called with that same handle.
     *
     */
    virtual const Key&
    get_key() const =0;

    /** @brief The version associated with the dependency.
     *
     *  Versions are hierarchical, strictly ordered, hashable, incrementable objects for which the
     *  default constructed value is always less than or equal to all values.  See Version concept
     *  for more information
     *
     */
    virtual const Version&
    get_version() const =0;

    /** @brief Set the version associated with the dependency
     *
     *  The backend may call set_version() on a given DependencyHandle at most one time between
     *  the handle's registration and release, and only if version_is_pending() returns true
     *  (which should only be the case if the handle was registered with Runtime::register_fetching_handle()
     *  and the handle's actual version is pending the resolution of the user version tag associated
     *  with that call).
     *
     *  @todo there is a race condition in which a subsequent is being created in the frontend on one thread
     *  while the version is being resolve on another thread.  This isn't an issue in the 0.2 spec
     *  since fetching handles can't have subsequents, but this needs to be addressed (existing thread safety
     *  requirements don't necessarily suffice here necessarily since set_version() is in the frontend and
     *  register_fetching_handle() is in the backend)
     */
    virtual void
    set_version(const Version& v) =0;

    /** @brief returns true if the handle's version is pending
     *
     *  That is, if the handle was registered with Runtime::register_fetching_handle()
     *  and the handle's actual version is pending the resolution of the user version tag associated
     *  with that call, so set_version() hasn't been called yet.
     */
    virtual bool
    version_is_pending() const =0;

    /** @brief return the serialization manager describing how to allocate, pack, and unpack the data
     *  associated with the dependency
     *
     *  @remark In the 0.2.0 spec implementation, the only relevant piece of information the SerializationManager
     *  returns is the size of the metadata (i.e., the stack size; literally sizeof(T) for some type T) so that
     *  the backend can allocate data for data blocks.
     *
     *  @todo 0.3 spec: more discussion as SerializationManager spec matures
     *
     */
    virtual SerializationManager*
    get_serialization_manager() =0;

    /** @brief Satisfy the dependency handle with a DataBlock containing the requested data or an
     *  allocated buffer into which data may be written.
     *
     *  The underlying data in the data block may not be written to until the backend calls
     *  DependencyHandle::allow_writes() on this handle.  This method should be called at most
     *  once in the lifetime of a DependencyHandle, and it should only be called zero times if
     *  no tasks were registered during its lifetime that reported a dependence on the handle.
     *
     *  @param data A (non-owning) pointer to the DataBlock for the DependencyHandle to use for
     *  all data operations.  If any tasks have been registered that make read only usage of
     *  the handle, or if the up to one
     *  modify usage (see Runtime::release_read_only_usage() for details) of the handle is a task
     *  that returns true for Task::needs_read_data(handle), the readable data must be available
     *  at the time of this invocation.  Otherwise, a buffer of the correct size to contain
     *  the metadata as described by the serialization manager should be allocated.  As of
     *  the 0.2 spec, the handle and data block should not be moved from the time
     *  satisfy_with_data_block() is called until the time Runtime::release_handle() is called
     *  (i.e., the pointer passed in here should be valid at least until Runtime::release_handle()
     *  is called).
     *
     *  @todo 0.4: spec protocol for migrating data blocks and handles after a handle has been satisfied
     *
     */
    virtual void
    satisfy_with_data_block(
      abstract::backend::DataBlock* const data
    ) =0;

    /** @brief Get the pointer to the data block passed into satisfy_with_data_block() by the backend
     *
     *  It is an (debug-mode) error to call this method before calling satisfy_with_data_block()
     *
     *  @todo 0.4: this should probably eventually return a DataBlock*& to facilitate migration without
     *  re-satisfaction for certain cases
     *
     */
    virtual abstract::backend::DataBlock*
    get_data_block() const =0;

    /** @brief returns true iff satisfy_with_data_block() has been called for this instance
     */
    virtual bool
    is_satisfied() const =0;

    /** @brief Notify the dependency handle that the last read-only usage of the handle has cleared
     *  and any additional usage until Runtime::release_handle() is called may safely modify the
     *  data in the associated data block.
     *
     *  It is a (debug-mode) error to call allow_writes() before calling satisfy_with_data_block().
     *  allow_writes() should be called at most once in the lifetime of a dependency handle, and
     *  it should only be called zero times for handles registered with register_fetching_handle().
     *
     */
    virtual void
    allow_writes() =0;

    /** @brief returns true iff allow_writes() has been called for this instance
     */
    virtual bool
    is_writable() const =0;

    virtual ~DependencyHandle() noexcept = default;
};


} // end namespace frontend

} // end namespace abstract

} // end namespace darma_runtime

#endif /* SRC_ABSTRACT_FRONTEND_DEPENDENCY_HANDLE_H_ */
