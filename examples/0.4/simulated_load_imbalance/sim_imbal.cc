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

#include "imbal_functions.h"

using namespace darma;
using namespace darma::keyword_arguments;
using namespace darma_runtime::keyword_arguments_for_collectives;

//==============================================================================
// <editor-fold desc="Actual imbalance functor"> {{{1

struct Load {

  void operator()(
    Index1D<size_t> index,
    int iteration,
    std::size_t random_seed, bool adjust_workload,
    AccessHandleCollection<double, Range1D<size_t>> workloads,
    AccessHandleCollection<double, Range1D<size_t>> values,
    double adjust_scale,
    bool create_imbalance
  ) const {

    auto my_workload = workloads[index].local_access();
    auto my_value = values[index].local_access();

    if (adjust_workload) {
      create_work([=] {
        auto n_indices = index.max_value + 1;
        double base_load = create_imbalance ?
          get_base_load_for_index(
            index.value, index.max_value + 1, random_seed
          ) :
          double(n_indices*(n_indices+1)) / double(2*n_indices);

        // std::cout << "i=" << index.value
        //           << ", base_load=" << base_load
        //           << std::endl;

        do_adjust_workload(
          my_workload.get_reference(),
          adjust_scale, base_load
        );
      });
    }

    create_work(reads(my_workload), [=]{
      my_value.get_reference() += do_workload(my_workload.get_value());

      int idx = (int)index.value;
      auto val = initial_access<int>("iter finished", idx);
      create_work([=]{
        val.set_value(index.value);
      });
      allreduce(
        in_out=val, piece=index.value, n_pieces=index.max_value+1, tag(iteration,"im_iter")
      );
      val.publish(version=iteration);
    });
  }

};

// </editor-fold> end Actual imbalance functor }}}1
//==============================================================================

template <typename TimeUnits>
int64_t time_now_as() {
  auto now = std::chrono::high_resolution_clock::now();
  return std::chrono::time_point_cast<TimeUnits>(now).time_since_epoch().count();
}

void darma_main_task(std::vector<std::string> args) {

  std::size_t n_iterations_total = std::atoi(args.at(1).c_str());
  std::size_t iterations_per_update = std::atoi(args.at(2).c_str());
  std::size_t n_workers = std::atoi(args.at(3).c_str());
  bool const is_imbalanced = std::atoi(args.at(4).c_str());

  std::cout << "Running workload for " << n_iterations_total << " iterations, ";
  std::cout << "changing the workload every " << iterations_per_update
            << " iterations, for " << n_workers << " work indices, "
            << "is imbalanced = " << is_imbalanced
            << std::endl;

  std::size_t random_seed;
  if(args.size() == 6) {
    random_seed = std::atoi(args.at(5).c_str());
  }
  else {
    random_seed = 0;
  }

  std::random_device rd;
  std::mt19937_64 seed_gen(rd());
  seed_gen.seed(random_seed);

  std::size_t current_random_seed;

  auto workloads = initial_access_collection<double>("workloads",
    index_range=Range1D<std::size_t>(n_workers)
  );
  auto results = initial_access_collection<double>("results",
    index_range=Range1D<std::size_t>(n_workers)
  );

  using namespace std::chrono;

  time_point<high_resolution_clock> start = high_resolution_clock::now();

  auto start_time_now = time_now_as<std::chrono::microseconds>();
  auto start_iter_time = initial_access<double>("total time");

  AccessHandle<int> finished;
  std::size_t i = 0;
  do {
    if(i % iterations_per_update == 0) {
      if(i % iterations_per_update == 0) {
        current_random_seed = seed_gen();
      }
    }

    create_concurrent_work<Load>(
      i, current_random_seed, i % iterations_per_update == 0,
      workloads, results,
      get_adjust_scale(i / iterations_per_update, n_iterations_total / iterations_per_update),
      is_imbalanced,
      index_range = Range1D<std::size_t>(n_workers)
    );

    finished = read_access<int>("iter finished", 0, version=i);
    create_work([=]{
      finished.get_value();
      auto const time_now = time_now_as<std::chrono::microseconds>();
      auto const dt = start_iter_time.get_value() == 0 ?
        time_now - start_time_now :
        time_now - start_iter_time.get_value();
      std::cout << i << ": iter time = " << (double)dt/1e6  << "s " << std::endl;
      start_iter_time.set_value(time_now);
    });

  } while (
    ++i < n_iterations_total &&
    create_condition([=]{
      start_iter_time.get_value(); finished.get_value(); return true;
    })
  );
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
