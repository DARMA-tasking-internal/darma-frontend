/*
//@HEADER
// ************************************************************************
//
//              simple_collection_ordering.cc
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

#include <darma.h>
#include <darma/impl/task_collection/handle_collection.h>
#include <darma/impl/task_collection/task_collection.h>
#include <darma/impl/array/index_range.h>
#include <iostream>

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_access_handle_collection;

int n_sum(int const n) {
  return (n*(n-1))/2;
}

struct FirstConcurrentWork {
  void operator()(
    Index1D<int> index,
    AccessHandleCollection<int, Range1D<int>> my_collection
  ) const {

    using darma_runtime::keyword_arguments_for_publication::version;
    using darma_runtime::keyword_arguments_for_publication::n_readers;

    int num_ranks = index.max_value + 1;
    int my_rank = index.value;

    auto my_handle = my_collection[index].local_access();

    my_handle.set_value(my_rank);

    my_handle.publish(version=0, n_readers=num_ranks-1);

    for (int rank=0 ; rank<num_ranks ; rank++) {
      if (rank != my_rank) {
        auto off_rank_handle = my_collection[rank].read_access(version=0);
        create_work([=]{
          int value = my_handle.get_value() + off_rank_handle.get_value();
          my_handle.set_value(value);

          #if DEBUG_SIMPLE_COLL_ORDERING
          std::cout << index.value
                    << ": other rank=" << rank
                    << ", first concurrent work setting val=" << my_handle.get_value()
                    << std::endl;
          #endif
        });
      }
    }

    create_work([=]{
      std::cout << index.value
                << ": first concurrent work val=" << my_handle.get_value()
                << std::endl;
      assert(my_handle.get_value() == n_sum(num_ranks));
    });
  }
};

struct SecondConcurrentWork {
  void operator()(
    Index1D<int> index,
    AccessHandleCollection<int, Range1D<int>> my_collection
  ) const {
    int num_ranks = index.max_value + 1;

    auto my_handle = my_collection[index].local_access();

    std::cout << index.value
              << ": second concurrent work val=" << my_handle.get_value()
              << std::endl;

    assert(my_handle.get_value() == n_sum(num_ranks));
  }
};

void darma_main_task(std::vector<std::string> args) {
  if (args.size() > 1 && args[1] == "--help"){
    std::cout << "Usage: ./simple_collection_ordering [Collection Size (int)]"
              << std::endl;
    return;
  }

  auto const num_ranks = std::atoi(args[1].c_str());

  auto my_collection = initial_access_collection<int>(
    "test", index_range=Range1D<int>(num_ranks)
  );

  create_concurrent_work<FirstConcurrentWork>(
    my_collection, index_range=Range1D<int>(num_ranks)
  );

  create_concurrent_work<SecondConcurrentWork>(
    my_collection, index_range=Range1D<int>(num_ranks)
  );
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
