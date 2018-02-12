/*
//@HEADER
// ************************************************************************
//
//                          test_tuple_zip.cc
//                         dharma_new
//              Copyright (C) 2017 NTESS, LLC
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


#include <iostream>
#include <tuple>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <darma/impl/meta/tuple_zip.h>
#include <darma/impl/util.h>
#include "blabbermouth.h"

using namespace darma_runtime;

typedef EnableCTorBlabbermouth<Default, String, Copy, Move> BlabberMouth;
//typedef EnableCTorBlabbermouth<Default, String, Copy, Move, Destructor> BlabberMouthWithDestructor;
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

#define _test_tup meta::tuple_zip( \
    std::forward_as_tuple( \
      1, 2, 3 \
    ), \
    std::forward_as_tuple( \
      4, 5, 6 \
    ) \
  )
  static_assert(std::tuple_size<std::decay_t<decltype(_test_tup)>>::value == 3, "");
  static_assert(std::tuple_size<std::decay_t<decltype(std::get<0>(_test_tup))>>::value == 2, "");

  ASSERT_EQ(std::get<0>(std::get<0>(_test_tup)), 1);
  ASSERT_EQ(std::get<1>(std::get<0>(_test_tup)), 4);
  ASSERT_EQ(std::get<0>(std::get<1>(_test_tup)), 2);
  ASSERT_EQ(std::get<1>(std::get<1>(_test_tup)), 5);
  ASSERT_EQ(std::get<0>(std::get<2>(_test_tup)), 3);
  ASSERT_EQ(std::get<1>(std::get<2>(_test_tup)), 6);

#undef _test_tup

}

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestTupleZip, basic_fwd) {

#define _test_tup meta::tuple_zip( \
    std::forward_as_tuple(1, 2, 3), \
    std::forward_as_tuple(4, 5, 6) \
  )
  static_assert(std::tuple_size<std::decay_t<decltype(_test_tup)>>::value == 3, "");
  static_assert(std::tuple_size<std::decay_t<decltype(std::get<0>(_test_tup))>>::value == 2, "");

  ASSERT_EQ(std::get<0>(std::get<0>(_test_tup)), 1);
  ASSERT_EQ(std::get<1>(std::get<0>(_test_tup)), 4);
  ASSERT_EQ(std::get<0>(std::get<1>(_test_tup)), 2);
  ASSERT_EQ(std::get<1>(std::get<1>(_test_tup)), 5);
  ASSERT_EQ(std::get<0>(std::get<2>(_test_tup)), 3);
  ASSERT_EQ(std::get<1>(std::get<2>(_test_tup)), 6);
#undef _test_tup

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestTupleZip, blabber_1) {
  using namespace ::testing;

  // The return of make_tuple will get moved if blabbermouth has a non-default destructor
  InSequence s;
  EXPECT_CALL(*listener, string_ctor()).Times(2);
  EXPECT_CALL(*listener, move_ctor()).Times(AtMost(2));
  EXPECT_CALL(*listener, string_ctor()).Times(2);
  EXPECT_CALL(*listener, move_ctor()).Times(AtMost(2));

  auto t1 = std::make_tuple(BlabberMouth("hello1"), BlabberMouth("hello2"));
  auto t2 = std::make_tuple(BlabberMouth("hello3"), BlabberMouth("hello4"));
  auto t = meta::tuple_zip(t1, t2);

  ASSERT_EQ(std::get<0>(std::get<0>(t)).data, "hello1");
  ASSERT_EQ(std::get<1>(std::get<0>(t)).data, "hello3");
  ASSERT_EQ(std::get<0>(std::get<1>(t)).data, "hello2");
  ASSERT_EQ(std::get<1>(std::get<1>(t)).data, "hello4");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestTupleZip, blabber_2) {
  using namespace ::testing;

  EXPECT_CALL(*listener, string_ctor()).Times(4);
  ASSERT_EQ(std::get<0>(std::get<0>(
    meta::tuple_zip(
      std::forward_as_tuple(BlabberMouth("hello1"), BlabberMouth("hello2")),
      std::forward_as_tuple(BlabberMouth("hello3"), BlabberMouth("hello4"))
    )
  )).data, "hello1");
  EXPECT_CALL(*listener, string_ctor()).Times(4);
  ASSERT_EQ(std::get<1>(std::get<0>(
    meta::tuple_zip(
      std::forward_as_tuple(BlabberMouth("hello1"), BlabberMouth("hello2")),
      std::forward_as_tuple(BlabberMouth("hello3"), BlabberMouth("hello4"))
    )
  )).data, "hello3");
  EXPECT_CALL(*listener, string_ctor()).Times(4);
  ASSERT_EQ(std::get<0>(std::get<1>(
    meta::tuple_zip(
      std::forward_as_tuple(BlabberMouth("hello1"), BlabberMouth("hello2")),
      std::forward_as_tuple(BlabberMouth("hello3"), BlabberMouth("hello4"))
    )
  )).data, "hello2");
  EXPECT_CALL(*listener, string_ctor()).Times(4);
  ASSERT_EQ(std::get<1>(std::get<1>(
    meta::tuple_zip(
      std::forward_as_tuple(BlabberMouth("hello1"), BlabberMouth("hello2")),
      std::forward_as_tuple(BlabberMouth("hello3"), BlabberMouth("hello4"))
    )
  )).data, "hello4");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestTupleZip, blabber_3) {
  using namespace ::testing;

  // The return of make_tuple will get moved if blabbermouth has a non-default destructor
  InSequence s;
  EXPECT_CALL(*listener, string_ctor()).Times(2 + 4*6);

  BlabberMouth b3("hello5");
  BlabberMouth b4("hello6");

#define _test_tup meta::tuple_zip( \
    std::forward_as_tuple(BlabberMouth("hello1"), BlabberMouth("hello2"), b3), \
    std::forward_as_tuple(BlabberMouth("hello3"), b4, BlabberMouth("hello4")) \
  )

  ASSERT_EQ(std::get<0>(std::get<0>(_test_tup)).data, "hello1");
  ASSERT_EQ(std::get<1>(std::get<0>(_test_tup)).data, "hello3");
  ASSERT_EQ(std::get<0>(std::get<1>(_test_tup)).data, "hello2");
  ASSERT_EQ(std::get<1>(std::get<1>(_test_tup)).data, "hello6");
  ASSERT_EQ(std::get<0>(std::get<2>(_test_tup)).data, "hello5");
  ASSERT_EQ(std::get<1>(std::get<2>(_test_tup)).data, "hello4");

#undef _test_tup

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestTupleZip, basic_lvalue) {

  int val = 2;
#define _test_tup meta::tuple_zip( \
    std::forward_as_tuple(1, val, 3), \
    std::forward_as_tuple(4, 5, 6) \
  )

  static_assert(std::tuple_size<std::decay_t<decltype(_test_tup)>>::value == 3, "");
  static_assert(std::tuple_size<std::decay_t<decltype(std::get<0>(_test_tup))>>::value == 2, "");

  ASSERT_EQ(std::get<0>(std::get<0>(_test_tup)), 1);
  ASSERT_EQ(std::get<1>(std::get<0>(_test_tup)), 4);
  ASSERT_EQ(std::get<0>(std::get<1>(_test_tup)), 2);
  ASSERT_EQ(std::get<1>(std::get<1>(_test_tup)), 5);
  ASSERT_EQ(std::get<0>(std::get<2>(_test_tup)), 3);
  ASSERT_EQ(std::get<1>(std::get<2>(_test_tup)), 6);

  std::get<0>(std::get<1>(_test_tup)) = 42;
  ASSERT_EQ(val, 42);

#undef _test_tup
}

////////////////////////////////////////////////////////////////////////////////

// Work around for libc++ bug #22806 required to make this work.
TEST_F(TestTupleZip, lrl_single) {
  auto x = "hello";
  int y = 2;
  auto z = "goodbye";
  decltype(auto) zipped = meta::tuple_zip(
    std::forward_as_tuple(x, 2, z)
  );
}
