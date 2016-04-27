/*
//@HEADER
// ************************************************************************
//
//                          serializer.h
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

#ifndef SRC_ABSTRACT_FRONTEND_SERIALIZATION_MANAGER_H_
#define SRC_ABSTRACT_FRONTEND_SERIALIZATION_MANAGER_H_

namespace darma_runtime {

namespace abstract {

namespace frontend {

/* LCOV_EXCL_START (exclude abstract class from coverage) */

/** @ingroup abstract
 *
 *  @class SerializationManager
 *
 *  @brief An immutable object allowing the backend to query various serialization sizes, offsets,
 *  behaviors, and data, known by the for a given handle and its associated data block.
 *
 *  @remark The only method that is valid to invoke for the 0.2.0 spec implementation is get_metadata_size()
 *
 *  @todo 0.3 spec or later: zero-copy migration schemes
 *
 */
class SerializationManager {

  public:

    /** @brief returns the size of the data as a contiguous C++ object in memory (i.e., sizeof(T))
     */
    virtual size_t
    get_metadata_size() const =0;

    /** @brief Get the size of the buffer that the pack_data() function needs for serialization
     *
     *  @param object_data pointer to the start of the C++ object to be serialized.  The object
     *  must be fully constructed and valid for use in any context where it could be used
     *  when unpacked ("could be used" is a user-defined concept here, but basically means that
     *  operations performed on the object must yield results and side-effects "as-if" the serialization
     *  had never happened).
     */
    virtual size_t
    get_packed_data_size(
      const void* const object_data
    ) const =0;

    /** @brief Packs the object data into the serialization buffer
     *
     *  @param object_data pointer to the start of the C++ object to be serialized.  Must be in the
     *  exact same state as when get_packed_data_size() was invoked with the same object.
     *
     *  @param serialization_buffer the buffer into which the data should be packed.  The backend
     *  must preallocate this buffer to be the size returned by get_packed_data_size() when invoked
     *  *immediately* prior to pack_data() with the same object_data pointer
     *
     *  @remark The backend must ensure that no running task has write access to the object_data between the
     *  time get_packed_data_size() is called and pack_data() returns, such that the state of object_data
     *  does not change in this time frame (under, of course, the allowed assumptions that the user has
     *  correctly specified aliasing characteristics of the handle or handles pointing to object_data).
     */
    virtual void
    pack_data(
      const void* const object_data,
      void* const serialization_buffer
    ) const =0;

    /** @brief Unpacks the object data from the serialization buffer into object_dest
     *
     *  Upon invocation, object_dest must be allocated (by the backend) to have size
     *  get_metadata_size(), but the unpack_data() method is responsible for construction
     *  of the object itself into this buffer.  Upon return, object_dest should point to the
     *  beginning of a C++ object that is fully constructed and valid for use in any context
     *  where it could have been used before it was packed (see get_packed_data_size() for
     *  clarification of "could have been used")
     *
     *  @param object_dest backend-allocated buffer of size get_metadata_size() into which the
     *  object should be constructed and deserialized
     *
     *  @param serialized_data a pointer to the beginning of a buffer of the same size and
     *  state as the second argument to pack_data() upon return of pack_data() for the corresponding
     *  object to be unpacked.
     */
    virtual void
    unpack_data(
      void* const object_dest,
      const void* const serialized_data
    ) const =0;

    /** TODO document this */
    virtual void*
    allocate_data() const =0;

    //////////////////////////////////////////

    virtual ~SerializationManager() = default;
};

/* LCOV_EXCL_STOP (exclude abstract class from coverage) */


} // end namespace frontend

} // end namespace abstract

} // end namespace darma_runtime



#endif /* SRC_ABSTRACT_FRONTEND_SERIALIZATION_MANAGER_H_ */
