/*
//@HEADER
// ************************************************************************
//
//                      range_2d.h
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

#ifndef DARMA_IMPL_ARRAY_RANGE_2D_H
#define DARMA_IMPL_ARRAY_RANGE_2D_H

#include <cassert>
#include <type_traits>

#include <darma/impl/polymorphic_serialization.h>

namespace darma_runtime {

// TODO this could obviously be generalized substantially

template <typename Integer>
struct Index2D {
  private:
    Integer idxs[2];
  public:
    Index2D() = default;
    Index2D(Integer const& in_x, Integer const& in_y) { idxs[0] = in_x; idxs[1] = in_y; }
    Integer const& x() const { return idxs[0]; }
    Integer const& y() const { return idxs[1]; }
    Integer const& component(int i) const {
      assert(i == 0 || i == 1);
      return idxs[i];
    }
    Integer const* const components() const {
      return idxs;
    }
};

template <typename Integer, typename DenseIndex = size_t>
struct Range2DDenseMapping;

template <typename Integer>
struct Range2D
  : detail::PolymorphicSerializationAdapter<
      Range2D<Integer>,
      abstract::frontend::IndexRange
    >
{
  private:

    Integer begin_[2], end_[2];

    template <typename, typename>
    friend class Range2DDenseMapping;

  public:

    using is_index_range_t = std::true_type;
    using mapping_to_dense_t = Range2DDenseMapping<Integer>;
    using index_t = Index2D<Integer>;

    Range2D() = default;

    Range2D(Integer end1, Integer end2) {
      begin_[0] = Integer(0);
      begin_[1] = Integer(0);
      end_[0] = end1;
      end_[1] = end2;
    }

    Range2D(
      Integer begin1, Integer end1,
      Integer begin2, Integer end2
    ) {
      begin_[0] = begin1;
      begin_[1] = end1;
      end_[0] = end1;
      end_[1] = end2;
    }

    Integer const&
    begin_of_dimension(int i) const {
      assert(i == 0 || i == 1);
      return begin_[i];
    }

    Integer const&
    end_of_dimension(int i) const {
      assert(i == 0 || i == 1);
      return end_[i];
    }

    template <typename ArchiveT>
    void serialize(ArchiveT& ar) {
      ar | begin_ | end_;
    }

    size_t size() const override {
      return (end_[1] - begin_[1]) * (end_[0] - begin_[0]);
    }

};


// just do row-major for now; could be generalized, of course
template <typename Integer, typename DenseIndex>
struct Range2DDenseMapping {

  private:

    Range2D<Integer> full_range;

  public:


    Range2DDenseMapping(
      Range2D<Integer> const& range
    ) : full_range(range)
    { }

    friend class Range2D<Integer>;

    Range2DDenseMapping() = default;

    using is_index_mapping = std::true_type;
    using from_index_type = Index2D<Integer>;
    using to_index_type = DenseIndex;

    to_index_type map_forward(from_index_type const& from) const {
      return (from.x() - full_range.begin_of_dimension(0))
        * (full_range.end_of_dimension(1) - full_range.begin_of_dimension(1))
          + (from.y() - full_range.begin_of_dimension(1));

    }

    from_index_type map_reverse(to_index_type const& to_idx) const {
      const Integer y_size = full_range.end_of_dimension(1) - full_range.begin_of_dimension(1);
      assert(full_range.size() != 0);
      return Index2D<Integer>(
        (to_idx / y_size) + full_range.begin_of_dimension(0),
        (to_idx % y_size) + full_range.begin_of_dimension(1)
      );
    }

    template <typename ArchiveT>
    void serialize(ArchiveT& ar) {
      ar | full_range;
    }
};


template <typename Integer>
Range2DDenseMapping<Integer> get_mapping_to_dense(
  Range2D<Integer> const& range
) {
  return Range2DDenseMapping<Integer>(range);
}





} // end namespace darma_runtime

#endif //DARMA_IMPL_ARRAY_RANGE_2D_H
