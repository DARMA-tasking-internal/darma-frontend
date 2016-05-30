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

#include <darma/impl/serialization/nonintrusive.h>
#include <darma/impl/serialization/archive.h>
#include <darma/impl/handle.h>


using namespace darma_runtime;
using namespace darma_runtime::detail;
using namespace darma_runtime::serialization;
using namespace darma_runtime::serialization::detail;

template <typename T> using Ser = typename serializability_traits<T>::serializer;

////////////////////////////////////////////////////////////////////////////////

class TestSerialize
  : public ::testing::Test
{
  protected:

    template <typename T, typename Archive=SimplePackUnpackArchive>
    T do_serdes(T const& val) const {
      using darma_runtime::detail::DependencyHandle_attorneys::ArchiveAccess;
      Ser<T> ser;
      SimplePackUnpackArchive ar;

      ArchiveAccess::start_sizing(ar);
      ser.compute_size(val, ar);
      size_t size = ArchiveAccess::get_size(ar);

      char data[size];
      ArchiveAccess::set_buffer(ar, data);
      ArchiveAccess::start_packing(ar);
      ser.pack(val, ar);

      char rv[sizeof(T)];
      ArchiveAccess::start_unpacking(ar);
      ser.unpack(&rv, ar);


      // Return by copy, so memory shouldn't be an issue
      return *(T*)rv;
    }

    virtual void SetUp() {

    }

    virtual void TearDown() {

    }
};

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSerialize, fundamental) {
  using darma_runtime::detail::DependencyHandle_attorneys::ArchiveAccess;

  {
    int value = 42;
    Ser<int> ser;
    SimplePackUnpackArchive ar;
    ArchiveAccess::start_sizing(ar);
    ser.compute_size(value, ar);
    ASSERT_EQ(ArchiveAccess::get_size(ar), sizeof(value));
  }

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSerialize, fundamental_chain) {
  using darma_runtime::detail::DependencyHandle_attorneys::ArchiveAccess;

  int value = 42;
  constexpr int n_reps = 10;
  Ser<int> ser;
  SimplePackUnpackArchive ar;
  ArchiveAccess::start_sizing(ar);

  // make sure it doesn't reset between repeats
  for(int i= 0; i < n_reps; ++i) ser.compute_size(value, ar);

  ASSERT_EQ(ArchiveAccess::get_size(ar), sizeof(value)*n_reps);

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSerialize, vector_simple) {
  using darma_runtime::detail::DependencyHandle_attorneys::ArchiveAccess;
  using namespace std;
  using namespace ::testing;

  vector<int> value = { 3, 1, 4, 1, 5, 9, 2, 6 };

  auto v_unpacked = do_serdes(value);

  ASSERT_THAT(v_unpacked, ElementsAre(3, 1, 4, 1, 5, 9, 2, 6));
  // Also assert that value is unchanged
  ASSERT_THAT(value, ElementsAre(3, 1, 4, 1, 5, 9, 2, 6));
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSerialize, map_simple) {
  using namespace std;
  using namespace ::testing;

  static_assert(meta::is_container<map<int, int>>::value, "map must be a Container");

  map<int, int> value = { {3, 1}, {4, 1}, {5, 9}, {2, 6} };

  auto v_unpacked = do_serdes(value);

  ASSERT_THAT(v_unpacked, ContainerEq(value));
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSerialize, unordered_map_simple) {
  using namespace std;
  using namespace ::testing;

  static_assert(meta::is_container<unordered_map<int, int>>::value, "unordered_map must be a Container");

  unordered_map<int, int> value = { {3, 1}, {4, 1}, {5, 9}, {2, 6} };

  auto v_unpacked = do_serdes(value);

  ASSERT_THAT(v_unpacked, ContainerEq(value));
}