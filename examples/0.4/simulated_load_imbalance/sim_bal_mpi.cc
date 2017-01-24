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

#include <mpi.h>

#include "imbal_functions.h"

//==============================================================================
// <editor-fold desc="Actual imbalance functor"> {{{1

struct BalancedLoad {

  void operator()(
    std::size_t index, std::size_t n_indices,
    std::size_t random_seed, bool adjust_workload,
    double& workload, double& value, double adjust_scale
  ) const {

    if (adjust_workload) {
      create_work([=]{
        double base_load = double(n_indices*(n_indices+1)) / double(2*n_indices);
        do_adjust_workload(workload, adjust_scale, base_load);
      });
    }

    value += do_workload(workload);

  }

};

// </editor-fold> end Actual imbalance functor }}}1
//==============================================================================


int main(int argc, char** argv) {

  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  assert(argc == 4 or argc == 5);

  std::size_t n_iterations_total = std::atoi(args[1]);
  std::size_t iterations_per_update = std::atoi(args[2]);
  std::size_t n_workers = std::atoi(args[3]);

  std::cout << "Running workload for " << n_iterations_total << " iterations, ";
  std::cout << "changing the workload every " << iterations_per_update
            << " iterations, for " << n_workers << " work indices." << std::endl;


  std::size_t random_seed;
  if(argc == 5) {
    random_seed = std::atoi(args[4]);
  }
  else {
    random_seed = 0;
  }

  std::random_device rd;
  std::mt19937_64 seed_gen(rd());
  seed_gen.seed(random_seed);

  std::size_t current_random_seed;

  int my_offset = ((n_workers / size) * rank) + std::min(n_workers % size, rank);
  int my_size = (n_workers / size) + int(rank < n_workers % size);

  double workloads[my_size];
  double results[my_size];

  for(int i = 0; i < my_size; ++i) {
    workloads[i] = results[i] = 0.0;
  }

  for(std::size_t i = 0; i < n_iterations_total; ++i) {

    if(i % iterations_per_update == 0) {
      current_random_seed = seed_gen();
    }

    for(int my_idx = my_offset; my_idx < my_offset + my_size; ++my_idx) {

      ImbalancedLoad()(
        my_idx, n_workers,
        current_random_seed, i % iterations_per_update == 0,
        workloads[my_idx - my_offset], results[my_idx - my_offset],
        get_adjust_scale(i / iterations_per_update, n_iterations_total / iterations_per_update)
      );

    }

  }

  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Finalize();

}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
