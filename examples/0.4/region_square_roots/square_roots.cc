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
#include <darma/impl/task_collection/handle_collection.h>
#include <darma/impl/task_collection/task_collection.h>

using namespace darma_runtime;

static void escape(void* p) {
  asm volatile("" : : "g"(p) : "memory");
}

static void clobber() {
  asm volatile("" : : : "memory");
}

// The parameter to the imbalance chi-squared distribution
static constexpr double imbalance_param = 3.0;

// Truncate the imbalance distribution at this multiplier
static constexpr double max_imbalance = 100.0;

struct SquareRoots {
  void operator()(
    Index1D<int> index,
    int iteration, int change_interval,
    int average_n_sqrts
  ) const {

    clobber();

    std::random_device rd;
    std::mt19937 gen(rd());
    gen.seed(index.value * (index.max_value + 1) + iteration/change_interval);
    std::uniform_real_distribution<> value_dis(1.0, 2.0);
    std::chi_squared_distribution<> imbalance_dis(1.0);
    // The chi-squared distribution has a mean equal to it's parameter, so
    // if we divide out the parameter, we'll get a mean at the average number of
    // square roots requested
    int n_sqrts = (int)((
      double(average_n_sqrts) * std::min(max_imbalance, imbalance_dis(gen))
    ) / imbalance_param);

    std::vector<double> results;
    for(int i = 0; i < n_sqrts; ++i) {
      results.push_back(
        std::sqrt(value_dis(gen))
      );
    }

    escape(results.data());

  }
};

void darma_main_task(std::vector<std::string> args) {
  using darma_runtime::keyword_arguments_for_create_concurrent_region::index_range;

  assert(args.size() == 5);

  size_t const num_ranks = std::atoi(args[1].c_str());
  size_t const num_iters = std::atoi(args[2].c_str());
  size_t const change_interval = std::atoi(args[3].c_str());
  size_t const average_per_iter = std::atoi(args[4].c_str());

  for(size_t iter = 0; iter < num_iters; ++iter) {
    create_concurrent_work<SquareRoots>(
      iter, change_interval, average_per_iter,
      index_range=Range1D<int>(num_ranks)
    );
  }
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
