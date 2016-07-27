/*
//@HEADER
// ************************************************************************
//
//                      range.h
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

#ifndef DARMA_IMPL_SERIALIZATION_RANGE_H
#define DARMA_IMPL_SERIALIZATION_RANGE_H

#include <type_traits>
#include <iterator>

#include <darma/impl/meta/is_contiguous.h>

#include "allocation.h"
#include "traits.h"

namespace darma_runtime {

namespace serialization {

namespace detail {

template <typename BeginIterator, typename EndIterator>
struct SerDesRange;

template <typename RandomAccessIterator>
struct SerDesRange<
  RandomAccessIterator&,
  RandomAccessIterator
> {
  private:
    RandomAccessIterator& begin_;
    RandomAccessIterator end_;

  public:

    using value_type = decltype( * std::declval<RandomAccessIterator>() );

    static constexpr auto is_contiguous =
      meta::is_contiguous_iterator<RandomAccessIterator>::value;
    static constexpr auto value_is_trivially_packable =
      serializability_traits<value_type>::is_directly_serializable;

    typedef RandomAccessIterator iterator;
    typedef std::iterator_traits<RandomAccessIterator> iterator_traits;

    SerDesRange(RandomAccessIterator& in_begin, RandomAccessIterator in_end)
      : begin_(in_begin), end_(in_end) { }

    RandomAccessIterator&
    begin() { return begin_; }

    RandomAccessIterator const&
    begin() const { return begin_; }

    RandomAccessIterator&
    end() { return end_; }

    RandomAccessIterator const&
    end() const { return end_; }
};

template <typename RandomAccessIterator>
struct SerDesRange<RandomAccessIterator&, RandomAccessIterator&> {
  private:
    RandomAccessIterator& begin_;
    RandomAccessIterator& end_;

  public:

    typedef RandomAccessIterator iterator;
    typedef std::iterator_traits<RandomAccessIterator> iterator_traits;

    SerDesRange(RandomAccessIterator& in_begin, RandomAccessIterator& in_end)
      : begin_(in_begin), end_(in_end) { }

    RandomAccessIterator&
    begin() const { return begin_; }

    RandomAccessIterator&
    end() const { return end_; }
};

} // end namespace detail

////////////////////////////////////////////////////////////////////////////////

template <typename BeginIter, typename EndIter>
auto
range(BeginIter&& begin, EndIter&& end) {
  return detail::SerDesRange<BeginIter, EndIter>(
    std::forward<BeginIter>(begin),
    std::forward<EndIter>(end)
  );
}

//////////////////////////////////////////////////////////////////////////////////
//
//template <typename RandomAccessIterator>
//struct Serializer<detail::SerDesRange<RandomAccessIterator>> {
//  private:
//    typedef detail::SerDesRange<RandomAccessIterator> range_t;
//    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type difference_type;
//    typedef typename std::iterator_traits<RandomAccessIterator>::value_type value_type;
//    using value_allocation_traits = detail::allocation_traits<value_type>;
//
//  public:
//    template <typename Archive>
//    void
//    compute_size(range_t const& range, Archive& ar) const {
//      for(auto const& v : range) {
//        ar.incorporate_size(v);
//      }
//    }
//
//    template <typename Archive>
//    void
//    pack(range_t const& range, Archive& ar) const {
//      // cast just to be safe, but that should be the return value of the subtraction operator
//      ar.pack_item(static_cast<difference_type>(range.end() - range.begin()));
//      for(auto const& v : range) {
//        ar.pack_item(v);
//      }
//    }
//
//    template <typename Archive>
//    range_t*
//    allocate(Archive& ar) const {
//      if(ar.is_unpacking()) {
//        // We want to allocate the actual data, not the range object
//        // "peek" at the item without advancing the archive
//        difference_type const& n_items = *static_cast<difference_type*>(
//          Serializer_attorneys::ArchiveAccess::spot(ar)
//        );
//        return value_allocation_traits::template allocate(ar, n_items);
//      }
//      else {
//        // otherwise, we're allocating the actual range object itself in some other context
//        auto alloc = std::allocator<range_t>();
//        return std::allocator_traits<decltype(alloc)>::allocate(alloc, 1);
//      }
//    }
//
//    template <typename Archive>
//    void
//    unpack(void* allocated, Archive& ar) const {
//      difference_type n_items = 0;
//      ar.unpack_item(n_items);
//      for(difference_type i = 0; i < n_items; ++i) {
//        ar.unpack_item((value_type*)allocated);
//        allocated = (char*)allocated + sizeof(value_type);
//      }
//
//    }

//};

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_RANGE_H
