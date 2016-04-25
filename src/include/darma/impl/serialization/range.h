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
#include "nonintrusive.h"

namespace darma_runtime {
namespace serialization {

namespace detail {

template <typename RandomAccessIterator>
struct SerDesRange {
  private:
    RandomAccessIterator begin_;
    RandomAccessIterator end_;

  public:

    SerDesRange(
      RandomAccessIterator in_begin,
      RandomAccessIterator in_end
    ) : begin_(in_begin), end_(in_end) { }

    RandomAccessIterator
    begin() const {
      return begin_;
    }

    RandomAccessIterator
    end() const {
      return end_;
    }
};

} // end namespace detail

////////////////////////////////////////////////////////////////////////////////

template <typename RandomAccessIterator>
auto
range(RandomAccessIterator begin, RandomAccessIterator end) {
  return detail::SerDesRange<std::decay_t<RandomAccessIterator>>( begin, end );
}

////////////////////////////////////////////////////////////////////////////////

template <typename RandomAccessIterator>
struct Serializer<detail::SerDesRange<RandomAccessIterator>> {
  private:
    typedef detail::SerDesRange<RandomAccessIterator> range_t;
    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type difference_type;
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type value_type;

  public:
    template <typename Archive>
    void
    compute_size(range_t const& range, Archive& ar) const {
      for(auto const& v : range) {
        ar.incorporate_size(v);
      }
    }

    template <typename Archive>
    void
    pack(range_t const& range, Archive& ar) const {
      // cast just to be safe, but that should be the return value of the subtraction operator
      ar.pack_item(static_cast<difference_type>(range.end() - range.begin()));
      for(auto const& v : range) {
        ar.pack_item(v);
      }
    }

    template <typename Archive>
    void
    unpack(void* allocated, Archive& ar) const {
      // Initialize to avoid compiler warnings
      difference_type n_items = 0;
      ar.unpack_item(n_items);
      for(difference_type i = 0; i < n_items; ++i) {


      }
    }

};

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_RANGE_H
