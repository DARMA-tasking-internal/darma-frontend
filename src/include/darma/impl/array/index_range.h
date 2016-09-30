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
#include <darma/impl/polymorphic_serialization.h>

namespace darma_runtime {
namespace detail {


// TODO move this
template <typename Index=size_t>
class counting_iterator : public abstract::frontend::IndexIterator {

  public:

    using difference_type = std::make_signed_t<Index>;
    using value_type = Index const;
    using pointer = Index const*;
    using iterator_category = std::random_access_iterator_tag;


  public:

    template <typename IndexT>
    struct IndexWrapper : abstract::frontend::Index {
      public:
        IndexWrapper(IndexT i) : value_(i) { }
        operator IndexT() { return value_; }
        IndexT value_;
    };

  private:

    IndexWrapper<Index> wrapper_;

  public:


    using reference = IndexWrapper<Index>&;

    counting_iterator(Index value = 0) : wrapper_(value) { };

    counting_iterator&
    operator+=(difference_type diff) { wrapper_.value_ += diff; return *this; }

    counting_iterator
    operator+(difference_type diff) const {
      return counting_iterator(wrapper_.value_ + diff);
    }

    counting_iterator&
    operator-=(difference_type diff) { wrapper_.value_ -= diff; return *this; }

    counting_iterator
    operator-(difference_type diff) const {
      return counting_iterator(wrapper_.value_ - diff);
    }

    counting_iterator
    operator-(counting_iterator const& other) const {
      return wrapper_.value_ - other.wrapper_.value_;
    }

    reference
    operator*() { return wrapper_; }

    bool
    operator!=(counting_iterator other) { return wrapper_.value_ != other.wrapper_.value_; }

    bool
    operator==(counting_iterator other) { return wrapper_.value_ == other.wrapper_.value_; }

    pointer
    operator->() const { return &wrapper_.value_; }

    counting_iterator
    operator++(int) { counting_iterator rv(wrapper_.value_); ++wrapper_.value_; return rv; }

    counting_iterator&
    operator++() { ++wrapper_.value_; return *this; }

    counting_iterator
    operator--(int) { counting_iterator rv(wrapper_.value_); --wrapper_.value_; return rv; }

    counting_iterator&
    operator--() { --wrapper_.value_; return *this; }

    bool
    operator<=(counting_iterator other) const { return wrapper_.value_ <= other.wrapper_.value_; }

    bool
    operator<(counting_iterator other) const { return wrapper_.value_ < other.wrapper_.value_; }

    bool
    operator>(counting_iterator other) const { return wrapper_.value_ > other.wrapper_.value_; }

    bool
    operator>=(counting_iterator other) const { return wrapper_.value_ >= other.wrapper_.value_; }
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

class IndexRangeBase
  : public abstract::frontend::IndexRange
{ };

class CompactIndexRangeBase
  : public abstract::frontend::CompactIndexRange
{ };

class ContiguousIndexRange
  : public CompactIndexRangeBase
{
  private:

    size_t size_;
    size_t offset_;

    using polymorphic_details = typename
      polymorphic_serialization_details<ContiguousIndexRange>
        ::template with_abstract_bases<abstract::frontend::CompactIndexRange>;


  public:

    ContiguousIndexRange(size_t size, size_t offset)
      : size_(size), offset_(offset)
    { }

    size_t get_packed_size() const override {
      return polymorphic_details::registry_frontmatter_size
        + sizeof(size_) + sizeof(offset_);
    }

    void pack(char* buffer) const override {
      polymorphic_details::add_registry_frontmatter_in_place(buffer);
      buffer += polymorphic_details::registry_frontmatter_size;
      *reinterpret_cast<size_t*>(buffer) = size_;
      buffer += sizeof(size_t);
      *reinterpret_cast<size_t*>(buffer) = offset_;
    }

    static
    std::unique_ptr<abstract::frontend::CompactIndexRange>
    unpack(char const* buffer, size_t size) {
      assert(size == 2*sizeof(size_t));
      std::unique_ptr<ContiguousIndexRange> rv;
      rv = std::make_unique<ContiguousIndexRange>(0, 0);
      rv->size_ = *reinterpret_cast<size_t const*>(buffer);
      buffer += sizeof(size_t);
      rv->offset_ = *reinterpret_cast<size_t const*>(buffer);
      return rv;
    }

  public:

    size_t size() const override { return size_; }

    size_t offset() const override { return offset_; }

    bool contiguous() const override { return true; }

    bool strided() const override { return false; }

    size_t stride() const override { return 1; }

    //std::unique_ptr<abstract::frontend::IndexIterator>
    //begin() override {
    //  return std::make_unique<counting_iterator<size_t>>(offset());
    //}

    //std::unique_ptr<abstract::frontend::IndexIterator>
    //end() override {
    //  return std::make_unique<counting_iterator<size_t>>(offset() + size());
    //}


};

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_ARRAY_INDEX_RANGE_H
