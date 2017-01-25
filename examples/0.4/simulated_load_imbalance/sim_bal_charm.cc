
#include <chrono>
#include <random>
#include <cmath>
#include <iostream>
#include <cassert>

#include "imbal_functions.h"

#include "sim_bal.decl.h"

/*readonly*/ CProxy_Main mainProxy;

struct Main : CBase_Main {
  std::size_t i = 0;
  CProxy_Load sim_bal_array;

  std::size_t n_iterations_total;
  std::size_t iterations_per_update;
  std::size_t random_seed;
  std::size_t current_random_seed;
  std::mt19937_64 seed_gen;
  double cur_time;

  bool is_imbalanced;

  Main(CkArgMsg* msg)  {
    CkAssert(msg->argc > 4);

    n_iterations_total = std::atoi(msg->argv[1]);
    iterations_per_update = std::atoi(msg->argv[2]);
    std::size_t n_workers = std::atoi(msg->argv[3]);
    is_imbalanced = std::atoi(msg->argv[4]);

    std::cout << "Running workload for " << n_iterations_total << " iterations, ";
    std::cout << "changing the workload every " << iterations_per_update
              << " iterations, for " << n_workers << " work indices, "
              << "is imbalanced = " << is_imbalanced
              << std::endl;

    if(msg->argc == 6) {
      random_seed = std::atoi(msg->argv[5]);
    }
    else {
      random_seed = 0;
    }

    std::random_device rd;
    std::mt19937_64 seed_gen_(rd());
    seed_gen_.seed(random_seed);
    seed_gen = seed_gen_;

    sim_bal_array = CProxy_Load::ckNew(n_workers, n_workers);
  }

  void do_iter() {
    if (i != 0) {
      std::cout << i << ": iter time = " << CkWallTimer() - cur_time  << "s " << std::endl;
    }

    if (i+1 >= n_iterations_total) {
      CkExit();
    } else {
      if(i % iterations_per_update == 0) {
        if(i % iterations_per_update == 0) {
          current_random_seed = seed_gen();
        }
      }

      cur_time = CkWallTimer();

      sim_bal_array.do_work(
        i, current_random_seed, i % iterations_per_update == 0,
        get_adjust_scale(i / iterations_per_update, n_iterations_total / iterations_per_update),
        is_imbalanced
      );
      i++;
    }
  }
};

struct Load : CBase_Load {
  double my_workload;
  double my_value;
  std::size_t n_indices;

  Load(std::size_t const in_n_indices) : n_indices(in_n_indices) {
    usesAtSync = true;

    contribute(CkCallback(CkReductionTarget(Main, do_iter), mainProxy));
  }
  Load(CkMigrateMessage*) { }

  void pup(PUP::er& p) {
    p | my_workload;
    p | my_value;
    p | n_indices;
  }

  void do_work(
    int iteration, std::size_t random_seed, bool adjust_workload,
    double adjust_scale, bool create_imbalance
  ) {
    if (adjust_workload) {
      double base_load = create_imbalance ?
        get_base_load_for_index(
          thisIndex, n_indices, random_seed
        )
        : double(n_indices*(n_indices+1)) / double(2*n_indices);

      // std::cout << "i=" << index.value
      //           << ", base_load=" << base_load
      //           << std::endl;

      do_adjust_workload(
        my_workload, adjust_scale, base_load
      );
    }

    my_value += do_workload(my_workload);

    AtSync();
  }

  void ResumeFromSync() {
    contribute(CkCallback(CkReductionTarget(Main, do_iter), mainProxy));
  }
};

#include "sim_bal.def.h"
