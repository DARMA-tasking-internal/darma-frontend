/*
//@HEADER
// ************************************************************************
//
//                      polymorphic_index_range.h
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

#ifndef DARMA_IMPL_INDEX_RANGE_POLYMORPHIC_INDEX_RANGE_H
#define DARMA_IMPL_INDEX_RANGE_POLYMORPHIC_INDEX_RANGE_H

#include <darma/interface/frontend/index_range.h>

#include "polymorphic_mapping.h"


namespace darma_runtime {

namespace indexing {

template <typename IndexT>
class PolymorphicIndexRange
  : public abstract::frontend::IndexRange
{
  public:

    //==============================================================================
    // <editor-fold desc="MappingToDense inner class"> {{{1

    class MappingToDense {
      public:

        MappingToDense() = default;

        virtual std::size_t
        map_forward(IndexT const& idx, PolymorphicIndexRange const& range) const override {
          return range.map_index_to_dense(idx);
        }

        virtual IndexT
        map_backward(size_t const& idx, PolymorphicIndexRange const& range) const override {
          return range.map_index_from_dense(idx);
        }
    };

    //==============================================================================
    // <editor-fold desc="Pure virtual methods"> {{{1

    /// Hook for MappingToDense to use
    virtual IndexT map_index_from_dense(std::size_t) const =0;

    /// Hook for MappingToDense to use
    virtual std::size_t map_index_to_dense(IndexT const&) const =0;

    /// Size of dense IndexRange that this maps to bijectively
    virtual std::size_t size() const =0;

    // </editor-fold> end Pure virtual methods }}}1
    //==============================================================================


    using mapping_to_dense = mapping_to_dense;
    using index_type = IndexT;
    using is_index_range = std::true_type;



};

//template <typename IndexT>
//std::size_t
//PolymorphicIndexRange<IndexT>::MappingToDense(
//  PolymorphicIndexRange const&
//)


} // end namespace indexing

} // end namespace darma_runtime

#endif //DARMA_IMPL_INDEX_RANGE_POLYMORPHIC_INDEX_RANGE_H