/*
//@HEADER
// ************************************************************************
//
//                      test_serialize.cc
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

#include <vector>
#include <map>
#include <unordered_map>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <darma/serialization/nonintrusive.h>
#include <darma/impl/handle.h>

#include "mock_backend.h"

#include <test_frontend.h>



////////////////////////////////////////////////////////////////////////////////

class TestSerialize
  : public ::testing::Test
{
  protected:

    virtual void SetUp() {

    }

    virtual void TearDown() {

    }
};

////////////////////////////////////////////////////////////////////////////////

//TEST_F(TestSerialize, fundamental) {
//  using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
//
//  {
//    int value = 42;
//    Ser<int> ser;
//    SimplePackUnpackArchive ar;
//    ArchiveAccess::start_sizing(ar);
//    ser.compute_size(value, ar);
//    ASSERT_EQ(ArchiveAccess::get_size(ar), sizeof(value));
//  }
//
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestSerialize, fundamental_chain) {
//  using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
//
//  int value = 42;
//  constexpr int n_reps = 10;
//  Ser<int> ser;
//  SimplePackUnpackArchive ar;
//  ArchiveAccess::start_sizing(ar);
//
//  // make sure it doesn't reset between repeats
//  for(int i= 0; i < n_reps; ++i) ser.compute_size(value, ar);
//
//  ASSERT_EQ(ArchiveAccess::get_size(ar), sizeof(value)*n_reps);
//
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestSerialize, vector_simple) {
//  using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
//  using namespace std;
//  using namespace ::testing;
//
//  std::vector<int> value = { 3, 1, 4, 1, 5, 9, 2, 6 };
//
//  auto v_unpacked = do_serdes(value);
//
//  ASSERT_THAT(v_unpacked, ElementsAre(3, 1, 4, 1, 5, 9, 2, 6));
//  // Also assert that value is unchanged
//  ASSERT_THAT(value, ElementsAre(3, 1, 4, 1, 5, 9, 2, 6));
//}
//
//STATIC_ASSERT_SERIALIZABLE_WITH_ARCHIVE(std::vector<double>, PolicyAwareArchive,
//  "std::vector<double> should be serializable with PolicyAwareArchive"
//);
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestSerialize, map_simple) {
//  using namespace std;
//  using namespace ::testing;
//
//  static_assert(meta::is_container<std::map<int, int>>::value, "map must be a Container");
//
//  std::map<int, int> value = { {3, 1}, {4, 1}, {5, 9}, {2, 6} };
//
//  auto v_unpacked = do_serdes(value);
//
//  ASSERT_THAT(v_unpacked, ContainerEq(value));
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestSerialize, unordered_map_simple) {
//  using namespace std;
//  using namespace ::testing;
//
//  static_assert(meta::is_container<std::unordered_map<int, int>>::value, "unordered_map must be a Container");
//
//  std::unordered_map<int, int> value = { {3, 1}, {4, 1}, {5, 9}, {2, 6} };
//
//  auto v_unpacked = do_serdes(value);
//
//  ASSERT_THAT(v_unpacked, ContainerEq(value));
//}

////////////////////////////////////////////////////////////////////////////////

//STATIC_ASSERT_SERIALIZABLE_WITH_ARCHIVE(std::string, SimplePackUnpackArchive,
//  "String should be serializable"
//);
//
//static_assert(meta::is_container<std::map<int, int>>::value, "map<int, int> must be a Container");
//static_assert(darma_runtime::serialization::Serializer<std::map<int, int>>::is_insertable,
//  "map<int, int> should be insertable"
//);
//using map_int_int = std::map<int, int>;
//STATIC_ASSERT_SERIALIZABLE_WITH_ARCHIVE(map_int_int, SimplePackUnpackArchive,
//  "map<int, int> should be serializable"
//);
//
//STATIC_ASSERT_SERIALIZABLE_WITH_ARCHIVE(std::vector<std::vector<int>>, SimplePackUnpackArchive,
//  "vector<vector<int>> should be serializable should be serializable"
//);
//STATIC_ASSERT_SERIALIZABLE_WITH_ARCHIVE(std::vector<std::vector<int>>, PolicyAwareArchive,
//  "vector<vector<int>> should be serializable should be serializable"
//);
//STATIC_ASSERT_SERIALIZABLE_WITH_ARCHIVE(std::vector<std::string>, PolicyAwareArchive,
//  "vector<vector<int>> should be serializable should be serializable"
//);

////////////////////////////////////////////////////////////////////////////////

//template <typename T>
//struct Foo {
//  T t;
//};
//
//namespace darma_runtime {
//namespace serialization {
//
//template <typename T>
//struct Serializer<Foo<T>> {
//
//  template <typename ArchiveT>
//  void serialize(Foo<T>& f, ArchiveT& ar) const {
//    ar | f.t;
//  }
//
//};
//
//} // end namespace serialization
//} // end namespace darma_runtime
//
//
//static_assert(darma_runtime::serialization::detail::serializability_traits<Foo<int>>
//    ::template has_nonintrusive_serialize<darma_runtime::serialization::SimplePackUnpackArchive
//  >::value,
//  "Foo<int> should have a nonintrusive serialize"
//);
//
//STATIC_ASSERT_SERIALIZABLE_WITH_ARCHIVE(Foo<int>, SimplePackUnpackArchive,
//  "Foo<int> should be serializable"
//);

////////////////////////////////////////////////////////////////////////////////

//TEST_F(TestSerialize, vector_policy) {
//  using namespace std;
//  using namespace ::testing;
//  using namespace mock_backend;
//  using namespace darma_runtime;
//  using namespace darma_runtime::serialization;
//  using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
//
//  std::vector<int> value = { 3, 1, 4, 1, 5, 9, 2, 6 };
//
//  // Simulate a zero-copy transfer as facilitated by the backend
//
//  MockSerializationPolicy ser_pol;
//
//  PolicyAwareArchive ar(&ser_pol);
//
//  EXPECT_CALL(ser_pol, packed_size_contribution_for_blob(value.data(), 8*sizeof(int)))
//    .WillOnce(Return(0));
//
//  ArchiveAccess::start_sizing(ar);
//  ar.incorporate_size(value);
//  size_t size = ArchiveAccess::get_size(ar);
//  ASSERT_THAT(size, Eq(sizeof(size_t)));
//
//  char buffer[8*sizeof(int)];
//
//  EXPECT_CALL(ser_pol, pack_blob(_, value.data(), 8*sizeof(int)))
//    .WillOnce(Invoke([&](void*& indirect_buff, void const* data, size_t nbytes){
//      memcpy(buffer, data, nbytes);
//    }));
//
//  char indirect_buff[256];
//  ArchiveAccess::set_buffer(ar, indirect_buff);
//  ArchiveAccess::start_packing(ar);
//  ar << value;
//
//  std::vector<int> value2;
//
//  EXPECT_CALL(ser_pol, unpack_blob(_, _, 8*sizeof(int)))
//    .WillOnce(Invoke([&](void*& indirect_buff, void* data, size_t nbytes){
//      memcpy(data, buffer, nbytes);
//    }));
//
//  ArchiveAccess::start_unpacking_with_buffer(ar, indirect_buff);
//  ar >> value2;
//
//  ASSERT_THAT(value2, ContainerEq(value));
//
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestSerialize, vector_string) {
//  using namespace std;
//  using namespace ::testing;
//
//  std::vector<std::string> value = { "hello", "world", "!", "test"};
//
//  auto v_unpacked = do_serdes(value);
//
//  ASSERT_THAT(v_unpacked, ContainerEq(value));
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestSerialize, foo) {
//  using namespace std;
//  using namespace ::testing;
//
//  Foo<int> value = { 42 };
//
//  auto v_unpacked = do_serdes(value);
//
//  ASSERT_THAT(v_unpacked.t, Eq(value.t));
//  ASSERT_THAT(v_unpacked.t, Eq(42));
//  ASSERT_THAT(value.t, Eq(42));
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestSerialize, map_map) {
//  using namespace std;
//  using namespace ::testing;
//
//  std::map<int,std::map<int, float>> value;
//  for(int i = 0; i < 5; ++i) {
//    value[i] = std::map<int, float>{ {i, i+1.0 }, { i*i, i*i + 1.0 } };
//  }
//
//  auto v_unpacked = do_serdes(value);
//
//  ASSERT_THAT(v_unpacked, ContainerEq(value));
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestSerialize, tuple_string_int) {
//  using namespace std;
//  using namespace ::testing;
//
//  std::tuple<std::string, int> value = std::make_tuple("hello", 42);
//
//  auto v_unpacked = do_serdes(value);
//
//  ASSERT_THAT(std::get<0>(v_unpacked), Eq(std::get<0>(value)));
//  ASSERT_THAT(std::get<1>(v_unpacked), Eq(std::get<1>(value)));
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestSerialize, key) {
//  using namespace std;
//  using namespace ::testing;
//
//  auto value = darma::make_key("hello", 42);
//
//  auto v_unpacked = do_serdes(value);
//
//  ASSERT_THAT(v_unpacked, Eq(value));
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//class TestSerializeWithMock
//  : public TestFrontend
//{
//  protected:
//
//    virtual void SetUp() {
//      using namespace ::testing;
//
//      setup_mock_runtime<::testing::NiceMock>();
//      TestFrontend::SetUp();
//      ON_CALL(*mock_runtime, get_running_task())
//        .WillByDefault(Return(top_level_task.get()));
//    }
//
//    virtual void TearDown() {
//      TestFrontend::TearDown();
//    }
//
//};
//
//TEST_F(TestSerializeWithMock, range) {
//  using namespace std;
//  using namespace ::testing;
//  using namespace darma_runtime::serialization;
//
//  int my_val[5] = {1, 2, 3, 4, 5};
//  int* value = &(my_val[0]);
//
//  using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
//  SimplePackUnpackArchive ar;
//
//  ArchiveAccess::start_sizing(ar);
//  ar % range(value, value+5);
//  size_t size = ArchiveAccess::get_size(ar);
//
//  char data[size];
//  ArchiveAccess::set_buffer(ar, data);
//  ArchiveAccess::start_packing(ar);
//  ar << range(value, value+5);
//
//  int* out = nullptr;
//  ArchiveAccess::start_unpacking_with_buffer(ar, data);
//  ar >> range(out, out+5);
//
//  ASSERT_THAT(value[0], Eq(out[0]));
//  ASSERT_THAT(value[1], Eq(out[1]));
//  ASSERT_THAT(value[2], Eq(out[2]));
//  ASSERT_THAT(value[3], Eq(out[3]));
//  ASSERT_THAT(value[4], Eq(out[4]));
//
//}
//
//TEST_F(TestSerializeWithMock, range_begin_end) {
//  using namespace std;
//  using namespace ::testing;
//  using namespace darma_runtime::serialization;
//
//  int my_val[5] = {1, 2, 3, 4, 5};
//  int* value = &(my_val[0]);
//
//  using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
//  SimplePackUnpackArchive ar;
//
//  ArchiveAccess::start_sizing(ar);
//  auto* v2 = value + 5;
//  ar % range(value, v2);
//  size_t size = ArchiveAccess::get_size(ar);
//
//  char data[size];
//  ArchiveAccess::set_buffer(ar, data);
//  ArchiveAccess::start_packing(ar);
//  ar << range(value, v2);
//
//  int* out = nullptr;
//  auto* out2 = out+5;
//  ArchiveAccess::start_unpacking_with_buffer(ar, data);
//  ar >> range(out, out2);
//
//  ASSERT_THAT(value[0], Eq(out[0]));
//  ASSERT_THAT(value[1], Eq(out[1]));
//  ASSERT_THAT(value[2], Eq(out[2]));
//  ASSERT_THAT(value[3], Eq(out[3]));
//  ASSERT_THAT(value[4], Eq(out[4]));
//  ASSERT_THAT(*(out2-5), Eq(out[0]));
//
//}
