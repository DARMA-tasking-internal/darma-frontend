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

#include <cassert>
#include <cstddef>
#include <string>
#include <utility>

#include <tinympl/always_true.hpp>
#include <tinympl/copy_traits.hpp>

#include <darma/impl/meta/detection.h>
#include <darma/impl/serialization/serialization_fwd.h>
#include <darma/impl/serialization/traits.h>
#include <cstring>

namespace darma_runtime {

namespace serialization {

////////////////////////////////////////////////////////////////////////////////

namespace Serializer_attorneys {

struct ArchiveAccess {
  template <typename ArchiveT>
  static inline
  typename tinympl::copy_cv_qualifiers<ArchiveT>::template apply<char*>::type&
  start(ArchiveT& ar) { return ar.start; }

  template <typename ArchiveT>
  static inline
  typename tinympl::copy_cv_qualifiers<ArchiveT>::template apply<char*>::type&
  spot(ArchiveT& ar) { return ar.spot; }

  template <typename ArchiveT>
  static inline
  typename tinympl::copy_cv_qualifiers<ArchiveT>::template apply<detail::SerializerMode>::type&
  mode(ArchiveT& ar) { return ar.mode; }

  template <typename ArchiveT>
  static inline size_t
  get_size(ArchiveT& ar) {
    assert(ar.is_sizing());
    return ar.spot - ar.start;
  }

  template <typename ArchiveT>
  static inline void
  start_sizing(ArchiveT& ar) {
    assert(not ar.is_sizing()); // for now, to avoid accidental resets
    ar.start = nullptr;
    ar.spot = nullptr;
    ar.mode = serialization::detail::SerializerMode::Sizing;
  }

  template <typename ArchiveT>
  static inline void
  start_packing(ArchiveT& ar) {
    ar.mode = serialization::detail::SerializerMode::Packing;
  }

  template <typename ArchiveT>
  static inline void
  start_unpacking(ArchiveT& ar) {
    ar.mode = serialization::detail::SerializerMode::Unpacking;
  }

};

} // end namespace Serializer_attorneys

////////////////////////////////////////////////////////////////////////////////

// Default implementation expresses pass-through to the intrusive interface non-intrusively
template <typename T, typename Enable>
struct Serializer {
  typedef detail::serializability_traits<T> traits;

  ////////////////////////////////////////////////////////////////////////////////

  // compute_size() method available version
  template <typename ArchiveT>
  std::enable_if_t<
    traits::has_intrusive_compute_size::value
      and tinympl::always_true<ArchiveT>::value
  >
  compute_size(T const& val, ArchiveT& ar) const {
    if(not ar.is_sizing()) Serializer_attorneys::ArchiveAccess::start_sizing(ar);
    Serializer_attorneys::ArchiveAccess::spot(ar) += val.compute_size();
  };

  // compute_size(Archive&) method available version
  template <typename ArchiveT>
  std::enable_if_t<
    not traits::has_intrusive_compute_size::value
      and traits::template has_intrusive_compute_size_with_archive<ArchiveT>::value
  >
  compute_size(T const& val, ArchiveT& ar) const {
    if(not ar.is_sizing()) Serializer_attorneys::ArchiveAccess::start_sizing(ar);
    Serializer_attorneys::ArchiveAccess::spot(ar) += val.compute_size(ar);
  };

  // only serialize() method available version
  template <typename ArchiveT>
  std::enable_if_t<
    not traits::has_intrusive_compute_size::value
      and not traits::template has_intrusive_compute_size_with_archive<ArchiveT>::value
      and traits::template has_intrusive_serialize<ArchiveT>::value
  >
  compute_size(T const& val, ArchiveT& ar) const {
    if(not ar.is_sizing()) Serializer_attorneys::ArchiveAccess::start_sizing(ar);
    // const-cast necessary for general serialize method to work, which should be
    // non-const to work with deserialization
    const_cast<T&>(val).serialize(ar);
  };

  ////////////////////////////////////////////////////////////////////////////////

  // pack() method version
  template <typename ArchiveT>
  std::enable_if_t<traits::template has_intrusive_pack<ArchiveT>::value>
  pack(T const& val, ArchiveT& ar) const {
    if(not ar.is_packing()) Serializer_attorneys::ArchiveAccess::start_packing(ar);
    val.pack(ar);
  };

  // serialize() method version
  template <typename ArchiveT>
  std::enable_if_t<
    traits::template has_intrusive_serialize<ArchiveT>::value
      and not traits::template has_intrusive_pack<ArchiveT>::value
  >
  pack(T const& val, ArchiveT& ar) const {
    if(not ar.is_packing()) Serializer_attorneys::ArchiveAccess::start_packing(ar);
    // const-cast necessary for general serialize() method to work, which should be
    // non-const to work with deserialization
    const_cast<T&>(val).serialize(ar);
  };

  ////////////////////////////////////////////////////////////////////////////////

  // unpack() method version
  template <typename ArchiveT>
  std::enable_if_t<traits::template has_intrusive_pack<ArchiveT>::value>
  unpack(T& val, ArchiveT& ar) const {
    if(not ar.is_unpacking()) Serializer_attorneys::ArchiveAccess::start_unpacking(ar);
    val.unpack(ar);
  };

  // serialize() method version
  template <typename ArchiveT>
  std::enable_if_t<
    traits::template has_intrusive_serialize<ArchiveT>::value
      and not traits::template has_intrusive_unpack<ArchiveT>::value
  >
  unpack(T& val, ArchiveT& ar) const {
    if(not ar.is_unpacking()) Serializer_attorneys::ArchiveAccess::start_unpacking(ar);
    val.serialize(ar);
  };
};


////////////////////////////////////////////////////////////////////////////////
// Specialization for std::string (covered by container now)
// TODO optimized specialization for std::string

//template <typename CharT, typename Traits, typename Allocator>
//struct Serializer<std::basic_string<CharT, Traits, Allocator>,
//  std::enable_if_t</*TODO*/ true>
//> {
//  typedef std::basic_string<CharT, Traits, Allocator> T;
//  template <typename ArchiveT>
//  size_t
//  compute_size(T const&, ArchiveT&) const {
//    /* TODO */
//    return 0;
//  }
//  template <typename ArchiveT>
//  void
//  pack(T const& val, ArchiveT& ar) const {
//    /* TODO */
//  }
//  template <typename ArchiveT>
//  void
//  unpack(T& val, ArchiveT& ar) const {
//    /* TODO */
//  }
//};

} // end namespace serialization

} // end namespace darma_runtime

// TODO eventually we might want to (very carefully) move these includes to a different file to reduce compile times for power users
#include "builtin.h"
#include "stl_containers.h"

#endif /* DARMA_IMPL_SERIALIZATION_NONINTRUSIVE_H_ */
