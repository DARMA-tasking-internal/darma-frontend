/*
//@HEADER
// ************************************************************************
//
//                      test_functor_migrate.cc
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

#include "test_functor.h"

#include <darma/interface/frontend/unpack_task.h>

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simple_migrate) {
  using namespace ::testing;
  using namespace mock_backend;

  testing::internal::CaptureStdout();

  mock_runtime->save_tasks = true;

  MockFlow f_init, f_set_42_out, f_null;
  use_t* set_42_use, *hello_use, *migrated_use;
  set_42_use = hello_use = migrated_use = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, make_key("hello"));

  EXPECT_LEAF_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(f_init, f_set_42_out, set_42_use, value);

  EXPECT_REGISTER_TASK(set_42_use);

  EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR_AND_SET_BUFFER(f_set_42_out, hello_use, value);

  EXPECT_REGISTER_TASK(hello_use);

  EXPECT_FLOW_ALIAS(f_set_42_out, f_null);

  //============================================================================
  // Code to actually be tested
  {
    struct SetTo42 {
      //void operator()(AccessHandle<int>& val) const { val.set_value(42); }
      void operator()(int& val) const { val = 42; }
    };

    struct HelloWorldNumber {
      void operator()(int const& val) const {
        std::cout << "Hello World " << val << std::endl;
      }
    };

    auto tmp = initial_access<int>("hello");

    create_work<SetTo42>(tmp);

    create_work<HelloWorldNumber>(tmp);

  }
  //============================================================================

  EXPECT_RELEASE_USE(set_42_use);

  // run the first task...
  mock_runtime->registered_tasks.front()->run();
  mock_runtime->registered_tasks.pop_front();

  // Now migrate the task...
  auto& task_to_migrate = mock_runtime->registered_tasks.front();
  size_t task_packed_size = task_to_migrate->get_packed_size();

  char buffer[task_packed_size];

  task_to_migrate->pack(buffer);

  EXPECT_RELEASE_USE(hello_use);

  // Release the task on the "origin node"
  task_to_migrate = nullptr;

  EXPECT_CALL(*mock_runtime, reregister_migrated_use(_))
    .WillOnce(Invoke([&](auto&& rereg_use) {
      // Set up the flows again...
      rereg_use->set_in_flow(f_set_42_out);
      rereg_use->set_out_flow(f_set_42_out);
      rereg_use->get_data_pointer_reference() = &value;
      migrated_use = rereg_use;
    }));

  auto migrated_task = darma_runtime::frontend::unpack_task(buffer);

  EXPECT_RELEASE_USE(migrated_use);

  migrated_task->run();

  migrated_task = nullptr;

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "Hello World 42\n"
  );
}

////////////////////////////////////////////////////////////////////////////////
