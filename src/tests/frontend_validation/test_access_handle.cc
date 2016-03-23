/*
//@HEADER
// ************************************************************************
//
//                          test_access_handle.h
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

////////////////////////////////////////////////////////////////////////////////

// Forward declarations for friend usage
class TestAccessHandle;
class TestAccessHandle_get_reference_Test;
class TestAccessHandle_set_value_Test;
class TestAccessHandle_publish_MN_Test;
class TestAccessHandle_publish_MM_Test;
#define DARMA_TEST_FRONTEND_VALIDATION 1

////////////////////////////////////////////////////////////////////////////////

#include <iomanip>
#include <iostream>

#include <gtest/gtest.h>

#include "mock_backend.h"
#include "test_frontend.h"

#include <darma/interface/app/access_handle.h>
#include <darma/interface/app/create_work.h>
#include <darma/interface/app/initial_access.h>
#include <darma/interface/app/read_access.h>

////////////////////////////////////////////////////////////////////////////////

class TestAccessHandle
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

TEST_F(TestAccessHandle, get_reference) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  handle_t* h0 = nullptr, *h1 = nullptr;

  EXPECT_CALL(*mock_runtime, register_handle(_))
    .WillOnce(SaveArg<0>(&h0))
    .WillOnce(SaveArg<0>(&h1));

  int data = 0;
  MockDataBlock db;
  EXPECT_CALL(db, get_data())
    .Times(AnyNumber())
    .WillRepeatedly(Return(&data));

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    handle_in_get_dependencies(h0), Not(needs_read_handle(h0)), needs_write_handle(h0)
  )))
    .WillOnce(InvokeWithoutArgs([&h0,&db]{ h0->satisfy_with_data_block(&db); }));

  {
    auto tmp = initial_access<int>("hello");

    EXPECT_THAT(h0, NotNull());

    create_work([=,&h0,&h1,&data]{
      EXPECT_THAT(tmp.dep_handle_.get(), Eq(h0));
      EXPECT_THAT(h1, NotNull());
      EXPECT_THAT(&tmp.get_reference(), Eq(&data));
    });
    EXPECT_THAT(tmp.dep_handle_.get(), Eq(h1));
  }

  while(not mock_runtime->registered_tasks.empty()) {
    mock_runtime->registered_tasks.front()->run();
    mock_runtime->registered_tasks.pop_front();
  }

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, set_value) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  handle_t* h0 = nullptr, *h1 = nullptr;

  EXPECT_CALL(*mock_runtime, register_handle(_))
    .WillOnce(SaveArg<0>(&h0))
    .WillOnce(SaveArg<0>(&h1));

  int data = 0;
  MockDataBlock db;
  EXPECT_CALL(db, get_data())
    .Times(AnyNumber())
    .WillRepeatedly(Return(&data));

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    handle_in_get_dependencies(h0), Not(needs_read_handle(h0)), needs_write_handle(h0)
  )))
    .WillOnce(InvokeWithoutArgs([&h0,&db]{ h0->satisfy_with_data_block(&db); }));

  {
    auto tmp = initial_access<int>("hello");

    create_work([=,&h0,&h1,&data]{
      tmp.set_value(42);
    });

  }

  while(not mock_runtime->registered_tasks.empty()) {
    mock_runtime->registered_tasks.front()->run();
    mock_runtime->registered_tasks.pop_front();
  }

  EXPECT_THAT(data, Eq(42));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, publish_MN) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = false;

  handle_t* h0 = nullptr, *h1 = nullptr;
  constexpr auto version_str = "hello";
  key_t pub_version = make_key(version_str);

  EXPECT_CALL(*mock_runtime, register_handle(_))
    .WillOnce(SaveArg<0>(&h0))
    .WillOnce(SaveArg<0>(&h1));

  int data = 0;
  NiceMock<MockDataBlock> db;
  ON_CALL(db, get_data())
    .WillByDefault(Return(&data));

  ON_CALL(*mock_runtime, register_task_gmock_proxy(_))
    .WillByDefault(InvokeWithoutArgs([&h0,&db]{ h0->satisfy_with_data_block(&db); }));

  EXPECT_CALL(*mock_runtime, publish_handle(
    Eq(ByRef(h1)), Eq(pub_version), Eq(1), _
  ));

  {
    auto tmp = initial_access<int>("hello");

    create_work([=]{
      FAIL() << "should not be invoked in this test";
      tmp.set_value(42);
    });
    ASSERT_EQ(tmp.state_, AccessHandle<int>::State::Modify_None);

    tmp.publish(version=version_str);

  }

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, publish_MM) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  handle_t* h0, *h1, *h2;
  h0 = h1 = h2 = nullptr;
  constexpr auto version_str = "hello";
  key_t pub_version = make_key(version_str);
  version_t v0, v1, v2;

  EXPECT_CALL(*mock_runtime, register_handle(_))
    .WillOnce(SaveArg<0>(&h0))
    .WillOnce(SaveArg<0>(&h1))
    .WillOnce(SaveArg<0>(&h2));

  int data = 0;
  NiceMock<MockDataBlock> db;
  ON_CALL(db, get_data())
    .WillByDefault(Return(&data));

  ON_CALL(*mock_runtime, register_task_gmock_proxy(_))
    .WillByDefault(InvokeWithoutArgs([&h0,&db]{ h0->satisfy_with_data_block(&db); }));

  // Handle done with version depth shouldn't be called for h0
  EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Eq(ByRef(h1))));
  EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Eq(ByRef(h2))));

  EXPECT_CALL(*mock_runtime, publish_handle(
    Eq(ByRef(h2)), Eq(pub_version), Eq(1), _
  ));

  {
    auto tmp = initial_access<int>("hello");

    // tmp.dep_handle_ == h0
    EXPECT_THAT(tmp.dep_handle_.get(), Eq(h0));
    create_work([=,&h0,&h2,&h1,&v0,&v2]{
      ASSERT_EQ(tmp.state_, AccessHandle<int>::State::Modify_Modify);
      v0 = h0->get_version();
      tmp.set_value(42);
      tmp.publish(version=version_str);
      EXPECT_THAT(tmp.dep_handle_.get(), Eq(h2));
      v2 = h2->get_version();
      ASSERT_EQ(tmp.state_, AccessHandle<int>::State::Modify_Read);
    });
    EXPECT_THAT(tmp.dep_handle_.get(), Eq(h1));
    v1 = h1->get_version();
  } // h1 deleted

  while(not mock_runtime->registered_tasks.empty()) {
    mock_runtime->registered_tasks.front()->run();
    mock_runtime->registered_tasks.pop_front();
  }

  EXPECT_THAT(data, Eq(42));
  ++v0;
  EXPECT_EQ(v0, v2);
  v2.pop_subversion();
  ++v2;
  EXPECT_EQ(v1, v2);


}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, death_capture_released) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  EXPECT_DEATH(
    {
      auto tmp = initial_access<int>("hello");

      tmp = 0;

      create_work([=]{
        FAIL() << "should not be invoked in this test";
        tmp.set_value(2);
      });

    },
    "[Hh]andle used after release"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, death_publish_released) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  EXPECT_DEATH(
    {
      auto tmp = initial_access<int>("hello");

      tmp = 0;

      tmp.publish();

    },
    "publish\\(\\) called on handle after release"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, death_get_value) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  EXPECT_DEATH(
    {
      auto tmp = initial_access<int>("hello");
      tmp.get_value();
    },
    "get_value\\(\\) called on handle not in immediately readable state.*"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, death_set_value) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  EXPECT_DEATH(
    {
      auto tmp = initial_access<int>("hello");
      tmp.set_value(10);
    },
    "set_value\\(\\) called on handle not in immediately modifiable state.*"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, death_get_reference) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  EXPECT_DEATH(
    {
      auto tmp = initial_access<int>("hello");
      tmp.get_reference();
    },
    "get_reference\\(\\) called on handle not in immediately modifiable state.*"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, death_set_value_2) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  EXPECT_DEATH(
    {
      mock_runtime->save_tasks = true;

      auto tmp = initial_access<int>("hello");
      create_work(reads(tmp), [=]{
        tmp.set_value(10);
      });

      run_all_tasks();
    },
    "set_value\\(\\) called on handle not in immediately modifiable state.*"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, death_get_value_after_release) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  EXPECT_DEATH(
    {
      mock_runtime->save_tasks = true;

      auto tmp = initial_access<int>("hello");

      create_work([=]{
        tmp = 0;
        tmp.get_value();
      });

      run_all_tasks();
    },
    "get_value\\(\\) called on handle after release"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, death_emplace_value_after_release) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  EXPECT_DEATH(
    {
      mock_runtime->save_tasks = true;

      auto tmp = initial_access<int>("hello");

      create_work([=]{
        tmp = 0;
        tmp.emplace_value();
      });

      run_all_tasks();
    },
    "emplace_value\\(\\) called on handle after release"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, death_dereference) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  EXPECT_DEATH(
    {
      auto tmp = initial_access<int>("hello");
      *tmp = 5;
      std::cout << *tmp;
    },
    "handle dereferenced in state without immediate access to data"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, death_dereference_arrow) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  EXPECT_DEATH(
    {
      auto tmp = initial_access<std::string>("hello");
      std::cout << tmp->size();
    },
    "handle dereferenced in state without immediate access to data"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, death_get_key_after_release) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  EXPECT_DEATH(
    {
      auto tmp = initial_access<std::string>("hello");
      tmp = 0;
      tmp.get_key();
    },
    "get_key\\(\\) called on handle after release"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, death_release_read_only) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  EXPECT_DEATH(
    {
      auto tmp = read_access<std::string>("hello", version="world");
      create_work(reads(tmp), [=]{ tmp = 0; });
      run_all_tasks();
    },
    "release\\(\\) called on handle without Modify-schedule priviledges"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestAccessHandle, death_release_read_only_2) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  EXPECT_DEATH(
    {
      auto tmp = initial_access<std::string>("hello");
      create_work(reads(tmp), [=]{ tmp = 0; });
      run_all_tasks();
    },
    "release\\(\\) called on handle without Modify-schedule priviledges"
  );

}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
