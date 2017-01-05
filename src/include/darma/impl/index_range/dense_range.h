/*
//@HEADER
// ************************************************************************
//
//                      dense_range.h
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

#ifndef DARMA_IMPL_INDEX_RANGE_DENSE_RANGE_H
#define DARMA_IMPL_INDEX_RANGE_DENSE_RANGE_H

#include <darma/interface/frontend/index_range.h>
#include <darma/impl/polymorphic_serialization.h>

namespace darma_runtime {
namespace indexing {

namespace detail {

template <typename Integer>
class basic_dense_index_range
  : public abstract::frontend::IndexRange,
    public darma_runtime::detail::PolymorphicSerializationAdapter<
      basic_dense_index_range<Integer>,
      abstract::frontend::IndexRange
    >
{
  private:

    Integer size_;

  public:

    using index_type = Integer;

    explicit basic_dense_index_range(Integer size) : size_(size) { }

    virtual std::size_t size() const override { return size_; }

};

} // end namespace detail

// TODO make this a simple case of basic_index_range
using dense_index_range = detail::basic_dense_index_range<size_t>;

} // end namespace indexing
} // end namespace darma_runtime

#endif //DARMA_IMPL_INDEX_RANGE_DENSE_RANGE_H
