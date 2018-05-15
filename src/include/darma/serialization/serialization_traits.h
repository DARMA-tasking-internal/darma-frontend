/*
//@HEADER
// ************************************************************************
//
//                      serialization_traits.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMAFRONTEND_SERIALIZATION_TRAITS_H
#define DARMAFRONTEND_SERIALIZATION_TRAITS_H

#include <darma/serialization/serialization_traits_fwd.h>

#include <type_traits>

namespace darma {
namespace serialization {

template <typename T, typename Enable=void>
struct is_directly_serializable_enabled_if : std::false_type { };

template <typename T>
struct is_directly_serializable
  // fall back to SFINAE-compatible version
  : is_directly_serializable_enabled_if<T, void>
{ };


template <typename T, typename SizingArchive, typename Enable=void>
struct is_sizable_with_archive_enabled_if
  // A type is not sizable if it doesn't specialize one of the
  // is_sizable_with_archive* customization points
  : std::false_type { };

template <typename T, typename SizingArchive>
struct is_sizable_with_archive
  // fall back to SFINAE-compatible version
  : is_sizable_with_archive_enabled_if<T, SizingArchive>
{ };


template <typename T, typename PackingArchive, typename Enable=void>
struct is_packable_with_archive_enabled_if
  // A type is not packable if it doesn't specialize one of the
  // is_packable_with_archive* customization points
  : std::false_type { };

template <typename T, typename PackingArchive>
struct is_packable_with_archive
  // fall back to SFINAE-compatible version
  : is_packable_with_archive_enabled_if<T, PackingArchive>
{ };


template <typename T, typename UnpackingArchive, typename Enable=void>
struct is_unpackable_with_archive_enabled_if
  // A type is not unpackable if it doesn't specialize one of the
  // is_unpackable_with_archive* customization points
  : std::false_type
{ };

template <typename T, typename UnpackingArchive>
struct is_unpackable_with_archive
  // fall back to SFINAE-compatible version
  : is_unpackable_with_archive_enabled_if<T, UnpackingArchive, void>
{ };


} // end namespace serialization
} // end namespace darma

#include <darma/serialization/serialization_traits.impl.h>

namespace darma {
namespace serialization {

template <typename T, typename SizingArchive>
void compute_size(T const& obj, SizingArchive& ar) {
  impl::compute_size_impl(obj, ar);
};

/**
 *  @brief Customization point with a less generic name.
 *
 *  The default implementation just invokes compute_size() on the arguments
 *  as an unqualified name (allowing for ADL).
 *
 *  Internal DARMA serialization facilities will invoke this customization point
 *  in case the user has a template in some other namespace containing T that
 *  has this same signature (this would/will not be as big of a deal if/when
 *  we constrain the second template parameter to meet the SizingArchive
 *  concept).
 *
 */
template <typename T, typename SizingArchive>
void darma_compute_size(T const& obj, SizingArchive& ar) {
  compute_size(obj, ar);
};

template <typename T, typename PackingArchive>
void pack(T const& obj, PackingArchive& ar) {
  impl::pack_impl(obj, ar);
};

/**
 *  @brief Customization point with a less generic name.
 *
 *  The default implementation just invokes pack() on the arguments
 *  as an unqualified name (allowing for ADL).
 *
 *  Internal DARMA serialization facilities will invoke this customization point
 *  in case the user has a template in some other namespace containing T that
 *  has this same signature (this would/will not be as big of a deal if/when
 *  we constrain the second template parameter to meet the SizingArchive
 *  concept).
 *
 */
template <typename T, typename PackingArchive>
void darma_pack(T const& obj, PackingArchive& ar) {
  pack(obj, ar);
};

/**
 *  A non-owning pointer wrapper class for the first argument to `unpack()` and
 *  `darma_unpack()` that allows argument dependent lookup (ADL) to be used with
 *  these functions (since the specification of explicit template parameters
 *  disables ADL).
 *
 *  @tparam T The type of the data to be unpacked.  Note that the namespace of
 *            T will be added to the ADL namespace set
 */
template <typename T>
struct allocated_buffer_for {
  allocated_buffer_for(void* allocated) : pointer(allocated) { }
  allocated_buffer_for(allocated_buffer_for&&) = default;
  void* pointer;
  operator void*() { return pointer; }
};

template <typename T, typename UnpackingArchive>
void unpack(allocated_buffer_for<T> allocated, UnpackingArchive& ar) {
  impl::unpack_impl<T>(allocated.pointer, ar);
};

/**
 *  @brief Customization point with a less generic name.
 *
 *  The default implementation just invokes unpack() on the arguments
 *  as an unqualified name (allowing for ADL).
 *
 *  Internal DARMA serialization facilities will invoke this customization point
 *  in case the user has a template in some other namespace containing T that
 *  has this same signature (this would/will not be as big of a deal if/when
 *  we constrain the second template parameter to meet the SizingArchive
 *  concept).
 *
 */
template <typename T, typename UnpackingArchive>
void darma_unpack(allocated_buffer_for<T> allocated, UnpackingArchive& ar) {
  unpack<T>(allocated.pointer, ar);
};

} // end namespace serialization
} // end namespace darma

#endif //DARMAFRONTEND_SERIALIZATION_TRAITS_H
