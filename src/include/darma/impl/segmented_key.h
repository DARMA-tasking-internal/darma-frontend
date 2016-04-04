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

#define DEBUG_SEGMENTED_KEY_BYTES 0

#include <cstddef>
#include <stdalign.h>
#include <cstdint>
#include <memory>
#include <limits>
#include <cassert>
#include <cstring>
#include <sstream>
#include <type_traits>

#include <tinympl/plus.hpp>
#include <tinympl/variadic/all_of.hpp>
#include <tinympl/vector.hpp>

#include <darma/impl/meta/detection.h>
#include <darma/impl/key_concept.h>
#include <darma/impl/bytes_convert.h>
#include <darma/impl/meta/tuple_for_each.h>
#include "util.h"


namespace darma_runtime {

namespace detail {


namespace m = tinympl;
namespace mv = tinympl::variadic;

using std::uint8_t;

class SegmentedKey;

namespace _segmented_key_impl {

template <typename T, typename Enable=void>
struct as_impl {
  T operator()(void* data, size_t n_bytes) const {
    return bytes_convert<T>().get_value(data, n_bytes);
  }
};

template <typename T>
struct as_impl<T,
  std::enable_if_t<can_reinterpret_cast_bytes<T>::value>
>
{
  T operator()(void* data, size_t) const {
    return *reinterpret_cast<T*>(data);
  }
};

}


class
SegmentedKeyPartBase {
  public:

    template <typename T>
    T as() const {
#if DEBUG_SEGMENTED_KEY_BYTES
      char* debug_spot = (char*)data;
      for(int i = 0; i < n_bytes; ++i) {
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<0));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<1));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<2));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<3));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<4));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<5));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<6));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<7));
        std::cout << " ";
        debug_spot++;
      }
#endif
      return _segmented_key_impl::as_impl<T>()(data, n_bytes);
    }

  private:
    void* data;
    size_t n_bytes;

    friend class SegmentedKey;

    explicit
    SegmentedKeyPartBase(size_t n_bytes_in)
      : data(::operator new(n_bytes_in)),
        n_bytes(n_bytes_in)
    { }

};

// <editor-fold desc="MultiSegmentKey and corresponding segment structures">

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

  virtual size_t hash() const = 0;

  virtual bool equal(MultiSegmentKeyBase const& other) const = 0;

  virtual ~MultiSegmentKeyBase() = default;
  uint8_t n_parts = uninitialized;
};

template <unsigned NExtraSegments>
class alignas(64) MultiSegmentKey
  : public MultiSegmentKeyBase
{
  public:

    template <typename... Args>
    MultiSegmentKey(
      variadic_constructor_arg_t,
      Args&&... args
    ) {
      n_parts = sizeof...(args);
      memset(&first_segment_, 0, sizeof(FirstKeySegment));
      if(NExtraSegments) memset(segments_, 0, sizeof(KeySegment)*NExtraSegments);

      uint8_t remain_current_seg = FirstKeySegment::payload_size;
      unsigned next_extra_segment = 0;
      char* data_spot = first_segment_.data;
      first_segment_.is_last = true; // will get changed if not true
      uint8_t current_n_pieces = 0;
      meta::tuple_for_each_zipped(
        std::forward_as_tuple(bytes_convert<std::remove_reference_t<Args>>()...),
        std::forward_as_tuple(std::forward<Args>(args)...),
        [&](auto&& bytes_converter, auto&& val) {

          size_t data_size_remain = bytes_converter.get_size(std::forward<decltype(val)>(val));

#if DEBUG_SEGMENTED_KEY_BYTES
          std::cout << "val = " << val << std::endl;
          std::cout << "  data_size_remain = " << (int)data_size_remain << std::endl;
          std::cout << "  remain_current_seg = " << (int)remain_current_seg << std::endl;
#endif

          if(remain_current_seg == 0) {
            if(next_extra_segment == 0) {
              first_segment_.n_pieces = current_n_pieces;
              first_segment_.is_last = false;
            }
            else {
              segments_[next_extra_segment-1].n_pieces = current_n_pieces;
              segments_[next_extra_segment-1].is_last = false;
            }
            // If the last part ended at the end of a piece, we need to add a new piece
            data_spot = segments_[next_extra_segment].data;
            segments_[next_extra_segment].is_last = true; // we'll change this if it's not true
            ++next_extra_segment;
            current_n_pieces = 0;
            remain_current_seg = KeySegment::payload_size;
          }

          // make room for the metadata
          remain_current_seg -= sizeof(KeySegmentPieceMetadata);
          // and put the metadata in place
          KeySegmentPieceMetadata* current_piece_md = reinterpret_cast<KeySegmentPieceMetadata*>(data_spot);
          current_piece_md->is_first = true;
          ++current_n_pieces;
          data_spot += sizeof(KeySegmentPieceMetadata);

          // potentially multi-piece part spanning more than one segment
          size_t data_offset = 0;
          while(data_size_remain) {
            if(data_size_remain <= remain_current_seg) {
              // Last piece
              // update the remaining space in the current segment
              remain_current_seg -= data_size_remain;
              // copy the actual bytes into place
              bytes_converter(
                std::forward<decltype(val)>(val), data_spot,
                data_size_remain, data_offset
              );
              // advance the spot
              data_spot += data_size_remain;
              // reset the offset
              data_offset = 0;
              // Mark this piece as the last in the part
              current_piece_md->is_last = true;
              current_piece_md->piece_size = (uint8_t)data_size_remain;
              // End the loop
              data_size_remain = 0;
#if DEBUG_SEGMENTED_KEY_BYTES
              char* debug_spot = (char*)first_segment_.data;
              for(int i = 0; debug_spot < data_spot; ++i) {
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<0));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<1));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<2));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<3));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<4));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<5));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<6));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<7));
                std::cout << " ";
                debug_spot++;
              }
              std::cout << std::endl;
              debug_spot = (char*)&val;
              for(int i = 0; i < sizeof(val); ++i) {
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<0));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<1));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<2));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<3));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<4));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<5));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<6));
                std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<7));
                std::cout << " ";
                debug_spot++;
              }
              std::cout << std::endl;
