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

#include "helpers.h"

#include <darma.h>

struct TestBackend
  : testing::Test
{
 protected:
  virtual void SetUp() {
    using namespace darma_runtime;

    // Emulate argc and argv
    argc_ = 1;
    argv_ = new char*[1];
    argv_[0] = new char[256];
    sprintf(argv_[0], "<mock frontend test>");

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
    delete[] argv_[0];
    delete[] argv_;
  }

 protected:
  int argc_;
  char** argv_;
};

#endif //DARMA_TEST_BACKEND_H_H
