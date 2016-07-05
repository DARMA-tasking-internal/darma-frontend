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
// Questions? Contact Nicole Slattengren (nlslatt@sandia.gov)
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
#include "helpers.h"

using namespace darma_runtime;

class TestCreateWorkBE
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

// test task with no dependencies
// builds upon TestInitBE::rank_size()
// additionally calls runtime::register_task()
TEST_F(TestCreateWorkBE, no_access){
  std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    // task with no access handles involved
    create_work([=]{
      (*execution_order_check)++;
    });
    darma_finalize();
  }
  // make sure task not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure task actually ran
  ASSERT_EQ(execution_order_check->load(), 1);
}

// test task with write permissions (mod capture when MN)
// builds upon TestCreateWorkBE::no_access()
// additionally calls runtime::make_initial_flow(), runtime::make_null_flow(),
//   runtime::register_use(), runtime::make_same_flow(),
//   runtime::make_next_flow(), runtime::release_use()
TEST_F(TestCreateWorkBE, initial_access_alloc){
  std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    // task that initializes data
    create_work([=]{
      (*execution_order_check)++;
      // make sure the backend allocated data space
      EXPECT_FALSE(&(h.get_reference()) == nullptr);
    });
    darma_finalize();
  }
  // make sure task not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure task actually ran
  ASSERT_EQ(execution_order_check->load(), 1);
}

// test task with read permissions (ro capture when MN)
// builds upon TestCreateWorkBE::initial_access_alloc()
TEST_F(TestCreateWorkBE, ro_capture_MN){
  std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    create_work([=]{
      // make sure this task runs first
      EXPECT_EQ((*execution_order_check)++, 0);
      h.set_value(7);
      // make sure other task didn't start running yet
      EXPECT_EQ((*execution_order_check)++, 1);
    });
    // task that reads data
    create_work(reads(h),[=]{
      // make sure this task runs last
      EXPECT_EQ((*execution_order_check)++, 2);
      // make sure we see the correct initial value
      EXPECT_EQ(h.get_value(), 7);
    });
    darma_finalize();
  }
  // make sure tasks not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure tasks actually ran
  ASSERT_EQ(execution_order_check->load(), 3);
}

// test task with read-write permissions (mod capture when MN)
// builds upon TestCreateWorkBE::ro_capture_MN()
TEST_F(TestCreateWorkBE, mod_capture_MN){
  std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    create_work([=]{
      // make sure this task runs first
      EXPECT_EQ((*execution_order_check)++, 0);
      h.set_value(7);
    });
    // task that modifies value
    create_work([=]{
      // make sure this task runs second
      EXPECT_EQ((*execution_order_check)++, 1);
      // make sure we see the correct initial value
      EXPECT_EQ(h.get_value(), 7);
      // but then modify it
      h.set_value(21);
      // make sure other task didn't start running yet
      EXPECT_EQ((*execution_order_check)++, 2);
    });
    create_work(reads(h),[=]{
      // make sure this task runs last
      EXPECT_EQ((*execution_order_check)++, 3);
      // make sure we see the modified value
      EXPECT_EQ(h.get_value(), 21);
    });
    darma_finalize();
  }
  // make sure tasks not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure tasks actually ran
  ASSERT_EQ(execution_order_check->load(), 4);
}

