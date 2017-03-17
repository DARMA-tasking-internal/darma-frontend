/*
//@HEADER
// ************************************************************************
//
//                          test_initial_access.cc
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

#include <darma/impl/handle.h>
#include <darma/interface/app/initial_access.h>

#include "mock_backend.h"
#include "test_frontend.h"

////////////////////////////////////////////////////////////////////////////////

class TestInitialAccess
  : public TestFrontend
{
  protected:

    virtual void SetUp() {
      setup_mock_runtime<::testing::NiceMock>();
      TestFrontend::SetUp();
    }

    virtual void TearDown() {
      TestFrontend::TearDown();
    }
};

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestInitialAccess, call_sequence) {
  using namespace ::testing;
  using namespace darma_runtime;

  mock_backend::MockFlow f_in_1, f_out_1;

  Sequence s1, s2;

  EXPECT_CALL(*mock_runtime, make_initial_flow(_))
    .InSequence(s1)
    .WillOnce(Return(&f_in_1));

  EXPECT_CALL(*mock_runtime, make_null_flow(_))
    .InSequence(s2)
    .WillOnce(Return(&f_out_1));


  EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_in_1, &f_out_1))
    .InSequence(s1, s2);

  EXPECT_RELEASE_FLOW(f_in_1);
  EXPECT_RELEASE_FLOW(f_out_1);

  {
    auto tmp = initial_access<int>("hello");
  } // tmp deleted

}

////////////////////////////////////////////////////////////////////////////////

// Same as call_sequence, but uses helper to verify that other uses of helper should be valid
TEST_F(TestInitialAccess, call_sequence_helper) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;

  MockFlow f_in, f_out;

  EXPECT_INITIAL_ACCESS(f_in, f_out, make_key("hello"));

  EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_in, &f_out));

  EXPECT_RELEASE_FLOW(f_in);
  EXPECT_RELEASE_FLOW(f_out);

  {
    auto tmp = initial_access<int>("hello");
  } // tmp deleted

}

////////////////////////////////////////////////////////////////////////////////

// Same as call_sequence, but with delayed assignment
TEST_F(TestInitialAccess, call_sequence_helper_2) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;

  DECLARE_MOCK_FLOWS(f_in, f_out, f_in_2, f_out_2);

  EXPECT_INITIAL_ACCESS(f_in, f_out, make_key("hello"));

  EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_in, &f_out));

  EXPECT_RELEASE_FLOW(f_in);
  EXPECT_RELEASE_FLOW(f_out);


  {
    AccessHandle<int> tmp{};
    AccessHandle<int> tmp2{};

    STATIC_ASSERT_VALUE_EQUAL(
      tmp.is_known_not_copy_assignable,
      false
    );

    tmp = initial_access<int>("hello");
    tmp2 = tmp;



  } // tmp2, tmp deleted

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestInitialAccess, call_sequence_assign) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_backend::MockFlow f_in_1, f_out_1, f_in_2, f_out_2;

  Sequence s1, s2, s5, s6, s7;


  // These next two calls can come in either order
  EXPECT_CALL(*mock_runtime, make_initial_flow(_))
    .InSequence(s1, s5)
    .WillOnce(Return(&f_in_1));
  EXPECT_CALL(*mock_runtime, make_null_flow(_))
    .InSequence(s2, s6)
    .WillOnce(Return(&f_out_1));

  // These next two calls can come in either order
  EXPECT_CALL(*mock_runtime, make_initial_flow(_))
    .InSequence(s5, s1)
    .WillOnce(Return(&f_in_2));
  EXPECT_CALL(*mock_runtime, make_null_flow(_))
    .InSequence(s6, s2)
    .WillOnce(Return(&f_out_2));

  // This must be the last thing in all sequences
  EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_in_1, &f_out_1))
    .InSequence(s1, s2, s7);

  EXPECT_RELEASE_FLOW(f_in_1).InSequence(s1);
  EXPECT_RELEASE_FLOW(f_out_1).InSequence(s2);

  // This must be the last thing in all sequences
  EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_in_2, &f_out_2))
    .InSequence(s5, s6, s7);

  EXPECT_RELEASE_FLOW(f_in_2).InSequence(s5);
  EXPECT_RELEASE_FLOW(f_out_2).InSequence(s6);

  {

    auto tmp1 = initial_access<int>("hello");

    // Replace tmp1
    tmp1 = initial_access<int>("world");

  } // tmp1

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestInitialAccess, call_sequence_copy_assign) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::detail;
  using namespace darma_runtime::keyword_arguments_for_publication;

  DECLARE_MOCK_FLOWS(f_in_1, f_out_1, f_in_2, f_out_2);

  {
    InSequence s;

    EXPECT_INITIAL_ACCESS(f_in_2, f_out_2, make_key("world"));
    EXPECT_INITIAL_ACCESS(f_in_1, f_out_1, make_key("hello"));

    EXPECT_FLOW_ALIAS(f_in_1, f_out_1);

    EXPECT_CALL(*sequence_marker, mark_sequence("in between"));

    EXPECT_FLOW_ALIAS(f_in_2, f_out_2);
  }

  {
    auto tmp2 = initial_access<int>("world");
    auto tmp1 = initial_access<int,
      darma::advanced::access_handle_traits::copy_assignable<true>
    >("hello");

    // Replace tmp1, since that should be allowed now
    tmp1 = tmp2;

    sequence_marker->mark_sequence("in between");

  }

}

////////////////////////////////////////////////////////////////////////////////
