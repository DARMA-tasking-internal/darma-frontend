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
>
{
  private:
    RandomAccessIterator& begin_;
    RandomAccessIterator end_;

  public:

    typedef RandomAccessIterator iterator;
    typedef std::iterator_traits<RandomAccessIterator> iterator_traits;

    using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;

    static constexpr auto is_contiguous =
      meta::is_contiguous_iterator<std::remove_cv_t<RandomAccessIterator>>::value;

    static constexpr auto value_is_directly_serializable =
      serializability_traits<value_type>::is_directly_serializable;

    SerDesRange(RandomAccessIterator& in_begin, RandomAccessIterator&& in_end)
      : begin_(in_begin), end_(in_end) { }

    RandomAccessIterator&
    begin() { return begin_; }

    RandomAccessIterator const&
    begin() const { return begin_; }

    RandomAccessIterator const&
    end() const { return end_; }
};

template <typename RandomAccessIterator>
struct SerDesRange<RandomAccessIterator&, RandomAccessIterator&>
{
  private:
    RandomAccessIterator& begin_;
    RandomAccessIterator& end_;

  public:

    typedef RandomAccessIterator iterator;
    typedef std::iterator_traits<RandomAccessIterator> iterator_traits;

    using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;

    static constexpr auto is_contiguous =
      meta::is_contiguous_iterator<std::remove_cv_t<RandomAccessIterator>>::value;

    static constexpr auto value_is_directly_serializable =
      serializability_traits<value_type>::is_directly_serializable;

    SerDesRange(RandomAccessIterator& in_begin, RandomAccessIterator& in_end)
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
struct SerDesRange<RandomAccessIterator, RandomAccessIterator>
{
  private:
    RandomAccessIterator begin_;
    RandomAccessIterator end_;

  public:

    using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;

    static constexpr auto is_contiguous =
      meta::is_contiguous_iterator<std::remove_cv_t<RandomAccessIterator>>::value;

    static constexpr auto value_is_directly_serializable =
      serializability_traits<value_type>::is_directly_serializable;

    typedef RandomAccessIterator iterator;
    typedef std::iterator_traits<RandomAccessIterator> iterator_traits;

    SerDesRange(RandomAccessIterator&& in_begin, RandomAccessIterator&& in_end)
      : begin_(in_begin), end_(in_end) { }

    RandomAccessIterator const&
    begin() const { return begin_; }

    RandomAccessIterator const&
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

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_RANGE_H
