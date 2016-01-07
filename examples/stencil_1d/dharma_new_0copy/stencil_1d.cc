/*
//@HEADER
// ************************************************************************
//
//                          stencil_1d.cc
//                         dharma_mockup
//              Copyright (C) 2015 Sandia Corporation
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

#include <iostream>
#include <iomanip>

#include "examples/stencil_1d/common.h"
#include "mock_interface.h"
#include "mock_dependencies.h"

constexpr size_t n_data_total = 5;
constexpr size_t n_iter = 10;
constexpr bool print_data = true;

using namespace dharma_mockup;

int main(int argc, char** argv)
{
  using namespace dharma_mockup::keyword_arguments_for_key_specification;
  using namespace dharma_mockup::keyword_arguments_for_policy_specification;
  // Convenience shortcut.  Key expressions are used everywhere, and make_key_expression would be extremely tedious
  constexpr auto Kex = dharma_mockup::make_key_expression;

  size_t me = dharma_spmd_rank();
  size_t n_spmd = dharma_spmd_size();

  // Figure out how much local data we have
  size_t my_n_data = n_data_total / n_spmd;
  if(me < n_data_total % n_spmd) ++my_n_data;

  // figure out the total amount of data that will be passed
  // to do_stencil(), including ghosts
  size_t my_total_data = my_n_data;
  if(me != 0) ++my_total_data;
  if(me != n_spmd - 1) ++my_total_data;

  // Figure out my neighbors.  If I'm 0 or n_spmd-1, I am my own
  // neighbor on the left and right
  const bool is_leftmost = me == 0;
  const size_t left_neighbor = is_leftmost ? me : me - 1;
  const bool is_rightmost = me == n_spmd - 1;
  const size_t right_neighbor = me == is_rightmost ? me : me + 1;

  Dependency<double*> data_with_ghosts("data_with_ghosts", me, overwrite_policy=sequential_semantics);

  Dependency<double*> left_ghost_buffer("left_ghost_buffer", me,
      // Names need to be thought through more here
      aliasing_policy=subscribe("ghosted_right", left_neighbor)
  );
  Dependency<double*> right_ghost_buffer("right_ghost_buffer", me,
      aliasing_policy=subscribe("ghosted_left", right_neighbor)
  );
  Dependency<double*> data("data", me, overwrite_policy=sequential_semantics);
  data_with_ghosts.includes_subsets(left_ghost_buffer, data, right_ghost_buffer, disjoint=true);

  Dependency<double*> shared_to_left("ghosted_left", me,
      overwrite_policy=aliased_in(data)
  );
  Dependency<double*> shared_to_right("ghosted_right", me,
      overwrite_policy=aliased_in(data)
  );

  // Setup the initial values, publish the initial ghosts
  create_work(
    writes(data_with_ghosts),
    [=]{
      // allocate the data
      data_with_ghosts.allocate(my_total_data);
      data.subset_offset((int)!is_leftmost, in=data_with_ghosts);
      right_ghost_buffer.subset_offset(my_n_data + (int)!is_leftmost, in=data_with_ghosts);
      left_ghost_buffer.subset_offset(0, in=data_with_ghosts);

      shared_to_left.subset_offset(0, in=data, size=!is_leftmost);
      shared_to_right.subset_offset(my_n_data, in=data, size=!is_rightmost);
    }
  );

  // Now do the stencil
  for(int iter = 0; iter < n_iter; ++iter) {

    create_work(
      reads(data_with_ghosts), writes(data),
      [=]{
        // Do the actual stencil computation
        do_stencil(data_with_ghosts.get(), my_n_data - is_leftmost - is_rightmost);
      }
    );

    data_with_ghosts = data_with_ghosts.next_version();

  }

  // Now output the data, one SPMD worker at a time
  if(print_data) {

    Dependency<> prev_node_finished_writing("write_done", me-1);

    create_work(
      [=]{
        // Wait until previous node is done writing before we write
        prev_node_finished_writing.wait();

        std::cout << "On worker " << me << ": ";
        do_print_data(data, my_n_data);

        return make_output<>("write_done", me);
      }
    );

    // If we're the first node, start the chain in motion
    if(me == 0) create_work([=]{ return make_output<>("write_done", me-1) });
  }

}

