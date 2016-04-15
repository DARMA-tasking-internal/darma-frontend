/*
//@HEADER
// ************************************************************************
//
//                         archive.h
//                          DARMA
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

#ifndef DARMA_IMPL_SERIALIZATION_ARCHIVE_H_
#define DARMA_IMPL_SERIALIZATION_ARCHIVE_H_

#include <darma/impl/serialization/nonintrusive.h>
#include <darma/impl/serialization/serialization_fwd.h>

namespace darma_runtime {

namespace serialization {

////////////////////////////////////////////////////////////////////////////////

// A simple frontend object for interacting with user-defined serializations
// TODO Archives that check for pointer loops and stuff

class Archive {
  private:
    // readability alias
    using byte = char;

    byte* start = nullptr;
    byte* spot = nullptr;

    detail::SerializerMode mode = detail::SerializerMode::None;

  public:

    bool is_sizing() const { return mode == detail::SerializerMode::Sizing; }
    bool is_packing() const { return mode == detail::SerializerMode::Packing; }
    bool is_unpacking() const { return mode == detail::SerializerMode::Unpacking; }

    template <typename T>
    void incorporate_size(T const& val) {
      typename detail::serializability_traits<T>::serializer ser;
      ser.get_packed_size(val, *this);
    }

    template <typename T>
    void pack_item(T const& val) {
      typename detail::serializability_traits<T>::serializer ser;
      ser.pack(val, *this);
    }

    template <typename T>
    void unpack_item(T& val) {
      typename detail::serializability_traits<T>::serializer ser;
      ser.unpack(val, *this);
    }

    // TODO bells and whistles like |, <<, >>, iterator ranges, etc

  private:

    friend class Serializer_attorneys::ArchiveAccess;
    friend class darma_runtime::detail::DependencyHandle_attorneys::ArchiveAccess;

};

////////////////////////////////////////////////////////////////////////////////

} // end namespace serialization

} // end namespace darma_runtime

#endif /* DARMA_IMPL_SERIALIZATION_ARCHIVE_H_ */
