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

#include <darma.h>
#include <darma/impl/create_if_then.h>

////////////////////////////////////////////////////////////////////////////////

class TestCreateWorkIf
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

////////////////////////////////////////////////////////////////////////////////

using namespace darma_runtime;
using namespace darma_runtime::experimental;

#if 0
TEST_F(TestCreateWorkIf, basic_different_always_true) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace darma_runtime::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow f_cond_init("f_cond_init"), f_cond_null("f_cond_null");
  MockFlow f_cap_init("f_cap_init"), f_cap_null("f_cap_null"),
    f_cap_if_out("f_cap_if_out"), f_cap_cont("f_cap_cont");
  use_t* cond_use = nullptr;
  use_t* cap_if_use = nullptr;
  use_t* cap_then_use = nullptr;
  use_t* cap_then_cont_use = nullptr;

  int cond_value = 0;
  int cap_value = 0;

  EXPECT_INITIAL_ACCESS(f_cond_init, f_cond_null, make_key("hello"));

  EXPECT_INITIAL_ACCESS(f_cap_init, f_cap_null, make_key("world"));

  EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR_AND_SET_BUFFER(f_cond_init, cond_use, cond_value);

  {
    // TODO decide if this should be done differently...

    InSequence two_nexts;
    EXPECT_CALL(*mock_runtime, make_next_flow(f_cap_init))
      .WillOnce(Return(f_cap_if_out));

    EXPECT_CALL(*mock_runtime, make_next_flow(f_cap_init))
      .WillOnce(Return(f_cap_cont));

  }

  EXPECT_REGISTER_USE(cap_if_use, f_cap_init, f_cap_if_out, Modify, None);

  EXPECT_REGISTER_USE_AND_SET_BUFFER(cap_then_use, f_cap_init, f_cap_cont, Modify, Modify, cap_value);

  // TODO get rid of this in the code
  EXPECT_REGISTER_USE(cap_then_cont_use, f_cap_cont, f_cap_if_out, Modify, None);


  EXPECT_REGISTER_TASK(cond_use, cap_if_use);

  EXPECT_REGISTER_TASK(cap_then_use);

  //============================================================================
  // actual code being tested
  {

    auto cond_val = initial_access<int>("hello");
    auto cap_val = initial_access<int>("world");

    create_work_if([=]{
      return cond_val.get_value() != 42; // should always be true
    }).then_([=]{
      cap_val.set_value(73);
    });

  }
  //============================================================================

  EXPECT_RELEASE_USE(cond_use);
  EXPECT_RELEASE_USE(cap_if_use);
  EXPECT_RELEASE_USE(cap_then_cont_use);
  EXPECT_FLOW_ALIAS(f_cap_cont, f_cap_if_out);

  run_one_task();

  EXPECT_RELEASE_USE(cap_then_use);

  run_one_task();

  EXPECT_THAT(cap_value, Eq(73));

}
#endif
