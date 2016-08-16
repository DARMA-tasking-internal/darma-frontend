/*
//@HEADER
// ************************************************************************
//
//                      test_compressed_pair.cc
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

#include <gtest/gtest.h>

#include <darma/impl/util/compressed_pair.h>

#include "../metatest_helpers.h"

using namespace darma_runtime::detail;

struct Empty { };

STATIC_ASSERT_SIZE_IS(compressed_pair<Empty, int>, sizeof(int));
STATIC_ASSERT_SIZE_IS(compressed_pair<int, Empty>, sizeof(int));
STATIC_ASSERT_SIZE_IS(compressed_pair<int, int>, 2*sizeof(int));


TEST(TestCompressedPair, empty_int) {
  compressed_pair<Empty, int> cp1;
  cp1.second() = 25;
  ASSERT_EQ(cp1.second(), 25);
}

TEST(TestCompressedPair, int_empty) {
  compressed_pair<int, Empty> cp2(
    std::piecewise_construct,
    std::forward_as_tuple(25),
    std::forward_as_tuple()
  );
  ASSERT_EQ(cp2.first(), 25);
}

TEST(TestCompressedPair, int_int) {
  compressed_pair<int, int> cp3(25, 42);
  ASSERT_EQ(cp3.first(), 25);
  ASSERT_EQ(cp3.second(), 42);
}

TEST(TestCompressedPair, empty_empty) {
  compressed_pair<Empty, Empty> cp4;
}

