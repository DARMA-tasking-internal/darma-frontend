/*
//@HEADER
// ************************************************************************
//
//                      pair.h
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

#ifndef DARMAFRONTEND_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_PAIR_H
#define DARMAFRONTEND_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_PAIR_H

#include <darma/serialization/nonintrusive.h>
#include <darma/serialization/serialization_traits.h>

#include <darma/serialization/serializers/const.h>

#include <utility>

namespace darma_runtime {
namespace serialization {

//==============================================================================

template <typename T, typename U, typename Archive>
struct is_sizable_with_archive<std::pair<T, U>, Archive>
  : tinympl::and_<
      is_sizable_with_archive<T, Archive>,
      is_sizable_with_archive<U, Archive>
    >
{ };

template <typename T, typename U, typename Archive>
struct is_packable_with_archive<std::pair<T, U>, Archive>
  : tinympl::and_<
      is_packable_with_archive<T, Archive>,
      is_packable_with_archive<U, Archive>
    >
{ };

template <typename T, typename U, typename Archive>
struct is_unpackable_with_archive<std::pair<T, U>, Archive>
  : tinympl::and_<
    is_unpackable_with_archive<T, Archive>,
    is_unpackable_with_archive<U, Archive>
  >
{ };

template <typename T, typename U>
struct is_directly_serializable<std::pair<T, U>>
  : tinympl::and_<
      is_directly_serializable<T>,
      is_directly_serializable<U>
    >
{ };

//==============================================================================

// Only need to implement the non-directly-serializable version, since the
// directly serializable version will be handled directly
template <typename T, typename U>
struct Serializer_enabled_if<
  std::pair<T, U>,
  std::enable_if_t<not is_directly_serializable<std::pair<T, U>>::value>
>
{
  using pair_t = std::pair<T, U>;

  template <typename Archive>
  static void get_packed_size(pair_t const& obj, Archive& ar) {
    ar % obj.first % obj.second;
  }

  template <typename Archive>
  static void pack(pair_t const& obj, Archive& ar) {
    ar << obj.first << obj.second;
  }

  template <typename Archive>
  static void unpack(void* allocated, Archive& ar) {
    // The standard library probably doesn't guarantee that this works, but
    // it's faster and it works with every implementation I've ever heard of
    auto* obj_ptr = static_cast<pair_t*>(allocated);
    ar.template unpack_next_item_at<T>(&obj_ptr->first);
    ar.template unpack_next_item_at<U>(&obj_ptr->second);
  }
};

//==============================================================================

template <typename T, typename U>
struct Serializer<std::pair<T const, U>> : Serializer<std::pair<T, U>> { };

template <typename T, typename U>
struct Serializer<std::pair<T, U const>> : Serializer<std::pair<T, U>> { };

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_PAIR_H
