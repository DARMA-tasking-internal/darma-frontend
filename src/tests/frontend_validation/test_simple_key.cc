/*
//@HEADER
// ************************************************************************
//
//                       test_segmented_key.cc
//                         darma
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


#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <darma/impl/key/simple_key.h>
#include <darma/key/key_concept.h>

#include "../meta/blabbermouth.h"

using namespace darma_runtime;

DARMA_STATIC_ASSERT_VALID_KEY_TYPE(detail::SimpleKey);

typedef EnableCTorBlabbermouth<Default, String, Copy, Move> BlabberMouth;
static MockBlabbermouthListener* listener;

////////////////////////////////////////////////////////////////////////////////

class TestSimpleKey
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

TEST_F(TestSimpleKey, simple_int) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<SimpleKey>::maker{};
  SimpleKey k = maker(2,4,8);
  ASSERT_EQ(k.component<0>().as<int>(), 2);
  ASSERT_EQ(k.component<1>().as<int>(), 4);
  ASSERT_EQ(k.component<2>().as<int>(), 8);
  ASSERT_EQ(k.component(0).as<int>(), 2);
  ASSERT_EQ(k.component(1).as<int>(), 4);
  ASSERT_EQ(k.component(2).as<int>(), 8);
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSimpleKey, simple_string) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<SimpleKey>::maker{};
  SimpleKey k = maker("hello", 2, "world!");
  ASSERT_EQ(k.component<0>().as<std::string>(), "hello");
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<2>().as<std::string>(), "world!");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSimpleKey, string_split) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<SimpleKey>::maker{};
  SimpleKey k = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  ASSERT_EQ(k.component<0>().as<std::string>(), "hello");
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<6>().as<std::string>(), "world!");
  ASSERT_EQ(k.component<7>().as<std::string>(), "How is it going today?");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSimpleKey, ints_split) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<SimpleKey>::maker{};
  SimpleKey k = maker(
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096
  );
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<12>().as<int>(), 4096);
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSimpleKey, ints_exact) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<SimpleKey>::maker{};
  SimpleKey k = maker(
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, "A"
  );
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<10>().as<std::string>(), "A");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSimpleKey, ints_exact_2) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<SimpleKey>::maker{};
  SimpleKey k = maker(
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, "Ab",
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, "AB"
  );
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<23>().as<std::string>(), "AB");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSimpleKey, string_span_3) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<SimpleKey>::maker{};
  const std::string s = ""
    "hello, there.  How is it going today? "
    " Blah blah blah blah blah blah blah 42";
  SimpleKey k = maker(s, 42, s);
  ASSERT_EQ(k.component<2>().as<std::string>(), s);
}
////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSimpleKey, equal_multipart) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<SimpleKey>::maker{};
  SimpleKey k1 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  SimpleKey k2 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  ASSERT_TRUE(key_traits<SimpleKey>::key_equal()(k1, k2));
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSimpleKey, hash_multipart) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<SimpleKey>::maker{};
  SimpleKey k1 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  SimpleKey k2 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  ASSERT_EQ(key_traits<SimpleKey>::hasher()(k1), key_traits<SimpleKey>::hasher()(k2));
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSimpleKey, without_last_component_simple) {
  using namespace darma_runtime::detail;
  typedef typename key_traits<SimpleKey>::internal_use_access access;
  auto maker = typename key_traits<SimpleKey>::maker{};
  SimpleKey k = maker(2,4);
  SimpleKey k2 = access::add_internal_last_component(k, 8);
  SimpleKey k3 = access::without_internal_last_component(k2);
  ASSERT_EQ(k3.component<0>().as<int>(), 2);
  ASSERT_EQ(k3.component<1>().as<int>(), 4);
  ASSERT_EQ(k.n_components(), 2);
  ASSERT_EQ(k2.n_components(), 2);
  ASSERT_EQ(k3.n_components(), 2);
  ASSERT_EQ(access::get_internal_last_component(k2).as<int>(), 8);
}

//////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSimpleKey, without_last_component_multipart) {
  using namespace darma_runtime::detail;
  typedef typename key_traits<SimpleKey>::internal_use_access access;
  auto maker = typename key_traits<SimpleKey>::maker{};
  SimpleKey k = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  SimpleKey k2 = access::add_internal_last_component(k, "asdf");
  SimpleKey k3 = access::without_internal_last_component(k2);
  EXPECT_EQ(k2.component<1>().as<int>(), 2);
  EXPECT_EQ(k2.component<2>().as<int>(), 3);
  EXPECT_EQ(k2.component<7>().as<std::string>(), "How is it going today?");
  EXPECT_EQ(k3.component<1>().as<int>(), 2);
  EXPECT_EQ(k3.component<2>().as<int>(), 3);
  EXPECT_EQ(k3.component<7>().as<std::string>(), "How is it going today?");
  EXPECT_EQ(k.n_components(), 8);
  EXPECT_EQ(k2.n_components(), 8);
  EXPECT_EQ(k3.n_components(), 8);

  ASSERT_FALSE(access::has_internal_last_component(k3));
  ASSERT_EQ(access::get_internal_last_component(k2).as<std::string>(), "asdf");
}

////////////////////////////////////////////////////////////////////////////////

//TEST_F(TestSimpleKey, bytes_copy) {
//  using namespace darma_runtime::detail;
//  auto maker = typename key_traits<SimpleKey>::maker{};
//  SimpleKey k = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
//
//  bytes_convert<SimpleKey> bc;
//  size_t size = bc.get_size(k);
//  char key_data[size];
//  bc(k, key_data, size, 0);
//
//  SimpleKey k2 = bc.get_value(key_data, size);
//
//  EXPECT_EQ(k.component<0>().as<std::string>(), "hello");
//  EXPECT_EQ(k.component<1>().as<int>(), 2);
//  EXPECT_EQ(k.component<6>().as<std::string>(), "world!");
//  EXPECT_EQ(k.component<7>().as<std::string>(), "How is it going today?");
//
//  ASSERT_EQ(k2.component<0>().as<std::string>(), "hello");
//  ASSERT_EQ(k2.component<1>().as<int>(), 2);
//  ASSERT_EQ(k2.component<6>().as<std::string>(), "world!");
//  ASSERT_EQ(k2.component<7>().as<std::string>(), "How is it going today?");

//  ASSERT_TRUE(key_traits<SimpleKey>::key_equal()(k, k2));
//}

////////////////////////////////////////////////////////////////////////////////

typedef enum EnumTestA { OneA=1, TwoA=2, ThreeA=3 } EnumTestA;
typedef enum EnumTestB { OneB=1, TwoB=2, ThreeB=3 } EnumTestB;

TEST_F(TestSimpleKey, enums) {
  using namespace darma_runtime::detail;
  using namespace ::testing;
  auto maker = typename key_traits<SimpleKey>::maker{};
  auto kA = maker(OneA, TwoB, ThreeA);
  auto kB1 = maker(OneB, TwoA, ThreeB);
  auto kB2 = maker(OneB, TwoA, ThreeB);
  EXPECT_THAT(kA, Not(Eq(kB1)));
  EXPECT_EQ(kB1, kB2);
}
