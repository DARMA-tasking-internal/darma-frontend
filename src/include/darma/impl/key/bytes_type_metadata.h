/*
//@HEADER
// ************************************************************************
//
//                      bytes_type_metadata.h
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

#ifndef DARMA_IMPL_KEY_BYTES_TYPE_METADATA_H
#define DARMA_IMPL_KEY_BYTES_TYPE_METADATA_H

#include <cassert>
#include <cstdint>

#include <darma/impl/serialization/nonintrusive.h>


namespace darma_runtime {

namespace detail {

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="bytes_type_metadata and castable sibling structs">

struct alignas(1) bytes_type_metadata {
  bool is_string_like : 1; // if true, cast to string_like_type_metadata
  bool is_int_like_type : 1; // if true, cast to int_like_type_metadata
  bool is_floating_point_like_type : 1; // if true; cast to float_like_type_metadata
  bool is_user_defined_type : 1; // otherwise, use type index
  char _unused : 4;
};
static_assert(sizeof(bytes_type_metadata) == 1,
  "bytes_type_metadata must be compiled as size 1");

// TODO wide strings?
struct alignas(1) string_like_type_metadata {
  bool _always_true : 1; // the bit indicating this is string-like
  uint8_t size : 7; // the number of characters in the string-like construct
  static constexpr uint8_t max_string_size = 127;
};
static_assert(sizeof(string_like_type_metadata) == 1,
  "string_like_type_metadata must be compiled as size 1");

struct alignas(1) int_like_type_metadata {
  bool _always_false : 1; // the bit indicating this is not string-like
  bool _always_true : 1; // the bit indicating this is int-like
  bool is_enumerated : 1; // whether or not the type is an enumerated type.  if true,
  // stop here and cast to enum_like_type_metadata
  bool is_negative : 1; // whether or not the value is negative.  0 is never negative
  uint8_t int_size_exponent : 4; // the exponent N of the number of bytes, 2**N
  // occupied by the int when its absolute value
  // is packed into an integer number of bytes giving
  // the smallest N possible without overflow.
  // For instance, if the value is -243, N = 0
  // If the value x is 83,246, N = 2 (x > 2**16, so
  // this requires a 32-bit integer)
  // With this format, the maximum integer that can
  // be used in a key is 2**(8*(2**4-1))-1, or 1.15E77,
  // which should be sufficient for most purposes.
  static constexpr uint8_t max_int_size_exponent = 15;
};
static_assert(sizeof(int_like_type_metadata) == 1,
  "int_like_type_metadata must be compiled as size 1");

struct alignas(1) enum_like_type_metadata {
  bool _always_false : 1; // the bit indicating this is not string-like
  bool _always_true_1 : 1; // the bit indicating this is int-like
  bool _always_true_2 : 1; // the bit indicating this is an enum
  uint8_t enum_category : 5; // the index of the enum *type* being used here
  // if the index is 31 or greater, this value will
  // be 31 (== use_extension_byte_for_category) and the
  // next byte will contain the category
  static constexpr uint8_t use_extension_byte_for_category = 31;
};
static_assert(sizeof(enum_like_type_metadata) == 1,
  "enum_like_type_metadata must be compiled as size 1");

struct alignas(1) float_like_type_metadata {
  bool _always_false_1 : 1; // the bit indicating this is not string-like
  bool _always_false_2 : 1; // the bit indicating this is int-like
  bool _always_true : 1; // the bit indicating this is float-like
  char _unused : 5;
};
static_assert(sizeof(float_like_type_metadata) == 1,
  "float_like_type_metadata must be compiled as size 1");

struct alignas(1) user_defined_type_metadata {
  bool _always_false_1 : 1; // the bit indicating this is not string-like
  bool _always_false_2 : 1; // the bit indicating this is int-like
  bool _always_false_3 : 1; // the bit indicating this is float-like
  bool _always_true : 1; // the bit indicating this describes a user-defined
  // type category.
  uint8_t user_category : 4; // the index in the registry of the user-defined type
  // category.  If the index is 15 or greater, the value
  // will be 15 (==use_extension_byte_for_category) and the
  // next byte will contain the category
  static constexpr uint8_t use_extension_byte_for_category = 15;
};
static_assert(sizeof(float_like_type_metadata) == 1,
  "user_defined_type_metadata must be compiled as size 1");

template <uint8_t NExtraBytes>
struct alignas(1) category_extension_byte {
  uint8_t n_extra_bytes : 2; // allows up to 4 bytes to express categories
  uint32_t value : (6 + NExtraBytes*8);
};

bool has_category_extension_byte(bytes_type_metadata* md) {
  if(not md->is_string_like) {
    if(md->is_int_like_type) {
      auto* md_int = reinterpret_cast<int_like_type_metadata*>(md);
      if(md_int->is_enumerated) {
        auto* md_enum = reinterpret_cast<enum_like_type_metadata*>(md_int);
        return md_enum->enum_category
          == enum_like_type_metadata::use_extension_byte_for_category;
      }
    }
    else if(not md->is_floating_point_like_type) {
      if(md->is_user_defined_type) {
        auto* md_user = reinterpret_cast<user_defined_type_metadata*>(md);
        return md_user->user_category
          == user_defined_type_metadata::use_extension_byte_for_category;
      }
    }
  }
  return false;
}

uint8_t
category_extension_bytes_size(void* md) {
  assert(has_category_extension_byte((bytes_type_metadata*)md));
  auto* extra_0 = reinterpret_cast<category_extension_byte<0>*>(
    (char*)md + sizeof(bytes_type_metadata)
  );
  return extra_0->n_extra_bytes + (uint8_t)1;
}

uint32_t
category_extension_bytes_value(void* md) {
  assert(has_category_extension_byte((bytes_type_metadata*)md));
  uint8_t n_bytes = category_extension_bytes_size(md);
  switch(n_bytes) {
    case 1: {
      auto* extra_0 = reinterpret_cast<category_extension_byte<0>*>(
        (char*)md + sizeof(bytes_type_metadata)
      );
      return extra_0->value;
    }
    case 2: {
      auto* extra_1 = reinterpret_cast<category_extension_byte<1>*>(
        (char*)md + sizeof(bytes_type_metadata)
      );
      return extra_1->value;
    }
    case 3: {
      auto* extra_2 = reinterpret_cast<category_extension_byte<2>*>(
        (char*)md + sizeof(bytes_type_metadata)
      );
      return extra_2->value;
    }
    case 4: {
      auto* extra_3 = reinterpret_cast<category_extension_byte<3>*>(
        (char*)md + sizeof(bytes_type_metadata)
      );
      return extra_3->value;
    }
    default:
      assert(false); // something went horribly wrong!
  }
  return 0; // unreachable;
}

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

} // end namespace detail

namespace serialization {

template <>
struct serialize_as_pod<darma_runtime::detail::bytes_type_metadata> : std::true_type { };

} // end namespace serialization

} // end namespace darma_runtime

#endif //DARMA_IMPL_KEY_BYTES_TYPE_METADATA_H
