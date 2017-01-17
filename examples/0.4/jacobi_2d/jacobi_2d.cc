/*
//@HEADER
// ************************************************************************
//
//                      jacobi_2d.cc
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
// Questions? Contact Jonathan Lifflander (jliffla@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include <darma.h>
#include <darma/impl/array/index_range.h>
#include <darma/impl/array/range_2d.h>
#include <iomanip>
#include <iostream>
#include <vector>
#include <map>

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_publication;
using namespace darma_runtime::keyword_arguments_for_collectives;
using namespace darma_runtime::keyword_arguments_for_create_concurrent_region;

#define INDEX(i,j) ((i)*(size_y_bound)+(j))
#define JACOBI_DEBUG 0
#define PRINT_JACOBI 0

constexpr int const async_interval = 1;

// Maximum error tolerance
constexpr double const error_threshold = 0.004;

// Values used to initialze the outermost stencil boundaries
constexpr double const left_bound = 1.0;
constexpr double const right_bound = 1.0;
constexpr double const top_bound = 1.0;
constexpr double const bottom_bound = 1.0;

constexpr int const print_freq = 10;

// Reduction operator (boolean &&) used to determine global convergence
struct LogicalAnd {
  static constexpr auto is_indexed = false;
  template <typename BoolLike>
  void reduce(BoolLike const& src, BoolLike& dest) const {
    dest = src and dest;
  }
};

// Boundary traversal helpers
constexpr int const num_boundaries = 4;
enum Boundary { Top=2, Bottom=~Top, Left=4, Right=~Left };
static std::map<Boundary, std::pair<int, int>> boundary_dir = {
  {Right, {0, -1}}, {Left, {0, 1}}, {Top, {1, 0}}, {Bottom, {-1, 0}}
};
static std::vector<Boundary> bounds = { Top, Bottom, Left, Right };

template <typename Callable, typename Index>
void for_each_boundary(
  Index idx, int max_x, int max_y, int size_x, int size_y, Callable&& call
) {
  auto const tx = idx.x(), ty = idx.y();
  for (auto i = 0; i < num_boundaries; i++) {
    Boundary bound = bounds[i];
    if (bound == Top    && tx != 0)       call(bound, size_y);
    if (bound == Bottom && tx != max_x-1) call(bound, size_y);
    if (bound == Left   && ty != 0)       call(bound, size_x);
    if (bound == Right  && ty != max_y-1) call(bound, size_x);
  }
}

void printer(
  int const this_x, int const this_y, double* const v1,
  int const size_x_bound, int const size_y_bound
) {
  std::cout << "index=(x=" << this_x << ",y=" << this_y << ")" << std::endl;

  for (auto i = 0; i < size_x_bound; i++) {
    for (auto j = 0; j < size_y_bound; j++) {
      std::cout << std::setw(3) << std::setfill('0') << std::fixed
		<< std::setprecision(2) << v1[INDEX(i,j)] << " ";
    }
    std::cout << std::endl;
  }
}

struct PrintJacobi {
  void operator()(
    ConcurrentRegionContext<Range2D<int>> context,
    int max_x, int max_y, int size_x, int size_y, int v_in
  ) {
    auto const this_x = context.index().x(), this_y = context.index().y();
    auto const size_x_bound = size_x+2, size_y_bound = size_y+2;

    auto idx2 = v_in % 2 != 0 ? "data v1" : "data v2";
    auto v1 = acquire_ownership<std::vector<double>>(idx2, this_x, this_y, version=v_in);

    // pretty printing of the data in each block for debugging
    if (this_x == 0 && this_y == 0) {
      create_work([=]{ printer(this_x, this_y, &(*v1)[0], size_x_bound, size_y_bound); });
      for (auto i = 0; i < max_x; i++) {
	for (auto j = 0; j < max_y; j++) {
	  if (i != this_x || j != this_y) {
	    auto to_print = read_access<std::vector<double>>(idx2, i, j, version=v_in+1);
	    create_work([=]{ printer(i, j, &(*to_print)[0], size_x_bound, size_y_bound); });
	  }
	}
      }
    } else {
      v1.publish(n_readers=1, version=v_in+1);
    }
  }
};

struct InitJacobi {
  void operator()(
    ConcurrentRegionContext<Range2D<int>> context,
    int max_x, int max_y, int size_x, int size_y
  ) {
    auto const this_x = context.index().x(), this_y = context.index().y();

    #if JACOBI_DEBUG
      std::cout << "index=(x=" << this_x << ",y=" << this_y << ")" << std::endl;
    #endif

    auto v1 = initial_access<std::vector<double>>("data v1", this_x, this_y);
    auto v2 = initial_access<std::vector<double>>("data v2", this_x, this_y);

    auto const size_x_bound = size_x+2, size_y_bound = size_y+2;

    // resize the main vectors
    create_work([=]{
      v1->resize(size_x_bound*size_y_bound);
      v2->resize(size_x_bound*size_y_bound);
    });

    // initialize, touch all data, along with boundary initialization
    create_work([=]{
      double* const v1_buf = &(*v1)[0];
      double* const v2_buf = &(*v2)[0];

      for (auto i = 0; i < size_x_bound; i++) {
        for (auto j = 0; j < size_y_bound; j++) {
          v1_buf[INDEX(i,j)] = 0.0;
          v2_buf[INDEX(i,j)] = 0.0;
        }
      }
      if (this_x == 0) {
        for (auto j = 0; j < size_y_bound; j++) {
          v1_buf[INDEX(0,j)] = top_bound;
          v2_buf[INDEX(0,j)] = top_bound;
        }
      }
      if (this_x == max_x - 1) {
        for (auto j = 0; j < size_y_bound; j++) {
          v1_buf[INDEX(size_x_bound-1,j)] = bottom_bound;
          v2_buf[INDEX(size_x_bound-1,j)] = bottom_bound;
        }
      }
      if (this_y == 0) {
        for (auto i = 0; i < size_x_bound; i++) {
          v1_buf[INDEX(i,0)] = left_bound;
          v2_buf[INDEX(i,0)] = left_bound;
        }
      }
      if (this_y == max_y - 1) {
        for (auto i = 0; i < size_x_bound; i++) {
          v1_buf[INDEX(i,size_y_bound-1)] = right_bound;
          v2_buf[INDEX(i,size_y_bound-1)] = right_bound;
        }
      }
    });

    v1.publish_out(reader_hint=context.index(), region_context=context, version=0);
    v2.publish_out(reader_hint=context.index(), region_context=context, version=0);

    for_each_boundary(
      context.index(), max_x, max_y, size_x_bound, size_y_bound,
      [&](Boundary b, int bound_size){
        auto bound = initial_access<std::vector<double>>(b, this_x, this_y);
        create_work([=]{
          bound->resize(bound_size);
          // touch the boundary memory
          double* const bound_buf = &(*bound)[0];
          for (auto i = 0; i < bound_size; i++) {
            bound_buf[i] = 0.0;
          }
        });
        bound.publish_out(reader_hint=context.index(), region_context=context, version=0);
      }
    );

    auto finished_init = initial_access<bool>("init finished");
    create_work([=]{
      finished_init.set_value(true);
      allreduce<LogicalAnd>(
        in_out=finished_init, piece=this_x*max_x+this_y, n_pieces=max_x*max_y, tag="init"
      );
    });
    if (this_x == 0 && this_y == 0) {
      finished_init.publish(version="init");
    }
  }
};

struct Jacobi {
  void operator()(
    ConcurrentRegionContext<Range2D<int>> context,
    int max_x, int max_y, int size_x, int size_y, int t, bool do_reduce
  ) {
    auto const tx = context.index().x(), ty = context.index().y();
    auto const size_x_bound = size_x+2, size_y_bound = size_y+2;

    #if JACOBI_DEBUG
      std::cout << "index=(x=" << tx << ",y=" << ty << "): timestep="
                << t << std::endl;
    #endif

    if (tx == 0 && ty == 0 && t % print_freq == 0) {
      std::cout << "Jacobi: timestep=" << t << std::endl;
    }

    auto idx1 = t % 2 == 0 ? "data v1" : "data v2";
    auto idx2 = t % 2 != 0 ? "data v1" : "data v2";

    auto v1 = acquire_ownership<std::vector<double>>(idx1, tx, ty, version=t);
    auto v2 = acquire_ownership<std::vector<double>>(idx2, tx, ty, version=t);

    // publish boundaries
    for_each_boundary(
      context.index(), max_x, max_y, size_x_bound, size_y_bound,
      [&](Boundary b, int bound_size){
        auto bound = acquire_ownership<std::vector<double>>(b, tx, ty, version=t);
        create_work([=]{
          double* const v1_buf = &(*v1)[0], *bound_buf = &(*bound)[0];
          switch (b) {
          case Top:
            std::memcpy(bound_buf, v1_buf + INDEX(1,0), bound_size*sizeof(double));
            break;
          case Bottom:
            std::memcpy(bound_buf, v1_buf + INDEX(size_x_bound-2,0), bound_size*sizeof(double));
            break;
          case Left:
            for (auto i = 0; i < bound_size; i++) {
              bound_buf[i] = v1_buf[INDEX(i,1)];
            }
            break;
          case Right:
            for (auto i = 0; i < bound_size; i++) {
              bound_buf[i] = v1_buf[INDEX(i,size_y_bound-2)];
            }
            break;
          }
        });
        bound.publish(version(t,0), n_readers=1);
        bound.publish_out(version=t+1, reader_hint=context.index(), region_context=context);
      }
    );

    // read boundaries
    for_each_boundary(
      context.index(), max_x, max_y, size_x_bound, size_y_bound,
      [&](Boundary b, int bound_size){
        auto inverse = (Boundary)~b;
        auto off = boundary_dir[inverse];
        auto bound = read_access<std::vector<double>>(inverse, tx + off.first, ty + off.second, version(t,0));

        create_work([=]{
          double* const v1_buf = &(*v1)[0], *bound_buf = &(*bound)[0];
          switch (b) {
          case Bottom:
            std::memcpy(v1_buf + INDEX(size_x_bound-1,0), bound_buf, bound_size*sizeof(double));
            break;
          case Top:
            std::memcpy(v1_buf + INDEX(0,0), bound_buf, bound_size*sizeof(double));
            break;
          case Right:
            for (auto i = 0; i < bound_size; i++) {
              v1_buf[INDEX(i,size_y_bound-1)] = bound_buf[i];
            }
            break;
          case Left:
            for (auto i = 0; i < bound_size; i++) {
              v1_buf[INDEX(i,0)] = bound_buf[i];
            }
            break;
          };
        });
      }
    );

    auto local_converged = initial_access<bool>("local_converged", tx, ty);

    // run the kernel, calculating the local error along the way
    create_work([=]{
      #if JACOBI_DEBUG
        std::cout << "index=(x=" << tx << ",y=" << ty << "): KERNEL" << std::endl;
      #endif

      double* const v1_buf = &(*v1)[0], *v2_buf = &(*v2)[0];
      double diff = 0.0, local_error = 0.0;
      for (auto i = 1; i < size_x_bound-1; i++) {
        for (auto j = 1; j < size_y_bound-1; j++) {
          v2_buf[INDEX(i,j)] = 0.2 * (v1_buf[INDEX(i,j  )] + v1_buf[INDEX(i-1,j)] +
                                      v1_buf[INDEX(i,j-1)] + v1_buf[INDEX(i+1,j)] +
                                      v1_buf[INDEX(i,j+1)]);
          diff = v2_buf[INDEX(i,j)] - v1_buf[INDEX(i,j  )];
          if (diff < 0.0) {
            diff *= -1.0;
          }
          local_error = local_error > diff ? local_error : diff;
        }
      }

      local_converged.set_value(local_error <= error_threshold);
    });

    if (do_reduce) {
      // calculate the global convergence criteria
      allreduce<LogicalAnd>(in_out=local_converged, piece=tx*max_x+ty, n_pieces=max_x*max_y, tag=t);

      if (tx == 0 && ty == 0) {
        local_converged.publish(version=t);
      }
    }

    // publish for next iteration
    v1.publish_out(reader_hint=context.index(), region_context=context, version=t+1);
    v2.publish_out(reader_hint=context.index(), region_context=context, version=t+1);
  }
};

void timestep_until_conv(
  int t, int len_x, int len_y, int size_x, int size_y, int max_iter,
  AccessHandle<int> iter_conv, AccessHandle<double> total_time
) {
  using namespace std::chrono;

  time_point<high_resolution_clock> start = high_resolution_clock::now();

  for (auto t0 = t; t0 < t + async_interval; t0++) {
    create_concurrent_region<Jacobi>(
      index_range=Range2D<int>(len_x,len_y), len_x, len_y, size_x, size_y, t0, t0 == t
    );
  }

  auto converged = read_access<bool>("local_converged", 0, 0, version=t);

  create_work([=]{
    duration<double> dur(high_resolution_clock::now() - start);
    total_time.set_value(dur.count() + total_time.get_value());

    ///std::cout << "reading converged = " << converged.get_value() << std::endl;

    if (not converged.get_value()) {
      if (t + async_interval < max_iter) {
        timestep_until_conv(
          t + async_interval, len_x, len_y, size_x, size_y, max_iter, iter_conv, total_time
        );
      } else {
        iter_conv.set_value(-1);
      }
    } else {
      iter_conv.set_value(t + 1);
    }
  });
}

void darma_main_task(std::vector<std::string> args) {
  if (args.size() != 6) {
    std::cerr << "usage: " << args[0] << ": "
              << "<boxes-x> <boxes-y> <size-x> <size-y> <max-iter>"
              << std::endl;
    exit(1);
  }

  auto const len_x = std::atoi(args[1].c_str());
  auto const len_y = std::atoi(args[2].c_str());
  auto const size_x = std::atoi(args[3].c_str());
  auto const size_y = std::atoi(args[4].c_str());
  auto const max_iter = std::atoi(args[5].c_str());

  std::cout << "Running Jacobi2D with configuration: "
            << "len=[" << len_x << "," << len_y << "], "
            << "size=[" << size_x << "," << size_y << "], "
            << "max_iter=" << max_iter << std::endl;

  create_concurrent_region<InitJacobi>(
    index_range=Range2D<int>(len_x,len_y), len_x, len_y, size_x, size_y
  );

  auto finished_init = read_access<bool>("init finished", version="init");
  create_work([=]{
    // wait init initialization is finished, otherwise timing will be wrong for
    // first iteration
    std::cout << "Finished initialization = " << finished_init.get_value() << std::endl;

    auto iter_converged = initial_access<int>("final iteration");
    auto total_time = initial_access<double>("total time");
    timestep_until_conv(0, len_x, len_y, size_x, size_y, max_iter, iter_converged, total_time);

    create_work([=]{
      auto iter = iter_converged.get_value();
      if (iter == -1) {
        std::cout << "Failed to converage after " << max_iter
                  << " timesteps "
                  << std::endl;
      } else {
        std::cout << "Converged after " << iter << " timesteps"
                  << " with error theshold=" << error_threshold
                  << std::endl;
      }

      auto total_iters = iter == -1 ? max_iter : iter;
      std::cout << "Total time = " << total_time.get_value() << "s "
                << "Average time per iter = " << total_time.get_value()/total_iters << "s "
                << std::endl;


      #if PRINT_JACOBI
      std::cout << "Print Jacobi is on" << std::endl;
      create_concurrent_region<PrintJacobi>(
	index_range=Range2D<int>(len_x,len_y), len_x, len_y, size_x, size_y, total_iters
      );
      #endif
    });
  });
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
