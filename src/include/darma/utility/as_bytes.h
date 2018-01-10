/*
//@HEADER
// ************************************************************************
//
//                      as_bytes.h
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

#ifndef DARMAFRONTEND_AS_BYTES_H
#define DARMAFRONTEND_AS_BYTES_H

#include <darma/utility/largest_aligned.h>

#include <cstdint>

namespace darma_runtime {
namespace detail {

//   http://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
template <class T>
inline void
hash_combine(std::size_t& seed, const T& v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

template <typename T>
inline size_t
hash_as_bytes(T const& val) {
  typedef typename utility::largest_aligned_int<T>::type largest_int_t;
  constexpr size_t n_parts = sizeof(T) / sizeof(largest_int_t);
  largest_int_t* spot = (largest_int_t*)&val;
  size_t rv = 0;
  for(int i = 0; i < n_parts; ++i, ++spot) {
    hash_combine(rv, *spot);
  }
  return rv;
}

template <typename T, typename U>
inline bool
equal_as_bytes(T const& a, U const& b) {
  if(sizeof(T) != sizeof(U)) return false;
  typedef typename utility::largest_aligned_int<T>::type largest_int_t;
  constexpr size_t n_parts = sizeof(T) / sizeof(largest_int_t);
  largest_int_t* a_spot = (largest_int_t*)&a;
  largest_int_t* b_spot = (largest_int_t*)&b;
  for(int i = 0; i < n_parts; ++i, ++a_spot, ++b_spot) {
    if(*a_spot != *b_spot) return false;
  }
  return true;
}

inline bool
less_as_bytes(const char* a, size_t a_size, const char* b, size_t b_size) {
  if(a_size != b_size) return a_size < b_size;
  if(a_size % sizeof(uint64_t) == 0) {
    const uint64_t* a_64 = reinterpret_cast<uint64_t const*>(a);
    const uint64_t* b_64 = reinterpret_cast<uint64_t const*>(b);
    const size_t n_iter = a_size / sizeof(uint64_t);
    for(int i = 0; i < n_iter; ++i, ++a_64, ++b_64) {
      if(*a_64 != *b_64) return *a_64 < *b_64;
    }
  }
  else {
    for (int i = 0; i < a_size; ++i, ++a, ++b) {
      if (*a != *b) return *a < *b;
    }
  }
  return false;
}

inline bool
equal_as_bytes(const char* a, size_t a_size, const char* b, size_t b_size) {
  if(a_size != b_size) return false;
  if(a_size % sizeof(uint64_t) == 0) {
    const uint64_t* a_64 = reinterpret_cast<uint64_t const*>(a);
    const uint64_t* b_64 = reinterpret_cast<uint64_t const*>(b);
    const size_t n_iter = a_size / sizeof(uint64_t);
    for(int i = 0; i < n_iter; ++i, ++a_64, ++b_64) {
      if(*a_64 != *b_64) return false;
    }
  }
  else {
    for (int i = 0; i < a_size; ++i, ++a, ++b) {
      if (*a != *b) return false;
    }
  }
  return true;
}

inline size_t
hash_as_bytes(const char* a, size_t a_size) {
  if(a_size % sizeof(uint64_t) == 0) {
    size_t rv = 0;
    const uint64_t* a_64 = reinterpret_cast<uint64_t const*>(a);
    const size_t n_iter = a_size / sizeof(uint64_t);
    for(int i = 0; i < n_iter; ++i, ++a_64) {
      hash_combine(rv, *a_64);
    }
    return rv;
  }
  else {
    size_t rv = 0;
    for(int i = 0; i < a_size; ++i, ++a) {
      hash_combine(rv, *a);
    }
    return rv;
  }
}

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_AS_BYTES_H
