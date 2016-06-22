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
#include "helpers.h"

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_publication;

class TestPublishBE
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

// test publish from create_work
TEST_F(TestPublishBE, publish_in_cw){
  std::shared_ptr<std::atomic<int>> check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    create_work([=]{
      // make sure this task runs first
      EXPECT_EQ((*check)++, 0);
      h.set_value(7);
      h.publish(version="a");
    });
    {
      auto h2 = read_access<mydata>("dummy", version="a");
      create_work([=]{
        // make sure this task runs second
        EXPECT_EQ((*check)++, 1);
        // make sure we fetched the right value
        EXPECT_EQ(h2.get_value(), 7);
      });
    }
    darma_finalize();
  }
  // make sure task not still queued
  ASSERT_TRUE(check.unique());
  // make sure task actually ran
  ASSERT_EQ(check->load(), 2);
}

// test publish after create_work
TEST_F(TestPublishBE, publish_after_cw){
  std::shared_ptr<std::atomic<int>> check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    create_work([=]{
      // make sure this task runs first
      EXPECT_EQ((*check)++, 0);
      h.set_value(7);
    });
    h.publish(version="a");
    {
      auto h2 = read_access<mydata>("dummy", version="a");
      create_work([=]{
        // make sure this task runs second
        EXPECT_EQ((*check)++, 1);
        // make sure we fetched the right value
        EXPECT_EQ(h2.get_value(), 7);
      });
    }
    darma_finalize();
  }
  // make sure task not still queued
  ASSERT_TRUE(check.unique());
  // make sure task actually ran
  ASSERT_EQ(check->load(), 2);
}

// test read_access after handle goes out of scope
TEST_F(TestPublishBE, read_access_after_scope){
  std::shared_ptr<std::atomic<int>> check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    {
      auto h = initial_access<mydata>("dummy");
      create_work([=]{
        // make sure this task runs first
        EXPECT_EQ((*check)++, 0);
        h.set_value(7);
      });
      h.publish(version="a");
    }
    {
      auto h2 = read_access<mydata>("dummy", version="a");
      create_work([=]{
        // make sure this task runs second
        EXPECT_EQ((*check)++, 1);
        // make sure we fetched the right value
        EXPECT_EQ(h2.get_value(), 7);
      });
    }
    darma_finalize();
  }
  // make sure task not still queued
  ASSERT_TRUE(check.unique());
  // make sure task actually ran
  ASSERT_EQ(check->load(), 2);
}

// make sure that modify waits until after the publish finishes
TEST_F(TestPublishBE, modify_after_publish_nice){
  std::shared_ptr<std::atomic<int>> check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    create_work([=]{
      // make sure this task runs first
      EXPECT_EQ((*check)++, 0);
      h.set_value(7);
    });
    h.publish(version="a");
    {
      auto h2 = read_access<mydata>("dummy", version="a");
      create_work([=]{
        // make sure this task runs second
        EXPECT_EQ((*check)++, 1);
        // make sure we fetched the original value
        EXPECT_EQ(h2.get_value(), 7);
        // make sure the modify task hasn't started yet
        EXPECT_EQ((*check)++, 2);
      });
    }
    create_work([=]{
      // make sure this task runs last
      EXPECT_EQ((*check)++, 3);
      h.set_value(11);
    });
    darma_finalize();
  }
  // make sure task not still queued
  ASSERT_TRUE(check.unique());
  // make sure task actually ran
  ASSERT_EQ(check->load(), 4);
}

// make sure that modify waits until after the publish finishes
TEST_F(TestPublishBE, modify_after_publish_nasty){
  std::shared_ptr<std::atomic<int>> check(new std::atomic<int>(0));
  {
    darma_init(argc_, argv_);
    auto h = initial_access<mydata>("dummy");
    create_work([=]{
      // make sure this task runs first
      EXPECT_EQ((*check)++, 0);
      h.set_value(7);
    });
    h.publish(version="a");
    create_work([=]{
      // make sure this task runs last
      EXPECT_EQ((*check)++, 3);
      h.set_value(11);
    });
    {
      auto h2 = read_access<mydata>("dummy", version="a");
      create_work([=]{
        // make sure this task runs second
        EXPECT_EQ((*check)++, 1);
        // make sure we fetched the original value
        EXPECT_EQ(h2.get_value(), 7);
        // make sure the modify task hasn't started yet
        EXPECT_EQ((*check)++, 2);
      });
    }
    darma_finalize();
  }
  // make sure task not still queued
  ASSERT_TRUE(check.unique());
  // make sure task actually ran
  ASSERT_EQ(check->load(), 4);
}

//////////////////////////////////////////////////////////////////////////////////

