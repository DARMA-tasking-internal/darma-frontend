/*
//@HEADER
// ************************************************************************
//
//                      test_simple_std_string.cc
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include <darma/serialization/serializers/standard_library/set.h>
#include <darma/serialization/serializers/arithmetic_types.h>

#include <darma/serialization/serializers/standard_library/string.h>

#include <darma/serialization/simple_handler.h>

#include "test_simple_common.h"

using namespace darma::serialization;
using namespace ::testing;

STATIC_ASSERT_SIZABLE(SimpleSizingArchive, std::set<int>);
STATIC_ASSERT_PACKABLE(SimplePackingArchive<>, std::set<int>);
STATIC_ASSERT_UNPACKABLE(SimpleUnpackingArchive<>, std::set<int>);

STATIC_ASSERT_SIZABLE(SimpleSizingArchive, std::set<std::string>);
STATIC_ASSERT_PACKABLE(SimplePackingArchive<>, std::set<std::string>);
STATIC_ASSERT_UNPACKABLE(SimpleUnpackingArchive<>, std::set<std::string>);

TEST_F(TestSimpleSerializationHandler, set_int) {
  using T = std::set<int>;
  T input{1, 2, 3, 4, 5};
  auto buffer = SimpleSerializationHandler<>::serialize(input);
  auto output = SimpleSerializationHandler<>::deserialize<T>(buffer);
  EXPECT_THAT(input, ContainerEq(output));
}

TEST_F(TestSimpleSerializationHandler, set_string) {
  using T = std::set<std::string>;
  T input{"hello", "there", "world", "goodbye"};
  auto buffer = SimpleSerializationHandler<>::serialize(input);
  auto output = SimpleSerializationHandler<>::deserialize<T>(buffer);
  EXPECT_THAT(input, ContainerEq(output));
}
