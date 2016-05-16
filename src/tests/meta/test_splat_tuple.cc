/*
//@HEADER
// ************************************************************************
//
//                          test_splat_tuple.cc
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

#include <darma/impl/meta/tuple_for_each.h>
#include "blabbermouth.h"

using namespace darma_runtime;

typedef EnableCTorBlabbermouth<String, Copy, Move> BlabberMouth;
static MockBlabbermouthListener* listener;

////////////////////////////////////////////////////////////////////////////////

class TestSplatTuple
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

//struct BlabberMouth {
//  BlabberMouth(const std::string& str) : str_(str) { std::cout << "#!! String constructor: " << str_ << std::endl; }
//  BlabberMouth(const BlabberMouth& other) : str_(other.str_) { std::cout << "#!! copy constructor: " << str_ << std::endl; };
//  BlabberMouth(BlabberMouth&& other) : str_(other.str_) { std::cout << "#!! move constructor: " << str_ << std::endl; };
//  std::string str_;
//};

template <typename Tuple>
void test_const(const Tuple& tup) {
  meta::splat_tuple(tup, [](
    const int& a,
    const std::string& b,
    const double& c,
    const BlabberMouth& d
  ){
    std::cout << a << ", " << b << ", " << c << std::endl;
  });
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSplatTuple, old_1) {
  using namespace ::testing;
  testing::internal::CaptureStdout();
  Sequence s;

  // Should have one move constructor if the nondefault destructor is givien
  EXPECT_CALL(*listener, string_ctor()).InSequence(s);
  EXPECT_CALL(*listener, move_ctor()).Times(AtMost(1)).InSequence(s);
  auto tup = std::make_tuple(5, "hello", 47.32, BlabberMouth("blabber"));
  meta::splat_tuple(tup, [](const int& a, const std::string& b, const double& c, const BlabberMouth& d){
    std::cout << a << ", " << b << ", " << c << std::endl;
  });

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "5, hello, 47.32\n"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSplatTuple, old_2) {
  using namespace ::testing;
  testing::internal::CaptureStdout();
  Sequence s;

  EXPECT_CALL(*listener, string_ctor()).InSequence(s);
  meta::splat_tuple(std::forward_as_tuple(5, "hello", 47.32, BlabberMouth("blabber")),
    [](const int& a, const std::string& b, const double& c, const BlabberMouth& d){
      std::cout << a << ", " << b << ", " << c << std::endl;
    });

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "5, hello, 47.32\n"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSplatTuple, old_seq) {
  // Note: these tests are together to ensure splat_tuple has no unintented consequences (e.g., moving and
  // causing a value to expire, etc.)

  using namespace ::testing;

  testing::internal::CaptureStdout();

  Sequence s;

  // Should have one move constructor if a nondefault destructor is given
  EXPECT_CALL(*listener, string_ctor()).InSequence(s);
  EXPECT_CALL(*listener, move_ctor()).Times(AtMost(1)).InSequence(s);
  auto tup = std::make_tuple(5, "hello", 47.32, BlabberMouth("blabber"));
  meta::splat_tuple(tup, [](const int& a, const std::string& b, const double& c, const BlabberMouth& d){
    std::cout << a << ", " << b << ", " << c << std::endl;
  });

  // Should have only a string constructor
  EXPECT_CALL(*listener, string_ctor()).InSequence(s);
  meta::splat_tuple(std::forward_as_tuple(5, "hello", 47.32, BlabberMouth("blabber")),
    [](const int& a, const std::string& b, const double& c, const BlabberMouth& d){
      std::cout << a << ", " << b << ", " << c << std::endl;
  });

  // Should call no constructors
  test_const(tup);

  // Should have only a string constructor
  EXPECT_CALL(*listener, string_ctor()).InSequence(s);
  test_const(std::forward_as_tuple(5, "hello", 47.32, BlabberMouth("blabber")));

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "5, hello, 47.32\n"
    "5, hello, 47.32\n"
    "5, hello, 47.32\n"
    "5, hello, 47.32\n"
  );
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSplatTuple, lvalue_1) {
  using namespace ::testing;
  testing::internal::CaptureStdout();
  Sequence s;

  int my_int = 5;

  meta::splat_tuple(std::forward_as_tuple(my_int, "hello"),
    [](int& val, std::string const& str){
      std::cout << str << std::endl;
      val = 42;
    });

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "hello\n"
  );
  ASSERT_EQ(my_int, 42);

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSplatTuple, lvalue_2) {
  using namespace ::testing;
  Sequence s;

  auto tup = std::make_tuple(1, 2);

  meta::splat_tuple(tup,
    [](int& val, int& val2){
      val = 3;
      val2 = 4;
    });

  ASSERT_EQ(std::get<0>(tup), 3);
  ASSERT_EQ(std::get<1>(tup), 4);
}

////////////////////////////////////////////////////////////////////////////////

void takes_rvalues(int&& i, double&& j, BlabberMouth&& b) {
  std::cout << i << j << b.data << std::endl;
}

TEST_F(TestSplatTuple, rvalue) {

  EXPECT_CALL(*listener, string_ctor());

  testing::internal::CaptureStdout();
  meta::splat_tuple(
    std::forward_as_tuple(5, 10, BlabberMouth("hello")),
    takes_rvalues
  );
  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "510hello\n"
  );

}

////////////////////////////////////////////////////////////////////////////////

void takes_mixed(int& i, double const& j, BlabberMouth&& b) {
  std::cout << i << j << b.data << std::endl;
}

TEST_F(TestSplatTuple, mixed_lrvalue) {
  EXPECT_CALL(*listener, string_ctor()).Times(2);
  testing::internal::CaptureStdout();

  int my_i = 5;
  meta::splat_tuple(
    std::forward_as_tuple(my_i, 10, BlabberMouth("hello")),
    takes_mixed
  );

  double value = 10;
  BlabberMouth b("hello");
  meta::splat_tuple(
    std::forward_as_tuple(my_i, value, std::move(b)),
    takes_mixed
  );

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "510hello\n510hello\n"
  );

}
