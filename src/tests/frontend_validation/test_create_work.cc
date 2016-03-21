/*
//@HEADER
// ************************************************************************
//
//                          test_create_work.cc
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

#ifndef SRC_TESTS_FRONTEND_VALIDATION_TEST_CREATE_WORK_CC_
#define SRC_TESTS_FRONTEND_VALIDATION_TEST_CREATE_WORK_CC_

#include <gtest/gtest.h>

#include "mock_backend.h"
#include "test_frontend.h"

#include <darma/interface/app/initial_access.h>
#include <darma/interface/app/read_access.h>
#include <darma/interface/app/create_work.h>

////////////////////////////////////////////////////////////////////////////////

namespace {

class TestCreateWork
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

    template <typename SameHandle>
    auto in_get_dependencies(SameHandle&& same_handle) {
      return ::testing::Truly([&handle=same_handle.handle](auto* task_arg) {
        const auto& task_deps = task_arg->get_dependencies();
        return task_deps.find(handle) != task_deps.end();
      });
    }

    template <typename SameHandle>
    auto needs_read_of(SameHandle&& same_handle) {
      return ::testing::Truly([&handle=same_handle.handle](auto* task_arg) {
        return task_arg->needs_read_data(handle);
      });
    }

    template <typename SameHandle>
    auto needs_write_of(SameHandle&& same_handle) {
      return ::testing::Truly([&handle=same_handle.handle](auto* task_arg) {
        return task_arg->needs_write_data(handle);
      });
    }

    template <typename SameHandle>
    void expect_handle_life_cycle(SameHandle&& same_handle,
      ::testing::Sequence& s = ::testing::Sequence(),
      bool read_only = false,
      bool calls_last_at_version_depth = true
    ) {
      using namespace ::testing;
      Sequence s2;

      if(read_only) {
        EXPECT_CALL(*mock_runtime, register_fetching_handle(Truly(same_handle), _))
          .InSequence(s, s2);
      }
      else {
        EXPECT_CALL(*mock_runtime, register_handle(Truly(same_handle)))
          .InSequence(s, s2);
      }

      EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(same_handle)))
        .InSequence(s);

      if(not read_only and calls_last_at_version_depth) {
        EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Truly(same_handle)))
          .InSequence(s2);
      }

      EXPECT_CALL(*mock_runtime, release_handle(Truly(same_handle)))
        .InSequence(s, s2);

    }

};

} // end anonymous namespace

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, capture_initial_access) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  Sequence s1, s2;

  auto hm1 = make_same_handle_matcher();
  auto hm2 = make_same_handle_matcher();

  mock_runtime->save_tasks = true;

  EXPECT_CALL(*mock_runtime, register_handle(Truly(hm1)))
    .InSequence(s1);

  // Release read-only usage should happen immedately for initial access,
  // i.e., before runnign the next line of code that triggers the next register_handle
  EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(hm1)))
    .InSequence(s1);

  EXPECT_CALL(*mock_runtime, register_handle(AllOf(Truly(hm2), Not(Eq(hm1.handle)))))
    .InSequence(s1);

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
      in_get_dependencies(hm1),
      needs_write_of(hm1),
      Not(needs_read_of(hm1))
    ))).InSequence(s1, s2);

  // order is not specified for release read only usage on continuing context
  // and handle_done_with_version_depth
  EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(hm2)))
    .InSequence(s1);
  EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Truly(hm2)))
    .InSequence(s2);

  EXPECT_CALL(*mock_runtime, release_handle(Truly(hm2)))
    .Times(Exactly(1))
    .InSequence(s1, s2);

  {
    auto tmp = initial_access<int>("hello");

    create_work([=]{
      // This code doesn't run in this example
      tmp.set_value(5);
      FAIL() << "This code block shouldn't be running in this example";
    });

  } // tmp deleted

  // We expect the captured handle won't be released until the task is deleted, which we'll do here
  // Also, that handle should call done with version depth upon deletion of the task
  EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Truly(hm1)))
    .Times(Exactly(1))
    .InSequence(s1);
  EXPECT_CALL(*mock_runtime, release_handle(Truly(hm1)))
    .Times(Exactly(1))
    .InSequence(s1);

  mock_runtime->registered_tasks.clear();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, capture_initial_access_vector) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  Sequence s1, s2, s3, s4;

  auto hm1_1 = make_same_handle_matcher();
  auto hm2_1 = make_same_handle_matcher();
  auto hm1_2 = make_same_handle_matcher();
  auto hm2_2 = make_same_handle_matcher();

  mock_runtime->save_tasks = true;

  EXPECT_CALL(*mock_runtime, register_handle(Truly(hm1_1)))
    .InSequence(s1);
  EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(hm1_1)))
    .InSequence(s1);

  EXPECT_CALL(*mock_runtime, register_handle(AllOf(Truly(hm2_1), Not(Eq(hm1_1.handle)))))
    .InSequence(s1);
  EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(hm2_1)))
    .InSequence(s1, s2);

  // Expect continuing context registrations.  Order not specified
  EXPECT_CALL(*mock_runtime, register_handle(AllOf(Truly(hm1_2), Not(Eq(hm1_1.handle)))))
    .InSequence(s1);
  EXPECT_CALL(*mock_runtime, register_handle(AllOf(Truly(hm2_2), Not(Eq(hm2_1.handle)))))
    .InSequence(s2);

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
      in_get_dependencies(hm1_1), needs_write_of(hm1_1), Not(needs_read_of(hm1_1)),
      in_get_dependencies(hm2_1), needs_write_of(hm2_1), Not(needs_read_of(hm2_1))
  ))).InSequence(s1, s2, s3, s4);

  EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(hm1_2)))
    .InSequence(s1);
  EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Truly(hm1_2)))
    .InSequence(s2);
  EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(hm2_2)))
    .InSequence(s3);
  EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Truly(hm2_2)))
    .InSequence(s4);

  EXPECT_CALL(*mock_runtime, release_handle(Truly(hm1_2)))
    .Times(Exactly(1))
    .InSequence(s1, s2);
  EXPECT_CALL(*mock_runtime, release_handle(Truly(hm2_2)))
    .Times(Exactly(1))
    .InSequence(s3, s4);

  {
    std::vector<AccessHandle<int>> handles;

    handles.push_back(initial_access<int>("hello"));
    handles.emplace_back(initial_access<int>("world"));

    create_work([=]{
      handles[0].set_value(5);
      FAIL() << "This code block shouldn't be running in this example";
    });

  } // handles deleted

  EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Truly(hm1_1)))
    .Times(Exactly(1))
    .InSequence(s1, s2);
  EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Truly(hm2_1)))
    .Times(Exactly(1))
    .InSequence(s3, s4);
  EXPECT_CALL(*mock_runtime, release_handle(Truly(hm1_1)))
    .Times(Exactly(1))
    .InSequence(s1);
  EXPECT_CALL(*mock_runtime, release_handle(Truly(hm2_1)))
    .Times(Exactly(1))
    .InSequence(s3);

  mock_runtime->registered_tasks.clear();
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, ro_capture_RN) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  mock_runtime->save_tasks = true;

  Sequence s_hm1;
  auto hm1 = make_same_handle_matcher();
  expect_handle_life_cycle(hm1, s_hm1, true);

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    in_get_dependencies(hm1), Not(needs_write_of(hm1)), needs_read_of(hm1)
  )));

  {
    auto tmp = read_access<int>("hello", version="world");
    create_work([=]{
      std::cout << tmp.get_value();
      FAIL() << "This code block shouldn't be running in this example";
    });
  }

  mock_runtime->registered_tasks.clear();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, capture_read_access_2) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;

  mock_runtime->save_tasks = true;

  Sequence s_hm1;
  auto hm1 = make_same_handle_matcher();
  expect_handle_life_cycle(hm1, s_hm1, true);

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    in_get_dependencies(hm1), Not(needs_write_of(hm1)), needs_read_of(hm1)
  ))).Times(Exactly(2));

  {
    auto tmp = read_access<int>("hello", version="world");
    create_work([=,&hm1]{
      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm1.handle));
    });
    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm1.handle));
    create_work([=,&hm1]{
      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm1.handle));
    });
    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm1.handle));
  }

  while(not mock_runtime->registered_tasks.empty()) {
    mock_runtime->registered_tasks.front()->run();
    mock_runtime->registered_tasks.pop_front();
  }

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, mod_capture_MN) {
  using namespace ::testing;
  using namespace darma_runtime;
  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;

  mock_runtime->save_tasks = true;

  // Reverse order of expected usage because of the way expectations work
  Sequence s_hm3;
  auto hm3 = make_same_handle_matcher();
  expect_handle_life_cycle(hm3, s_hm3);
  Sequence s_hm2;
  auto hm2 = make_same_handle_matcher();
  expect_handle_life_cycle(hm2, s_hm2);
  Sequence s_hm1;
  auto hm1 = make_same_handle_matcher();
  expect_handle_life_cycle(hm1, s_hm1);

  {
    InSequence s;
    EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
      in_get_dependencies(hm1), needs_write_of(hm1), Not(needs_read_of(hm1))
    )));
    EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
      in_get_dependencies(hm2), needs_write_of(hm2), needs_read_of(hm2)
    )));
  }

  {
    auto tmp = initial_access<int>("hello");
    create_work([=,&hm1]{
      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm1.handle));
    });
    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm2.handle));
    create_work([=,&hm2]{
      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm2.handle));
    });
    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm3.handle));
  }

  while(not mock_runtime->registered_tasks.empty()) {
    mock_runtime->registered_tasks.front()->run();
    mock_runtime->registered_tasks.pop_front();
  }

  ASSERT_THAT(hm1.handle, Not(Eq(hm2.handle)));
  ASSERT_THAT(hm2.handle, Not(Eq(hm3.handle)));


}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, mod_capture_MM) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;

  mock_runtime->save_tasks = true;

  // Reverse order of expected usage because of the way expectations work
  Sequence s_hm5;
  auto hm5 = make_same_handle_matcher();
  expect_handle_life_cycle(hm5, s_hm5);
  Sequence s_hm4;
  auto hm4 = make_same_handle_matcher();
  expect_handle_life_cycle(hm4, s_hm4);
  Sequence s_hm3;
  auto hm3 = make_same_handle_matcher();
  expect_handle_life_cycle(hm3, s_hm3);
  Sequence s_hm2;
  auto hm2 = make_same_handle_matcher();
  expect_handle_life_cycle(hm2, s_hm2, /*read_only=*/false, /*calls_last_at_version_depth=*/false);
  Sequence s_hm1;
  auto hm1 = make_same_handle_matcher();
  expect_handle_life_cycle(hm1, s_hm1);

  Sequence s;
  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    in_get_dependencies(hm1), needs_write_of(hm1), Not(needs_read_of(hm1))
  ))).InSequence(s);
  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    in_get_dependencies(hm2), needs_write_of(hm2), needs_read_of(hm2)
  ))).InSequence(s);

  {
    auto tmp = initial_access<int>("hello");
    create_work([=]{
      // tmp.handle == hm1
      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm1.handle));
    });
    // tmp.handle == hm2
    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm2.handle));
    create_work([=]{
      // tmp.handle == hm2
      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm2.handle));
      EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(_)).InSequence(s);
      // Note that last at version depth shouldn't be called for hm2
      create_work([=]{
        // tmp.handle == hm4
        ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm4.handle));
      });
      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm5.handle));
      // tmp.handle == hm5
    });
    // tmp.handle == hm3
    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm3.handle));
  }

  while(not mock_runtime->registered_tasks.empty()) {
    mock_runtime->registered_tasks.front()->run();
    mock_runtime->registered_tasks.pop_front();
  }

  ASSERT_THAT(hm1.handle, Not(Eq(hm2.handle)));
  ASSERT_THAT(hm2.handle, Not(Eq(hm3.handle)));
  ASSERT_THAT(hm3.handle, Not(Eq(hm4.handle)));
  ASSERT_THAT(hm4.handle, Not(Eq(hm5.handle)));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, ro_capture_MM) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;

  mock_runtime->save_tasks = true;

  // Reverse order of expected usage because of the way expectations work
  Sequence s_hm4;
  auto hm4 = make_same_handle_matcher();
  expect_handle_life_cycle(hm4, s_hm4);
  Sequence s_hm3;
  auto hm3 = make_same_handle_matcher();
  expect_handle_life_cycle(hm3, s_hm3);
  Sequence s_hm2;
  auto hm2 = make_same_handle_matcher();
  expect_handle_life_cycle(hm2, s_hm2, /*read_only=*/false, /*calls_last_at_version_depth=*/false);
  Sequence s_hm1;
  auto hm1 = make_same_handle_matcher();
  expect_handle_life_cycle(hm1, s_hm1);

  Sequence s;
  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    in_get_dependencies(hm1), needs_write_of(hm1), Not(needs_read_of(hm1))
  ))).InSequence(s);
  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    in_get_dependencies(hm2), needs_write_of(hm2), needs_read_of(hm2)
  ))).InSequence(s);

  {
    auto tmp = initial_access<int>("hello");
    create_work([=]{
      // tmp.handle == hm1
      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm1.handle));
    });
    // tmp.handle == hm2
    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm2.handle));
    create_work([=]{
      // tmp.handle == hm2
      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm2.handle));
      EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(_)).InSequence(s);
      // Note that last at version depth shouldn't be called for hm2, but a
      // new handle should be created
      create_work(reads(tmp), [=]{
        // tmp.handle == hm4
        ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm4.handle));
      });
      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm4.handle));
      // tmp.handle == hm4
      // Note: tmp should be in state Modify_Read
    });
    // tmp.handle == hm3
    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm3.handle));
  }

  while(not mock_runtime->registered_tasks.empty()) {
    mock_runtime->registered_tasks.front()->run();
    mock_runtime->registered_tasks.pop_front();
  }

  // Assert that the handles aren't the same.
  ASSERT_THAT(hm1.handle, Not(Eq(hm2.handle)));
  ASSERT_THAT(hm2.handle, Not(Eq(hm3.handle)));
  ASSERT_THAT(hm3.handle, Not(Eq(hm4.handle)));

  // Assert that the version relationships match the chart
  auto v_hm1 = hm1.handle->get_version();
  auto v_hm2 = hm2.handle->get_version();
  auto v_hm3 = hm3.handle->get_version();
  auto v_hm4 = hm4.handle->get_version();
  v_hm1.pop_subversion();
  ++v_hm1;
  ASSERT_EQ(v_hm2, v_hm1);
  ++v_hm2;
  ASSERT_EQ(v_hm4, v_hm2);
  v_hm2.pop_subversion();
  ++v_hm2;
  ASSERT_EQ(v_hm3, v_hm2);
  ASSERT_EQ(hm1.key, hm2.key);
  ASSERT_EQ(hm2.key, hm3.key);
  ASSERT_EQ(hm3.key, hm4.key);



}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


#endif /* SRC_TESTS_FRONTEND_VALIDATION_TEST_CREATE_WORK_CC_ */
