/*
//@HEADER
// ************************************************************************
//
//                      test_create_work_if.cc
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#include <darma/impl/create_work_while.h>
#include <darma/interface/app/initial_access.h>

////////////////////////////////////////////////////////////////////////////////

class TestCreateWorkWhile
  : public TestFrontend
{
  protected:

    virtual void SetUp() {
      using namespace ::testing;

      setup_mock_runtime<::testing::NiceMock>();
      TestFrontend::SetUp();
      ON_CALL(*mock_runtime, get_running_task())
        .WillByDefault(Return(top_level_task.get()));
    }

    virtual void TearDown() {
      TestFrontend::TearDown();
    }

};

using namespace darma_runtime;
using namespace darma_runtime::experimental;

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkWhile, basic_same_always_false) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_while_out
  );
  use_t* while_use = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_while_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, f_while_out, Modify, Read, value);

  EXPECT_REGISTER_TASK(while_use);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");

    create_work_while([=]{
      return tmp.get_value() != 0; // should always be False
    }).do_([=]{
      tmp.set_value(73);
      FAIL() << "Ran do clause when while part should have been false";
    });

  }
  //============================================================================


  EXPECT_RELEASE_USE(while_use);

  run_all_tasks();


}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkWhile, basic_same_one_iteration) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_while_out, f_do_out,
    f_while_fwd, f_while_out_2
  );
  use_t* while_use = nullptr;
  use_t* do_use = nullptr;
  use_t* while_use_2 = nullptr;
  use_t* do_cont_use = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_while_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, f_while_out, Modify, Read, value);

  EXPECT_REGISTER_TASK(while_use);

  EXPECT_FLOW_ALIAS(f_while_out, f_null);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");

    create_work_while([=]{
      return tmp.get_value() != 73; // should only be false the first time
    }).do_([=]{
      tmp.set_value(73);
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_do_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(do_use, f_init, f_do_out, Modify, Modify, value);

  EXPECT_RELEASE_USE(while_use);

  EXPECT_REGISTER_TASK(do_use);

  EXPECT_FLOW_ALIAS(f_do_out, f_while_out);

  run_one_task(); // the first while

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_forwarding_flow(f_init))
    .WillOnce(Return(f_while_fwd));
  EXPECT_CALL(*mock_runtime, make_next_flow(f_while_fwd))
    .WillOnce(Return(f_while_out_2));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use_2, f_while_fwd, f_while_out_2,
    Modify, Read, value
  );

  EXPECT_RELEASE_USE(do_use);

  EXPECT_REGISTER_TASK(while_use_2);

  EXPECT_FLOW_ALIAS(f_while_out_2, f_do_out);

  run_one_task(); // the do part

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_RELEASE_USE(while_use_2);

  run_one_task(); // the second while

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_THAT(value, Eq(73));

}

////////////////////////////////////////////////////////////////////////////////
