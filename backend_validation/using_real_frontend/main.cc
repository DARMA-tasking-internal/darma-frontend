/*
//@HEADER
// ************************************************************************
//
//                          main.cc
//                         darma
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
// Questions? Contact Nicole Slattengren (nlslatt@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include "helpers.h"
#include "test_backend.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <atomic>

#include <darma/interface/defaults/darma_main.h>

#include <darma.h>

std::atomic<int> mydata::count_(0);
std::vector<std::string> TestBackend::orig_args_;

int darma_main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::InitGoogleMock(&argc, argv);
  TestBackend::store_cmdline_args(argc, argv);
  int ret = RUN_ALL_TESTS();
  ::darma_runtime::detail::backend_runtime = 0;  // make sure main() doesn't double-delete
#ifdef DARMA_COVERAGE
  // returning an error code prevents coverage from being computed
  if (ret)
    std::cerr << "WARNING: Actual error code is " << ret << "; "
              << "however, returning 0 for coverage purposes\n";
  return 0;
#else
  return ret;
#endif
}

