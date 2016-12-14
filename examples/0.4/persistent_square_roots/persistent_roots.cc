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

static void escape(void* p) {
  asm volatile("" : : "g"(p) : "memory");
}

static void clobber() {
  asm volatile("" : : : "memory");
}


struct SquareRoots {
  void operator()(
    ConcurrentRegionContext<Range1D<int>> context,
    int iteration, int change_interval,
    int min_per_iter, int max_per_iter
  ) const {

    using darma_runtime::keyword_arguments_for_publication::version;
    using darma_runtime::keyword_arguments_for_publication::region_context;
    using darma_runtime::keyword_arguments_for_publication::reader_hint;

    AccessHandle<double> averaged;
    if(iteration == 0) averaged = initial_access<double>(context.index().value, "average");
    else averaged = acquire_ownership<double>(context.index().value, "average", version=iteration);

    create_work([=]{
      std::random_device rd;
      std::mt19937 gen(rd());
      gen.seed(context.index().value * context.index().max_value + iteration/change_interval);
      std::uniform_int_distribution<> dis(min_per_iter, max_per_iter);
      std::uniform_real_distribution<> value_dis(1.0, 2.0);
      int n_sqrts = dis(gen);

      double results_accumulated = 0.0;
      for(int i = 0; i < n_sqrts; ++i) {
        results_accumulated += std::sqrt(value_dis(gen));
      }

      averaged.set_value(
        ((averaged.get_value() * iteration) + results_accumulated/n_sqrts) / (iteration+1)
      );

    });

    averaged.publish_out(reader_hint=context.index(),
      region_context=context, version=iteration+1
    );

  }
};

struct GatherResults {
  void operator()(
    ConcurrentRegionContext<Range1D<int>> context,
    int num_iters
  ) const {

    using darma_runtime::keyword_arguments_for_collectives::in_out;
    using darma_runtime::keyword_arguments_for_collectives::piece;
    using darma_runtime::keyword_arguments_for_collectives::n_pieces;
    using darma_runtime::keyword_arguments_for_publication::version;

    auto averaged = acquire_ownership<double>(context.index().value, "average",
      version=num_iters
    );

    int n_idxs = context.index().max_value;

    allreduce(in_out=averaged, piece=context.index().value, n_pieces=n_idxs);

    if(context.index().value == 0) {
      create_work(reads(averaged), [=]{
        std::cout << "Final average: " << (averaged.get_value() / n_idxs) << std::endl;
      });
    }
  }

};

void darma_main_task(std::vector<std::string> args) {
  using darma_runtime::keyword_arguments_for_create_concurrent_region::index_range;
  assert(args.size() == 6);

  size_t const num_ranks = std::atoi(args[1].c_str());
  size_t const num_iters = std::atoi(args[2].c_str());
  size_t const change_interval = std::atoi(args[3].c_str());
  size_t const min_per_iter = std::atoi(args[4].c_str());
  size_t const max_per_iter = std::atoi(args[5].c_str());

  for(size_t iter = 0; iter < num_iters; ++iter) {
    create_concurrent_region<SquareRoots>(
      iter, change_interval, min_per_iter, max_per_iter,
      index_range = Range1D<int>(num_ranks)
    );
  }
  create_concurrent_region<GatherResults>(num_iters,
    index_range = Range1D<int>(num_ranks)
  );
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);