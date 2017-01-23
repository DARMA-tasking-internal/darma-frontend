/*
//@HEADER
// ************************************************************************
//
//                      fib_region.cc
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

#include <chrono>
#include <random>
#include <cmath>

#include <darma.h>

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments;


//==============================================================================
// <editor-fold desc="Functions adjusting behavior"> {{{1

double do_workload(double factor) {
  //std::cout << "do workload: " << factor << std::endl;
  assert(factor >= 0.0);
  std::size_t iters = (std::size_t)(factor * 1000);
  double value = 0;
  for (std::size_t i = 1; i <= iters; ++i) {
    value += std::sqrt(i * 3.14);
  }
  return value;
}

void do_adjust_workload(
  double& workload,
  double adjust_scale,
  double adjust_amount
) {
  workload += adjust_scale * adjust_amount;
}

double get_adjust_scale(
  std::size_t adjustment_number,
  std::size_t total_adjustments
) {
  // Simple function, for now:
  return 1.0 / ((double)adjustment_number + 1.0);
  //return std::pow(-1.0, adjustment_number) / ((double)adjustment_number + 1.0);
}

double get_base_load_for_index(
  std::size_t index,
  std::size_t n_indices,
  std::size_t random_seed
) {
  //std::cout << "get base load for index: " << index << " " << n_indices << " " << std::endl;
  std::vector<double> loads;
  loads.reserve(n_indices);
  for (std::size_t i = 1; i <= n_indices; ++i) {
    loads.push_back((double)i);
  }

  std::random_device rd;
  std::mt19937_64 g(rd());
  g.seed(random_seed);

  std::shuffle(loads.begin(), loads.end(), g);

  return loads.at(index);

}

// </editor-fold> end Functions adjusting behavior }}}1
//==============================================================================



//==============================================================================
// <editor-fold desc="Actual imbalance functor"> {{{1

struct ImbalancedLoad {

  void operator()(
    ConcurrentContext<Index1D<size_t>> ctxt,
    std::size_t random_seed, bool adjust_workload,
    AccessHandleCollection<double, Range1D<size_t>> workloads,
    AccessHandleCollection<double, Range1D<size_t>> values,
    double adjust_scale
  ) const {

    auto my_workload = workloads[ctxt.index()].local_access();
    auto my_value = values[ctxt.index()].local_access();

    if (adjust_workload) {
      create_work([=] {
        double base_load = get_base_load_for_index(
          ctxt.index().value, ctxt.index_count(), random_seed
        );

        do_adjust_workload(
          my_workload.get_reference(),
          adjust_scale, base_load
        );
      });
    }

    create_work(reads(my_workload), [=]{
      my_value.get_reference() += do_workload(my_workload.get_value());
    });

  }

};

// </editor-fold> end Actual imbalance functor }}}1
//==============================================================================


void darma_main_task(std::vector<std::string> args) {

  std::size_t n_iterations_total = std::atoi(args.at(1).c_str());
  std::size_t iterations_per_update = std::atoi(args.at(2).c_str());
  std::size_t n_workers = std::atoi(args.at(3).c_str());

  std::cout << "Running workload for " << n_iterations_total << " iterations, ";
  std::cout << "changing the workload every " << iterations_per_update
            << " iterations, for " << n_workers << " work indices." << std::endl;

  // TODO allow this to be a command line option
  std::size_t current_random_seed;

  auto workloads = initial_access_collection<double>("workloads",
    index_range=Range1D<std::size_t>(n_workers)
  );
  auto results = initial_access_collection<double>("results",
    index_range=Range1D<std::size_t>(n_workers)
  );

  for(std::size_t i = 0; i < n_iterations_total; ++i) {

    if(i % iterations_per_update == 0) {
      current_random_seed = static_cast<std::size_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()
      );
    }

    create_concurrent_work<ImbalancedLoad>(
      current_random_seed, i % iterations_per_update == 0,
      workloads, results,
      get_adjust_scale(i / iterations_per_update, n_iterations_total / iterations_per_update),
      index_range = Range1D<std::size_t>(n_workers)
    );
  }
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
