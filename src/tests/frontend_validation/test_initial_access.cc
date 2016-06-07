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

  EXPECT_CALL(*mock_runtime, register_use(_))
    .InSequence(s1, s2);

  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&f_in_1, &f_out_1, use_t::Modify, use_t::None)
  )).InSequence(s1);

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
  use_t* use_ptr;

  expect_initial_access(f_in, f_out, use_ptr, make_key("hello"));

  {
    auto tmp = initial_access<int>("hello");
    ASSERT_THAT(use_ptr->get_in_flow(), Eq(&f_in));
    ASSERT_THAT(use_ptr->get_out_flow(), Eq(&f_out));
  } // tmp deleted

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestInitialAccess, call_sequence_assign) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_backend::MockFlow f_in_1, f_out_1, f_in_2, f_out_2;

  Sequence s1, s2, s3, s4, s5, s6;


  // These next two calls can come in either order
  EXPECT_CALL(*mock_runtime, make_initial_flow(_))
    .InSequence(s1, s5)
    .WillOnce(Return(&f_in_1));
  EXPECT_CALL(*mock_runtime, make_null_flow(_))
    .InSequence(s2, s6)
    .WillOnce(Return(&f_out_1));

  // Must follow make_initial_flow and make_null_flow
  EXPECT_CALL(*mock_runtime, register_use(_))
    .InSequence(s1, s2, s5);

  // This can come before or after the registration and setup of the new handle
  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&f_in_1, &f_out_1, use_t::Modify, use_t::None)
  )).InSequence(s1, s2);


  // These next two calls can come in either order
  EXPECT_CALL(*mock_runtime, make_initial_flow(_))
    .InSequence(s5, s3)
    .WillOnce(Return(&f_in_2));
  EXPECT_CALL(*mock_runtime, make_null_flow(_))
    .InSequence(s6, s4)
    .WillOnce(Return(&f_out_2));
  // This must come after the registration and setup of the first handle
  EXPECT_CALL(*mock_runtime, register_use(_))
    .InSequence(s3, s4, s5);

  // This must be the last thing in all sequences
  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&f_in_2, &f_out_2, use_t::Modify, use_t::None)
  )).InSequence(s1, s2, s3, s4, s5);

  {

    auto tmp1 = initial_access<int>("hello");

    // Replace tmp1
    tmp1 = initial_access<int>("world");

  } // tmp1

}

////////////////////////////////////////////////////////////////////////////////

