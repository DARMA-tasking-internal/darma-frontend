/*
//@HEADER
// ************************************************************************
//
//                          serializer.h
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

#ifndef SRC_ABSTRACT_FRONTEND_SERIALIZATION_MANAGER_H_
#define SRC_ABSTRACT_FRONTEND_SERIALIZATION_MANAGER_H_

namespace dharma_runtime {

namespace abstract {

namespace frontend {

// TODO this needs to be changed into a static class Concept and used as a template parameter since all of it's methods are const

class SerializationManager {

  public:

    typedef size_t zero_copy_slot_t;

    //////////////////////////////////////////
    //// Callbacks indicating that the
    //// packing/unpacking of a block of data
    //// has begun or is completing

    //virtual void
    //begin_packing(
    //  const void* const deserialized_data
    //)
    //{ /* default implementation is a no-op */ }

    //virtual void
    //finish_packing(
    //  const void* const deserialized_data,
    //  void* const serialized_metadata,
    //  void* const serialized_data
    //)
    //{ /* default implementation is a no-op */ }

    //virtual void
    //begin_unpacking(
    //  void* const serialized_metadata,
    //  void* const serialized_data
    //)
    //{ /* default implementation is a no-op */ }

    //virtual void
    //finish_unpacking(
    //  const void* const serialized_metadata,
    //  const void* const serialized_data,
    //  void* const deserialized_data
    //)
    //{ /* default implementation is a no-op */ }

    ////////////////////////////////////////
    // Metadata serialization phase

    virtual size_t
    get_metadata_size(
      const void* const deserialized_data
    ) const =0;

    virtual void
    serialize_metadata(
      const void* const deserialized_data,
      void* const buffer
    ) const
    { /* default implementation is a no-op */ }

    virtual void
    deserialize_metadata(
      const void* const serialized_data,
      void* const buffer
    ) const
    { /* default implementation is a no-op */ }

    ////////////////////////////////////////
    // regular data serialization phase

    virtual bool
    needs_serialization() const
    { return false; }

    virtual size_t
    get_packed_data_size(
      const void* const deserialized_data
    ) const
    { return 0; }

    virtual void
    serialize_data(
      const void* const deserialized_data,
      void* const serialization_buffer
    ) const
    {
      // No-op by default, since contiguous types
      // don't need to do anything.  Strictly won't
      // get called unless needs_serialization() returns
      // true.
    }

    virtual void
    deserialize_data(
      const void* const serialized_data,
      void* const deserialized_metadata_and_dest
    ) const
    {
      // No-op by default, since contiguous types
      // don't need to do anything.  Strictly won't
      // get called unless needs_serialization() returns
      // true.
    }

    ////////////////////////////////////////
    // zero-copy slot serialization phase

    virtual size_t
    get_n_zero_copy_slots(
      const void* const deserialized_metadata,
    ) const
    { return 0; }

    virtual size_t
    get_zero_copy_data_size(
      const void* const deserialized_metadata,
      zero_copy_slot_t slot
    ) const
    { return 0; }

    virtual void*&
    get_zero_copy_slot(
      void* const deserialized_metadata,
      zero_copy_slot_t slot
    ) const =0;

    ////////////////////////////////////////

    virtual ~SerializationManager() = default;
};


} // end namespace frontend

} // end namespace abstract

} // end namespace dharma_runtime



#endif /* SRC_ABSTRACT_FRONTEND_SERIALIZATION_MANAGER_H_ */
