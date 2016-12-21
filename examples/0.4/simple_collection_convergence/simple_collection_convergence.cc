/*
//@HEADER
// ************************************************************************
//
//             simple_collection_convergence.cc
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

#include <stdlib.h>

#include <darma.h>
#include <darma/impl/task_collection/handle_collection.h>
#include <darma/impl/task_collection/task_collection.h>
#include <darma/impl/array/index_range.h>
#include <assert.h>

#define DEBUG_PRINT_CONV 0

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
using namespace darma_runtime::keyword_arguments_for_collectives;
using namespace darma_runtime::keyword_arguments_for_publication;

constexpr const double converged_constant = 1000.;
constexpr const double weight = 0.1;

struct SimpleCollectionInit {
  void operator()(
    Index1D<int> index,
    AccessHandleCollection<double, Range1D<int>> c1
  ) {
    auto handle = c1[index].local_access();
    handle.set_value(index.value * weight);
  }
};

struct SimpleCollectionIter {
  void operator()(
    Index1D<int> index,
    AccessHandleCollection<double, Range1D<int>> c1,
    AccessHandleCollection<double, Range1D<int>> test_convergence,
    size_t iter
  ) {
    auto handle = c1[index].local_access();
    auto const elems = index.max_value + 1;

    handle.set_value(
      handle.get_value() + (index.value * weight)
    );

    auto test_convergence_local = test_convergence[index].local_access();
    test_convergence_local.set_value(
      handle.get_value()
    );

    allreduce(
      in_out=test_convergence_local, piece=index.value, n_pieces=elems, tag=iter
    );

    create_work([=]{
      #if DEBUG_PRINT_CONV
      std::cout << "iter=" << iter << ", "
                << "index=" << index.value << ", "
                << "local value=" << handle.get_value() << ", "
                << "sum=" << test_convergence_local.get_value()
                << std::endl;
      #endif
      if (index.value == 0) {
        auto converged_result = initial_access<bool>("converged");
        create_work([=]{
          converged_result.set_value(
            test_convergence_local.get_value() > converged_constant
          );
          converged_result.publish(version=iter);
        });
      }
    });
  }
};

int n_sum(int const n) {
  return (n*(n-1))/2;
}

void darma_main_task(std::vector<std::string> args) {
  assert(args.size() == 3);

  size_t const col_size = std::atoi(args[1].c_str());
  size_t const max_iter = std::atoi(args[2].c_str());

  auto c1 = initial_access_collection<double>(
    "data", index_range=Range1D<int>(col_size)
  );
  auto test_conv = initial_access_collection<double>(
    "test convergence", index_range=Range1D<int>(col_size)
  );

  create_concurrent_work<SimpleCollectionInit>(
    c1, index_range=Range1D<int>(col_size)
  );

  size_t iter = 0;
  AccessHandle<bool> converged;

  do {
    create_concurrent_work<SimpleCollectionIter>(
      c1, test_conv, iter, index_range=Range1D<int>(col_size)
    );

    converged = read_access<bool>("converged", version=iter);
  } while (
    create_condition([=]{ return !converged.get_value(); }) &&
    ++iter < max_iter
  );

  if (iter != max_iter) {
    auto const sum_single_iter = n_sum(col_size) * weight;
    auto const required_iter = converged_constant / sum_single_iter;
    auto const actual_iter = iter + 1; // +1 for initialization

    std::cout << "Converged at iteration=" << actual_iter << std::endl;

    std::cout << "Closed form: "
              << "sum_single_iter=" << sum_single_iter << ", "
              << "required_iter=" << required_iter << ", "
              << "actual_iter=" << iter+1
              << std::endl;

    assert((int)required_iter == actual_iter);
  } else {
    std::cout << "Reached max_iter=" << max_iter << std::endl;
  }
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
