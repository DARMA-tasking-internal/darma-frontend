/*
//@HEADER
// ************************************************************************
//
//                      array.h
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

#ifndef DARMA_IMPL_SERIALIZATION_ARRAY_H
#define DARMA_IMPL_SERIALIZATION_ARRAY_H

#include <cstddef>

#include "nonintrusive.h"
#include "traits.h"

namespace darma_runtime {
namespace serialization {

template <typename T, size_t N>
struct Serializer<T[N]> {
  private:

    typedef detail::serializability_traits<T> serdes_traits;

  public:

    using directly_serializable = std::integral_constant<bool,
      serdes_traits::is_directly_serializable
    >;

  private:

    template <typename Archive>
    using enable_if_ser = std::enable_if_t<
      serdes_traits::template is_serializable_with_archive<Archive>::value
    >;

    template <typename Archive>
    using enable_if_ser_direct = std::enable_if_t<
      directly_serializable::value and
      serdes_traits::template is_serializable_with_archive<Archive>::value
    >;

    template <typename Archive>
    using enable_if_sizable_indirect = std::enable_if_t<
      not directly_serializable::value and
        serdes_traits::template is_sizable_with_archive<Archive>::value
    >;

    template <typename Archive>
    using enable_if_packable_indirect = std::enable_if_t<
      not directly_serializable::value and
        serdes_traits::template is_packable_with_archive<Archive>::value
    >;

    template <typename Archive>
    using enable_if_unpackable_indirect = std::enable_if_t<
      not directly_serializable::value and
        serdes_traits::template is_unpackable_with_archive<Archive>::value
    >;

  public:

    // Note: compile-time sized arrays of objects should be packed indirectly.
    // To pack/unpack large contiguous buffers of array types, the parent
    // type's serializer should call pack_direct
    // should call the archive's *_direct() methods itself

    template <typename Archive>
    enable_if_ser_direct<Archive>
    compute_size(const T val[N], Archive& ar) const {
      ar.add_to_size_indirect(sizeof(T) * N);
    }

    template <typename Archive>
    enable_if_sizable_indirect<Archive>
    compute_size(const T val[N], Archive& ar) const {
      for(int i = 0; i < N; ++i) {
        ar.incorporate_size(val[i]);
      }
    }

    template <typename Archive>
    enable_if_ser_direct<Archive>
    pack(const T val[N], Archive& ar) const {
      ar.pack_indirect(val, val+N);
    }

    template <typename Archive>
    enable_if_packable_indirect<Archive>
    pack(const T val[N], Archive& ar) const {
      for(int i = 0; i < N; ++i) {
        ar.pack_item(val[i]);
      }
    }

    template <typename Archive, typename AllocatorT>
    enable_if_ser_direct<Archive>
    unpack(void* allocated, Archive& ar, AllocatorT&&) const {
      ar.template unpack_indirect<T>( (T*)allocated, N );
    }

    template <typename Archive, typename AllocatorT>
    enable_if_unpackable_indirect<Archive>
    unpack(void* allocated, Archive& ar, AllocatorT&& alloc) const {
      for(int i = 0; i < N; ++i) {
        ar.unpack_item(((T*)allocated, std::forward<AllocatorT>(alloc))[i]);
      }
    }
};

} // end namespace serialization

} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_ARRAY_H
