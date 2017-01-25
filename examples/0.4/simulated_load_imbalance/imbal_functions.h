/*
//@HEADER
// ************************************************************************
//
//                      imbal_functions.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMA_IMBAL_FUNCTIONS_H
#define DARMA_IMBAL_FUNCTIONS_H

//==============================================================================
// <editor-fold desc="Functions adjusting behavior"> {{{1

double do_workload(double factor) {
  //std::cout << "do workload: " << factor << std::endl;
  assert(factor >= 0.0);
  std::size_t iters = (std::size_t)(factor * 10000000);
  double value = 0;
  for (std::size_t i = 1; i <= iters; ++i) {
    value += (double)i * value;
  }
  return value;
}

void do_adjust_workload(
  double& workload,
  double adjust_scale,
  double adjust_amount
) {
  workload += adjust_scale * adjust_amount;
}

double get_adjust_scale(
  std::size_t adjustment_number,
  std::size_t total_adjustments
) {
  // Simple function, for now:
  return 1.0 / ((double)adjustment_number + 1.0);
  //return std::pow(-1.0, adjustment_number) / ((double)adjustment_number + 1.0);
}

double get_base_load_for_index(
  std::size_t index,
  std::size_t n_indices,
  std::size_t random_seed
) {
  //std::cout << "get base load for index: " << index << " " << n_indices << " " << std::endl;
  std::vector<double> loads;
  loads.reserve(n_indices);
  for (std::size_t i = 1; i <= n_indices; ++i) {
    loads.push_back((double)i);
  }

  std::random_device rd;
  std::mt19937_64 g(rd());
  g.seed(random_seed);

  std::shuffle(loads.begin(), loads.end(), g);

  return loads.at(index);

}

// </editor-fold> end Functions adjusting behavior }}}1
//==============================================================================


#endif //DARMA_IMBAL_FUNCTIONS_H
