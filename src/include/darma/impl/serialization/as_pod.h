/*
//@HEADER
// ************************************************************************
//
//                      as_pod.h
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

#ifndef DARMA_IMPL_SERIALIZATION_AS_POD_H
#define DARMA_IMPL_SERIALIZATION_AS_POD_H

#include <type_traits>
#include "nonintrusive.h"

namespace darma_runtime {

namespace serialization {


// TODO merge with fundamental from builtin.h?

template <typename T>
struct serialize_as_pod : std::false_type { };

template <typename T>
struct Serializer<T, std::enable_if_t<serialize_as_pod<T>::value>> {

  static_assert(
    std::is_trivially_constructible<T>::value,
    "Values serialized as POD must be trivially constructable"
  );
  static_assert(
    std::is_standard_layout<T>::value,
    "Values serialized as POD must be standard layout types"
  );

  template <typename Archive>
  void compute_size(T const& val, Archive& ar) {
    assert(ar.is_sizing());
    Serializer_attorneys::ArchiveAccess::spot(ar) += sizeof(T);
  }

  template <typename Archive>
  void pack(T const& val, Archive& ar) {
    using Serializer_attorneys::ArchiveAccess;
    assert(ar.is_packing());
    std::memcpy(ArchiveAccess::spot(ar), &val, sizeof(T));
    ArchiveAccess::spot(ar) += sizeof(T);
  }

  template <typename ArchiveT>
  void
  unpack(void* val, ArchiveT& ar) const {
    // This approach should be valid for trivially-constructible, standard layout types
    using Serializer_attorneys::ArchiveAccess;
    assert(ar.is_unpacking());
    std::memcpy(val, ArchiveAccess::spot(ar), sizeof(T));
    ArchiveAccess::spot(ar) += sizeof(T);
  }
};


} // end namespace serialization

} // end namespace darma_runtime


#endif //DARMA_IMPL_SERIALIZATION_AS_POD_H
