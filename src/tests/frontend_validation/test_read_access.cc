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

    virtual void SetUp() override {
      setup_mock_runtime<::testing::StrictMock>();
      TestFrontend::SetUp();
    }

    virtual void TearDown() override {
      TestFrontend::TearDown();
    }
};

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestReadAccess, call_sequence) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  auto my_version_tag = darma_runtime::make_key("my_version_tag");
  MockFlow f_in, f_out;

  Sequence s1, s2;

  EXPECT_CALL(*mock_runtime, make_fetching_flow(is_handle_with_key(make_key("hello")), Eq(my_version_tag)))
    .InSequence(s1)
    .WillOnce(Return(&f_in));
  EXPECT_CALL(*mock_runtime, make_null_flow(is_handle_with_key(make_key("hello"))))
    .InSequence(s1)
    .WillOnce(Return(&f_out));


  EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_in, &f_out))
    .InSequence(s1);

  EXPECT_RELEASE_FLOW(f_in);
  EXPECT_RELEASE_FLOW(f_out);

  {
    auto tmp = read_access<int>("hello", version=my_version_tag);
  }

}

////////////////////////////////////////////////////////////////////////////////

// Same as call_sequence, but uses helper to verify that other uses of helper should be valid
TEST_F(TestReadAccess, call_sequence_helper) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  MockFlow f_in, f_out;

  EXPECT_READ_ACCESS(f_in, f_out,
    make_key("hello"),
    make_key("my_version_tag")
  );

  EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_in, &f_out));

  EXPECT_RELEASE_FLOW(f_in);
  EXPECT_RELEASE_FLOW(f_out);

  {
    auto tmp = read_access<int>("hello", version="my_version_tag");
  }

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestReadAccess, call_sequence_assign) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_backend::MockFlow f_in_1, f_out_1, f_in_2, f_out_2;


  EXPECT_READ_ACCESS(f_in_1, f_out_1, make_key("hello"),
    make_key("my_version_tag")
  );

  EXPECT_READ_ACCESS(f_in_2, f_out_2, make_key("world"),
    make_key("other_version_tag")
  );

  Expectation alias1 =
    EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_in_1, &f_out_1));

  Expectation release_fin1 = EXPECT_RELEASE_FLOW(f_in_1).After(alias1);
  Expectation release_fout1 = EXPECT_RELEASE_FLOW(f_out_1).After(alias1);

  Expectation alias2 =
    EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_in_2, &f_out_2))
      .After(release_fin1, release_fout1);

  EXPECT_RELEASE_FLOW(f_in_2).After(alias2);
  EXPECT_RELEASE_FLOW(f_out_2).After(alias2);


  {
    auto tmp1 = read_access<int>("hello", version="my_version_tag");

    tmp1 = read_access<int>("world", version="other_version_tag");

  } // tmp1

}

////////////////////////////////////////////////////////////////////////////////

