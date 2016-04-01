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

#ifndef DARMA_SEGMENTED_KEY_H
#define DARMA_SEGMENTED_KEY_H

#include <cstddef>
#include <stdalign.h>
#include <cstdint>
#include <memory>
#include <limits>
#include <cassert>
#include <cstring>
#include <sstream>
#include <darma/impl/meta/detection.h>

namespace darma_runtime {

namespace detail {

struct alignas(1) KeySegmentPieceMetadata {
  bool is_first : 1;
  bool is_last : 1;
  uint8_t piece_size : 6;
};

struct alignas(1) FirstKeySegment {
  static constexpr uint8_t payload_size = 54;

  bool _unused : 1;
  bool is_last : 1;
  uint8_t n_pieces : 6;
  /**
   *  data is a series of n_pieces { KeySegmentPieceMetadata, <payload> } pairs,
   *  where the size of <payload> is given by KeySegmentPieceMetadata.piece_size
   *  Leave room for the vtable pointer and the n_parts_cached_ members on the
   *  first cache line
   */
  char data[payload_size];
};

struct alignas(64) KeySegment {
  static constexpr uint8_t payload_size = 63;

  bool _unused : 1;
  bool is_last : 1;
  uint8_t n_pieces : 6;
  /**
   *  data is a series of n_pieces { KeySegmentPieceMetadata, <payload> } pairs,
   *  where the size of <payload> is given by KeySegmentPieceMetadata.piece_size
   */
  char data[payload_size];
};


struct MultiSegmentKeyBase {
  static constexpr uint8_t uninitialized = std::numeric_limits<uint8_t>::max();
  static constexpr uint8_t max_n_parts = std::numeric_limits<uint8_t>::max() - 1;

  virtual size_t get_part_size(uint8_t part) const =0;

  virtual void get_part_bytes(uint8_t part, char* dest) const =0;

