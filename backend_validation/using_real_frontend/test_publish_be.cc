/*
//@HEADER
// ************************************************************************
//
//                   test_publish_be.cc
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
// builds upon TestCreateWorkBE::initial_access_alloc()
// additionally calls runtime::publish_use() and runtime::make_fetching_flow()
TEST_F(TestPublishBE, publish_in_cw){
  static std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  struct test_task {
    void operator()(std::vector<std::string> args) {
      *execution_order_check = 0;
      {
        auto h = initial_access<mydata>("dummy");
        create_work([=]{
          // make sure this task runs first
          EXPECT_EQ((*execution_order_check)++, 0);
          h.set_value(7);
          h.publish(version="a");
        });
        {
          auto h2 = read_access<mydata>("dummy", version="a");
          create_work([=]{
            // make sure this task runs second
            EXPECT_EQ((*execution_order_check)++, 1);
            // make sure we fetched the right value
            EXPECT_EQ(h2.get_value(), 7);
          });
        }

      }
    }
  };
  DARMA_REGISTER_TOP_LEVEL_FUNCTOR(test_task);
  backend_init_finalize(argc_, argv_);
  // make sure task not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure task actually ran
  ASSERT_EQ(execution_order_check->load(), 2);
}

// test publish after create_work
// same as TestPublishBE::publish_in_cw() but with the call to
//   runtime::publish_use() coming from the top-level task
TEST_F(TestPublishBE, publish_after_cw){
  static std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  struct test_task {
    void operator()(std::vector<std::string> args) {
      *execution_order_check = 0;
      {
        auto h = initial_access<mydata>("dummy");
        create_work([=]{
          // make sure this task runs first
          EXPECT_EQ((*execution_order_check)++, 0);
          h.set_value(7);
        });
        h.publish(version="a");
        {
          auto h2 = read_access<mydata>("dummy", version="a");
          create_work([=]{
            // make sure this task runs second
            EXPECT_EQ((*execution_order_check)++, 1);
            // make sure we fetched the right value
            EXPECT_EQ(h2.get_value(), 7);
          });
        }

      }
    }
  };
  DARMA_REGISTER_TOP_LEVEL_FUNCTOR(test_task);
  backend_init_finalize(argc_, argv_);
  // make sure task not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure task actually ran
  ASSERT_EQ(execution_order_check->load(), 2);
}

// test read_access after handle goes out of scope
// same as TestPublishBE::publish_after_cw() but with the call to
//   runtime::release_use() on the published use coming before the
//   runtime::make_fetching_flow() call
TEST_F(TestPublishBE, read_access_after_scope){
  static std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  struct test_task {
    void operator()(std::vector<std::string> args) {
      *execution_order_check = 0;
      {
        auto h = initial_access<mydata>("dummy");
        create_work([=]{
          // make sure this task runs first
          EXPECT_EQ((*execution_order_check)++, 0);
          h.set_value(7);
        });
        h.publish(version="a");
      }
      {
        auto h2 = read_access<mydata>("dummy", version="a");
        create_work([=]{
          // make sure this task runs second
          EXPECT_EQ((*execution_order_check)++, 1);
          // make sure we fetched the right value
          EXPECT_EQ(h2.get_value(), 7);
        });
      }
    }
  };
  DARMA_REGISTER_TOP_LEVEL_FUNCTOR(test_task);
  backend_init_finalize(argc_, argv_);
  // make sure task not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure task actually ran
  ASSERT_EQ(execution_order_check->load(), 2);
}

// make sure that modify waits until after the publish finishes
// or makes a copy
// builds on TestPublishBE::publish_after_cw()
TEST_F(TestPublishBE, modify_after_publish_nice){
  static std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  static std::shared_ptr<int> rt1(new int);
  static std::shared_ptr<int> rt2(new int);
  static std::shared_ptr<int> mt1(new int);
  static std::shared_ptr<int> mt2(new int);
  struct test_task {
    void operator()(std::vector<std::string> args) {
      *execution_order_check = 0;
      *rt1 = 0;
      *rt2 = 0;
      *mt1 = 0;
      *mt2 = 0;
      {
        auto h = initial_access<mydata>("dummy");
        create_work([=]{
          // make sure this task runs first
          EXPECT_EQ((*execution_order_check)++, 0);
          h.set_value(7);
        });
        h.publish(version="a");
        {
          auto h2 = read_access<mydata>("dummy", version="a");
          create_work([=]{
            // record that this task started
            *rt1 = (*execution_order_check)++;
            // make sure we fetched the original value
            EXPECT_EQ(h2.get_value(), 7);
            // record that this task finished
            *rt2 = (*execution_order_check)++;
          });
        }
        create_work([=]{
          // record that this task started
          *mt1 = (*execution_order_check)++;
          h.set_value(11);
          // record that this task finished
          *mt2 = (*execution_order_check)++;
        });
      }
    }
  };
  DARMA_REGISTER_TOP_LEVEL_FUNCTOR(test_task);
  backend_init_finalize(argc_, argv_);
  // make sure task not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure task actually ran
  ASSERT_EQ(execution_order_check->load(), 5);
  // if tasks ran simultaneously, test was inconclusive
  if (!(*mt2 < *rt1) && !(*rt2 < *mt1))
    std::cout << "A pass for this test should be considered inconclusive"
              << "due to a race condition in the test.\n";
}

// make sure that modify waits until after publish with n_readers>1 finishes
// or makes a copy
// builds on TestPublishBE::modify_after_publish_nice()
TEST_F(TestPublishBE, modify_after_publish_nreaders_nice){
  static std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  static std::shared_ptr<int> rt1(new int);
  static std::shared_ptr<int> rt2(new int);
  static std::shared_ptr<int> rt3(new int);
  static std::shared_ptr<int> rt4(new int);
  static std::shared_ptr<int> mt1(new int);
  static std::shared_ptr<int> mt2(new int);
  struct test_task {
    void operator()(std::vector<std::string> args) {
      *execution_order_check = 0;
      *rt1 = 0;
      *rt2 = 0;
      *rt3 = 0;
      *rt4 = 0;
      *mt1 = 0;
      *mt2 = 0;
      {
        auto h = initial_access<mydata>("dummy");
        create_work([=]{
          // make sure this task runs first
          EXPECT_EQ((*execution_order_check)++, 0);
          h.set_value(7);
        });
        h.publish(version="a",n_readers=2);
        {
          auto h2 = read_access<mydata>("dummy", version="a");
          create_work([=]{
            // record that this task started
            *rt1 = (*execution_order_check)++;
            // make sure we fetched the original value
            EXPECT_EQ(h2.get_value(), 7);
            // record that this task finished
            *rt2 = (*execution_order_check)++;
          });
        }
        {
          auto h3 = read_access<mydata>("dummy", version="a");
          create_work([=]{
            // record that this task started
            *rt3 = (*execution_order_check)++;
            // make sure we fetched the original value
            EXPECT_EQ(h3.get_value(), 7);
            // record that this task finished
            *rt4 = (*execution_order_check)++;
          });
        }
        create_work([=]{
          // record that this task started
          *mt1 = (*execution_order_check)++;
          h.set_value(11);
          // record that this task finished
          *mt2 = (*execution_order_check)++;
        });

      }
    }
  };
  DARMA_REGISTER_TOP_LEVEL_FUNCTOR(test_task);
  backend_init_finalize(argc_, argv_);
  // make sure task not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure task actually ran
  ASSERT_EQ(execution_order_check->load(), 7);
  // if tasks ran simultaneously, test was inconclusive
  if (!(*mt2 < *rt1) && !(*mt2 < *rt3) && !((*rt2 < *mt1) && (*rt4 < *mt1)))
    std::cout << "A pass for this test should be considered inconclusive"
              << "due to a race condition in the test.\n";
}

// make sure that modify waits until after multiple publishes finish
// or makes a copy
// builds on TestPublishBE::modify_after_publish_nice()
TEST_F(TestPublishBE, modify_after_multipublish_nice){
  static std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  static std::shared_ptr<int> rt1(new int);
  static std::shared_ptr<int> rt2(new int);
  static std::shared_ptr<int> rt3(new int);
  static std::shared_ptr<int> rt4(new int);
  static std::shared_ptr<int> mt1(new int);
  static std::shared_ptr<int> mt2(new int);
  struct test_task {
    void operator()(std::vector<std::string> args) {
      *execution_order_check = 0;
      *rt1 = 0;
      *rt2 = 0;
      *rt3 = 0;
      *rt4 = 0;
      *mt1 = 0;
      *mt2 = 0;
      {
        auto h = initial_access<mydata>("dummy");
        create_work([=]{
          // make sure this task runs first
          EXPECT_EQ((*execution_order_check)++, 0);
          h.set_value(7);
        });
        h.publish(version="a",n_readers=1);
        h.publish(version="b",n_readers=1);
        {
          auto h2 = read_access<mydata>("dummy", version="a");
          create_work([=]{
            // record that this task started
            *rt1 = (*execution_order_check)++;
            // make sure we fetched the original value
            EXPECT_EQ(h2.get_value(), 7);
            // record that this task finished
            *rt2 = (*execution_order_check)++;
          });
        }
        {
          auto h3 = read_access<mydata>("dummy", version="b");
          create_work([=]{
            // record that this task started
            *rt3 = (*execution_order_check)++;
            // make sure we fetched the original value
            EXPECT_EQ(h3.get_value(), 7);
            // record that this task finished
            *rt4 = (*execution_order_check)++;
          });
        }
        create_work([=]{
          // record that this task started
          *mt1 = (*execution_order_check)++;
          h.set_value(11);
          // record that this task finished
          *mt2 = (*execution_order_check)++;
        });
      }
    }
  };
  DARMA_REGISTER_TOP_LEVEL_FUNCTOR(test_task);
  backend_init_finalize(argc_, argv_);
  // make sure task not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure task actually ran
  ASSERT_EQ(execution_order_check->load(), 7);
  // if tasks ran simultaneously, test was inconclusive
  if (!(*mt2 < *rt1) && !(*mt2 < *rt3) && !((*rt2 < *mt1) && (*rt4 < *mt1)))
    std::cout << "A pass for this test should be considered inconclusive"
              << "due to a race condition in the test.\n";
}

// make sure that modify waits until after the publish finishes
// or makes a copy
// similar to TestPublishBE::modify_after_publish_nice() but puts read_access
//   after create_work that overwrites it (probably not legal within a single
//   rank but could happen across ranks)
TEST_F(TestPublishBE, DISABLED_modify_after_publish_nasty){
  static std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  static std::shared_ptr<int> rt1(new int);
  static std::shared_ptr<int> rt2(new int);
  static std::shared_ptr<int> mt1(new int);
  static std::shared_ptr<int> mt2(new int);
  struct test_task {
    void operator()(std::vector<std::string> args) {
      *execution_order_check = 0;
      *rt1 = 0;
      *rt2 = 0;
      *mt1 = 0;
      *mt2 = 0;
      {
        auto h = initial_access<mydata>("dummy");
        create_work([=]{
          // make sure this task runs first
          EXPECT_EQ((*execution_order_check)++, 0);
          h.set_value(7);
        });
        h.publish(version="a");
        create_work([=]{
          // record that this task started
          *mt1 = (*execution_order_check)++;
          h.set_value(11);
          // record that this task finished
          *mt2 = (*execution_order_check)++;
        });
        {
          auto h2 = read_access<mydata>("dummy", version="a");
          create_work([=]{
            // record that this task started
            *rt1 = (*execution_order_check)++;
            // make sure we fetched the original value
            EXPECT_EQ(h2.get_value(), 7);
            // record that this task finished
            *rt2 = (*execution_order_check)++;
          });
        }

      }
    }
  };
  DARMA_REGISTER_TOP_LEVEL_FUNCTOR(test_task);
  backend_init_finalize(argc_, argv_);
  // make sure task not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure task actually ran
  ASSERT_EQ(execution_order_check->load(), 5);
  // if tasks ran simultaneously, test was inconclusive
  if (!(*mt2 < *rt1) && !(*rt2 < *mt1))
    std::cout << "A pass for this test should be considered inconclusive"
              << "due to a race condition in the test.\n";
}

//////////////////////////////////////////////////////////////////////////////////

