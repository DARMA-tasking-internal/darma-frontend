/*
//@HEADER
// ************************************************************************
//
//                          common.h
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

#ifndef EXAMPLES_STENCIL_1D_COMMON_H_
#define EXAMPLES_STENCIL_1D_COMMON_H_

/**
 * For each point with index i, do_stencil() averages
 * data[i-1], data[i], and data[i+1] and stores the new value
 * in data[i].  data should contain n_data+2 values, as the
 * values of the first and last points are unchanged
 */
void
do_stencil(double* data, size_t n_data)
{
  if(n_data == 0) return;
  double trailing = data[0];
  for(int i = 1; i <= n_data; ++i) {
    double tmp = (trailing + data[i] + data[i+1]) / 3.0;
    trailing = data[i];
    data[i] = tmp;
  }
}

/**
 * print out the data
 */
void
do_print_data(double* data, size_t total_n_data)
{
  std::cout << "Resulting data:" << std::endl;
  for(int i = 0; i < total_n_data; ++i) {
    std::cout << std::setw(8) << std::fixed << std::setprecision(4) << data[i];
  }
  std::cout << std::endl;
}




#endif /* EXAMPLES_STENCIL_1D_COMMON_H_ */
