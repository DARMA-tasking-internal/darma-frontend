/*
//@HEADER
// ************************************************************************
//
//                          test_initial_access.cc
//                         dharma_new
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

#include <darma/impl/handle.h>
#include <darma/interface/app/initial_access.h>

#include <darma/serialization/serializers/all.h>

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
  using namespace darma;

  DECLARE_MOCK_FLOWS(f_in_1, f_out_1);
  use_t* use_init = nullptr;

  {
    InSequence s;

    EXPECT_CALL(*mock_runtime, make_initial_flow(_))
      .WillOnce(Return(f_in_1));

    EXPECT_CALL(*mock_runtime, make_null_flow(_))
      .WillOnce(Return(f_out_1));

    EXPECT_REGISTER_USE(use_init, f_in_1, f_out_1, Modify, None);

    EXPECT_FLOW_ALIAS(f_in_1, f_out_1);

    EXPECT_RELEASE_USE(use_init);
  }



  //============================================================================
  // Actual code being tested
  {
    auto tmp = initial_access<int>("hello");
  } // tmp deleted
  //============================================================================

}

////////////////////////////////////////////////////////////////////////////////

// Same as call_sequence, but uses helper to verify that other uses of helper should be valid
TEST_F(TestInitialAccess, call_sequence_helper) {
  using namespace ::testing;
  using namespace darma;
  using namespace mock_backend;

  DECLARE_MOCK_FLOWS(f_in, f_out);
  use_t* use;

  {
    InSequence s;

    EXPECT_INITIAL_ACCESS(f_in, f_out, use, make_key("hello"));

    EXPECT_FLOW_ALIAS(f_in, f_out);

    EXPECT_RELEASE_USE(use);
  }

  //============================================================================
  // Actual code being tested
  {
    auto tmp = initial_access<int>("hello");
  } // tmp deleted
  //============================================================================

}

////////////////////////////////////////////////////////////////////////////////

// Same as call_sequence, but with delayed assignment
TEST_F(TestInitialAccess, call_sequence_helper_2) {
  using namespace ::testing;
  using namespace darma;
  using namespace mock_backend;

  DECLARE_MOCK_FLOWS(f_in, f_out, f_in_2, f_out_2);
  use_t* use_init;

  EXPECT_INITIAL_ACCESS(f_in, f_out, use_init, make_key("hello"));

  EXPECT_FLOW_ALIAS(f_in, f_out);
  EXPECT_RELEASE_USE(use_init);

  //============================================================================
  // Actual code being tested
  {
    AccessHandleWithTraits<int,
      advanced::access_handle_traits::allow_copy_assignment_from_this<true>
    > tmp;
    AccessHandle<int> tmp2;

    tmp = initial_access<int>("hello");
    tmp2 = tmp;

  } // tmp2, tmp deleted
  //============================================================================

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestInitialAccess, call_sequence_assign) {
  using namespace ::testing;
  using namespace darma;
  using namespace darma::keyword_arguments_for_publication;

  DECLARE_MOCK_FLOWS(f_in_1, f_out_1, f_in_2, f_out_2);
  use_t* use_init1 = nullptr, *use_init2 = nullptr;

  {
    InSequence s;
    EXPECT_INITIAL_ACCESS(f_in_1, f_out_1, use_init1, make_key("hello"));

    EXPECT_INITIAL_ACCESS(f_in_2, f_out_2, use_init2, make_key("world"));

    EXPECT_FLOW_ALIAS(f_in_1, f_out_1);
    EXPECT_RELEASE_USE(use_init1);

    EXPECT_FLOW_ALIAS(f_in_2, f_out_2);
    EXPECT_RELEASE_USE(use_init2);
  }


  //============================================================================
  // Actual code being tested
  {

    auto tmp1 = initial_access<int>("hello");

    // Replace tmp1
    tmp1 = initial_access<int>("world");

  } // tmp1
  //============================================================================

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestInitialAccess, call_sequence_copy_assign) {
  using namespace ::testing;
  using namespace darma;
  using namespace darma::detail;
  using namespace darma::keyword_arguments_for_publication;

  DECLARE_MOCK_FLOWS(f_in_1, f_out_1, f_in_2, f_out_2);
  use_t* use1 = nullptr, *use2 = nullptr;

  {
    InSequence s;

    EXPECT_INITIAL_ACCESS(f_in_2, f_out_2, use2, make_key("world"));
    EXPECT_INITIAL_ACCESS(f_in_1, f_out_1, use1, make_key("hello"));

    EXPECT_FLOW_ALIAS(f_in_2, f_out_2);
    EXPECT_RELEASE_USE(use2);

    EXPECT_CALL(*sequence_marker, mark_sequence("in between"));

    EXPECT_FLOW_ALIAS(f_in_1, f_out_1);
    EXPECT_RELEASE_USE(use1);
  }

  //============================================================================
  // Actual code being tested
  {
    auto tmp2 = initial_access<int>("world");
    auto tmp1 = initial_access<int,
      advanced::access_handle_traits::allow_copy_assignment_from_this<true>
    >("hello");

    // Replace tmp2 with tmp1 (leaving a hanging alias to tmp1), since that
    // should be allowed now
    tmp2 = tmp1;

    sequence_marker->mark_sequence("in between");

  }
  //============================================================================

}

////////////////////////////////////////////////////////////////////////////////
