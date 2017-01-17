/*
//@HEADER
// ************************************************************************
//
//                       index_md.h
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
// Questions? Contact Jonathan Lifflander (jliffla@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef __INDEX_RANGE_INDEXMD__
#define __INDEX_RANGE_INDEXMD__

template <typename Integer, int ndim = 1>
struct IntegerArray {
  using integer_type_t = Integer;

  // leading..trailing dimension ordering
  Integer dims[ndim] = {};

  IntegerArray() = default;

  explicit IntegerArray(Integer off) {
    for (auto i = 0; i < ndim; i++) {
      dims[i] = off;
    }
  }

  IntegerArray<Integer, ndim-1>
  reduce_dim() const {
    IntegerArray<Integer, ndim-1> val;
    for (auto i = 0; i < ndim-1; i++) {
      val.dims[i] = dims[i];
    }
    return val;
  }

  IntegerArray<Integer, ndim+1>
  increase_dim(Integer in) const {
    IntegerArray<Integer, ndim+1> val;
    for (auto i = 0; i < ndim; i++) {
      val.dims[i] = dims[i];
    }
    val.dims[ndim] = in;
    return val;
  }

  Integer trailing() const {
    return dims[ndim-1];
  }

  Integer leading() const {
    return dims[0];
  }

  size_t get_total_size() const {
    Integer sz = 1;
    for (auto i = 0; i < ndim; i++) {
      sz *= dims[i];
    }
    return sz;
  }

  template <typename Other>
  IntegerArray<Integer, ndim> operator+(Other const& other) const {
    IntegerArray<Integer, ndim> val;
    for (auto i = 0; i < ndim; i++) {
      val.dims[i] = dims[i] + other.dims[i];
    }
    return val;
  }

  template <typename Other>
  IntegerArray<Integer, ndim> operator-(Other const& other) const {
    IntegerArray<Integer, ndim> val;
    for (auto i = 0; i < ndim; i++) {
      val.dims[i] = dims[i] - other.dims[i];
    }
    return val;
  }

  template <typename Other>
  bool operator==(Other const& other) const {
    for (int i = ndim-1; i >= 0; i--) {
      if (dims[i] != other.dims[i]) {
        return false;
      }
    }
    return true;
  }

  template <typename Other>
  bool operator<(Other const& other) const {
    for (auto i = ndim-1; i >= 0; i--) {
      if (dims[i] < other.dims[i]) {
        return true;
      } else if (dims[i] > other.dims[i]) {
        return false;
      }
    }
    return false;
  }

  template <typename ArchiveT>
  void serialize(ArchiveT& ar) {
    for (auto i = 0; i < ndim; i++) {
      ar | dims[i];
    }
  }
};

template <typename Integer, int ndim = 1, typename Array = IntegerArray<Integer,ndim>>
struct IndexMD {
  using integer_array_t = Array;

  integer_array_t value, min_value, max_value;

  IndexMD() = default;
  IndexMD(integer_array_t const& in_value) : value(in_value), min_value(), max_value() { }

  IndexMD(
    integer_array_t const& in_value, integer_array_t const& in_min, integer_array_t const& in_max
  ) : value(in_value), min_value(in_min), max_value(in_max) { }

  IndexMD<Integer, ndim-1>
  reduce_dim() const {
    return IndexMD<Integer, ndim-1>(
      value.reduce_dim(), min_value.reduce_dim(), max_value.reduce_dim()
    );
  }

  IndexMD<Integer, ndim+1>
  increase_dim(Integer val, Integer min_val, Integer max_val) const {
    return IndexMD<Integer, ndim+1>(
      value.increase_dim(val), min_value.increase_dim(min_val),
      max_value.increase_dim(max_val)
    );
  }

  bool operator==(integer_array_t const& other) const {
    return other.value == value;
  }

  bool operator<(IndexMD<Integer, ndim, Array> const& other) const {
    return value < other.value;
  }
};

template <typename Integer, int ndim, typename Array, typename DenseInteger=size_t>
struct IndexRangeMDMapping;

template <typename Integer, int ndim, typename Array = IntegerArray<Integer, ndim>>
struct RangeMD {
  using integer_array_t = Array;

  using mapping_to_dense = IndexRangeMDMapping<Integer, ndim, Array>;
  using index_type = IndexMD<Integer, ndim, Array>;

  RangeMD() = default;
  RangeMD(
    integer_array_t const& in_size, integer_array_t const& in_offset = integer_array_t()
  ) : size_(in_size), offset_(in_offset) { }

  size_t size() const {
    return size_.get_total_size();
  }

  integer_array_t size_, offset_;

  template <typename ArchiveT>
  void serialize(ArchiveT& ar) {
    ar | size_;
    ar | offset_;
  }
};

template <typename Integer, int ndim, typename Array, typename DenseInteger>
struct IndexRangeMDMapping {
  using integer_array_t = Array;

  IndexRangeMDMapping() = default;

  explicit IndexRangeMDMapping(RangeMD<Integer, ndim, Array> const& in_range)
    : range(in_range) { }

  using from_index_type = IndexMD<Integer, ndim, Array>;
  using to_index_type = DenseInteger;

  to_index_type map_forward(from_index_type const& from) const {
    DenseInteger val = 0;
    DenseInteger dim_size = 1;
    for (auto i = ndim-1; i >= 0; i--) {
      val += dim_size * (from.value.dims[i] - from.min_value.dims[i]);
      dim_size *= from.max_value.dims[i] - from.min_value.dims[i] + 1;
    }
    return val;
  }

  from_index_type map_backward(to_index_type const& to) const {
    Array value;
    Array const& min_value = range.offset_;
    Array const& max_value = range.offset_ + range.size_ - integer_array_t(1);

    auto span = 1;
    for (auto i = ndim-1; i >= 0; i--) {
      auto cur_span = range.size_.dims[i];
      value.dims[i] = (to / span) % cur_span;
      span *= cur_span;
    }

    return IndexMD<Integer, ndim, Array>(
      value, min_value, max_value
    );
  }

  template <typename ArchiveT>
  void serialize(ArchiveT& ar) {
    ar | range;
  }

private:
  RangeMD<Integer, ndim, Array> range;
};

template <typename Integer, int ndim, typename Array>
IndexRangeMDMapping<Integer, ndim, Array>
get_mapping_to_dense(RangeMD<Integer, ndim, Array> const& range) {
  return IndexRangeMDMapping<Integer, ndim, Array>(range);
}

template <typename Integer, int ndim, typename Array, typename DenseInteger=size_t>
DenseInteger
linearize_md(IndexMD<Integer, ndim, Array> index) {
  RangeMD<Integer, ndim, Array> range;
  for (auto i = 0; i < ndim; i++) {
    range.offset_.dims[i] = index.min_value.dims[i];
    range.size_.dims[i] = index.max_value.dims[i] - index.min_value.dims[i] + 1;
  }
  IndexRangeMDMapping<Integer, ndim, Array> map(range);
  return map.map_forward(index);
}

template <typename Array, typename... Args>
Array make_integer_array(Args&&... args) {
  Array a;
  // static_cast here to allow narrowing conversions in numeric arguments
  std::vector<typename Array::integer_type_t> arr = {
    static_cast<typename Array::integer_type_t>(args)...
  };
  for (auto i = 0; i < arr.size(); i++) {
    a.dims[i] = arr[i];
  }
  return a;
}

template <typename Range, typename... Args>
Range make_range(Args&&... args) {
  return Range(
    make_integer_array<typename Range::integer_array_t, Args...>(
      std::forward<Args>(args)...
    )
  );
}

template <typename Integer, int ndim>
struct DimensionReductionMap {
  using from_range_type = RangeMD<Integer, ndim>;
  using to_range_type = RangeMD<Integer, ndim - 1>;

  using from_index_type = IndexMD<Integer, ndim>;
  using to_index_type = IndexMD<Integer, ndim - 1>;
  using from_multi_index_type = std::vector<from_index_type>;

  Integer dim_size, dim_offset;

  // for serializability
  DimensionReductionMap() = default;

  // parameter `to' is only used for checking that ranges match
  DimensionReductionMap(
    from_range_type const& from, to_range_type const& to
  ) : dim_size(from.size_.dims[ndim-1]), dim_offset(from.offset_.dims[ndim-1]) {
    // sanity check: ndim-1 leading dimensions must match
    for (auto i = 0; i < ndim-1; i++) {
      assert(from.size_.dims[i] == to.size_.dims[i]);
      assert(from.offset_.dims[i] == to.offset_.dims[i]);
    }
  }

  to_index_type map_forward(from_index_type const& from) const {
    return from.reduce_dim();
  }

  from_multi_index_type map_backward(to_index_type const& to) const {
    from_multi_index_type vec;
    for (auto i = dim_offset; i < dim_offset + dim_size; i++) {
      vec.push_back(to.increase_dim(i, dim_offset, dim_size-1));
    }
    return vec;
  }

  template <typename ArchiveT>
  void serialize(ArchiveT& ar) {
    ar | dim_size;
    ar | dim_offset;
  }
};

#endif
