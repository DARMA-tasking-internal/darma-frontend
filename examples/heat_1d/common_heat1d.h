/*
//@HEADER
// ************************************************************************
//
//                          common_heat1d.h
//                         darma_mockup
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
// Questions? Contact Francesco Rizzi (fnrizzi@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef EXAMPLES_HEAT_1D_COMMON_H_
#define EXAMPLES_HEAT_1D_COMMON_H_


constexpr int n_iter  = 1;  // num of iterations in time
constexpr double deltaT = 0.0025; // time step 
constexpr double alpha  = 0.0075; // diffusivity

constexpr int nx = 10;        // total number of grid points  
constexpr double x_min = 0.0;     // domain start x 
constexpr double x_max = 1.0;     // domain end x
constexpr double deltaX = (x_max-x_min)/( (double) (nx-1) );  // grid cell spacing
constexpr double cfl = alpha * deltaT / (deltaX * deltaX );   // cfl condition for stability
static_assert( cfl < 0.5, "cfl not small enough");  
constexpr double alphadtOvdxSq = (alpha * deltaT) / (deltaX * deltaX );   // alpha * DT/ DX^2

constexpr double Tl = 100.0;     // left BC for temperature
constexpr double Tr = 10.0;      // right BC for temperature


#endif /* EXAMPLES_HEAT_1D_COMMON_H_ */
