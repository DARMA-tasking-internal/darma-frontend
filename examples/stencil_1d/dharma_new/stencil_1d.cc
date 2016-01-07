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

#include <dharma.h>

constexpr size_t n_data_total = 5;
constexpr size_t n_iter = 10;
constexpr bool print_data = true;

using namespace dharma_runtime;

int main(int argc, char** argv)
{
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

  // overwrite_policy=sequential_semantics specifies that data is automatically
  // versioned and overwritten using sequential semantics.  If a create_work()
  // that writes `data` precedes another that reads `data`, the second call will
  // implicitly depend on the first.  This implies that `data` *cannot* be accessed
  // in nonlocal scope, and that its lifetime will be strictly tied to the
  // Dependency<> object below.
  auto data = make_dependency<double*>("data", me); //, overwrite_policy=sequential_semantics);

  // Setup the initial values, publish the initial ghosts
  create_work(
    writes(data),
    [=]{
      // Declare the output dependencies
      Dependency<double*> ghosted_left, ghosted_right;

      // allocate the data
      data.allocate(my_n_data);

      // set the initial values
      memset(data.get(), 0, my_n_data*sizeof(double));
      if(me == 0) data[0] = 1.0;
      if(me == n_spmd - 1) data[my_n_data - 1] = 1.0;

      if(me != 0) {
        ghosted_left.allocate(1);
        ghosted_left[0] = data[0];
      }
      if(n_iter != 0) ghosted_left.publish("ghosted_left", me, 0, expected_fetches=1);


      if(me != n_spmd - 1) {
        ghosted_right.allocate(1);
        ghosted_right[0] = data[my_n_data - 1];
      }
      if(n_iter != 0) ghosted_right.publish("ghosted_right", me, 0, expected_fetches=1);

    }
  );

  // Now do the stencil
  for(int iter = 0; iter < n_iter; ++iter) {

    // This is the nonlocal scope mentioned above (in "nonlocal_scopes=1")
    auto left_ghost = fetch_dependency<double*>("ghosted_right", left_neighbor, iter);
    auto right_ghost = fetch_dependency<double*>("ghosted_left", right_neighbor, iter);

    create_work(
      reads_writes(data),
      [=]{
        Dependency<double*> ghosted_left, ghosted_right;

        // In this basic example, we'll just copy over the data and ghosts to a
        // contiguous block of local memory.  In more complicated examples, we'll
        // show how to avoid these copies.  Note that if we are on the far left
        // or far right, the size of the ghost will be 0 and nothing will be copied
        double data_with_ghosts[my_total_data];
        size_t offset = 0;
        memcpy(data_with_ghosts, left_ghost.get(), left_ghost.size()*sizeof(double));
        offset += left_ghost.size();
        memcpy(data_with_ghosts + offset, data.input().get(), my_n_data*sizeof(double));
        offset += my_n_data;
        memcpy(data_with_ghosts + offset, right_ghost.get(), right_ghost.size()*sizeof(double));

        // Do the actual stencil computation
        do_stencil(data_with_ghosts, my_n_data - is_leftmost - is_rightmost);

        // Now copy stuff back and publish stuff
        memcpy(data.output().get(), data_with_ghosts + left_ghost.size(), my_n_data*sizeof(double));

        // and publish the ghosts
        if(me != 0) {
          ghosted_left.allocate(1);
          ghosted_left[0] = data_with_ghosts[1];
        }
        if(iter != n_iter - 1)
          ghosted_left.publish("ghosted_left", me, iter, expected_fetches=1);

        if(me != n_spmd) {
          ghosted_right.allocate(1);
          ghosted_right[0] = data_with_ghosts[my_total_data - 2];
        }
        if(iter != n_iter - 1)
          ghosted_right.publish("ghosted_right", me, iter, expected_fetches=1);
      }
    );

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

        Dependency<>().publish("write_done", me);
      }
    );

    // If we're the first node, start the chain in motion
    if(me == 0) create_work([=]{ Dependency<>().publish("write_done", me-1) });
  }

}

