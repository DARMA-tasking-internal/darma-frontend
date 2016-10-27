/*
//@HEADER
// ************************************************************************
//
//                      test_sso_key.cc
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
#include <gmock/gmock.h>

#include "test_frontend.h"
#include "mock_backend.h"

#include <darma/impl/key/SSO_key.h>
#include <darma/impl/key_concept.h>
#include <darma/impl/util/static_assertions.h>
#include <darma/impl/serialization/policy_aware_archive.h>
#include <darma/impl/serialization/manager.h>


#include <darma/impl/darma.h>

using namespace darma_runtime;
using namespace darma_runtime::detail;

////////////////////////////////////////////////////////////////////////////////

DARMA_STATIC_ASSERT_VALID_KEY_TYPE(SSOKey<>);
STATIC_ASSERT_SIZE_IS(SSOKey<>, 64);


using sso_key_t = SSOKey<>;

////////////////////////////////////////////////////////////////////////////////

class TestSSOKey
  : public TestFrontend
{
  protected:

    virtual void SetUp() {
      using namespace ::testing;
      setup_mock_runtime<::testing::NiceMock>();
      TestFrontend::SetUp();
    }

    virtual void TearDown() {
      TestFrontend::TearDown();
    }

};

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, sso_int) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k = maker(2,4,8);
  ASSERT_EQ(k.component<0>().as<int>(), 2);
  ASSERT_EQ(k.component<1>().as<int>(), 4);
  ASSERT_EQ(k.component<2>().as<int>(), 8);
  ASSERT_EQ(k.component(0).as<int>(), 2);
  ASSERT_EQ(k.component(1).as<int>(), 4);
  ASSERT_EQ(k.component(2).as<int>(), 8);
}

TEST_F(TestSSOKey, simple_string) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k = maker("hello", 2, "world!");
  ASSERT_EQ(k.component<0>().as<std::string>(), "hello");
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<2>().as<std::string>(), "world!");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, string_split) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  ASSERT_EQ(k.component<0>().as<std::string>(), "hello");
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<6>().as<std::string>(), "world!");
  ASSERT_EQ(k.component<7>().as<std::string>(), "How is it going today?");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, ints_split) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k = maker(
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096
  );
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<12>().as<int>(), 4096);
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, ints_exact) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k = maker(
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, "A"
  );
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<10>().as<std::string>(), "A");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, ints_exact_2) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k = maker(
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, "Ab",
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, "AB"
  );
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<23>().as<std::string>(), "AB");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, string_span_3) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  const std::string s = ""
    "hello, there.  How is it going today? "
    " Blah blah blah blah blah blah blah 42";
  sso_key_t k = maker(s, 42, s);
  ASSERT_EQ(k.component<2>().as<std::string>(), s);
}
////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, equal_multipart) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k1 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  sso_key_t k2 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k1, k2));
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, hash_multipart) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k1 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  sso_key_t k2 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  ASSERT_EQ(key_traits<sso_key_t>::hasher()(k1), key_traits<sso_key_t>::hasher()(k2));
}

////////////////////////////////////////////////////////////////////////////////

typedef enum EnumTestA { OneA=1, TwoA=2, ThreeA=3 } EnumTestA;
typedef enum EnumTestB { OneB=1, TwoB=2, ThreeB=3 } EnumTestB;

TEST_F(TestSSOKey, enums) {
  using namespace darma_runtime::detail;
  using namespace ::testing;
  auto maker = typename key_traits<sso_key_t>::maker{};
  auto kA = maker(OneA, TwoB, ThreeA);
  auto kB1 = maker(OneB, TwoA, ThreeB);
  auto kB2 = maker(OneB, TwoA, ThreeB);
  EXPECT_THAT(kA, Not(Eq(kB1)));
  EXPECT_EQ(kB1, kB2);
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, equal_key_key) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k1 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  sso_key_t k2 = maker(k1);
  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k1, k2));
  sso_key_t k3 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?", "done");
  sso_key_t k4 = maker(k1, "done");
  sso_key_t k5 = maker(k2, "done");
  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k3, k4));
  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k3, k5));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, equal_backend_assigned) {
  using namespace darma_runtime::detail;
  using namespace ::testing;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t backendk1 = maker();
  sso_key_t backendk2 = maker();
  ASSERT_EQ(backendk1, backendk2);
  auto backend_maker = typename key_traits<sso_key_t>::backend_maker{};
  auto backendk3 = backend_maker(0);
  auto backendk4 = backend_maker(1);
  ASSERT_THAT(backendk3, Not(Eq(backendk4)));
  ASSERT_THAT(backendk2, Not(Eq(backendk4)));
}

////////////////////////////////////////////////////////////////////////////////

//TEST_F(TestSSOKey, serialize_long) {
//  using namespace darma_runtime::detail;
//  using namespace darma_runtime::serialization;
//  using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
//  auto maker = typename key_traits<sso_key_t>::maker{};
//  sso_key_t k1 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
//
//  // Use the default which uses indirect in place of direct
//  auto ser_pol = abstract::backend::SerializationPolicy{};
//  serialization::PolicyAwareArchive ar(&ser_pol);
//  serialization::Serializer<sso_key_t> ser;
//
//  ArchiveAccess::start_sizing(ar);
//  ser.compute_size(k1, ar);
//  size_t size = ArchiveAccess::get_size(ar);
//
//  char buffer[size];
//  ArchiveAccess::start_packing_with_buffer(ar, buffer);
//  ser.pack(k1, ar);
//
//  char allocated[sizeof(sso_key_t)];
//  ArchiveAccess::start_unpacking_with_buffer(ar, buffer);
//  ser.unpack(allocated, ar);
//  sso_key_t& k2 = *reinterpret_cast<sso_key_t*>(&(allocated[0]));
//
//  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k1, k2));
//
//}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestSSOKey, serialize_serman_long) {
  using namespace darma_runtime::detail;
  using namespace darma_runtime::serialization;
  using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k1 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");

  // Use the default which uses indirect in place of direct
  auto ser_pol = abstract::backend::SerializationPolicy{};
  auto size = k1.get_packed_data_size(&k1, &ser_pol);

  char buffer[size];
  k1.pack_data(&k1, buffer, &ser_pol);

  char allocated[k1.get_metadata_size()];
  // expect allocation from the indirect deserializer
  EXPECT_CALL(*mock_runtime, allocate(_, _))
    .WillOnce(Invoke([](auto alloc_size, auto&&...){
      return ::operator new(alloc_size);
    }));
  k1.unpack_data(allocated, buffer, &ser_pol);
  auto& k2 = *reinterpret_cast<sso_key_t*>(allocated);

  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k1, k2));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, serialize_serman_small) {
  using namespace darma_runtime::detail;
  using namespace darma_runtime::serialization;
  using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k1 = maker("me", 2);

  // Use the default which uses indirect in place of direct
  auto ser_pol = abstract::backend::SerializationPolicy{};
  auto size = k1.get_packed_data_size(&k1, &ser_pol);

  char buffer[size];
  k1.pack_data(&k1, buffer, &ser_pol);

  char allocated[k1.get_metadata_size()];
  k1.unpack_data(allocated, buffer, &ser_pol);
  auto& k2 = *reinterpret_cast<sso_key_t*>(allocated);

  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k1, k2));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, serialize_serman_backend_assigned) {
  using namespace darma_runtime::detail;
  using namespace darma_runtime::serialization;
  using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
  auto maker = typename key_traits<sso_key_t>::backend_maker{};
  sso_key_t k1 = maker(314ul);

  // Use the default which uses indirect in place of direct
  auto ser_pol = abstract::backend::SerializationPolicy{};
  auto size = k1.get_packed_data_size(&k1, &ser_pol);

  char buffer[size];
  k1.pack_data(&k1, buffer, &ser_pol);

  char allocated[k1.get_metadata_size()];
  k1.unpack_data(allocated, buffer, &ser_pol);
  auto& k2 = *reinterpret_cast<sso_key_t*>(allocated);

  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k1, k2));

}
