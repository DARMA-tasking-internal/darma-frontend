/*
//@HEADER
// ************************************************************************
//
//                       segmented_key.h
//                         darma
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

#ifndef DARMA_BYTES_CONVERT_H
#define DARMA_BYTES_CONVERT_H

#include <cstddef>
#include <stdalign.h>
#include <cstdint>
#include <limits>
#include <cassert>
#include <cstring>
#include <sstream>
#include <darma/impl/meta/detection.h>
#include <darma/impl/meta/is_streamable.h>
#include "util.h"

namespace darma_runtime {

namespace detail {

namespace _bytes_convert_impl {


} // end namespace _bytes_convert_impl

//template <typename T>
//void to_bytes(T&& val, void* dest, size_t const n_bytes, size_t const offset = 0) {
//  bytes_convert<std::remove_reference_t<T>>()(
//    std::forward<T>(val), dest, n_bytes, offset
//  );
//}

// <editor-fold desc="bytes_convert and its specializations">

template <typename T, typename Enable=void>
struct bytes_convert;

template <typename CharT, typename Traits, typename Allocator>
struct bytes_convert<std::basic_string<CharT, Traits, Allocator>> {
  static constexpr bool size_known_statically = false;
  typedef std::basic_string<CharT, Traits, Allocator> string_t;

  inline size_t
  get_size(string_t const& val) const {
    return val.size() * sizeof(CharT);
  }

  inline constexpr void
  operator()(string_t const &val, void *dest, const size_t n_bytes, const size_t offset) const {
    const size_t size = get_size(val);
    ::memcpy(dest, val.data() + offset, n_bytes);
  }

  inline string_t
  get_value(void* data, size_t size) const {
    return string_t((CharT*)data, size / sizeof(CharT));
  }
};

template <typename T, typename Enable>
struct bytes_convert {
  static constexpr bool size_known_statically = false;

  inline
  std::enable_if_t<
    meta::is_out_streamable<T, std::ostringstream>::value
  >
  get_size(T const& val) const {
    std::ostringstream osstr;
    osstr << val;
    return bytes_convert<std::string>().get_size(osstr.str());
  }

  inline
  std::enable_if_t<
    meta::is_out_streamable<T, std::ostringstream>::value
  >
  operator()(T const &val, void *dest, const size_t n_bytes, const size_t offset) const {
    std::ostringstream osstr;
    osstr << val;
    bytes_convert<std::string>()(osstr.str(), dest, n_bytes, offset);
  }

  inline
  std::enable_if_t<
    meta::is_in_streamable<T, std::istringstream>::value,
    T
  >
  get_value(void* data, size_t size) const {
    std::istringstream isstr(bytes_convert<std::string>().get_value(data, size));
    T rv;
    isstr >> rv;
    return rv;
  }
};

template <typename T>
struct bytes_convert<T, std::enable_if_t<std::is_fundamental<T>::value>> {
  static constexpr bool size_known_statically = true;
  static constexpr bool can_reinterpret_cast = true;
  static constexpr size_t size = sizeof(T);
  inline constexpr size_t get_size(T const&) const { return size; }
  inline constexpr void
  operator()(T const &val, void *dest, const size_t n_bytes, const size_t offset) const {
    ::memcpy(dest, (char*)(&val) + offset, n_bytes);
  }
};

template <typename T>
using has_char_traits_archetype = std::char_traits<T>;

template <typename T>
using has_char_traits = meta::is_detected<has_char_traits_archetype, T>;

template <typename T, size_t N>
struct bytes_convert<T[N], std::enable_if_t<
  // Char strings should be treated specially (i.e., ignoring the trailing '\0')
  std::is_fundamental<T>::value and not has_char_traits<std::remove_cv_t<T>>::value
>> {
  static constexpr bool size_known_statically = true;
  static constexpr bool can_reinterpret_cast = true;
  static constexpr size_t size = sizeof(T) * N;
  inline constexpr size_t get_size(T const val[N]) const { return size; }
  inline constexpr void
  operator()(T const val[N], void* dest, const size_t n_bytes, const size_t offset) const {
    ::memcpy(dest, val + offset, n_bytes);
  }
};

// e.g. const char* and other c-style strings with variable length.
template <typename CharT>
struct bytes_convert<CharT*,
 std::enable_if_t<has_char_traits<std::remove_cv_t<CharT>>::value>
> {
  static constexpr bool size_known_statically = false;
  typedef std::char_traits<std::remove_cv_t<CharT>> traits_t;
  inline size_t
  get_size(CharT* val) const {
    return traits_t::length(val);
  }
  inline constexpr void
  operator()(CharT* val, void* dest, const size_t n_bytes, const size_t offset) const {
    const size_t size = get_size(val);
    ::memcpy(dest, ((char*)val) + offset, n_bytes);
  }
  // Can't be converted back
};

template <typename CharT, size_t N>
struct bytes_convert<CharT[N],
  std::enable_if_t<has_char_traits<std::remove_cv_t<CharT>>::value>
> {
  static constexpr bool size_known_statically = false;
  typedef std::char_traits<std::remove_cv_t<CharT>> traits_t;
  inline size_t
  get_size(CharT const val[N]) const {
    return traits_t::length(val);
  }
  inline constexpr void
  operator()(CharT const val[N], void* dest, const size_t n_bytes, const size_t offset) const {
    const size_t size = get_size(val);
    ::memcpy(dest, ((char*)val) + offset, n_bytes);
  }
  // Can't be converted back
};

// </editor-fold>

namespace _impl {
template<typename T>
using convertible_to_bytes_archetype = decltype(bytes_convert<T>()(
  std::declval<T>(), std::declval<void *>(), size_t(0), size_t(0)
));
} // end namespace _impl
template <typename T>
using convertible_to_bytes = meta::is_detected<_impl::convertible_to_bytes_archetype, T>;

namespace _impl {
template<typename T>
using convertible_from_bytes_archetype = decltype(std::declval<bytes_convert<T>>().get_value(
  std::declval<void *>(), size_t(0)
));
} // end namespace _impl
template <typename T>
using convertible_from_bytes = meta::is_detected_convertible<T, _impl::convertible_from_bytes_archetype, T>;

namespace _impl {
template<typename T>
using can_reinterpret_cast_bytes_archetype =
  std::integral_constant<bool, bytes_convert<T>::can_reinterpret_cast>;
} // end namespace _impl
template <typename T>
using can_reinterpret_cast_bytes =
  meta::detected_or<std::false_type, _impl::can_reinterpret_cast_bytes_archetype, T>;

namespace _impl {
template<typename T>
using bytes_size_known_statically_archetype =
std::integral_constant<bool, bytes_convert<T>::size_known_statically>;
} // end namespace _impl
template <typename T>
using bytes_size_known_statically =
  meta::detected_or<std::false_type, _impl::bytes_size_known_statically_archetype, T>;

struct raw_bytes {
  public:
    explicit
    raw_bytes(size_t nbytes)
      : size(nbytes),
        data(new char[nbytes])
    { }
    std::unique_ptr<char[]> data = nullptr;
    size_t get_size() const { return size; }
    //~raw_bytes() { if(data) delete[] data; }
  private:
    size_t size;
};

template <>
struct bytes_convert<raw_bytes> {
  static constexpr bool size_known_statically = false;
  inline size_t
  get_size(raw_bytes const& bytes) const {
    return bytes.get_size();
  }
  inline void
  operator()(raw_bytes const& bytes, void* dest, const size_t n_bytes, const size_t offset) const {
    ::memcpy(dest, bytes.data.get() + offset, n_bytes);
  }
  inline raw_bytes
  get_value(void* data, size_t size) const {
    raw_bytes bytes(size);
    memcpy(bytes.data.get(), data, size);
    return bytes;
  }
};

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_BYTES_CONVERT_H
