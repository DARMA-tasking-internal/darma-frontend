/*
//@HEADER
// ************************************************************************
//
//                      test_collectives.cc
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


#include <gtest/gtest.h>

#include <vector>

#include <darma/impl/collective/allreduce.h>

#include "test_frontend.h"

////////////////////////////////////////////////////////////////////////////////

class TestCollectives
  : public TestFrontend
{
  protected:

    virtual void SetUp() {
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

TEST_F(TestCollectives, simple_allreduce) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_collectives;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow f_init, f_null, f_task_out, f_collect_out;
  use_t* task_use = nullptr;
  use_t* reduce_use = nullptr;

  expect_initial_access(f_init, f_null, make_key("hello"));

  expect_mod_capture_MN_or_MR(f_init, f_task_out, task_use);

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(task_use))));

  EXPECT_CALL(*mock_runtime, make_next_flow(&f_task_out))
    .WillOnce(Return(&f_collect_out));

  EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows(
    &f_task_out, &f_collect_out, use_t::None, use_t::Modify
  ))).WillOnce(SaveArg<0>(&reduce_use));

  // TODO check piece and n_pieces

  EXPECT_CALL(*mock_runtime, allreduce_use(
    Eq(ByRef(reduce_use)), Eq(ByRef(reduce_use)), _, Eq(make_key("world"))
  ));

  EXPECT_CALL(*mock_runtime, release_use(Eq(ByRef(reduce_use))))
    .WillOnce(Invoke([&](auto&&...){ reduce_use = nullptr; }));

  {
    auto tmp = initial_access<int>("hello");

    create_work([=]{
      tmp.set_value(42);
    });

    allreduce(tmp, piece=0, n_pieces=10, tag="world");

  }

  EXPECT_CALL(*mock_runtime, release_use(Eq(ByRef(task_use))))
    .WillOnce(Invoke([&](auto&&...){ task_use = nullptr; }));

  mock_runtime->registered_tasks.clear();

}
