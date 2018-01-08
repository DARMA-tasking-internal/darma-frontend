/*
//@HEADER
// ************************************************************************
//
//                      vector.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_SERIALIZATION_STANDARD_LIBRARY_VECTOR_H
#define DARMAFRONTEND_SERIALIZATION_STANDARD_LIBRARY_VECTOR_H

#include <darma/serialization/nonintrusive.h>
#include <darma/serialization/serialization_traits.h>

#include <vector>

namespace darma_runtime {
namespace serialization {

// TODO vectors with non-standard/non-empty allocators ?

//==============================================================================

template <typename T, typename Archive>
struct is_sizable_with_archive<std::vector<T>, Archive>
  : is_sizable_with_archive<T, Archive>
{ };

template <typename T, typename Archive>
struct is_packable_with_archive<std::vector<T>, Archive>
  : is_packable_with_archive<T, Archive>
{ };

template <typename T, typename Archive>
struct is_unpackable_with_archive<std::vector<T>, Archive>
  : is_unpackable_with_archive<T, Archive>
{ };

//==============================================================================

// Basic case: T not directly serializable. Can't really optimize further
// than just unpacking each item one at a time
template <typename T>
struct Serializer_enabled_if<
  std::vector<T>, std::enable_if_t<not is_directly_serializable<T>::value>
>
{
  using vector_t = std::vector<T>;

  template <typename SizingArchive>
  static void compute_size(vector_t const& obj, SizingArchive& ar) {
    ar | obj.size();
    for(auto&& val : obj) {
      ar | val;
    }
  }

  template <typename Archive>
  static void pack(vector_t const& obj, Archive& ar) {
    ar | obj.size();
    for(auto&& val : obj) {
      ar | val;
    }
  }

  template <typename Archive>
  static void unpack(void* allocated, Archive& ar) {
    auto size = ar.template unpack_next_item_as<typename vector_t::size_type>();
    auto& obj = *(new (allocated) vector_t(
      ar.template get_allocator_as<typename vector_t::allocator_type>())
    );
    obj.reserve(size);
    for(int64_t i = 0; i < size; ++i) {
      obj.emplace_back(ar.template unpack_next_item_as<T>());
    }
  }
};

//==============================================================================

// Directly serializable T specialization of std::vector<T>.  (This is an
// optimization for performance purposes only)
template <typename T>
struct Serializer_enabled_if<
  std::vector<T>, std::enable_if_t<is_directly_serializable<T>::value>
>
{
  using vector_t = std::vector<T>;

  template <typename Archive>
  static void compute_size(vector_t const& obj, Archive& ar) {
    ar | obj.size();
    ar.add_to_size_raw(sizeof(T) * obj.size());
  }

  template <typename Archive>
  static void pack(vector_t const& obj, Archive& ar) {
    ar | obj.size();
    ar.pack_data_raw(obj.data(), obj.data() + obj.size());
  }

  template <typename Archive>
  static void unpack(void* allocated, Archive& ar) {
    auto size = ar.template unpack_next_item_as<typename vector_t::size_type>();
    auto& obj = *(new (allocated) vector_t(
      size, ar.template get_allocator_as<typename vector_t::allocator_type>()
    ));
    ar.template unpack_data_raw<T const>(obj.data(), size);
  }
};

//==============================================================================

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_SERIALIZATION_STANDARD_LIBRARY_VECTOR_H
