/*
//@HEADER
// ************************************************************************
//
//                 simple_collection_remap.cc
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

#include <vector>
#include <numeric>
#include <algorithm>

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_access_handle_collection;

enum Phase {
  Identity, ReadIdentity, ReadReverse
};

struct ReverseMap {
  using from_index_type = Index1D<int>;
  using to_index_type = Index1D<int>;
  using is_index_mapping = std::true_type;

  to_index_type map_forward(from_index_type const& from) const {
    return Index1D<int>{from.max_value - from.value};
  }

  from_index_type map_backward(to_index_type const& to) const {
    return Index1D<int>{to.max_value - to.value};
  }
};

constexpr const size_t block_factor = 4;

template <size_t Factor, bool reverse=false>
struct MultiBlockMap {
  using from_index_type = Index1D<int>;
  using from_multi_index_type = std::vector<Index1D<int>>;
  using to_index_type = Index1D<int>;
  using is_index_mapping = std::true_type;

  to_index_type map_forward(from_index_type const& from) const {
    if (reverse) {
      return Index1D<int>{(from.max_value - from.value) / Factor};
    } else {
      return Index1D<int>{from.value / Factor};
    }
  }

  from_multi_index_type map_backward(to_index_type const& to) const {
    std::vector<Index1D<int>> vec(Factor);
    if (reverse) {
      std::iota(vec.begin(), vec.end(), (to.max_value - to.value)*Factor);
    } else {
      std::iota(vec.begin(), vec.end(), to.value*Factor);
    }
    return vec;
  }
};

template <typename Index, typename Handle>
void print_value(Phase const& phase, Index const& index, Handle const& handle) {
  std::cout << "phase=" << phase << ", "
            << "index=" << index.value << ", "
            << "handle=" << handle.get_value()
            << std::endl;
}

template <typename Index, typename HandleCollection>
void operate_blocked_range(
  Index const& index, HandleCollection const& hcol, Phase const& phase
) {
  std::vector<int> hi(block_factor);
  auto offset = phase == ReadReverse ? index.max_value - index.value : index.value;
  std::iota(hi.begin(), hi.end(), offset*block_factor);
  std::for_each(hi.begin(), hi.end(), [&](int const& n){
    auto d_handle = hcol[n].local_access();
    if (phase == ReadIdentity) {
      assert(d_handle.get_value() == index.value);
    } else if (phase == Identity) {
      d_handle.set_value(index.value);
    } else if (phase == ReadReverse) {
      assert(d_handle.get_value() == index.max_value - index.value);
    }
  });
}

struct PhaseCol {
  void operator()(
    Index1D<int> index,
    AccessHandleCollection<int, Range1D<int>> c1,
    AccessHandleCollection<int, Range1D<int>> c2,
    Phase const phase
  ) {
    using darma_runtime::keyword_arguments_for_publication::version;

    switch (phase) {
    case Identity: {
      auto handle = c1[index].local_access();
      handle.set_value(index.value);
      print_value(phase, index, handle);

      operate_blocked_range(index, c2, phase);

      break;
    }
    case ReadIdentity: {
      auto handle = c1[index].local_access();
      print_value(phase, index, handle);
      assert(handle.get_value() == index.value);

      operate_blocked_range(index, c2, phase);

      break;
    }
    case ReadReverse: {
      auto handle = c1[index.max_value - index].local_access();
      print_value(phase, index, handle);
      assert(handle.get_value() == index.max_value - index.value);

      operate_blocked_range(index, c2, phase);

      break;
    }
    }
  }
};

constexpr const int num_iterations = 3;

void darma_main_task(std::vector<std::string> args) {
  assert(args.size() == 2);

  auto const range_size = std::atoi(args[1].c_str());
  auto const range = Range1D<int>(range_size);

  auto const d_range = Range1D<int>(range_size*block_factor);
  auto const d_map = MultiBlockMap<block_factor>();
  auto const d_map_reverse = MultiBlockMap<block_factor, true>();

  auto c1 = initial_access_collection<int>("data", index_range=range);
  auto c2 = initial_access_collection<int>("data2", index_range=d_range);

  create_concurrent_work<PhaseCol>(
    c1, c2.mapped_with(d_map), Phase::Identity,
    index_range=range
  );

  for (auto i = 0; i < num_iterations; i++) {
    create_concurrent_work<PhaseCol>(
      c1, c2.mapped_with(d_map), Phase::ReadIdentity,
      index_range=range
    );
    create_concurrent_work<PhaseCol>(
      c1.mapped_with(ReverseMap()), c2.mapped_with(d_map_reverse),
      Phase::ReadReverse, index_range=range
    );
  }

}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
