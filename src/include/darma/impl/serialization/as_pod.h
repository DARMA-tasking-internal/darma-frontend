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

template <typename T, typename Enable=void>
struct serialize_as_pod_if : std::false_type { };

template <typename T>
struct serialize_as_pod : serialize_as_pod_if<T> { };

namespace detail {

template <typename T>
struct Serializer_enabled_if<T, std::enable_if_t<serialize_as_pod<T>::value>> {

  static_assert(
    std::is_trivially_constructible<T>::value,
    "Values serialized as POD must be trivially constructable"
  );
  static_assert(
    std::is_standard_layout<T>::value,
    "Values serialized as POD must be standard layout types"
  );

  using directly_serializable = std::true_type;

  // Note: individual POD objects should be packed indirectly.  To pack/unpack
  // large contiguous buffers of POD objects, the serializer of the parent object
  // should call the archive's *_direct() methods itself

  template <typename Archive>
  void compute_size(T const &val, Archive &ar) {
    assert(ar.is_sizing());
    ar.add_to_size_indirect(sizeof(T));
  }

  template <typename Archive>
  void pack(T const &val, Archive &ar) {
    assert(ar.is_packing());
    ar.pack_indirect(&val, &val + 1);
  }

  template <typename ArchiveT>
  void
  unpack(void *val, ArchiveT &ar) const {
    assert(ar.is_unpacking());
    ar.template unpack_indirect<T>(reinterpret_cast<T *>(val), 1);
  }
};

} // end namespace detail

} // end namespace serialization

} // end namespace darma_runtime


#endif //DARMA_IMPL_SERIALIZATION_AS_POD_H
