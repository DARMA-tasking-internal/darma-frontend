/*
//@HEADER
// ************************************************************************
//
//                       test_segmented_key.cc
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

#include <darma/impl/segmented_key.h>
#include <darma/impl/key_concept.h>

#include "../meta/blabbermouth.h"

using namespace darma_runtime;

DARMA_STATIC_ASSERT_VALID_KEY_TYPE(detail::SegmentedKey);

typedef EnableCTorBlabbermouth<Default, String, Copy, Move> BlabberMouth;
static MockBlabbermouthListener* listener;

////////////////////////////////////////////////////////////////////////////////

class TestSegmentedKey
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

TEST_F(TestSegmentedKey, simple_int) {
  using namespace darma_runtime::detail;
  auto maker = typename key_traits<SegmentedKey>::maker{};
  SegmentedKey k = maker(2,4,8);
  ASSERT_EQ(k.component<0>().as<int>(), 2);
  ASSERT_EQ(k.component<1>().as<int>(), 4);
  ASSERT_EQ(k.component<2>().as<int>(), 8);
}