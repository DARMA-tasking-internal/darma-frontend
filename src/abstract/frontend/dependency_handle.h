/*
//@HEADER
// ************************************************************************
//
//                          dependency.h
//                         dharma_new
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


namespace dharma_runtime {

namespace abstract {

// Forward declaration of DataBlock
namespace backend {
  class DataBlock;
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
     *  Versions are strictly ordered, hashable, incrementable objects for which the default constructed
     *  value is always less than or equal to all values.  See Version concept for more information
     *
     */
    virtual const Version&
    get_version() const =0;

    /** @brief Set the version associated with the dependency
     *
     *  The backend may call set_version() on a given DependencyHandle at most one time between
     *  the handle's registration and release, and only if set_version_is_pending(true) has been
     *  called (which should only be done if this handle or another with an earlier version from
     *  which this handle was derived was registered with Runtime::register_fetching_handle()
     *  and the handle's actual version is pending the resolution of the user version tag associated
     *  with that call).
     */
    virtual void
    set_version(const Version& v) =0;

    /** @brief Indicate that the version of the handle is pending the resolution of a version tag for a fetch.
     *
     *  This allows the frontend to continue creating and registering tasks even while waiting to find out
     *  what version of a handle to fetch.  See set_version() for more info.
     *
     */
    virtual void
    set_version_is_pending(bool is_pending=true) =0;

    /** @brief returns true if the backend has called set_version_is_pending(true) on the handle
     *  and if set_version_is_pending(false) and/or set_version() has not been called since.
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
    SerializationManager*
    get_serialization_manager() =0;

    /** @brief Satisfy the dependency handle with data or with an allocated buffer into which data may be written
     *
     *  @TODO finish this and figure out how we will distinguish read-satisfied and write-satisfied
     *
     */
    virtual void
    satisfy_with_data_block(
      abstract::backend::DataBlock* const data
    ) =0;

    virtual abstract::backend::DataBlock*
    get_data_block() =0;

    // TODO figure out if we should distinguish here between read-satisfied and write-satisfied....
    virtual bool
    is_satisfied() const =0;

    virtual ~DependencyHandle() noexcept { };
};


} // end namespace frontend

} // end namespace abstract

} // end namespace dharma_runtime

#endif /* SRC_ABSTRACT_FRONTEND_DEPENDENCY_HANDLE_H_ */
