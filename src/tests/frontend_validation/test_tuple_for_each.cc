/*
//@HEADER
// ************************************************************************
//
//                       test_tuple_for_each.cc
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
// Questions? Contact David S. Hollman (dshollm@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <darma/impl/meta/tuple_for_each.h>

using namespace darma_runtime::meta;

TEST(TupleForEach, basic) {
  std::vector<std::string> vals;
  tuple_for_each(std::forward_as_tuple("hello", "world"), [&](auto&& val) {
    vals.push_back(val);
  });
  ASSERT_EQ(vals[0], "hello");
  ASSERT_EQ(vals[1], "world");
}


TEST(TupleForEach, basic_zip) {
  using namespace ::testing;
  std::vector<std::string> vals;
  tuple_for_each_zipped(
    std::forward_as_tuple("hello 1", "world 1"),
    std::forward_as_tuple("hello 2", "world 2"),
    [&](auto&& val1, auto&& val2) {
      vals.push_back(val1);
      vals.push_back(val2);
    }
  );
  ASSERT_THAT(vals, ElementsAre("hello 1", "hello 2", "world 1", "world 2"));
}

TEST(TupleZip, basic) {
  auto t = tuple_zip(
    std::forward_as_tuple(1, 2, 3),
    std::forward_as_tuple(4, 5, 6)
  );
  static_assert(std::tuple_size<std::decay_t<decltype(t)>>::value == 3, "");
  static_assert(std::tuple_size<std::decay_t<decltype(std::get<0>(t))>>::value == 2, "");

  ASSERT_EQ(std::get<0>(std::get<0>(t)), 1);
  ASSERT_EQ(std::get<1>(std::get<0>(t)), 4);
  ASSERT_EQ(std::get<0>(std::get<1>(t)), 2);
  ASSERT_EQ(std::get<1>(std::get<1>(t)), 5);
  ASSERT_EQ(std::get<0>(std::get<2>(t)), 3);
  ASSERT_EQ(std::get<1>(std::get<2>(t)), 6);
}

TEST(TuplePopBack, basic) {
  auto t = std::make_tuple(1, 2, 3);
  auto t2 = tuple_pop_back(t);
  static_assert(std::tuple_size<std::decay_t<decltype(t)>>::value == 3, "");
  static_assert(std::tuple_size<std::decay_t<decltype(t2)>>::value == 2, "");

  ASSERT_EQ(std::get<0>(t2), 1);
  ASSERT_EQ(std::get<1>(t2), 2);
  ASSERT_EQ(std::get<0>(t), 1);
  ASSERT_EQ(std::get<1>(t), 2);
  ASSERT_EQ(std::get<2>(t), 3);
}
