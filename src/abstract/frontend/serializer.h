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

#ifndef SRC_ABSTRACT_FRONTEND_SERIALIZER_H_
#define SRC_ABSTRACT_FRONTEND_SERIALIZER_H_


namespace dharma_runtime {

namespace abstract {

namespace frontend {

// TODO this needs to be changed into a static class Concept and used as a template parameter since all of it's methods are const

class Serializer {

  public:

    virtual bool
    needs_serialization() const =0;

    // Get the packed size if the data type needs serialization
    // Strictly won't get called unless needs_serialization()
    // returns true.
    virtual size_t
    get_packed_size() const
    {
      // return 0 if no packing needs to be done
      return 0;
    }

    virtual void
    serialize_data(
      void* const deserialized_data,
      void* const serialization_buffer
    ) const
    {
      // No-op by default, since contiguous types
      // don't need to do anything.  Strictly won't
      // get called unless needs_serialization() returns
      // true.
    }

    // Return the size of the *deserialized* object
    // [note that this doesn't include, e.g., the data
    // in any heap-pointer member variables.  In most
    // cases, for a data type T, the data_size will be
    // exactly sizeof(T)]
    virtual size_t
    get_data_size() const =0;

    virtual void
    deserialize_data(
      void* const serialized_data,
      void*& deserialize_destination
    ) const {
      // No-op by default, since contiguous types
      // don't need to do anything.  Strictly won't
      // get called unless needs_serialization() returns
      // true.
    }

    virtual ~Serializer() { }
};

/* Sample trivial implementation for contiguous data:
 *
 * template <typename T>
 * struct contiguous_serializer
 *   : public abstract::frontend::Serializer
 * {
 *   explicit
 *   contiguous_serializer(size_t size) : size_(size) { }
 *
 *   size_t get_data_size() const { return size_; }
 *
 *   bool needs_serialization() const { return false; }
 * };
 */

} // end namespace frontend

} // end namespace abstract

} // end namespace dharma_runtime



#endif /* SRC_ABSTRACT_FRONTEND_SERIALIZER_H_ */
