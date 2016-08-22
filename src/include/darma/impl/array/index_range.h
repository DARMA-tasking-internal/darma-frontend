/*
//@HEADER
// ************************************************************************
//
//                      index_range.h
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

#ifndef DARMA_IMPL_ARRAY_INDEX_RANGE_H
#define DARMA_IMPL_ARRAY_INDEX_RANGE_H

#include <darma/interface/frontend/index_range.h>

namespace darma_runtime {
namespace detail {

class ContiguousIndexRange
  : public abstract::frontend::IndexRange
{
  private:

    size_t size_;
    size_t offset_;

  public:

    size_t size() const override { return size_; }

    size_t offset() const override { return offset_; }

    bool contiguous() const override { return true; }

    bool strided() const override { return false; }

    size_t stride() const override { return 1; }

};

namespace _impl {

// TODO move this
template <typename Index=size_t>
class counting_iterator {

  public:

    using difference_type = std::make_signed_t<Index>;
    using value_type = Index const;
    using pointer = Index const*;
    using reference = Index const;
    using iterator_category = std::random_access_iterator_tag;

  private:

    Index value_;

  public:

    counting_iterator(Index value = 0) : value_(value) { };

    counting_iterator&
    operator+=(difference_type diff) { value_ += diff; return *this; }

    counting_iterator
    operator+(difference_type diff) const {
      return counting_iterator(value_ + diff);
    }

    counting_iterator&
    operator-=(difference_type diff) { value_ -= diff; return *this; }

    counting_iterator
    operator-(difference_type diff) const {
      return counting_iterator(value_ - diff);
    }

    counting_iterator
    operator-(counting_iterator const& other) const {
      return value_ - other.value_;
    }

    reference
    operator[](difference_type n) const {
      return value_ + n;
    }

    reference
    operator*() const { return value_; }

    bool
    operator!=(counting_iterator other) { return value_ != other.value_; }

    bool
    operator==(counting_iterator other) { return value_ == other.value_; }

    pointer
    operator->() const { return &value_; }

    counting_iterator
    operator++(int) { counting_iterator rv(value_); ++value_; return rv; }

    counting_iterator&
    operator++() { ++value_; return *this; }

    counting_iterator
    operator--(int) { counting_iterator rv(value_); --value_; return rv; }

    counting_iterator&
    operator--() { --value_; return *this; }

    bool
    operator<=(counting_iterator other) const { return value_ <= other.value_; }

    bool
    operator<(counting_iterator other) const { return value_ < other.value_; }

    bool
    operator>(counting_iterator other) const { return value_ > other.value_; }

    bool
    operator>=(counting_iterator other) const { return value_ >= other.value_; }
};

template <typename Index>
counting_iterator<Index>
operator+(
  counting_iterator<Index> const& iter,
  typename counting_iterator<Index>::difference_type diff
) {
  return counting_iterator<Index>(
    *iter + diff
  );
}

} // end namespace _impl

inline _impl::counting_iterator<size_t>
begin(abstract::frontend::IndexRange const& range) {
  assert(range.contiguous() and not range.strided());
  return _impl::counting_iterator(range.offset());
}

inline _impl::counting_iterator<size_t>
end(abstract::frontend::IndexRange const& range) {
  assert(range.contiguous() and not range.strided());
  return _impl::counting_iterator(range.offset() + range.size());
}

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_ARRAY_INDEX_RANGE_H