#endif
            }
            else {
              // Middle piece, subtract space from data_size_remain
              data_size_remain -= remain_current_seg;
              // copy the actual bytes into place
              bytes_converter(
                std::forward<decltype(val)>(val), data_spot,
                remain_current_seg, data_offset
              );
              // update the offset
              data_offset += remain_current_seg;
              // update piece metadata
              current_piece_md->is_last = false;
              current_piece_md->piece_size = remain_current_seg;
              // Start another piece in the next segment
              // close out this segment
              segments_[next_extra_segment-1].n_pieces = current_n_pieces;
              segments_[next_extra_segment-1].is_last = false;

              data_spot = segments_[next_extra_segment].data;
              segments_[next_extra_segment].is_last = true; // we'll change this if it's not true
              current_n_pieces = 1; // (the one we're about to create)
              // Set up the metadata for the next piece
              remain_current_seg = KeySegment::payload_size;
              remain_current_seg -= sizeof(KeySegmentPieceMetadata);
              current_piece_md = reinterpret_cast<KeySegmentPieceMetadata*>(data_spot);
              current_piece_md->is_first = false; // it's a continuation piece
              current_piece_md->is_last = data_size_remain <= remain_current_seg;
              // advance data spot past the piece header
              data_spot += sizeof(KeySegmentPieceMetadata);
              ++next_extra_segment;
            }
          } // end while data_size_remain

          if(remain_current_seg == 0) {
            // in case it fits exactly, we need to fix up current_n_pieces
            if(next_extra_segment == 0) first_segment_.n_pieces = current_n_pieces;
            else segments_[next_extra_segment-1].n_pieces = current_n_pieces;
          }

        }
      );

      // fix up the final n_pieces and is_last
      if(next_extra_segment == 0) {
        first_segment_.n_pieces = current_n_pieces;
        first_segment_.is_last = true;
      }
      else {
        segments_[next_extra_segment-1].n_pieces = current_n_pieces;
        segments_[next_extra_segment-1].is_last = true;
      }

#if DEBUG_SEGMENTED_KEY_BYTES
      char* debug_spot = (char*)this;
      for(int i = 0; i < sizeof(*this); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)*debug_spot;
        debug_spot++;
      }
      std::cout << std::endl;
      debug_spot = (char*)this;
      for(int i = 0; i < sizeof(*this); ++i) {
        std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<0));
        std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<1));
        std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<2));
        std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<3));
        std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<4));
        std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<5));
        std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<6));
        std::cout << std::setw(1) << (bool)(((char)*debug_spot)&(1<<7));
        std::cout << " ";
        debug_spot++;
      }
      std::cout << std::endl;
      debug_spot = (char*)first_segment_.data;
      for(int i = 0; i < FirstKeySegment::payload_size; ++i) {
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<0));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<1));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<2));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<3));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<4));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<5));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<6));
        std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<7));
        std::cout << " ";
        debug_spot++;
      }
      std::cout << std::endl;
