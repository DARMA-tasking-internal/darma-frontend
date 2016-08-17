/*
//@HEADER
// ************************************************************************
//
//                      details.h
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

#ifndef DARMA_IMPL_COLLECTIVE_DETAILS_H
#define DARMA_IMPL_COLLECTIVE_DETAILS_H

#include <darma/interface/frontend/collective_details.h>
#include <darma/impl/util/compressed_pair.h>

#include "reduce_op.h"

namespace darma_runtime {
namespace detail {

namespace _impl {

template <typename ReduceOp>
ReduceOp const*
_get_static_reduce_op_instance() {
  static const ReduceOp instance = { };
  return &instance;
}

} // end namespace _impl




template <typename ReduceOp>
class SimpleCollectiveDetails
  : public abstract::frontend::CollectiveDetails

{
  private:

    size_t piece_;
    size_t n_pieces_;

  public:

    SimpleCollectiveDetails(size_t piece, size_t n_pieces)
      : piece_(piece), n_pieces_(n_pieces)
    { }

    size_t
    this_contribution() const override { return piece_; }

    size_t
    n_contributions() const override { return n_pieces_; }

    abstract::frontend::ReduceOp const*
    reduce_operation() const override {
      return _impl::_get_static_reduce_op_instance<
        detail::ReduceOperationWrapper<ReduceOp, typename ReduceOp::value_type>
      >();
    }


};

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_COLLECTIVE_DETAILS_H
