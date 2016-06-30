/*
//@HEADER
// ************************************************************************
//
//                   test_create_work_be.cc
//                         darma
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
#include <gmock/gmock.h>
#include <memory>
#include <atomic>

#ifdef TEST_BACKEND_INCLUDE
#  include TEST_BACKEND_INCLUDE
#endif

#include "test_backend.h"

using namespace darma_runtime;
using darma_runtime::detail::backend_runtime;
typedef darma_runtime::abstract::backend::Flow flow_t;
typedef darma_runtime::detail::VariableHandle<int> handle_t;
typedef darma_runtime::detail::HandleUse use_t;
typedef darma_runtime::abstract::backend::Runtime runtime_t;

class TestFlows
  : public TestBackend
{
 protected:
  virtual void SetUp() {
    TestBackend::SetUp();
  }

  virtual void TearDown() {
    TestBackend::TearDown();
  }
};

//////////////////////////////////////////////////////////////////////////////////

// test making an initial flow that we never bother to register
TEST_F(TestFlows, make_initial){
  darma_init(argc_, argv_);

  types::key_t k = make_key("a");
  handle_t handle(k);

  flow_t *f1 = backend_runtime->make_initial_flow(&handle);
  EXPECT_FALSE(f1 == nullptr);

  darma_finalize();
}

// test making a null flow that we never bother to register
TEST_F(TestFlows, make_null){
  darma_init(argc_, argv_);

  types::key_t k = make_key("a");
  handle_t handle(k);

  flow_t *f1 = backend_runtime->make_null_flow(&handle);
  EXPECT_FALSE(f1 == nullptr);

  darma_finalize();
}

// test registering an initial use
// builds upon TestFlows::make_initial() and TestFlows::make_null()
TEST_F(TestFlows, register_initial_use){
  darma_init(argc_, argv_);

  types::key_t k = make_key("a");
  handle_t handle(k);

  flow_t *f1 = backend_runtime->make_initial_flow(&handle);
  EXPECT_FALSE(f1 == nullptr);

  flow_t *f2 = backend_runtime->make_null_flow(&handle);
  EXPECT_FALSE(f2 == nullptr);

  EXPECT_FALSE(f2 == f1);

  use_t use(&handle, f1, f2, use_t::Modify, use_t::None);
  backend_runtime->register_use(&use);
  backend_runtime->release_use(&use);

  darma_finalize();
}

// test making a same flow that we never bother to register
// builds upon TestFlows::register_initial_use()
TEST_F(TestFlows, make_same){
  darma_init(argc_, argv_);

  types::key_t k = make_key("a");
  handle_t handle(k);

  flow_t *f1 = backend_runtime->make_initial_flow(&handle);
  flow_t *f2 = backend_runtime->make_null_flow(&handle);

  use_t use(&handle, f1, f2, use_t::Modify, use_t::None);
  backend_runtime->register_use(&use);

  flow_t *f3 = backend_runtime->make_same_flow(f1, runtime_t::Input);
  EXPECT_FALSE(f3 == nullptr);

  EXPECT_FALSE(f3 == f1);

  backend_runtime->release_use(&use);

  darma_finalize();
}

// test making a next flow that we never bother to register
// builds upon TestFlows::make_same()
TEST_F(TestFlows, make_next){
  darma_init(argc_, argv_);

  types::key_t k = make_key("a");
  handle_t handle(k);

  flow_t *f1 = backend_runtime->make_initial_flow(&handle);
  flow_t *f2 = backend_runtime->make_null_flow(&handle);

  use_t use(&handle, f1, f2, use_t::Modify, use_t::None);
  backend_runtime->register_use(&use);

  flow_t *f3 = backend_runtime->make_same_flow(f1, runtime_t::Input);
  flow_t *f4 = backend_runtime->make_next_flow(f3, runtime_t::Output);
  EXPECT_FALSE(f4 == nullptr);

  EXPECT_FALSE(f4 == f3);

  backend_runtime->release_use(&use);

  darma_finalize();
}

// test registering a next use that we never actually associate with a task
// builds upon TestFlows::make_next()
TEST_F(TestFlows, register_next_use){
  // FIXME: this test contains no expectations
  darma_init(argc_, argv_);

  types::key_t k = make_key("a");
  handle_t handle(k);

  flow_t *f1 = backend_runtime->make_initial_flow(&handle);
  flow_t *f2 = backend_runtime->make_null_flow(&handle);

  use_t use(&handle, f1, f2, use_t::Modify, use_t::None);
  backend_runtime->register_use(&use);

  flow_t *f3 = backend_runtime->make_same_flow(f1, runtime_t::Input);
  flow_t *f4 = backend_runtime->make_next_flow(f3, runtime_t::Output);

  use_t next_use(&handle, f3, f4, use_t::Modify, use_t::Modify);
  backend_runtime->register_use(&next_use);
  backend_runtime->release_use(&use);

  backend_runtime->release_use(&next_use);

  darma_finalize();
}

// test making a forwarding flow that we never bother to register
// builds upon TestFlows::register_next_use()
TEST_F(TestFlows, make_forwarding){
  darma_init(argc_, argv_);

  types::key_t k = make_key("a");
  handle_t handle(k);

  flow_t *f1 = backend_runtime->make_initial_flow(&handle);
  flow_t *f2 = backend_runtime->make_null_flow(&handle);

  use_t use(&handle, f1, f2, use_t::Modify, use_t::None);
  backend_runtime->register_use(&use);

  flow_t *f3 = backend_runtime->make_same_flow(f1, runtime_t::Input);
  flow_t *f4 = backend_runtime->make_next_flow(f3, runtime_t::Output);

  use_t next_use(&handle, f3, f4, use_t::Modify, use_t::Modify);
  backend_runtime->register_use(&next_use);

  flow_t *f5 = backend_runtime->make_forwarding_flow(f3, runtime_t::ForwardingChanges);
  flow_t *f6 = backend_runtime->make_next_flow(f5, runtime_t::Output);
  EXPECT_FALSE(f5 == nullptr);
  EXPECT_FALSE(f6 == nullptr);

  EXPECT_FALSE(f5 == f3);
  EXPECT_FALSE(f6 == f5);

  backend_runtime->release_use(&use);
  backend_runtime->release_use(&next_use);

  darma_finalize();
}

// test registering a forwarding use that we never actually associate with a task
// builds upon TestFlows::make_forwarding()
TEST_F(TestFlows, register_forwarding_use){
  // FIXME: this test contains no expectations
  darma_init(argc_, argv_);

  types::key_t k = make_key("a");
  handle_t handle(k);

  flow_t *f1 = backend_runtime->make_initial_flow(&handle);
  flow_t *f2 = backend_runtime->make_null_flow(&handle);

  use_t use(&handle, f1, f2, use_t::Modify, use_t::None);
  backend_runtime->register_use(&use);

  flow_t *f3 = backend_runtime->make_same_flow(f1, runtime_t::Input);
  flow_t *f4 = backend_runtime->make_next_flow(f3, runtime_t::Output);

  use_t next_use(&handle, f3, f4, use_t::Modify, use_t::Modify);
  backend_runtime->register_use(&next_use);
  backend_runtime->release_use(&use);

  flow_t *f5 = backend_runtime->make_forwarding_flow(f3, runtime_t::ForwardingChanges);
  flow_t *f6 = backend_runtime->make_next_flow(f5, runtime_t::Output);

  use_t fwd_use(&handle, f5, f6, use_t::Modify, use_t::Modify);
  backend_runtime->register_use(&fwd_use);
  backend_runtime->release_use(&next_use);

  backend_runtime->release_use(&fwd_use);

  darma_finalize();
}

// test making a same flow (that we never bother to register) for the output of
//   a read operation
// builds upon TestFlows::make_forwarding()
TEST_F(TestFlows, make_same_read_output){
  darma_init(argc_, argv_);

  types::key_t k = make_key("a");
  handle_t handle(k);

  flow_t *f1 = backend_runtime->make_initial_flow(&handle);
  flow_t *f2 = backend_runtime->make_null_flow(&handle);

  use_t use(&handle, f1, f2, use_t::Modify, use_t::None);
  backend_runtime->register_use(&use);

  flow_t *f3 = backend_runtime->make_same_flow(f1, runtime_t::Input);
  flow_t *f4 = backend_runtime->make_next_flow(f3, runtime_t::Output);

  use_t next_use(&handle, f3, f4, use_t::Modify, use_t::Modify);
  backend_runtime->register_use(&next_use);
  backend_runtime->release_use(&use);

  flow_t *f5 = backend_runtime->make_forwarding_flow(f3, runtime_t::ForwardingChanges);
  flow_t *f6 = backend_runtime->make_same_flow(f5, runtime_t::OutputFlowOfReadOperation);
  EXPECT_FALSE(f5 == nullptr);

  EXPECT_FALSE(f6 == f5);

  backend_runtime->release_use(&next_use);

  darma_finalize();
}

// test registering a use (that we never actually associate with a task) for the
//   output of a read operation
// builds upon TestFlows::make_same_read_output()
TEST_F(TestFlows, register_read_only_use){
  // FIXME: this test contains no expectations
  darma_init(argc_, argv_);

  types::key_t k = make_key("a");
  handle_t handle(k);

  flow_t *f1 = backend_runtime->make_initial_flow(&handle);
  flow_t *f2 = backend_runtime->make_null_flow(&handle);

  use_t use(&handle, f1, f2, use_t::Modify, use_t::None);
  backend_runtime->register_use(&use);

  flow_t *f3 = backend_runtime->make_same_flow(f1, runtime_t::Input);
  flow_t *f4 = backend_runtime->make_next_flow(f3, runtime_t::Output);

  use_t next_use(&handle, f3, f4, use_t::Modify, use_t::Modify);
  backend_runtime->register_use(&next_use);
  backend_runtime->release_use(&use);

  flow_t *f5 = backend_runtime->make_forwarding_flow(f3, runtime_t::ForwardingChanges);
  flow_t *f6 = backend_runtime->make_next_flow(f5, runtime_t::OutputFlowOfReadOperation);

  use_t read_use(&handle, f5, f6, use_t::Modify, use_t::Read);
  backend_runtime->register_use(&read_use);
  backend_runtime->release_use(&next_use);

  backend_runtime->release_use(&read_use);

  darma_finalize();
}

//////////////////////////////////////////////////////////////////////////////////