// test nested task with read-write permissions (mod capture when MM)
// builds upon TestCreateworkBE::mod_capture_MN()
// additionally calls runtime::make_forwarding_flow()
TEST_F(TestCreateWorkBE, mod_capture_MM){
  std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    create_work([=]{
      // make sure this task runs first
      EXPECT_EQ((*execution_order_check)++, 0);
      h.set_value(7);
    });
    create_work([=]{
      // make sure this task runs second
      EXPECT_EQ((*execution_order_check)++, 1);
      // make sure we see the correct initial value
      EXPECT_EQ(h.get_value(), 7);
      // but then modify it
      h.set_value(21);
      // task that receives forwarded modifications
      create_work([=]{
        // make sure this task runs third
        EXPECT_EQ((*execution_order_check)++, 2);
        // make sure we see the forwarded value
        EXPECT_EQ(h.get_value(), 21);
        // but then modify it
        h.set_value(4);
        // make sure other task didn't start running yet
        EXPECT_EQ((*execution_order_check)++, 3);
      });
    });
    create_work(reads(h),[=]{
      // make sure this task runs last
      EXPECT_EQ((*execution_order_check)++, 4);
      // make sure we see the twice-modified value
      EXPECT_EQ(h.get_value(), 4);
    });
    darma_finalize();
  }
  // make sure tasks not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure tasks actually ran
  ASSERT_EQ(execution_order_check->load(), 5);
}

// test task with read permissions (ro capture when MM)
// builds upon TestCreateWorkBE::mod_capture_MN()
// additionally calls runtime::make_forwarding_flow()
//   and runtime::make_same_flow(purpose=OutputFlowOfReadOperation)
TEST_F(TestCreateWorkBE, ro_capture_MM){
  std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    create_work([=]{
      // make sure this task runs first
      EXPECT_EQ((*execution_order_check)++, 0);
      h.set_value(7);
    });
    create_work([=]{
      // make sure this task runs second
      EXPECT_EQ((*execution_order_check)++, 1);
      // make sure we see the correct initial value
      EXPECT_EQ(h.get_value(), 7);
      // but then modify it
      h.set_value(21);
      // task that reads modified value
      create_work(reads(h),[=]{
        // make sure this task runs last
        EXPECT_EQ((*execution_order_check)++, 2);
        // make sure we see the modified value
        EXPECT_EQ(h.get_value(), 21);
      });
    });
    darma_finalize();
  }
  // make sure tasks not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure tasks actually ran
  ASSERT_EQ(execution_order_check->load(), 3);
}

// test task with read permissions (ro capture when MR)
// builds upon TestCreateWorkBE::ro_capture_MM()
TEST_F(TestCreateWorkBE, ro_capture_MR){
  std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    create_work([=]{
      // make sure this task runs first
      EXPECT_EQ((*execution_order_check)++, 0);
      h.set_value(7);
    });
    create_work([=]{
      // make sure this task runs second
      EXPECT_EQ((*execution_order_check)++, 1);
      // make sure we see the correct initial value
      EXPECT_EQ(h.get_value(), 7);
      // but then modify it
      h.set_value(21);
      create_work(reads(h),[=]{
        // make sure this task runs
        (*execution_order_check)++;
        // make sure we see the modified value
        EXPECT_EQ(h.get_value(), 21);
      });
      // task that also reads modified value
      create_work(reads(h),[=]{
        // make sure this task runs
        (*execution_order_check)++;
        // make sure we see the modified value
        EXPECT_EQ(h.get_value(), 21);
      });
    });
    darma_finalize();
  }
  // make sure tasks not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure tasks actually ran
  ASSERT_EQ(execution_order_check->load(), 4);
}

// test task with read permissions (ro capture when RR)
// builds upon TestCreateWorkBE::ro_capture_MM()
TEST_F(TestCreateWorkBE, ro_capture_RR){
  std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    create_work([=]{
      // make sure this task runs first
      EXPECT_EQ((*execution_order_check)++, 0);
      h.set_value(7);
    });
    create_work([=]{
      // make sure this task runs second
      EXPECT_EQ((*execution_order_check)++, 1);
      // make sure we see the correct initial value
      EXPECT_EQ(h.get_value(), 7);
      // but then modify it
      h.set_value(21);
      create_work(reads(h),[=]{
        // make sure this task runs third
        EXPECT_EQ((*execution_order_check)++, 2);
        // make sure we see the modified value
        EXPECT_EQ(h.get_value(), 21);
        // task that also reads modified value
        create_work(reads(h),[=]{
          // make sure this task runs last
          EXPECT_EQ((*execution_order_check)++, 3);
          // make sure we see the modified value
          EXPECT_EQ(h.get_value(), 21);
        });
      });
    });
    darma_finalize();
  }
  // make sure tasks not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure tasks actually ran
  ASSERT_EQ(execution_order_check->load(), 4);
}

