/*
//@HEADER
// ************************************************************************
//
//                      list.h
//                         DARMA
//              Copyright (C) 2018 Sandia Corporation
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

#ifndef DARMAFRONTEND_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_LIST_H
#define DARMAFRONTEND_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_LIST_H

#include <darma/serialization/nonintrusive.h>
#include <darma/serialization/serialization_traits.h>

#include <list>

// TODO serialization of non-standard allocators

namespace darma {
namespace serialization {

template <typename T, typename Archive>
struct is_sizable_with_archive<std::list<T>, Archive>
  : is_sizable_with_archive<T, Archive>
{ };

template <typename T, typename Archive>
struct is_packable_with_archive<std::list<T>, Archive>
  : is_packable_with_archive<T, Archive>
{ };

template <typename T, typename Archive>
struct is_unpackable_with_archive<std::list<T>, Archive>
  : is_unpackable_with_archive<T, Archive>
{ };

template <typename T>
struct Serializer<std::list<T>> {

  using list_t = std::list<T>;

  template <typename SizingArchive>
  static void compute_size(list_t const& obj, SizingArchive& ar) {
    ar | obj.size();
    for(auto&& val : obj) {
      ar | val;
    }
  }

  template <typename Archive>
  static void pack(list_t const& obj, Archive& ar) {
    ar | obj.size();
    for(auto&& val : obj) {
      ar | val;
    }
  }

  template <typename Archive>
  static void unpack(void* allocated, Archive& ar) {
    auto size = ar.template unpack_next_item_as<typename list_t::size_type>();
    auto& obj = *(new (allocated) list_t(
      ar.template get_allocator_as<typename list_t::allocator_type>())
    );
    for(int64_t i = 0; i < size; ++i) {
      obj.emplace_back(ar.template unpack_next_item_as<T>());
    }
  }
};

} // end namespace serialization
} // end namespace darma

#endif //DARMAFRONTEND_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_LIST_H
