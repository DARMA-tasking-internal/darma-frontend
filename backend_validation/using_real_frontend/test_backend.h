/*
//@HEADER
// ************************************************************************
//
//                      test_backend.h.h
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

#ifndef DARMA_TEST_BACKEND_H_H
#define DARMA_TEST_BACKEND_H_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <atomic>
#include <vector>
#include <string>
#include <iostream>

#include "helpers.h"

#include <darma.h>

struct TestBackend
  : testing::Test
{
 protected:
  virtual void SetUp() {
    using namespace darma_runtime;

    // copy in argc and argv from command-line (after gtest and
    // gmock pull off their stuff)
    argc_ = orig_args_.size();
    argv_ = new char*[argc_];
    for (size_t i=0; i<argc_; i++){
      const size_t len = orig_args_[i].size();
      argv_[i] = new char[len+1];
      argv_[i][len] = 0;
      orig_args_[i].copy(argv_[i], len);
    }

    // make it obvious if darma_init() wasn't called
    detail::backend_runtime = 0;

    // track how many mydata objects leaked in this test
    mydata::reset_leaked_count();
  }

  virtual void TearDown() {
    using namespace darma_runtime;

    // make sure we didn't leak mydata objects
    // TODO: enable the line below when we properly call destructors
    //EXPECT_EQ(mydata::num_leaked(), 0);

    // delete the runtime at the end of EACH test
    if (detail::backend_runtime){
      delete detail::backend_runtime;
      detail::backend_runtime = 0;
    }

    // clean up args
    for (size_t i=0; i<argc_; i++){
      delete [] argv_[i];
    }
    delete [] argv_;
  }

 public:
  static void
  store_cmdline_args(int argc, char **argv) {
    orig_args_ = std::vector<std::string>(argv, argv+argc);
  }

 protected:
  static std::vector<std::string> orig_args_;
  int argc_;
  char** argv_;
};

#endif //DARMA_TEST_BACKEND_H_H