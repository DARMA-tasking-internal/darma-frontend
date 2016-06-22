/*
//@HEADER
// ************************************************************************
//
//                          test_register_task.cc
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
#include <gmock/gmock.h>

#ifdef TEST_BACKEND_INCLUDE
#  include TEST_BACKEND_INCLUDE
#endif

#include "test_backend.h"
#include "mock_frontend.h"
#include "helpers.h"

using namespace darma_runtime;
using namespace mock_frontend;

class TestRegisterTask
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

TEST_F(TestRegisterTask, initial_access_allocate) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::detail;
  using use_t = MockUse;
  using runtime_t = darma_runtime::abstract::backend::Runtime;

  Sequence s;

  MockHandle* handle = MockHandle::create<int>(make_key("hello"));
  NiceMock<MockUse> u_init(handle,
    backend_runtime->make_initial_flow(handle),
    backend_runtime->make_null_flow(handle),
    use_t::Modify, use_t::None
  );
  auto* cap_in = backend_runtime->make_same_flow(u_init.in_flow_, runtime_t::Input);
  auto* cap_out = backend_runtime->make_next_flow(cap_in, runtime_t::Output);
  NiceMock<MockUse> u_cap( handle, cap_in, cap_out, use_t::Modify, use_t::Modify );
  auto* con_in = backend_runtime->make_same_flow(u_cap.out_flow_, runtime_t::Input);
  auto* con_out = backend_runtime->make_same_flow(u_init.out_flow_, runtime_t::Output);
  NiceMock<MockUse> u_con( handle, con_in, con_out, use_t::Modify, use_t::None );

  backend_runtime->register_use(&u_init);
  backend_runtime->register_use(&u_cap);
  backend_runtime->register_use(&u_con);
  backend_runtime->release_use(&u_init);

  types::handle_container_template<abstract::frontend::Use*> captured_uses = { &u_cap };
  MockTask task;
  EXPECT_CALL(task, get_dependencies())
    .InSequence(s)
    .WillRepeatedly(ReturnRef(captured_uses));

  EXPECT_CALL(task, run_gmock_proxy())
    .InSequence(s)
    .WillOnce(Invoke([&]{
      ASSERT_THAT(*(int*)u_cap.data_, Eq(0));
      backend_runtime->release_use(&u_cap);
    })
  );

  backend_runtime->release_use(&u_con);

  delete handle;

  backend_runtime->finalize();

}

//////////////////////////////////////////////////////////////////////////////////
