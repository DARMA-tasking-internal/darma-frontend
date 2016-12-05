/*
//@HEADER
// ************************************************************************
//
//                      square_roots.cc
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

#include <random>

#include <darma.h>
#include <darma/impl/array/index_range.h>

using namespace darma_runtime;

struct AllReduce {
  void operator()(
    ConcurrentRegionContext<Range1D<int>> context
  ) const {
    using darma_runtime::keyword_arguments_for_collectives::in_out;
    using darma_runtime::keyword_arguments_for_collectives::piece;
    using darma_runtime::keyword_arguments_for_collectives::n_pieces;

    auto red_data = initial_access<size_t>(context.index().value, "red_data");

    create_work([=]{ red_data.set_value(context.index().value); });

    int const n_elems = context.index().max_value;

    allreduce(in_out=red_data, piece=context.index().value, n_pieces=n_elems);

    if(context.index().value == 0) {
      create_work(reads(red_data), [=]{
        std::cout << "Final average: " << (red_data.get_value() / n_elems) << std::endl;
      });
    }
  }
};

void darma_main_task(std::vector<std::string> args) {
  using darma_runtime::keyword_arguments_for_create_concurrent_region::index_range;
  assert(args.size() == 2);

  size_t const num_elems = std::atoi(args[1].c_str());

  create_concurrent_region<AllReduce>(
    index_range = Range1D<int>(num_elems)
  );
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);