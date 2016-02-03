/*
//@HEADER
// ************************************************************************
//
//                          stencil_1d.cc
//                         dharma_new
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

#include <iostream>
#include <iomanip>

#include "../common.h"
#include <dharma.h>

constexpr size_t n_data_total = 5;
constexpr bool print_data = true;
constexpr double convergence_min = 0.02;

/**
 *  TODO target a "safe" mode like this for late version 0.3/early 0.4
 *  (0.3- safe namespace and behavior itself, 0.4- supporting constructs like create_while)
 */

using namespace dharma_runtime;

////////////////////////////////////////////////////////////////////////////////
// A user data type for holding mesh data

template <typename T>
struct DataArray : public ZeroCopyable<1>
{
  public:

    // Some user methods
    void allocate(size_t n_data) {
      assert(data_ == nullptr)
      if(n_data) {
        data_ = malloc(n_data * sizeof(T));
        data_size_ = n_data;
      }
    }

    T* get() { return data_; }
    const T* get() const { return data_; }

    size_t size() const { return data_size_; }

    ~DataArray() {
      if(data_size_) free(data_);
    }

  protected:

    // Interface with dharma_runtime::ZeroCopyable<N>
    void*& get_zero_copy_slot(zero_copy_slot_t slot) {
      return data_;
    }

    size_t zero_copy_slot_size(zero_copy_slot_t slot) const {
      return data_size_;
    }

    void serialize_metadata(Archive& ar) {
      ar & data_size_;
    }

  private:

    T* data_ = nullptr;

    size_t data_size_ = 0;

};

////////////////////////////////////////////////////////////////////////////////
// Two functions that copy ghost data in and out of a local contiguous buffer

typedef AccessHandle<DataArray<double>> dep_t;

void copy_in_ghost_data(double* dest,
  const dep_t left_ghost, const dep_t main, const dep_t right_ghost
) {
  memcpy(dest, left_ghost->get(), left_ghost->size()*sizeof(double));
  dest += left_ghost->size();
  memcpy(dest, main->get(), main->size()*sizeof(double));
  dest += main->size();
  memcpy(dest, right_ghost->get(), right_ghost->size()*sizeof(double));
}

void copy_out_ghost_data(const double* src,
  dep_t send_left, dep_t main, dep_t send_right
) {
  memcpy(main->get(), src, main->size()*sizeof(double));
  memcpy(send_left->get(), main->get(), send_left->size());
  memcpy(send_right->get(), main->get() + main->size() - 1, send_right->size()*sizeof(double));
}

////////////////////////////////////////////////////////////////////////////////
// an is_converged() function

  // Some cheesy convergence criteria:
  //   Converge when all values greater than the minimum convergence_min
bool is_converged(double* data, size_t n_data) {
  for(size_t i = 0; i < n_data; ++i) {
    if(data[i] <= convergence_min) return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// main() function

int main(int argc, char** argv)
{

  dharma_init(argc, argv);

  using namespace dharma_runtime::keyword_arguments_for_publication;

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

  typedef DataArray<double> data_t;
  auto data = initial_access<data_t>("data", me);
  auto sent_to_left = initial_access<data_t>("sent_to_left", me, 0);
  auto sent_to_right = initial_access<data_t>("sent_to_right", me, 0);

  auto locally_converged = initial_access<bool>("local_conv", me);

  // pseudocode for something not yet specified:
  auto globally_converged = reduction_access<bool>("global_conv");
  auto iter_dep = initial_access<int>("iter", me);

  create_work(
    writes(data, sent_to_left, sent_to_right, iter_dep),
    [=](data_t& data_v, data_t& sent_to_left_v, data_t& sent_to_right_v, int& iter) {
      data_v.allocate(my_n_data);
      double* data_ptr = data_v.get();
      memset(data_ptr, 0, my_n_data*sizeof(double));

      if(me == 0) data_ptr[0] = 1.0;
      if(me == n_spmd - 1) data_ptr[my_n_data - 1] = 1.0;

      if(!is_leftmost) {
        sent_to_left_v.allocate(1);
        sent_to_left_v.get()[0] = data_ptr[0];
      }
      if(!is_rightmost) {
        sent_to_right_v.allocate(1);
        sent_to_right_v.get()[0] = data_ptr[my_n_data - 1];
      }
      iter = 0;
    }
  );

  sent_to_left.publish(n_readers=1);
  sent_to_right.publish(n_readers=1);

  // Now do the stencil
  //while(not globally_converged.get_value()) {
  create_while(
    reads(globally_converged),
    [=](const bool& gc_val) { return gc_val; }
  )(
    reads_writes(iter_dep),
    [=](int& iter){

      auto left_ghost = read_access<data_t>("sent_to_right", left_neighbor, iter);
      auto right_ghost = read_access<data_t>("sent_to_left", right_neighbor, iter);

      sent_to_left = initial_access<data_t>("sent_to_left", me, iter+1);
      sent_to_right = initial_access<data_t>("sent_to_right", me, iter+1);

      create_work(
        reads_writes(data), writes(sent_to_left), writes(sent_to_right), writes(locally_converged),
        [=](data_t& data_v, data_t& sent_to_left_v, data_t& sent_to_right_v, bool& locally_converged_v){
          double data_with_ghosts[my_total_data];
          copy_in_ghost_data(data_with_ghosts, left_ghost, data_v, right_ghost);

          do_stencil(data_with_ghosts, my_n_data - is_leftmost - is_rightmost);

          sent_to_left_v.allocate((int)!is_leftmost);
          sent_to_right_v.allocate((int)!is_rightmost);
          copy_out_ghost_data(data_with_ghosts, sent_to_left.get_value(), data_v, sent_to_right.get_value());

          locally_converged_v = is_converged(data_v.get(), my_n_data);
        }
      );

      sent_to_left.publish(n_readers=1);
      sent_to_right.publish(n_readers=1);

      // pseudocode for something not yet specified:
      reduce(logical_and, locally_converged, globally_converged);

      ++iter;
    }
  );

  //} // end while not converged

  if(print_data) {

    auto prev_node_finished_writing = read_access<void>("write_done", me-1);

    // The `waits()` tag is equivalent to calling prev_node_finished_writing.wait() inside the lambda
    create_work(
      waits(prev_node_finished_writing),
      [=]{
        std::cout << "On worker " << me << ": ";
        do_print_data(data->get(), my_n_data);

        initial_access<void>("write_done", me).publish(n_readers=1);
      }
    );

    // If we're the first node, start the chain in motion
    if(me == 0) {
      initial_access<void>("write_done", me-1).publish(n_readers=1);
    }

  }

  dharma_finalize();


}


