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

#define PRINT_BITS(data, n_bytes) { \
  char* debug_spot = (char*)data; \
  for(int __i = 0; __i < n_bytes; ++__i) { \
    std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<0)); \
    std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<1)); \
    std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<2)); \
    std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<3)); \
    std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<4)); \
    std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<5)); \
    std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<6)); \
    std::cout << std::setw(1) << (bool)(((int)*debug_spot)&(1<<7)); \
    std::cout << " "; \
    debug_spot++; \
  } \
  std::cout << std::endl; \
}

#define DEBUG_SEGMENTED_KEY_BYTES 0

#include <array>
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
#include <tinympl/logical_and.hpp>
#include <tinympl/vector.hpp>
#include <tinympl/all_of.hpp>

#include <darma/impl/meta/detection.h>
#include <darma/impl/key_concept.h>
#include <darma/impl/key/bytes_convert.h>
#include <darma/impl/meta/tuple_for_each.h>
#include <darma/impl/util.h>
#include <darma/impl/key/key_fwd.h>
#include <darma/impl/key/raw_bytes.h>


namespace darma_runtime {

namespace detail {


namespace m = tinympl;
namespace mv = tinympl::variadic;

using std::uint8_t;

// Some forward declarations
class SegmentedKey;
struct segmented_key_hasher;
struct segmented_key_equal;
struct segmented_key_internal_use_access;

static constexpr unsigned SegmentedKey_segment_size = 64;

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
    friend class segmented_key_internal_use_access;

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
  static constexpr uint8_t payload_size = 53;

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

  virtual size_t get_raw_bytes_size() const = 0;
  virtual void* get_raw_bytes_start() const = 0;

  virtual ~MultiSegmentKeyBase() = default;

