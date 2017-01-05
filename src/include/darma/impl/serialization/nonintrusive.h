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
#include <cstring>
#include <string>
#include <utility>

#include <tinympl/always_true.hpp>
#include <tinympl/copy_traits.hpp>

#include <darma/impl/meta/detection.h>

#include "serialization_fwd.h"
#include "serializer_attorneys.h"

namespace darma_runtime {

namespace serialization {

////////////////////////////////////////////////////////////////////////////////

// Default implementation expresses pass-through to the intrusive interface non-intrusively
namespace detail {

template <typename T, typename Enable>
struct Sizer {
  typedef detail::serializability_traits<T> traits;

  // compute_size() method available version
  template <typename ArchiveT>
  std::enable_if_t<
    traits::has_intrusive_compute_size::value
      and tinympl::always_true<ArchiveT>::value
  >
  compute_size(T const& val, ArchiveT& ar) const {
    ar.add_to_size(val.compute_size());
  };

  // compute_size(Archive&) method available version
  template <typename ArchiveT>
  std::enable_if_t<
    not traits::has_intrusive_compute_size::value
      and traits::template has_intrusive_compute_size_with_archive<ArchiveT>::value
  >
  compute_size(T const& val, ArchiveT& ar) const {
    val.compute_size(ar);
  };

  // only serialize() method available version
  template <typename ArchiveT>
  std::enable_if_t<
    not traits::has_intrusive_compute_size::value

      and traits::template has_intrusive_serialize<ArchiveT>::value
  >
  compute_size(T const& val, ArchiveT& ar) const {
    // const-cast necessary for general serialize method to work, which should be
    // non-const to work with deserialization
    const_cast<T&>(val).serialize(ar);
  };

};

} // end namespace detail

////////////////////////////////////////////////////////////////////////////////

namespace detail {

template <typename T, typename Enable>
struct Packer {
  typedef detail::serializability_traits<T> traits;

  // pack() method version
  template <typename ArchiveT>
  std::enable_if_t<traits::template has_intrusive_pack<ArchiveT>::value>
  pack(T const &val, ArchiveT &ar) const {
    val.pack(ar);
  };

  // serialize() method version
  template <typename ArchiveT>
  std::enable_if_t<
    traits::template has_intrusive_serialize<ArchiveT>::value
      and not traits::template has_intrusive_pack<ArchiveT>::value
  >
  pack(T const &val, ArchiveT &ar) const {
    // const-cast necessary for general serialize() method to work, which should be
    // non-const to work with deserialization
    const_cast<T &>(val).serialize(ar);
  };

};

} // end namespace detail

////////////////////////////////////////////////////////////////////////////////


namespace detail {

template <typename T, typename Enable>
struct Unpacker {
  typedef detail::serializability_traits<T> traits;

  // unpack() method version
  // Note that it is an error to have both an unpacking constructor and an unpack() method
  template <typename ArchiveT>
  std::enable_if_t<traits::template has_intrusive_unpack<ArchiveT>::value>
  unpack(void *allocated, ArchiveT &ar) const {
    T& v = traits::reconstruct(allocated, ar);
    v.unpack(ar);
  };

  // serialize() method version
  template <typename ArchiveT>
  std::enable_if_t<
    traits::template has_intrusive_serialize<ArchiveT>::value
      and not traits::template has_intrusive_unpack<ArchiveT>::value
  >
  unpack(void *allocated, ArchiveT &ar) const {
    T& v = traits::reconstruct(allocated, ar);
    v.serialize(ar);
  };

};

} // end namespace detail

////////////////////////////////////////////////////////////////////////////////

// Default implementation expresses pass-through to the intrusive interface non-intrusively
namespace detail {
  template <typename T, typename Enable>
  struct Serializer_enabled_if
    : public Sizer<T, Enable>,
      public Packer<T, Enable>,
      public Unpacker<T, Enable>
  { };
} // end namespace detail

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct Serializer
  : public detail::Serializer_enabled_if<T>
{ };

} // end namespace serialization

} // end namespace darma_runtime

// TODO eventually we might want to (very carefully) move these includes to a different file to reduce compile times for power users
#include "as_pod.h"
#include "builtin.h"
#include "stl_containers.h"
#include "stl_pair.h"
#include "array.h"
#include "range.h"
#include "stl_vector.h"
#include "tuple.h"
#include "string.h"

#endif /* DARMA_IMPL_SERIALIZATION_NONINTRUSIVE_H_ */
