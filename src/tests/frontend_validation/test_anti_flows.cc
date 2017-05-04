/*
//@HEADER
// ************************************************************************
//
//                      test_anti_flows.cc
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

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(anti_flows)

#include <gtest/gtest.h>
#include <gmock/gmock.h>


#include "mock_backend.h"
#include "test_frontend.h"

#include <darma/interface/app/initial_access.h>
#include <darma/interface/app/read_access.h>
#include <darma/interface/app/create_work.h>
#include <darma/impl/data_store.h>
#include <darma/impl/task_collection/task_collection.h>
#include <darma/impl/task_collection/handle_collection.h>
#include <darma/impl/index_range/mapping.h>
#include <darma/impl/array/index_range.h>
#include <darma/impl/task_collection/create_concurrent_work.h>

// TODO test conversion/assignment to default constructed AccessHandleCollection

////////////////////////////////////////////////////////////////////////////////


class TestAntiFlows
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

TEST_F(TestAntiFlows, high_level_example_0) {
  using namespace ::testing;
  using namespace darma_runtime;

  DECLARE_MOCK_FLOWS(f0, f1, f2);
  DECLARE_MOCK_ANTI_FLOWS(fbar0, fbar1);

  int value = 42;

  use_t* u0, *u1, *u2, *u3;

  {
    InSequence seq;

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS(u0,
      f0, Initial, nullptr,
      f1, Null, nullptr, false,
      nullptr, Insignificant, nullptr,
      fbar0, Initial, nullptr, false,
      Modify, None, false
    );

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS_AND_SET_BUFFER(u1,
      f0, Same, &f0,
      f2, Next, nullptr, true,
      fbar0, Same, &fbar0,
      nullptr, Insignificant, nullptr, false,
      None, Modify, true, value
    );

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS(u2,
      f2, Same, &f2,
      f1, Same, &f1, false,
      nullptr, Same, nullptr,
      fbar1, Next, &fbar0, false,
      Modify, None, false
    );

    EXPECT_NEW_RELEASE_USE(u0, false);

    EXPECT_REGISTER_TASK(u1);

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS_AND_SET_BUFFER(u3,
      f2, Same, &f2,
      nullptr, Insignificant, nullptr, false,
      nullptr, Insignificant, nullptr,
      fbar1, Same, &fbar1, false,
      None, Read, true, value
    );

    EXPECT_REGISTER_TASK(u3);

    EXPECT_NEW_RELEASE_USE(u2, true);

  }

  //============================================================================
  // actual code being tested
  {

    struct A { void operator()(int& v) const { ASSERT_EQ(v, 42); v = 73; } };

    struct B { void operator()(int const& v) const { ASSERT_EQ(v, 73); } };

    auto x = initial_access<int>("hello");

    // NM capture
    create_work<A>(x);

    // NR capture
    create_work<B>(x);

  }
  //============================================================================

  EXPECT_NEW_RELEASE_USE(u1, false);

  run_one_task();

  ASSERT_EQ(value, 73);

  EXPECT_NEW_RELEASE_USE(u3, false);

  run_one_task();

  ASSERT_EQ(value, 73);

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAntiFlows, high_level_example_1) {
  using namespace ::testing;
  using namespace darma_runtime;

  DECLARE_MOCK_FLOWS(f0, f1, f2, f3, f4, f5, f6);
  DECLARE_MOCK_ANTI_FLOWS(fbar0, fbar1, fbar2, fbar3, fbar4, fbar5);

  int value = 42;

  use_t* u0, *u1, *u2, *u3, *u4, *u5, *u6, *u8, *u9, *u10;

  {
    InSequence seq;

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS(u0,
      f0, Initial, nullptr,
      f1, Null, nullptr, false,
      nullptr, Insignificant, nullptr,
      fbar0, Initial, nullptr, false,
      Modify, None, false
    );

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS_AND_SET_BUFFER(u1,
      f0, Same, &f0,
      f2, Next, nullptr, true,
      fbar0, Same, &fbar0,
      fbar1, Next, nullptr, true,
      Modify, Modify, true, value
    );

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS(u8,
      f2, Same, &f2,
      f1, Same, &f1, false,
      nullptr, Same, nullptr,
      fbar1, Same, &fbar1, false,
      Modify, None, false
    );

    EXPECT_NEW_RELEASE_USE(u0, false);

    EXPECT_REGISTER_TASK(u1);

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS_AND_SET_BUFFER(u9,
      f2, Same, &f2,
      f6, Next, nullptr, true,
      fbar1, Same, &fbar1,
      fbar5, Next, nullptr, true,
      Modify, Modify, true, value
    );

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS(u10,
      f6, Same, &f6,
      f1, Same, &f1, false,
      nullptr, Same, nullptr,
      fbar5, Same, &fbar5, false,
      Modify, None, false
    );

    EXPECT_NEW_RELEASE_USE(u8, false);

    EXPECT_REGISTER_TASK(u9);

    EXPECT_NEW_RELEASE_USE(u10, true);

  }

  //============================================================================
  // actual code being tested
  {

    auto x = initial_access<int>("hello");
    create_work([=]{
      x.set_value(42);
      create_work([=]{ EXPECT_EQ(x.get_value(), 42); x.set_value(73); });
      create_work([=]{ EXPECT_EQ(x.get_value(), 73); x.set_value(314); });
      create_work(reads(x), [=]{ EXPECT_EQ(x.get_value(), 314); });
    });
    create_work([=]{ EXPECT_EQ(x.get_value(), 314); x.set_value(0xFEEDFACE); });


  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  {
    InSequence s1;

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS_AND_SET_BUFFER(u2,
      f3, Forwarding, &f0,
      f4, Next, nullptr, true,
      fbar2, Forwarding, &fbar1,
      fbar3, Next, nullptr, true,
      Modify, Modify, true, value
    );

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS(u3,
      f4, Same, &f4,
      f2, Same, &f2, false,
      fbar1, Same, &fbar1,
      fbar3, Same, &fbar3, false,
      Modify, None, false
    );

    EXPECT_NEW_RELEASE_USE(u1, false);

    EXPECT_REGISTER_TASK(u2);

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS_AND_SET_BUFFER(u4,
      f4, Same, &f4,
      f5, Next, nullptr, true,
      fbar3, Same, &fbar3,
      fbar4, Next, nullptr, true,
      Modify, Modify, true, value
    );

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS(u5,
      f5, Same, &f5,
      f2, Same, &f2, false,
      fbar1, Same, &fbar1,
      fbar4, Same, &fbar4, false,
      Modify, None, false
    );

    EXPECT_NEW_RELEASE_USE(u3, false);

    EXPECT_REGISTER_TASK(u4);

    EXPECT_REGISTER_USE_WITH_ANTI_FLOWS_AND_SET_BUFFER(u6,
      f5, Same, &f5,
      nullptr, Insignificant, nullptr, false,
      nullptr, Insignificant, nullptr,
      fbar4, Same, &fbar4, false,
      Read, Read, true, value
    );

    EXPECT_REGISTER_TASK(u6);

    EXPECT_NEW_RELEASE_USE(u5, true);

  }

  run_one_task();

  ASSERT_EQ(value, 42);

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  auto last_task = std::move(mock_runtime->registered_tasks.front());
  mock_runtime->registered_tasks.pop_front();

  EXPECT_NEW_RELEASE_USE(u2, false);

  run_one_task();

  ASSERT_EQ(value, 73);

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_NEW_RELEASE_USE(u4, false);

  run_one_task();

  ASSERT_EQ(value, 314);

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_NEW_RELEASE_USE(u6, false);

  run_one_task();

  ASSERT_EQ(value, 314);

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_NEW_RELEASE_USE(u9, false);

  last_task->run();
  last_task = nullptr;

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  ASSERT_EQ(mock_runtime->registered_tasks.size(), 0);

  ASSERT_EQ(value, 0xFEEDFACE);

}

////////////////////////////////////////////////////////////////////////////////
#endif