  virtual ~MultiSegmentKeyBase() = default;
  uint8_t n_parts = uninitialized;
};

template <unsigned NExtraSegments>
class alignas(64) MultiSegmentKey
  : public MultiSegmentKeyBase
{
  public:
    template <typename Func>
    inline void iter_pieces(Func&& f) const {
      char* spot = first_segment_.data;
      bool part_started = false;
      uint8_t current_part = 0;
      for(uint8_t ipiece = 0; ipiece < first_segment_.n_pieces; ++ipiece) {
        register KeySegmentPieceMetadata ip_md = *(KeySegmentPieceMetadata*)spot;
        spot += sizeof(KeySegmentPieceMetadata);
        if(not f(ip_md, spot, current_part)) break;
        if(!part_started) {
          assert(ip_md.is_first);
          if(ip_md.is_last) ++current_part;
          else part_started = true;
        }
        else if(ip_md.is_last) {
          ++current_part;
          part_started = false;
        }
        spot += ip_md.piece_size;
      }
      for(unsigned iseg = 0; iseg < NExtraSegments; ++iseg) {
        for(uint8_t ipiece = 0; ipiece < segments_[iseg].n_pieces; ++ipiece) {
          register KeySegmentPieceMetadata ip_md = *(KeySegmentPieceMetadata *) spot;
          spot += sizeof(KeySegmentPieceMetadata);
          if(not f(ip_md, spot, current_part)) break;
          if (!part_started) {
            assert(ip_md.is_first);
            if (ip_md.is_last) ++current_part;
            else part_started = true;
          }
          else if (ip_md.is_last) {
            ++current_part;
            part_started = false;
          }
          spot += ip_md.piece_size;
        }
      }
    }

    size_t get_part_size(uint8_t part) const override {
      assert(part < n_parts);
      size_t part_size = 0;
      iter_pieces([&](KeySegmentPieceMetadata ip_md, char* spot, uint8_t current_part) {
        if(current_part == part) {
          part_size += ip_md.piece_size;
          return ip_md.is_last; // break if last
        }
        return true; // don't break, continue
      });
      return part_size;
    }

    void get_part_bytes(uint8_t part, char* dest) const override {
      char* buffer_spot = dest;
      iter_pieces([&](KeySegmentPieceMetadata ip_md, char* spot, uint8_t current_part) {
        if(current_part == part) {
          ::memcpy(buffer_spot, spot, ip_md.piece_size);
          buffer_spot += ip_md.piece_size;
          return ip_md.is_last; // break if last piece
        }
        return true; // don't break
      });
    }

  private:
    FirstKeySegment first_segment_;
    KeySegment segments_[NExtraSegments];
};

static_assert(sizeof(MultiSegmentKey<0>) == 64);
static_assert(sizeof(MultiSegmentKey<1>) == 128);
static_assert(sizeof(MultiSegmentKey<2>) == 192);
static_assert(sizeof(MultiSegmentKey<3>) == 256);

template <typename T, typename Enable=void>
struct bytes_convert {
  constexpr bool size_known_statically = false;
  inline size_t
  get_size(T const& val) const {
    std::ostringstream osstr;
    osstr << val;
    return bytes_convert<std::string>().get_size(osstr.str());
  }
  inline constexpr void
  operator()(T const& val, void* dest, const size_t offset, const size_t n_bytes) const {
    std::ostringstream osstr;
    osstr << val;
    bytes_convert<std::string>()(osstr.str(), dest, offset, n_bytes);
  }
  inline T
  get_value(void* data, size_t size) const {
    std::istringstream isstr(std::string((char*)data, size));
    T rv;
    isstr >> rv;
    return rv;
  }
};

template <typename T>
struct bytes_convert<T, std::enable_if_t<std::is_fundamental<T>::value>> {
  constexpr bool size_known_statically = true;
  constexpr bool can_reinterpret_cast = true;
  constexpr size_t size = sizeof(T);
  inline constexpr void
  operator()(T const& val, void* dest, const size_t offset, const size_t n_bytes) const {
    ::memcpy(dest, &val + n_bytes, n_bytes);
  }
};

template <typename T, size_t N>
struct bytes_convert<T[N], std::enable_if_t<std::is_fundamental<T>::value>> {
  constexpr bool size_known_statically = true;
  constexpr bool can_reinterpret_cast = true;
  constexpr size_t size = sizeof(T) * N;
  inline constexpr void
  operator()(T const val[N], void* dest, const size_t offset, const size_t n_bytes) const {
    ::memcpy(dest, val + offset, n_bytes);
  }
};

template <typename CharT, typename Traits, typename Allocator>
struct bytes_convert<std::basic_string<CharT, Traits, Allocator>> {
  constexpr bool size_known_statically = false;
  typedef std::basic_string<CharT, Traits, Allocator> string_t;
  inline size_t
  get_size(string_t const& val) const {
    return val.size() * sizeof(CharT);
  }
  inline constexpr void
  operator()(string_t const& val, void* dest, const size_t offset, const size_t n_bytes) const {
    const size_t size = get_size(val);
    ::memcpy(dest, val.data() + offset, n_bytes);
  }
  inline string_t
  get_value(void* data, size_t size) const {
    return string_t((CharT*)data, size / sizeof(CharT));
  }
};

template <typename T>
using has_char_traits_archetype = std::char_traits<T>;

template <typename T>
using has_char_traits = meta::is_detected<has_char_traits_archetype, T>;

// e.g. const char* and other c-style strings with variable length.
template <typename CharT>
struct bytes_convert<CharT*,
  std::enable_if_t<has_char_traits<std::remove_cv_t<CharT>>::value>
> {
  constexpr bool size_known_statically = false;
  typedef std::char_traits<std::remove_cv_t<CharT>> traits_t;
  inline size_t
  get_size(CharT* val) const {
    return traits_t::length(val);
  }
  inline constexpr void
  operator()(CharT* val, void* dest, const size_t offset, const size_t n_bytes) const {
    const size_t size = get_size(val);
    ::memcpy(dest, ((char*)val) + offset, n_bytes);
  }
};



} // end namespace detail

namespace defaults {

class SegmentedKey {

    // TODO finish this!!!

  protected:

    std::shared_ptr<darma_runtime::detail::MultiSegmentKeyBase> key_impl_;

}; // end class SegmentedKey

} // end namespace defaults

} // end namespace darma_runtime

#endif //DARMA_SEGMENTED_KEY_H