// test write-after-read with MR permissions
// builds upon TestCreateWorkBE::ro_capture_MM()
TEST_F(TestCreateWorkBE, write_after_read_MR){
  std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    create_work([=]{
      // make sure this task runs first
      EXPECT_EQ((*execution_order_check)++, 0);
      h.set_value(7);
    });
    create_work([=]{
      // make sure this task runs second
      EXPECT_EQ((*execution_order_check)++, 1);
      // make sure we see the correct initial value
      EXPECT_EQ(h.get_value(), 7);
      // but then modify it
      h.set_value(21);
      create_work(reads(h),[=]{
        // make sure this task runs third
        EXPECT_EQ((*execution_order_check)++, 2);
        // make sure we see the modified value
        EXPECT_EQ(h.get_value(), 21);
        // make sure the next task still hasn't started
        EXPECT_EQ((*execution_order_check)++, 3);
      });
      create_work([=]{
        // make sure this task runs last
        EXPECT_EQ((*execution_order_check)++, 4);
        // make sure we see the modified value
        EXPECT_EQ(h.get_value(), 21);
        // but then modify it again
        h.set_value(4);
      });
    });
    darma_finalize();
  }
  // make sure tasks not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure tasks actually ran
  ASSERT_EQ(execution_order_check->load(), 5);
}

// test write-after-read with MM permissions
// builds upon TestCreateWorkBE::ro_capture_MM()
TEST_F(TestCreateWorkBE, write_after_read_MM){
  std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    create_work([=]{
      // make sure this task runs first
      EXPECT_EQ((*execution_order_check)++, 0);
      h.set_value(7);
    });
    create_work([=]{
      // make sure this task runs second
      EXPECT_EQ((*execution_order_check)++, 1);
      // make sure we see the correct initial value
      EXPECT_EQ(h.get_value(), 7);
      // but then modify it
      h.set_value(21);
      create_work(reads(h),[=]{
        // make sure this task runs third
        EXPECT_EQ((*execution_order_check)++, 2);
        // make sure we see the modified value
        EXPECT_EQ(h.get_value(), 21);
        // make sure the next task still hasn't started
        EXPECT_EQ((*execution_order_check)++, 3);
      });
      create_work([=]{
        create_work([=]{
          // make sure this task runs last
          EXPECT_EQ((*execution_order_check)++, 4);
          // make sure we see the modified value
          EXPECT_EQ(h.get_value(), 21);
          // but then modify it again
          h.set_value(4);
        });
      });
    });
    darma_finalize();
  }
  // make sure tasks not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure tasks actually ran
  ASSERT_EQ(execution_order_check->load(), 5);
}

// test write-after-read with MN permissions
// builds upon TestCreateWorkBE::ro_capture_MN()
TEST_F(TestCreateWorkBE, write_after_read_MN){
  std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    create_work([=]{
      // make sure this task runs first
      EXPECT_EQ((*execution_order_check)++, 0);
      h.set_value(7);
    });
    create_work(reads(h),[=]{
      // make sure this task runs second
      EXPECT_EQ((*execution_order_check)++, 1);
      // make sure we see the original value
      EXPECT_EQ(h.get_value(), 7);
      // make sure the next task still hasn't started
      EXPECT_EQ((*execution_order_check)++, 2);
    });
    create_work([=]{
      // make sure this task runs last
      EXPECT_EQ((*execution_order_check)++, 3);
      // make sure we see the original value
      EXPECT_EQ(h.get_value(), 7);
      // but then modify it
      h.set_value(21);
    });
    darma_finalize();
  }
  // make sure tasks not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure tasks actually ran
  ASSERT_EQ(execution_order_check->load(), 4);
}

//////////////////////////////////////////////////////////////////////////////////

