/*
//@HEADER
// ************************************************************************
//
//                          test_read_access.cc
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


#include <gtest/gtest.h>

#include "mock_backend.h"
#include "test_frontend.h"

#include <darma/interface/app/read_access.h>

////////////////////////////////////////////////////////////////////////////////

class TestReadAccess
  : public TestFrontend
{
  protected:

    virtual void SetUp() {
      setup_mock_runtime<::testing::StrictMock>();
      TestFrontend::SetUp();
    }

    virtual void TearDown() {
      TestFrontend::TearDown();
    }
};

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestReadAccess, call_sequence) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  types::key_t my_version_tag = darma_runtime::make_key("my_version_tag");

  auto hm1 = make_same_handle_matcher();

  Sequence s1, s2;

  EXPECT_CALL(*mock_runtime, register_fetching_handle(Truly(hm1), Eq(my_version_tag)))
    .Times(Exactly(1))
    .InSequence(s1, s2);

  // NOTE: read_access handles should not call handle_done_with_version_depth!
  EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Truly(hm1)))
    .Times(Exactly(0));

  EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(hm1)))
    .Times(Exactly(1))
    .InSequence(s2);

  EXPECT_CALL(*mock_runtime, release_handle(Truly(hm1)))
    .Times(Exactly(1))
    .InSequence(s1, s2);

  {
    auto tmp = read_access<int>("hello", version="my_version_tag");
    auto* tmp_handle = detail::create_work_attorneys::for_AccessHandle::get_dep_handle(tmp);
    ASSERT_THAT(tmp_handle, Eq(hm1.handle));
  }

}

////////////////////////////////////////////////////////////////////////////////

// Same as call_sequence, but uses helper to verify that other uses of helper should be valid
TEST_F(TestReadAccess, call_sequence_helper) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  types::key_t my_version_tag = darma_runtime::make_key("my_version_tag");

  auto hm1 = make_same_handle_matcher();
  Sequence s1;
  expect_handle_life_cycle(hm1, s1, /*read_only=*/true);

  {
    auto tmp = read_access<int>("hello", version="my_version_tag");
    auto* tmp_handle = detail::create_work_attorneys::for_AccessHandle::get_dep_handle(tmp);
    ASSERT_THAT(tmp_handle, Eq(hm1.handle));
  }

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestReadAccess, call_sequence_assign) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  types::key_t my_version_tag = darma_runtime::make_key("my_version_tag");
  types::key_t other_version_tag = darma_runtime::make_key("other_version_tag");

  auto hm1 = make_same_handle_matcher();
  auto hm2 = make_same_handle_matcher();

  Sequence s;

  EXPECT_CALL(*mock_runtime, register_fetching_handle(Truly(hm1), Eq(my_version_tag)))
    .Times(Exactly(1))
    .InSequence(s);
  EXPECT_CALL(*mock_runtime, register_fetching_handle(Truly(hm2), Eq(other_version_tag)))
    .Times(Exactly(1))
    .InSequence(s);
  EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(hm1)))
    .Times(Exactly(1))
    .InSequence(s);
  EXPECT_CALL(*mock_runtime, release_handle(Truly(hm1)))
    .Times(Exactly(1))
    .InSequence(s);
  EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(hm2)))
    .Times(Exactly(1))
    .InSequence(s);
  EXPECT_CALL(*mock_runtime, release_handle(Truly(hm2)))
    .Times(Exactly(1))
    .InSequence(s);

  // NOTE: read_access handles should not call handle_done_with_version_depth!
  // This should be enforced by the strict mock anyway, but just in case,
  EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(_))
    .Times(Exactly(0));


  {
    auto tmp1 = read_access<int>("hello", version="my_version_tag");

    tmp1 = read_access<int>("world", version="other_version_tag");

    auto* tmp_handle = detail::create_work_attorneys::for_AccessHandle::get_dep_handle(tmp1);

    ASSERT_THAT(tmp_handle, Eq(hm2.handle));
  }

}

////////////////////////////////////////////////////////////////////////////////

