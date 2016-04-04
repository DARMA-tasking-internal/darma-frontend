/*
//@HEADER
// ************************************************************************
//
//                          test_tuple_zip.cc
//                         dharma_new
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


#include <iostream>
#include <tuple>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <darma/impl/meta/tuple_zip.h>
#include "blabbermouth.h"

using namespace darma_runtime;

typedef EnableCTorBlabbermouth<Default, String, Copy, Move> BlabberMouth;
static MockBlabbermouthListener* listener;

////////////////////////////////////////////////////////////////////////////////

class TestTupleZip
  : public ::testing::Test
{
  protected:

    virtual void SetUp() {
      using namespace ::testing;
      listener_ptr = std::make_unique<StrictMock<MockBlabbermouthListener>>();
      BlabberMouth::listener = listener_ptr.get();
      listener = BlabberMouth::listener;
    }

    virtual void TearDown() {
      listener_ptr.reset();
    }

    std::unique_ptr<::testing::StrictMock<MockBlabbermouthListener>> listener_ptr;

};

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestTupleZip, basic_1) {

  auto t = meta::tuple_zip(
    std::make_tuple(1, 2, 3),
    std::make_tuple(4, 5, 6)
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

TEST_F(TestTupleZip, basic_2) {

  auto t1 = std::make_tuple(1, 2, 3);
  auto t2 = std::make_tuple(4, 5, 6);
  auto t = meta::tuple_zip(t1, t2);

  static_assert(std::tuple_size<std::decay_t<decltype(t)>>::value == 3, "");
  static_assert(std::tuple_size<std::decay_t<decltype(std::get<0>(t))>>::value == 2, "");

  ASSERT_EQ(std::get<0>(std::get<0>(t)), 1);
  ASSERT_EQ(std::get<1>(std::get<0>(t)), 4);
  ASSERT_EQ(std::get<0>(std::get<1>(t)), 2);
  ASSERT_EQ(std::get<1>(std::get<1>(t)), 5);
  ASSERT_EQ(std::get<0>(std::get<2>(t)), 3);
  ASSERT_EQ(std::get<1>(std::get<2>(t)), 6);
  ASSERT_EQ(std::get<0>(t1), 1);
  ASSERT_EQ(std::get<1>(t1), 2);
  ASSERT_EQ(std::get<2>(t1), 3);
  ASSERT_EQ(std::get<0>(t2), 4);
  ASSERT_EQ(std::get<1>(t2), 5);
  ASSERT_EQ(std::get<2>(t2), 6);

}

TEST_F(TestTupleZip, basic_fwd) {

  auto t = meta::tuple_zip(
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

TEST_F(TestTupleZip, blabber_1) {
  using namespace ::testing;

  InSequence s;
  EXPECT_CALL(*listener, string_ctor()).Times(4);

  auto t1 = std::make_tuple(BlabberMouth("hello1"), BlabberMouth("hello2"));
  auto t2 = std::make_tuple(BlabberMouth("hello3"), BlabberMouth("hello4"));
  auto t = meta::tuple_zip(t1, t2);

  ASSERT_EQ(std::get<0>(std::get<0>(t)).data, "hello1");
  ASSERT_EQ(std::get<1>(std::get<0>(t)).data, "hello3");
  ASSERT_EQ(std::get<0>(std::get<1>(t)).data, "hello2");
  ASSERT_EQ(std::get<1>(std::get<1>(t)).data, "hello4");
}

TEST_F(TestTupleZip, blabber_2) {
  using namespace ::testing;

  InSequence s;
  EXPECT_CALL(*listener, string_ctor()).Times(4);

  auto t = meta::tuple_zip(
    std::forward_as_tuple(BlabberMouth("hello1"), BlabberMouth("hello2")),
    std::forward_as_tuple(BlabberMouth("hello3"), BlabberMouth("hello4"))
  );

  ASSERT_EQ(std::get<0>(std::get<0>(t)).data, "hello1");
  ASSERT_EQ(std::get<1>(std::get<0>(t)).data, "hello3");
  ASSERT_EQ(std::get<0>(std::get<1>(t)).data, "hello2");
  ASSERT_EQ(std::get<1>(std::get<1>(t)).data, "hello4");
}

TEST_F(TestTupleZip, basic_lvalue) {

  int val = 2;
  auto t = meta::tuple_zip(
    std::forward_as_tuple(1, val, 3),
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

  std::get<0>(std::get<1>(t)) = 42;
  ASSERT_EQ(val, 42);
}
