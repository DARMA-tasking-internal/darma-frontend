/*
//@HEADER
// ************************************************************************
//
//                         nonintrusive.h
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

#ifndef DARMA_IMPL_SERIALIZATION_NONINTRUSIVE_H_
#define DARMA_IMPL_SERIALIZATION_NONINTRUSIVE_H_

#include <utility>
#include <darma/impl/meta/detection.h>
#include <cstddef>
#include <cassert>

namespace darma_runtime {

// Forward declarations
namespace serialization {
  template <typename T, typename Enable=void>
  struct Serializer;
} // end namespace serialization


namespace serialization {

namespace detail {

typedef enum SerializerMode {
  Sizing,
  Packing,
  Unpacking
} SerializerMode;

// TODO check things like is_polymorphic (and do something about it)

template <typename T, typename Enable=void>
struct serializability_traits {
  private:

    template <typename ArchiveT>
    using has_intrusive_serialize_archetype =
    decltype( std::declval<T>().serialize( std::declval<std::remove_reference_t<ArchiveT>&>() ) );

  public:
    template <typename ArchiveT>
    using has_intrusive_serialize = meta::is_detected<has_intrusive_serialize_archetype, std::decay_t<T>, ArchiveT>;

  private:
    using has_intrusive_get_packed_size_archetype = decltype( std::declval<T>().get_packed_size( ) );
    template <typename ArchiveT>
    using has_intrusive_get_packed_size_with_archive_archetype = decltype(
      std::declval<T>().get_packed_size( std::declval<std::remove_reference_t<ArchiveT>&>() )
    );

  public:
    using has_intrusive_get_packed_size =
      meta::is_detected<has_intrusive_get_packed_size_archetype, std::decay_t<T>>;
    template <typename ArchiveT>
    using has_intrusive_get_packed_size_with_archive =
      meta::is_detected<has_intrusive_get_packed_size_with_archive_archetype, ArchiveT>;

  private:
    template <typename ArchiveT>
    using has_intrusive_pack_archetype =
    decltype( std::declval<T>().pack( std::declval<std::remove_reference_t<ArchiveT>&>() ) );

  public:
    template <typename ArchiveT>
    using has_intrusive_pack = meta::is_detected<has_intrusive_pack_archetype, std::decay_t<T>, ArchiveT>;

  private:
    template <typename ArchiveT>
    using has_intrusive_unpack_archetype =
    decltype( std::declval<T>().unpack( std::declval<std::remove_reference_t<ArchiveT>&>() ) );

  public:
    template <typename ArchiveT>
    using has_intrusive_unpack = meta::is_detected<has_intrusive_pack_archetype, std::decay_t<T>, ArchiveT>;

};


} // end namespace detail


// Default implementation expresses the intrusive interface non-intrusively
template <typename T, typename Enable>
struct Serializer {
  typedef detail::serializability_traits<T> traits;

  ////////////////////////////////////////////////////////////////////////////////

  // get_packed_size() method available version
  template <typename ArchiveT,
    typename = std::enable_if_t< traits::has_intrusive_get_packed_size::value >
  >
  size_t
  get_packed_size(T const& val, ArchiveT&) const {
    return val.get_packed_size();
  };

  // get_packed_size(Archive&) method available version
  template <typename ArchiveT,
    typename = std::enable_if_t<
      not traits::has_intrusive_get_packed_size::value
      and traits::template has_intrusive_get_packed_size_with_archive<ArchiveT>::value
    >
  >
  size_t
  get_packed_size(T const& val, ArchiveT& archive) const {
    return val.get_packed_size(archive);
  };

  // only serialize() method available version
  template <typename ArchiveT,
    typename = std::enable_if_t<
      not traits::template has_intrusive_get_packed_size::value
      and not traits::template has_intrusive_get_packed_size_with_archive<ArchiveT>::value
      and traits::template has_intrusive_serialize<ArchiveT>::value
    >
  >
  size_t
  get_packed_size(T const& val, ArchiveT& archive) const {
    size_t start = archive.get_size();
    archive.mode = detail::Sizing;
    val.serialize(archive);
    size_t stop = archive.get_size();
    assert(stop >= start);
    return stop - start;
  };

  ////////////////////////////////////////////////////////////////////////////////

  // pack() method version
  template <typename ArchiveT,
    typename = std::enable_if_t<traits::template has_intrusive_pack<ArchiveT>::value>
  >
  void
  pack(T const& val, ArchiveT& archive) const {
    val.pack(archive);
  };

  // serialize() method version
  template <typename ArchiveT,
    typename = std::enable_if_t<
      traits::template has_intrusive_serialize<ArchiveT>::value
      and not traits::template has_intrusive_pack<ArchiveT>::value
    >
  >
  void
  pack(T& val, ArchiveT& archive) const {
    archive.mode = detail::Packing;
    val.serialize(archive);
  };

  ////////////////////////////////////////////////////////////////////////////////

  // unpack() method version
  template <typename ArchiveT,
    typename = std::enable_if_t<traits::template has_intrusive_pack<ArchiveT>::value>
  >
  void
  unpack(T const& val, ArchiveT& archive) const {
    val.pack(archive);
  };

  // serialize() method version
  template <typename ArchiveT,
    typename = std::enable_if_t<
      traits::template has_intrusive_serialize<ArchiveT>::value
        and not traits::template has_intrusive_unpack<ArchiveT>::value
    >
  >
  void
  unpack(T& val, ArchiveT& archive) const {
    archive.mode = detail::Unpacking;
    val.serialize(archive);
  };
};


} // end namespace serialization

} // end namespace darma_runtime

#endif /* DARMA_IMPL_SERIALIZATION_NONINTRUSIVE_H_ */