#endif


    }

  private:

    template <typename Func>
    inline void iter_pieces(Func&& f) const {
      const char* spot = first_segment_.data;
      bool part_started = false;
      uint8_t current_part = 0;
      for(uint8_t ipiece = 0; ipiece < first_segment_.n_pieces; ++ipiece) {
        KeySegmentPieceMetadata ip_md = *(KeySegmentPieceMetadata*)spot;
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
          KeySegmentPieceMetadata ip_md = *(KeySegmentPieceMetadata *) spot;
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

  public:

    size_t hash() const override {
      size_t rv = 0;
      return hash_as_bytes(*this);
      //iter_pieces([&](KeySegmentPieceMetadata ip_md, char* spot, uint8_t current_part) {
      //  for(int i = 0; i < ip_md.piece_size; ++i) {
      //    hash_combine(rv, std::hash<char>()(*spot));
      //    spot++;
      //  }
      //});
      //return rv;
    }

    bool equal(MultiSegmentKeyBase const& other) const override {
      MultiSegmentKey const* other_cast = dynamic_cast<MultiSegmentKey const*>(&other);
      if(other_cast == nullptr) return false;
      else return equal_as_bytes(*this, *other_cast);
      //size_t rv = 0;
      //iter_pieces([&](KeySegmentPieceMetadata ip_md, char* spot, uint8_t current_part) {
      //  for(int i = 0; i < ip_md.piece_size; ++i) {
      //    hash_combine(rv, std::hash<char>()(*spot));
      //    spot++;
      //  }
      //});
      //return rv;
    }

    size_t get_part_size(uint8_t part) const override {
      assert(part < n_parts);
      size_t part_size = 0;
      iter_pieces([&](KeySegmentPieceMetadata const ip_md, const char* spot, uint8_t current_part) {
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
      iter_pieces([&](KeySegmentPieceMetadata const ip_md, const char* spot, uint8_t current_part) {
        if(current_part == part) {
          ::memcpy(buffer_spot, spot, ip_md.piece_size);
          buffer_spot += ip_md.piece_size;
          return ip_md.is_last; // break if last piece
        }
        return true; // don't break
      });
      //debug_spot = (char*)first_segment_.data;
      //for(int i = 0; i < FirstKeySegment::payload_size; ++i) {
      //  std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<0));
      //  std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<1));
      //  std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<2));
      //  std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<3));
      //  std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<4));
      //  std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<5));
      //  std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<6));
      //  std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<7));
      //  std::cout << " ";
      //  debug_spot++;
      //}
      //std::cout << std::endl;
    }

  private:
    FirstKeySegment first_segment_;
    KeySegment segments_[NExtraSegments];
};

static_assert(sizeof(MultiSegmentKey<0>) == 64,
  "SegmentedKey with one segment must compile to a size of exactly 1 cache line");
static_assert(sizeof(MultiSegmentKey<1>) == 128,
  "SegmentedKey with two segments must compile to a size of exactly 2 cache line");
static_assert(sizeof(MultiSegmentKey<2>) == 192,
  "SegmentedKey with three segments must compile to a size of exactly 3 cache lines");
static_assert(sizeof(MultiSegmentKey<3>) == 256,
  "SegmentedKey with four segments must compile to a size of exactly 4 cache lines");

// </editor-fold>

template <unsigned N=0>
struct create_n_segment_key {
  template <typename... Args>
  inline std::shared_ptr<MultiSegmentKeyBase>
  operator()(unsigned n_segments, Args&&... args) const {
    if(n_segments == N) {
      std::shared_ptr<MultiSegmentKeyBase> rv = std::make_shared<MultiSegmentKey<N>>(
        variadic_constructor_arg,
        std::forward<Args>(args)...
      );
      return rv;
    }
    else {
      return create_n_segment_key<N+1>()(n_segments, std::forward<Args>(args)...);
    }
  }
};


template <
  bool all_sizes_known_statically /* = false */,
  typename... Args
>
struct create_multi_segment_key_impl {
  inline unsigned
  get_n_extra_segments(Args&&... args) const {
    size_t remain_current = FirstKeySegment::payload_size;
    unsigned n_extra_segments = 0;
    meta::tuple_for_each_zipped(
      std::forward_as_tuple(bytes_convert<std::remove_reference_t<Args>>()...),
      std::forward_as_tuple(std::forward<Args>(args)...),
      [&](auto&& bytes_converter, auto&& val) {

        size_t data_size_remain = bytes_converter.get_size(std::forward<decltype(val)>(val));
        if(remain_current == 0) {
          // If the last part ended at the end of a piece, we need to add a new piece
          ++n_extra_segments;
          remain_current = KeySegment::payload_size;
        }
        // make room for the metadata
        remain_current -= sizeof(KeySegmentPieceMetadata);
        // potentially multi-piece part spanning more than one segment
        while(data_size_remain) {
          if(data_size_remain <= remain_current) {
            // Last piece
            remain_current -= data_size_remain;
            data_size_remain = 0;
          }
          else {
            // Middle piece, subtract space from data_size_remain
            data_size_remain -= remain_current;
            // Start another piece
            ++n_extra_segments;
            remain_current = KeySegment::payload_size;
            remain_current -= sizeof(KeySegmentPieceMetadata);
          }
        } // end while data_size_remain

      }
    );
    return n_extra_segments;
  }
  inline std::shared_ptr<MultiSegmentKeyBase>
  operator()(Args&&... args) const {
    return create_n_segment_key<>()(get_n_extra_segments(
      std::forward<Args>(args)...
    ), std::forward<Args>(args)...);
  }

};

//template <typename... Args>
//struct create_multi_segment_key_impl<true, Args...> {
//  typedef m::vector<Args...> args_vector_t;
//  typedef m::vector<std::remove_reference_t<Args>...> noref_args_vector_t;
//  typedef m::vector<bytes_convert<std::remove_reference_t<Args>>...> bytes_convert_vector_t;
//
//  //static constexpr size_t total_static_size = m::plus<
//  //  std::integral_constant<size_t, bytes_convert<std::remove_reference_t<Args>>::size>...
//  //>::value + sizeof(KeySegmentPieceMetadata);
//};

template <typename... Args>
std::shared_ptr<MultiSegmentKeyBase>
create_multi_segment_key(Args&&... args) {
  static constexpr bool all_sizes_known_statically = false && // for now, just treat all as variable size
    mv::all_of<bytes_size_known_statically, std::remove_reference_t<Args>...>::type::value;
  return create_multi_segment_key_impl<all_sizes_known_statically, Args...>()(
    std::forward<Args>(args)...
  );
}

struct segmented_key_hasher;
struct segmented_key_equal;


class SegmentedKey {

  public:

    static constexpr unsigned max_extra_segments = 8;
    static constexpr unsigned max_num_parts = std::numeric_limits<uint8_t>::max() - 1;

    template <typename... Args>
    SegmentedKey(
      variadic_constructor_arg_t const,
      Args&&... args
    ) : key_impl_(create_multi_segment_key(std::forward<Args>(args)...))
    { }

    template <uint8_t N>
    SegmentedKeyPartBase
    component() const {
      size_t part_size = key_impl_->get_part_size(N);
      SegmentedKeyPartBase rv(part_size);
      key_impl_->get_part_bytes(N, (char*)rv.data);
      return rv;
    }

  protected:

    friend struct segmented_key_hasher;
    friend struct segmented_key_equal;
    std::shared_ptr<MultiSegmentKeyBase> key_impl_;

}; // end class SegmentedKey

struct segmented_key_hasher {
  inline
  size_t operator()(SegmentedKey const& k) const {
    return k.key_impl_->hash();
  }
};

struct segmented_key_equal {
  inline
  bool operator()(SegmentedKey const& k, SegmentedKey const& k2) const {
    return k.key_impl_->equal(*(k2.key_impl_.get()));
  }
};

// Specialization to stop recursion
template <>
struct create_n_segment_key<SegmentedKey::max_extra_segments> {
  template <typename... Args>
  inline std::shared_ptr<MultiSegmentKeyBase>
  operator()(unsigned n_segments, Args&&...) const {
    DARMA_ASSERT_MESSAGE(false,
      "Key expression is too large to use with SegmentedKey.  Increase"
        " SegmentedKey::max_extra_segments, use a smaller key expression,"
        " or build with a different key type (if supported by the current"
        " backend)"
    );
    return nullptr; // unreachable
  }
};

template <>
struct key_traits<SegmentedKey>
{
  struct maker {
    template <typename... Args>
    inline SegmentedKey
    operator()(Args&&... args) const {
      return SegmentedKey(
        detail::variadic_constructor_arg,
        std::forward<Args>(args)...
      );
    }
  };

  struct maker_from_tuple {
    template <typename Tuple>
    inline SegmentedKey
    operator()(Tuple&& data) const {
      meta::splat_tuple(std::forward<Tuple>(data), maker());
    }
  };

  typedef segmented_key_equal key_equal;
  typedef segmented_key_hasher hasher;

};

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_SEGMENTED_KEY_H
