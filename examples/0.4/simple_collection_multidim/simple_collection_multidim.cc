/*
//@HEADER
// ************************************************************************
//
//                simple_collection_multidim.cc
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
#include <assert.h>

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_access_handle_collection;

#include "index_md.h"

struct SimpleCollection {
  void operator()(
    IndexMD<int, 2> index,
    AccessHandleCollection<int, RangeMD<int, 3>> ahc,
    bool const first
  ) {
    // example manual loop that assumes you know the offset/size
    // not ideal because this can change
    // for (auto i = 0; i < 4; i++) {
    //   IndexMD<int, 3> idx = index.increase_dim(i, 0, 4);
    //   auto local_handle = ahc[idx].local_access();
    //   // ...
    // }

    // example more general loop
    auto range_3d = ahc.get_index_range();
    auto t_offset = range_3d.offset_.trailing();
    auto t_size = range_3d.size_.trailing();
    for (auto i = t_offset; i < t_offset + t_size; i++) {
      IndexMD<int, 3> idx = index.increase_dim(i, t_offset, t_size);
      auto local_handle = ahc[idx].local_access();
      if (first) {
        local_handle.set_value(linearize_md(idx));
      } else {
        assert(local_handle.get_value() == linearize_md(idx));
      }
    }
  }
};

void darma_main_task(std::vector<std::string> args) {
  if (args.size() > 1 && args[1] == "--help"){
    std::cout << "Usage: ./simple_collection_multidim [Size (x-dim)] [Size (y-dim)]"
              << std::endl;
    return;
  }

  assert(args.size() == 3);

  size_t const x_dim = std::atoi(args[1].c_str());
  size_t const y_dim = std::atoi(args[2].c_str());

  auto range_3d = make_range<RangeMD<int, 3>>(x_dim, y_dim, 4);
  auto range_2d = make_range<RangeMD<int, 2>>(x_dim, y_dim);

  auto dim_reduce = DimensionReductionMap<int, 3>(range_3d, range_2d);

  auto ahc = initial_access_collection<int>("test", index_range=range_3d);

  create_concurrent_work<SimpleCollection>(
    ahc.mapped_with(dim_reduce), true, index_range=range_2d
  );
  create_concurrent_work<SimpleCollection>(
    ahc.mapped_with(dim_reduce), false, index_range=range_2d
  );
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
