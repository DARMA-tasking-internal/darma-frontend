/*
//@HEADER
// ************************************************************************
//
//                      archive_base.h
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

#ifndef DARMA_IMPL_SERIALIZATION_ARCHIVE_PASSTHROUGH_H
#define DARMA_IMPL_SERIALIZATION_ARCHIVE_PASSTHROUGH_H

#include "traits.h"

namespace darma_runtime {

namespace serialization {

namespace detail {

template <typename ArchiveT>
class ArchivePassthroughMixin {

  protected:

    template <typename T>
    using traits = detail::serializability_traits<T>;
    template <typename T>
    using is_serializable_with_this =
      typename traits<T>::template is_serializable_with_archive<ArchiveT>;

    template <typename T>
    using has_only_serialize = std::integral_constant<bool,
      traits<T>::template has_nonintrusive_serialize<ArchiveT>::value
        and not (
          traits<T>::template has_nonintrusive_compute_size<ArchiveT>::value
            or traits<T>::template has_nonintrusive_pack<ArchiveT>::value
            or traits<T>::template has_nonintrusive_unpack<ArchiveT>::value
        )
    >;

    detail::SerializerMode mode = detail::SerializerMode::None;

  public:

    bool is_sizing() const { return mode == detail::SerializerMode::Sizing; }
    bool is_packing() const { return mode == detail::SerializerMode::Packing; }
    bool is_unpacking() const { return mode == detail::SerializerMode::Unpacking; }

    template <typename T>
    std::enable_if_t<is_serializable_with_this<T>::value>
    incorporate_size(T const& val) {
      traits<T>::compute_size(val, *static_cast<ArchiveT*>(this));
    }

    template <typename T>
    std::enable_if_t<is_serializable_with_this<T>::value>
    pack_item(T const& val) {
      traits<T>::template pack(val, *static_cast<ArchiveT*>(this));
    }

    template <typename T>
    std::enable_if_t<is_serializable_with_this<T>::value>
    unpack_item(void* val) {
      traits<T>::unpack( val, *static_cast<ArchiveT*>(this) );
    }

    template <typename T, typename AllocatorT>
    std::enable_if_t<is_serializable_with_this<T>::value>
    unpack_item(void* val, AllocatorT&& alloc) {
      traits<T>::unpack( val, *static_cast<ArchiveT*>(this),
        std::forward<AllocatorT>(alloc)
      );
    }

    template <typename T>
    std::enable_if_t<is_serializable_with_this<T>::value>
    unpack_item(T& val) {
      traits<T>::unpack(
        // May need to cast away constness if T is deduced to be const
        // TODO figure out where T is being deduced as const.  It shouldn't be
        const_cast<void*>(static_cast<const void*>(&val)),
        *static_cast<ArchiveT*>(this)
      );
    }

    template <typename T, typename AllocatorT>
    std::enable_if_t<is_serializable_with_this<T>::value>
    unpack_item(T& val, AllocatorT&& alloc) {
      traits<T>::unpack(
        const_cast<void*>(static_cast<const void*>(&val)),
        *static_cast<ArchiveT*>(this),
        std::forward<AllocatorT>(alloc)
      );
    }

    template <typename T>
    std::enable_if_t<
      is_serializable_with_this<T>::value and not has_only_serialize<T>::value
    >
    serialize_item(T& val) {
      if(is_sizing()) incorporate_size(val);
      else if(is_packing()) pack_item(val);
      else {
        assert(is_unpacking());
        unpack_item(val);
      }
    }

    template <typename T>
    std::enable_if_t<
      is_serializable_with_this<T>::value and has_only_serialize<T>::value
    >
    serialize_item(T& val) {
      typename traits<T>::serializer ser;
      if(is_unpacking()) new (&val) T;
      ser.serialize(val, *static_cast<ArchiveT*>(this));
    }

};

} // end namespace detail


} // end namespace serialization

} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_ARCHIVE_PASSTHROUGH_H
