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

#include <darma/serialization/serializers/standard_library/vector.h>
#include <darma/serialization/serializers/arithmetic_types.h>

#include <darma/serialization/serializers/standard_library/string.h>

#include <darma/serialization/simple_handler.h>

#include "test_simple_common.h"

using namespace darma::serialization;
using namespace ::testing;

STATIC_ASSERT_SIZABLE(SimpleSizingArchive, std::vector<int>);
STATIC_ASSERT_PACKABLE(SimplePackingArchive<>, std::vector<int>);
STATIC_ASSERT_UNPACKABLE(SimpleUnpackingArchive<>, std::vector<int>);

STATIC_ASSERT_SIZABLE(SimpleSizingArchive, std::vector<std::string>);
STATIC_ASSERT_PACKABLE(SimplePackingArchive<>, std::vector<std::string>);
STATIC_ASSERT_UNPACKABLE(SimpleUnpackingArchive<>, std::vector<std::string>);

TEST_F(TestSimpleSerializationHandler, vector_int) {
  using T = std::vector<int>;
  T input{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  auto buffer = SimpleSerializationHandler<>::serialize(input);
  auto output = SimpleSerializationHandler<>::deserialize<T>(buffer);
  EXPECT_THAT(input, ContainerEq(output));
}

TEST_F(TestSimpleSerializationHandler, vector_int_empty) {
  using T = std::vector<int>;
  T input{  };
  auto buffer = SimpleSerializationHandler<>::serialize(input);
  auto output = SimpleSerializationHandler<>::deserialize<T>(buffer);
  EXPECT_THAT(input, ContainerEq(output));
}

TEST_F(TestSimpleSerializationHandler, vector_string_empty) {
  using T = std::vector<std::string>;
  T input{  };
  auto buffer = SimpleSerializationHandler<>::serialize(input);
  auto output = SimpleSerializationHandler<>::deserialize<T>(buffer);
  EXPECT_THAT(input, ContainerEq(output));
}

TEST_F(TestSimpleSerializationHandler, vector_string) {
  using T = std::vector<std::string>;
  T input{ "hello", "world" };
  auto buffer = SimpleSerializationHandler<>::serialize(input);
  auto output = SimpleSerializationHandler<>::deserialize<T>(buffer);
  EXPECT_THAT(input, ContainerEq(output));
}

TEST_F(TestSimpleSerializationHandler, vector_long_strings) {
  using T = std::vector<std::string>;
  T input{
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Duis laoreet dui et odio tincidunt, nec imperdiet erat consectetur. Donec feugiat, leo sit amet dictum mollis, enim ligula sagittis mi, sagittis feugiat elit magna id nunc. Donec commodo magna eu nisl semper mollis. Aliquam erat volutpat. Ut quis turpis efficitur, cursus felis id, suscipit urna. Praesent vehicula consequat arcu, at semper neque malesuada in. Donec interdum finibus sem, rutrum varius eros feugiat vulputate.\n",
    "Maecenas quis justo eget diam scelerisque pharetra semper eu ipsum. Cras in sollicitudin nibh. Nulla ut dolor vitae nunc vehicula vestibulum ac non leo. Suspendisse orci enim, viverra tempor blandit ut, varius sed enim. Donec magna diam, molestie eget consequat eu, suscipit ut est. Integer auctor odio ut elit malesuada, sed hendrerit tortor vulputate. Aliquam purus lorem, posuere in vulputate sed, lacinia at metus. Phasellus bibendum metus at nisl lacinia, at varius mi interdum. Maecenas ipsum felis, viverra in eleifend in, molestie in est. Ut tempor bibendum mollis. Nunc nec ipsum viverra, rhoncus ante ac, dapibus augue. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Integer dictum quam a diam tristique, id mattis ante pellentesque. Aliquam fermentum ac enim a facilisis. Pellentesque nisl sapien, pellentesque vitae varius eu, hendrerit ac augue. Mauris a tempor arcu.\n",
    "Cras vitae semper lorem. Aenean viverra urna sit amet risus faucibus, ut feugiat dui consequat. Nam quis risus sit amet sem semper bibendum. Sed pharetra ac erat eu fermentum. Sed eget tincidunt sem. Duis ullamcorper in neque tempor egestas. Phasellus malesuada nulla erat, non pellentesque tellus condimentum eget.\n",
    "Nam massa velit, lobortis quis nulla quis, ullamcorper porta metus. Suspendisse potenti. Donec hendrerit scelerisque erat, eget dapibus justo vulputate et. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Aliquam fermentum, mi quis mattis porta, lorem augue gravida odio, volutpat vehicula nulla ex eget diam. Sed velit mauris, lacinia a enim id, commodo ullamcorper est. Proin non hendrerit neque, eu dignissim est. Vivamus finibus metus vitae lorem tincidunt scelerisque. Nullam egestas est ac ultrices tempor. Sed vel turpis in velit gravida semper. Vestibulum commodo suscipit nisi, quis cursus odio convallis sed.\n",
    "Morbi justo eros, tristique at risus sit amet, bibendum dapibus ex. Morbi quis enim lobortis, suscipit risus mattis, imperdiet ipsum. Fusce justo enim, rhoncus et bibendum in, commodo quis turpis. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Vestibulum arcu odio, dignissim et purus sed, iaculis luctus nisl. Suspendisse varius purus vitae vulputate porta. Pellentesque feugiat, nisl non dictum pharetra, risus quam porttitor ex, mollis fringilla purus augue in nunc. Suspendisse eu nisl eget nisl luctus tincidunt. Pellentesque tempus erat pretium est iaculis varius."
  };
  auto buffer = SimpleSerializationHandler<>::serialize(input);
  auto output = SimpleSerializationHandler<>::deserialize<T>(buffer);
  EXPECT_THAT(input, ContainerEq(output));
}
