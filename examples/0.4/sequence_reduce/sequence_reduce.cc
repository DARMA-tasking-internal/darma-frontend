/*
//@HEADER
// ************************************************************************
//
//                    sequence_reduce.cc
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
#include <darma/impl/task_collection/handle_collection.h>
#include <darma/impl/task_collection/task_collection.h>

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_access_handle_collection;

struct AllReduce {
  void operator()(
    Index1D<int> index
  ) const {
    using darma_runtime::keyword_arguments_for_collectives::in_out;
    using darma_runtime::keyword_arguments_for_collectives::piece;
    using darma_runtime::keyword_arguments_for_collectives::n_pieces;
    using darma_runtime::keyword_arguments_for_collectives::tag;

    auto const n_elems = index.max_value + 1;
    auto const my_elm = index.value;

    auto red_data = initial_access<size_t>(my_elm, "red_data");

    create_work([=]{
      red_data.set_value(my_elm);
    });

    for (auto i = 0; i < 10; i++) {
      allreduce(in_out=red_data,piece=index.value, n_pieces=n_elems, tag("test",i));
    }

    if(my_elm == 0) {
      create_work(reads(red_data), [=]{
        std::cout << "Final sum: " << red_data.get_value() << std::endl;
      });
    }
  }
};

void darma_main_task(std::vector<std::string> args) {
  assert(args.size() == 2);

  size_t const num_elems = std::atoi(args[1].c_str());

  create_concurrent_work<AllReduce>(
    index_range=Range1D<int>(num_elems)
  );
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
