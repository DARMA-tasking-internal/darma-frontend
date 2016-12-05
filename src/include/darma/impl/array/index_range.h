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

template <typename Integer, typename DenseIndex=size_t>
struct ContiguousIndexMapping;

} // end namespace detail

template <typename Integer>
struct ContiguousIndex {
  Integer value;
  Integer min_value;
  Integer max_value;

  bool operator < (ContiguousIndex const& other) const {
    return value < other.value;
  }
  template <typename IntegerConvertible>
  ContiguousIndex operator- (IntegerConvertible&& other) const {
    return { value - std::forward<IntegerConvertible>(other), min_value, max_value };
  }

  template <typename IntegerConvertible>
  ContiguousIndex operator+ (IntegerConvertible&& other) const {
    return { value + std::forward<IntegerConvertible>(other), min_value, max_value };
  }

  operator Integer() { return value; }
};

template <typename IntegerConvertible, typename Integer>
std::enable_if_t<
  not std::is_same<std::decay_t<IntegerConvertible>, ContiguousIndex<Integer>>::value,
  ContiguousIndex<Integer>
>
operator- (IntegerConvertible&& other, ContiguousIndex<Integer> const& idx) {
  return { std::forward<IntegerConvertible>(other) - idx.value,
    idx.min_value, idx.max_value
  };
}

template <typename IntegerConvertible, typename Integer>
std::enable_if_t<
  not std::is_same<std::decay_t<IntegerConvertible>, ContiguousIndex<Integer>>::value,
  ContiguousIndex<Integer>
>
operator+ (IntegerConvertible&& other, ContiguousIndex<Integer> const& idx) {
  return { std::forward<IntegerConvertible>(other) + idx.value,
    idx.min_value, idx.max_value
  };
}

template <typename Integer>
class ContiguousIndexRange
  : public detail::PolymorphicSerializationAdapter<
      ContiguousIndexRange<Integer>,
      abstract::frontend::IndexRange
    >
{
  private:

    Integer size_;
    Integer offset_;

  public:

    using mapping_to_dense = detail::ContiguousIndexMapping<Integer>;
    using index_type = ContiguousIndex<Integer>;
    using is_index_range = std::true_type;

    friend class detail::ContiguousIndexMapping<Integer>;

    ContiguousIndexRange() : size_(0), offset_(0) { }

    ContiguousIndexRange(Integer size, Integer offset = 0)
      : size_(size), offset_(offset)
    { }

    template <typename ArchiveT>
    void serialize(ArchiveT& ar) {
      ar | size_ | offset_;
    }


  public:

    size_t size() const override { return size_; }

};

template <typename Integer = int>
using Index1D = ContiguousIndex<Integer>;

template <typename Integer = int>
using Range1D = ContiguousIndexRange<Integer>;

namespace detail {

template <typename Integer, typename DenseIndex>
struct ContiguousIndexMapping {
  private:

    ContiguousIndexRange<Integer> range;

    friend class ContiguousIndexRange<Integer>;

  public:

    // TODO remove this once the default constructibility requirement emposed by ConcurrentRegionContext is lifted
    ContiguousIndexMapping() = default;

    explicit ContiguousIndexMapping(ContiguousIndexRange<Integer> const& rng)
      : range(rng)
    { }

    using is_index_mapping_t = std::true_type;
    using from_index_type = ContiguousIndex<Integer>;
    using to_index_type = DenseIndex;

    to_index_type map_forward(from_index_type const& from) const {
      return from.value - from.min_value;
    }

    from_index_type map_backward(to_index_type const& to) const {
      return { static_cast<Integer>(to), range.offset_, range.offset_ + range.size_ };
    }

    template <typename ArchiveT> void serialize(ArchiveT& ar) { ar | range; }
};


} // end namespace detail

template <typename Integer>
detail::ContiguousIndexMapping<Integer>
get_mapping_to_dense(
  ContiguousIndexRange<Integer> const& range
) {
  return detail::ContiguousIndexMapping<Integer>{ range };
}

} // end namespace darma_runtime

#endif //DARMA_IMPL_ARRAY_INDEX_RANGE_H
