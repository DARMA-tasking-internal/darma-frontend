/*
//@HEADER
// ************************************************************************
//
//                      tuple.h
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

#ifndef DARMAFRONTEND_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_TUPLE_H
#define DARMAFRONTEND_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_TUPLE_H

#include <darma/serialization/nonintrusive.h>
#include <darma/serialization/serialization_traits.h>

#include <tuple>

namespace darma {
namespace serialization {

//==============================================================================

template <typename... Ts, typename Archive>
struct is_sizable_with_archive<std::tuple<Ts...>, Archive>
  : tinympl::and_<
      is_sizable_with_archive<Ts, Archive>...
    >
{ };

template <typename... Ts, typename Archive>
struct is_packable_with_archive<std::tuple<Ts...>, Archive>
  : tinympl::and_<
      is_packable_with_archive<Ts, Archive>...
    >
{ };

template <typename... Ts, typename Archive>
struct is_unpackable_with_archive<std::tuple<Ts...>, Archive>
  : tinympl::and_<
      is_unpackable_with_archive<Ts, Archive>...
    >
{ };

template <typename... Ts>
struct is_directly_serializable<std::tuple<Ts...>>
  : tinympl::and_<
      is_directly_serializable<Ts>...
    >
{ };

//==============================================================================

// Only need to implement the non-directly-serializable version, since the
// directly serializable version will be handled directly
template <typename... Ts>
struct Serializer_enabled_if<
  std::tuple<Ts...>,
  std::enable_if_t<not is_directly_serializable<std::tuple<Ts...>>::value>
>
{
  using tuple_t = std::tuple<Ts...>;
  using idxs_t = std::index_sequence_for<Ts...>;
  using this_t = Serializer_enabled_if;

  //============================================================================

  template <typename Archive>
  inline static void _apply_bar_op_recursively(Archive& ar) { }

  template <typename Archive, typename Arg, typename... Args>
  inline static void _apply_bar_op_recursively(
    Archive& ar, Arg const& arg, Args const&... args
  ) {
    ar | arg;
    this_t::_apply_bar_op_recursively(ar, args...);
  };

  template <typename Archive, size_t... Idxs>
  static void _apply_bar_op_impl(
    tuple_t const& obj, Archive& ar, std::integer_sequence<size_t, Idxs...>
  ) {
    // Can't do simple fold emulation here because order matters
    this_t::_apply_bar_op_recursively(ar, std::get<Idxs>(obj)...);
  };

  template <typename Archive>
  static void compute_size(tuple_t const& obj, Archive& ar) {
    this_t::_apply_bar_op_impl(obj, ar, idxs_t{});
  }

  template <typename Archive>
  static void pack(tuple_t const& obj, Archive& ar) {
    this_t::_apply_bar_op_impl(obj, ar, idxs_t{});
  }

  //============================================================================

  template <typename Archive>
  inline static void _apply_unpack_recursively(Archive& ar) { }

  template <typename Archive, typename Arg, typename... Args>
  inline static void _apply_unpack_recursively(
    Archive& ar, Arg& arg, Args&... args
  ) {
    ar.template unpack_next_item_at<Arg>(&arg);
    this_t::_apply_unpack_recursively(ar, args...);
  };

  template <typename Archive, size_t... Idxs>
  static void _apply_unpack_impl(
    tuple_t& obj, Archive& ar, std::integer_sequence<size_t, Idxs...>
  ) {
    this_t::_apply_unpack_recursively(ar, std::get<Idxs>(obj)...);
  }

  template <typename Archive>
  static void unpack(void* allocated, Archive& ar) {
    // The standard library probably doesn't guarantee that this works, but
    // it's faster and it works with every implementation I've ever heard of
    auto* obj_ptr = static_cast<tuple_t*>(allocated);
    _apply_unpack_impl(*obj_ptr, ar, idxs_t{});
  }
};

} // end namespace serialization
} // end namespace darma

#endif //DARMAFRONTEND_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_TUPLE_H