  // do not reorder these.  get_raw_bytes_start() is dependent on it
  uint8_t n_parts = uninitialized;
  bool last_part_is_special = false;
};

struct constructor_from_iterables_t { };
constexpr constructor_from_iterables_t constructor_from_iterables = {};

template <unsigned NExtraSegments>
class alignas(64) MultiSegmentKey
  : public MultiSegmentKeyBase
{
  private:

    template <typename BytesConverter, typename Value>
    inline void add_part(
      BytesConverter&& bytes_converter,
      Value&& val,
      unsigned& next_extra_segment,
      uint8_t& remain_current_seg,
      char*& data_spot,
      uint8_t& current_n_pieces
    ) {
      size_t data_size_remain = bytes_converter.get_size(std::forward<decltype(val)>(val));

      if (remain_current_seg == 0) {
        if (next_extra_segment == 0) {
          first_segment_.n_pieces = current_n_pieces;
          first_segment_.is_last = false;
        }
        else {
          segments_[next_extra_segment - 1].n_pieces = current_n_pieces;
          segments_[next_extra_segment - 1].is_last = false;
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
      KeySegmentPieceMetadata *current_piece_md = reinterpret_cast<KeySegmentPieceMetadata *>(data_spot);
      current_piece_md->is_first = true;
      ++current_n_pieces;
      data_spot += sizeof(KeySegmentPieceMetadata);

      // potentially multi-piece part spanning more than one segment
      size_t data_offset = 0;
      while (data_size_remain) {
        if (data_size_remain <= remain_current_seg) {
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
          current_piece_md->piece_size = (uint8_t) data_size_remain;
          // End the loop
          data_size_remain = 0;
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
          if (next_extra_segment == 0) {
            first_segment_.n_pieces = current_n_pieces;
            first_segment_.is_last = false;
          }
          else {
            segments_[next_extra_segment - 1].n_pieces = current_n_pieces;
            segments_[next_extra_segment - 1].is_last = false;
          }

          data_spot = segments_[next_extra_segment].data;
          segments_[next_extra_segment].is_last = true; // we'll change this if it's not true
          current_n_pieces = 1; // (the one we're about to create)
          // Set up the metadata for the next piece
          remain_current_seg = KeySegment::payload_size;
          remain_current_seg -= sizeof(KeySegmentPieceMetadata);
          current_piece_md = reinterpret_cast<KeySegmentPieceMetadata *>(data_spot);
          current_piece_md->is_first = false; // it's a continuation piece
          current_piece_md->is_last = data_size_remain <= remain_current_seg;
          // advance data spot past the piece header
          data_spot += sizeof(KeySegmentPieceMetadata);
          ++next_extra_segment;
        }
      } // end while data_size_remain

      if (remain_current_seg == 0) {
        // in case it fits exactly, we need to fix up current_n_pieces
        if (next_extra_segment == 0) first_segment_.n_pieces = current_n_pieces;
        else segments_[next_extra_segment - 1].n_pieces = current_n_pieces;
      }
    }

  public:
    MultiSegmentKey() = default;

    template <typename... Args>
    MultiSegmentKey(
      constructor_from_iterables_t,
      Args&&... args
    ) {
      n_parts = 0;
      memset(&first_segment_, 0, sizeof(FirstKeySegment));
      if(NExtraSegments) memset(segments_, 0, sizeof(KeySegment)*NExtraSegments);

      uint8_t remain_current_seg = FirstKeySegment::payload_size;
      unsigned next_extra_segment = 0;
      char* data_spot = first_segment_.data;
      first_segment_.is_last = true; // will get changed if not true
      uint8_t current_n_pieces = 0;
      meta::tuple_for_each_zipped(
        std::forward_as_tuple(bytes_convert<
          std::remove_cv_t<std::remove_reference_t<
            typename std::remove_cv_t<std::remove_reference_t<Args>>::value_type
          >>
        >()...),
        std::forward_as_tuple(std::forward<Args>(args)...),
        [&](auto&& bytes_converter, auto&& iterator) {
          for (auto &&val : iterator) {
            ++n_parts;
            this->template add_part(bytes_converter, val,
              next_extra_segment, remain_current_seg,
              data_spot, current_n_pieces
            );
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
      PRINT_BITS(first_segment_.data, FirstKeySegment::payload_size);
      for(int i = 0; i < NExtraSegments; ++i) {
        PRINT_BITS(segments_[i].data, KeySegment::payload_size);
      }
#endif

    }

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
        std::forward_as_tuple(bytes_convert<std::remove_cv_t<std::remove_reference_t<Args>>>()...),
        std::forward_as_tuple(std::forward<Args>(args)...),
        [&](auto&& bytes_converter, auto&& val) {
          this->template add_part(bytes_converter, val,
            next_extra_segment, remain_current_seg,
            data_spot, current_n_pieces
          );
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
      PRINT_BITS(first_segment_.data, FirstKeySegment::payload_size);
      for(int i = 0; i < NExtraSegments; ++i) {
        PRINT_BITS(segments_[i].data, KeySegment::payload_size);
      }
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
        if(not f(ip_md, spot, current_part)) return;
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
        const char* spot = segments_[iseg].data;
        for(uint8_t ipiece = 0; ipiece < segments_[iseg].n_pieces; ++ipiece) {
          KeySegmentPieceMetadata ip_md = *(KeySegmentPieceMetadata *) spot;
          spot += sizeof(KeySegmentPieceMetadata);
          if(not f(ip_md, spot, current_part)) return;
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
          return not ip_md.is_last; // break if last
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
          return not ip_md.is_last; // break if last piece
        }
        else return true; // don't break
      });
    }

    void* get_raw_bytes_start() const override {
      return (void*)&n_parts;
    }

    size_t get_raw_bytes_size() const override {
      return sizeof(*this) - ((char*)get_raw_bytes_start() - ((char*)this));
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


template <typename T>
using has_value_type_archetype = typename std::decay_t<T>::value_type;
template <typename T>
using has_iterator_archetype = typename std::decay_t<T>::iterator;
template <typename T>
using has_bytes_convert_archetype = decltype( bytes_convert<T>{} );

// Grossly incomplete is_iterable
template <typename T>
using is_iterable = typename tinympl::and_<
  meta::is_detected<has_value_type_archetype, T>,
  meta::is_detected<has_iterator_archetype, T>
>::type;

static_assert(is_iterable<std::vector<int>>::value, "is_iterable<> not working");

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

  template <typename... Args>
  inline
  std::enable_if_t<
    tinympl::and_<is_iterable<Args>...>::value,
    std::shared_ptr<MultiSegmentKeyBase>
  >
  from_iterables(unsigned n_segments, Args&&... args) const {
    if(n_segments == N) {
      std::shared_ptr<MultiSegmentKeyBase> rv = std::make_shared<MultiSegmentKey<N>>(
        constructor_from_iterables,
        std::forward<Args>(args)...
      );
      return rv;
    }
    else {
      return create_n_segment_key<N+1>().template from_iterables(n_segments, std::forward<Args>(args)...);
    }
  }

  // Note:  only for reconstructing key from bytes
  auto
  generate_constructor_to_fit_raw(size_t raw_bytes_to_fit) const {
    return [raw_bytes_to_fit](auto&&... args) -> std::shared_ptr<MultiSegmentKeyBase> {
      if((N+1) * SegmentedKey_segment_size > raw_bytes_to_fit) {
        return std::make_shared<MultiSegmentKey<N>>(std::forward<decltype(args)>(args)...);
      }
      else {
        return create_n_segment_key<N+1>().generate_constructor_to_fit_raw(raw_bytes_to_fit)(
          std::forward<decltype(args)>(args)...
        );
      }
    };

  }
};



template <
  bool all_sizes_known_statically /* = false */
>
struct create_multi_segment_key_impl {
  template <
    typename BytesConverter,
    typename Value
  >
  inline void
  _get_extra_segments_contrib(
    BytesConverter&& bytes_converter,
    Value&& val,
    size_t& remain_current,
    unsigned& n_extra_segments
  ) const {
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

  template <typename... Args>
  inline unsigned
  get_n_extra_segments(Args&&... args) const {
    size_t remain_current = FirstKeySegment::payload_size;
    unsigned n_extra_segments = 0;
    meta::tuple_for_each_zipped(
      std::forward_as_tuple(bytes_convert<std::remove_cv_t<std::remove_reference_t<Args>>>()...),
      std::forward_as_tuple(std::forward<Args>(args)...),
      [&](auto const& bytes_converter, auto&& val) {
        this->template _get_extra_segments_contrib(
          bytes_converter, std::forward<decltype(val)>(val),
          remain_current, n_extra_segments
        );
      }
    );
    return n_extra_segments;
  }

  template<typename... Args>
  std::enable_if_t<
    tinympl::and_<is_iterable<Args>...>::value,
    unsigned
  >
  get_n_extra_segments_from_iterables(Args&&... args) const {
    size_t remain_current = FirstKeySegment::payload_size;
    unsigned n_extra_segments = 0;
    meta::tuple_for_each_zipped(
      std::forward_as_tuple(bytes_convert<
        std::remove_cv_t<std::remove_reference_t<
          typename std::remove_cv_t<std::remove_reference_t<Args>>::value_type
        >>
      >()...),
      std::forward_as_tuple(std::forward<Args>(args)...),
      [&](auto&& bytes_converter, auto&& iterable) {

        for(auto&& val : iterable) {
          this->template _get_extra_segments_contrib(
            bytes_converter, std::forward<decltype(val)>(val),
            remain_current, n_extra_segments
          );
        }
      }
    );
    return n_extra_segments;
  }

  template <typename... Args>
  inline
  std::enable_if_t<
    tinympl::and_<convertible_to_bytes<
      std::remove_cv_t<std::remove_reference_t<Args>>
    >...>::value,
    std::shared_ptr<MultiSegmentKeyBase>
  >
  operator()(Args&&... args) const {
    return create_n_segment_key<>()(get_n_extra_segments(
      std::forward<Args>(args)...
    ), std::forward<Args>(args)...);
  }

  template <typename... Args>
  inline
  std::enable_if_t<
    tinympl::and_<is_iterable<Args>...>::value,
    std::shared_ptr<MultiSegmentKeyBase>
  >
  from_iterables(Args&&... args) const {
    return create_n_segment_key<>().template from_iterables(get_n_extra_segments_from_iterables(
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
  static constexpr bool all_sizes_known_statically = false; // && // for now, just treat all as variable size
  //  mv::all_of<bytes_size_known_statically, std::remove_cv_t<std::remove_reference_t<Args>>...>::type::value;
  return create_multi_segment_key_impl<all_sizes_known_statically>()(
    std::forward<Args>(args)...
  );
}


////////////////////////////////////////////////////////////////////////////////


class SegmentedKey {

  protected:
    static constexpr uint8_t not_given = std::numeric_limits<uint8_t>::max();

  public:

    static constexpr unsigned segment_size = SegmentedKey_segment_size;
    static constexpr unsigned max_extra_segments = 8;
    static constexpr uint8_t max_num_parts = std::numeric_limits<uint8_t>::max() - (uint8_t)1;

    SegmentedKey(
      std::shared_ptr<MultiSegmentKeyBase> const& k_impl
    ) : key_impl_(k_impl)
    { }

    template <typename... Args>
    SegmentedKey(
      variadic_constructor_arg_t const,
      Args&&... args
    ) : key_impl_(create_multi_segment_key(std::forward<Args>(args)...))
    { }
    
    template <typename... Args>
    SegmentedKey(
      constructor_from_iterables_t const,
      Args&&... args
    ) : key_impl_(create_multi_segment_key_impl<false>().from_iterables(std::forward<Args>(args)...))
    { }

    template <uint8_t N=not_given>
    SegmentedKeyPartBase
    component(size_t N_dynamic=not_given) const {
      assert(N_dynamic == not_given || N_dynamic < (size_t)max_num_parts);
      static_assert(N == not_given || N < max_num_parts,
        "tried to get key component greater than max num parts");
      assert(N == not_given xor N_dynamic == not_given);
      const uint8_t actual_N = N == not_given ? (uint8_t)N_dynamic : N;
      assert(actual_N < n_components());
      size_t part_size = key_impl_->get_part_size(actual_N);
      SegmentedKeyPartBase rv(part_size);
      key_impl_->get_part_bytes(actual_N, (char*)rv.data);
      return rv;
    }

    /** @breif Return the number of *user-level* components
     */
    size_t n_components() const {
      return key_impl_->n_parts - key_impl_->last_part_is_special;
    }

  protected:

    SegmentedKey
    without_last_component() const {
      if(not key_without_last_component_impl_) {
        std::vector<raw_bytes> v;
        v.reserve(key_impl_->n_parts);
        for (int i = 0; i < n_components() - 1; ++i) {
          v.push_back(component(i).as<raw_bytes>());
        }
        key_without_last_component_impl_ = create_multi_segment_key_impl<false>().from_iterables(v);
      }
      return key_without_last_component_impl_;
    }

    friend struct segmented_key_hasher;
    friend struct segmented_key_equal;
    friend struct segmented_key_internal_use_access;
    friend struct bytes_convert<SegmentedKey>;

    std::shared_ptr<MultiSegmentKeyBase> key_impl_;
    // cached:
    mutable std::shared_ptr<MultiSegmentKeyBase> key_without_last_component_impl_;

}; // end class SegmentedKey

////////////////////////////////////////////////////////////////////////////////

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

// Specialization to stop recursion (has to go after SegmentedKey to get
// access to static member variable max_extra_segments
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

  template <typename... Args>
  std::shared_ptr<MultiSegmentKeyBase>
  from_iterables(unsigned n_segments, Args&&... args) const {
    DARMA_ASSERT_MESSAGE(false,
      "Key expression is too large to use with SegmentedKey.  Increase"
        " SegmentedKey::max_extra_segments, use a smaller key expression,"
        " or build with a different key type (if supported by the current"
        " backend)"
    );
    return nullptr; // unreachable
  }

  auto
  generate_constructor_to_fit_raw(size_t raw_bytes_to_fit) const {
    DARMA_ASSERT_MESSAGE(false,
      "Key expression is too large to use with SegmentedKey.  Increase"
        " SegmentedKey::max_extra_segments, use a smaller key expression,"
        " or build with a different key type (if supported by the current"
        " backend)"
    );
    return [](auto&&... args) -> std::shared_ptr<MultiSegmentKeyBase> { return nullptr; }; // unreachable
  }
};

template <>
struct bytes_convert<SegmentedKey> {
  static constexpr bool size_known_statically = false;
  inline size_t
  get_size(SegmentedKey const& k) const {
    return k.key_impl_->get_raw_bytes_size();
  }
  inline void
  operator()(SegmentedKey const& k, void* dest, const size_t n_bytes, const size_t offset) const {
    ::memcpy(dest, (char*)k.key_impl_->get_raw_bytes_start() + offset, n_bytes);
  }
  inline SegmentedKey
  get_value(void* data, size_t size) const {
    auto rv = SegmentedKey(
      create_n_segment_key<>().generate_constructor_to_fit_raw(size)()
    );
    ::memcpy((char*)rv.key_impl_->get_raw_bytes_start(), data, size);
    return rv;
  }
};

struct segmented_key_internal_use_access {
  template <typename T>
  static SegmentedKey
  add_internal_last_component(
    SegmentedKey const& k, T const& to_add
  ) {
    assert(!k.key_impl_->last_part_is_special);
    std::vector<raw_bytes> v;
    v.reserve(k.key_impl_->n_parts);
    for (int i = 0; i < k.key_impl_->n_parts; ++i) {
      v.push_back(k.component(i).as<raw_bytes>());
    }
    bytes_convert<std::remove_cv_t<std::remove_reference_t<T>>> bc;
    size_t add_size = bc.get_size(to_add);
    char last_item[add_size];
    bc(to_add, last_item, add_size, 0);
    v.push_back(bytes_convert<raw_bytes>().get_value(last_item, add_size));

    SegmentedKey rv(constructor_from_iterables, v);
    rv.key_without_last_component_impl_ = k.key_impl_;
    rv.key_impl_->last_part_is_special = true;
    return rv;
  }
  static SegmentedKey
  without_internal_last_component(SegmentedKey const& k) {
    assert(k.key_impl_->last_part_is_special);
    return k.without_last_component();
  }
  static bool
  has_internal_last_component(SegmentedKey const& k) {
    return k.key_impl_->last_part_is_special;
  }
  static SegmentedKeyPartBase
  get_internal_last_component(SegmentedKey const& k) {
    assert(k.key_impl_->last_part_is_special);
    size_t part_size = k.key_impl_->get_part_size(k.key_impl_->n_parts-(uint8_t)1);
    SegmentedKeyPartBase rv(part_size);
    k.key_impl_->get_part_bytes(k.key_impl_->n_parts-(uint8_t)1, (char*)rv.data);
    return rv;
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
  typedef segmented_key_internal_use_access internal_use_access;

};

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_SEGMENTED_KEY_H
