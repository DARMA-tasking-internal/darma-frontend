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

#include <gtest/gtest.h>

#include "mock_backend.h"
#include "test_frontend.h"

#include <darma/interface/app/initial_access.h>
#include <darma/interface/app/read_access.h>
#include <darma/interface/app/create_work.h>

////////////////////////////////////////////////////////////////////////////////

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

};

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, capture_initial_access) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;


  MockFlow fl_in_1, fl_out_1;
  use_t* use_1, *use_2, *use_3;

  Sequence s0, s3;
  auto ex_1 = expect_initial_access(fl_in_1, fl_out_1, use_1, make_key("hello"), s0, s3);

  Sequence s1, s2;
  MockFlow fl_in_2, fl_out_2;
  MockFlow fl_in_3, fl_out_3;

  // mod-capture of MN
  EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_in_1), MockRuntime::Input))
    .Times(1).InSequence(s1)
    .WillOnce(Return(&fl_in_2));
  EXPECT_CALL(*mock_runtime, make_next_flow(Eq(&fl_in_2), MockRuntime::Output))
    .Times(1).InSequence(s1)
    .WillOnce(Return(&fl_out_2));
  EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_out_2), MockRuntime::Input))
    .Times(1).InSequence(s1)
    .WillOnce(Return(&fl_in_3));
  EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_out_1), MockRuntime::Output))
    .Times(1).InSequence(s1, s2)
    .WillOnce(Return(&fl_out_3));
  EXPECT_CALL(*mock_runtime, register_use(AllOf(
    IsUseWithFlows(&fl_in_2, &fl_out_2, use_t::Modify, use_t::Modify),
    // expresses the requirement that register of use2 must
    // happen before use1 is released
    UseRefIsNonNull(ByRef(use_1))
  ))).Times(1).InSequence(s1)
    .WillOnce(SaveArg<0>(&use_2));


  EXPECT_CALL(*mock_runtime, register_use(AllOf(
    IsUseWithFlows(&fl_in_3, &fl_out_3, use_t::Modify, use_t::None),
    // expresses the requirement that register of use3 must
    // happen before use1 is released
    UseRefIsNonNull(ByRef(use_1))
  ))).Times(1).InSequence(s2)
    .WillOnce(SaveArg<0>(&use_3));

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(use_2))))
    .Times(1).InSequence(s1, s3);

  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_2, &fl_out_2, use_t::Modify, use_t::Modify)
  )).Times(1).InSequence(s1, s3)
    .WillOnce(Assign(&use_2, nullptr));
  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_3, &fl_out_3, use_t::Modify, use_t::None)
  )).Times(1).InSequence(s2)
    .WillOnce(Assign(&use_3, nullptr));


  {
    auto tmp = initial_access<int>("hello");

    create_work([=]{
      // This code doesn't run in this example
      tmp.set_value(5);
      FAIL() << "This code block shouldn't be running in this example";
    });

  } // tmp deleted

  mock_runtime->registered_tasks.clear();

}

//////////////////////////////////////////////////////////////////////////////////

// Same as the test above, but uses helper
TEST_F(TestCreateWork, mod_capture_MN_helper) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  Sequence s_reg_captured, s_reg_continuing, s_reg_initial, s_release_initial;

  MockFlow fl_in_1, fl_out_1;
  MockFlow fl_in_2, fl_out_2;
  MockFlow fl_in_3, fl_out_3;
  use_t *use_1, *use_2, *use_3;

  Sequence s0, s3;
  expect_initial_access(fl_in_1, fl_out_1, use_1, make_key("hello"), s_reg_initial, s_release_initial);
  expect_mod_capture_MN_or_MR(
    fl_in_1, fl_out_1, use_1,
    fl_in_2, fl_out_2, use_2,
    fl_in_3, fl_out_3, use_3,
    s_reg_captured, s_reg_continuing
  );

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(use_2))))
    .Times(1).InSequence(s_reg_captured, s_release_initial);

  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_2, &fl_out_2, use_t::Modify, use_t::Modify)
  )).Times(1).InSequence(s_reg_captured, s_reg_initial, s_release_initial);
  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_3, &fl_out_3, use_t::Modify, use_t::None)
  )).Times(1).InSequence(s_reg_continuing);


  {
    auto tmp = initial_access<int>("hello");

    create_work([=]{
      // This code doesn't run in this example
      tmp.set_value(5);
      FAIL() << "This code block shouldn't be running in this example";
    });

  } // tmp deleted

  mock_runtime->registered_tasks.clear();
}

//////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, capture_initial_access_vector) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  Sequence s0, s1;

  MockFlow fl_in_0, fl_out_0;
  MockFlow fl_in_1, fl_out_1;
  MockFlow fl_in_cap_2, fl_out_cap_2, fl_in_cap_3, fl_out_cap_3;
  MockFlow fl_in_con_2, fl_out_con_2, fl_in_con_3, fl_out_con_3;
  use_t *use_0, *use_1, *use_cap_2, *use_con_2, *use_cap_3, *use_con_3;

  expect_initial_access(fl_in_0, fl_out_0, use_0, make_key("hello"), s0);
  expect_initial_access(fl_in_1, fl_out_1, use_1, make_key("world"), s0);
  expect_mod_capture_MN_or_MR(
    fl_in_0, fl_out_0, use_0,
    fl_in_cap_2, fl_out_cap_2, use_cap_2,
    fl_in_con_2, fl_out_con_2, use_con_2,
    s0, s1
  );
  expect_mod_capture_MN_or_MR(
    fl_in_1, fl_out_1, use_1,
    fl_in_cap_3, fl_out_cap_3, use_cap_3,
    fl_in_con_3, fl_out_con_3, use_con_3,
    s0, s1
  );

  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_con_2, &fl_out_con_2, use_t::Modify, use_t::None)
  )).Times(1).InSequence(s0);
  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_con_3, &fl_out_con_3, use_t::Modify, use_t::None)
  )).Times(1);

  {
    std::vector<AccessHandle<int>> handles;

    handles.push_back(initial_access<int>("hello"));
    handles.emplace_back(initial_access<int>("world"));

    create_work([=]{
      handles[0].set_value(5);
      FAIL() << "This code block shouldn't be running in this example";
    });

  } // handles deleted

  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_cap_2, &fl_out_cap_2, use_t::Modify, use_t::Modify)
  )).Times(1).InSequence(s0);
  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_cap_3, &fl_out_cap_3, use_t::Modify, use_t::Modify)
  )).Times(1);

  mock_runtime->registered_tasks.clear();
}

//////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, ro_capture_RN) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  Sequence s1, s_release_read;

  MockFlow fl_in_0, fl_out_0;
  MockFlow fl_in_2, fl_out_2;
  use_t* use_0, *use_1;
  expect_read_access(fl_in_0, fl_out_0, use_0, make_key("hello"), make_key("world"), s1, s_release_read);

  // ro-capture of RN
  EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_in_0), MockRuntime::Input))
    .Times(1).InSequence(s1)
    .WillOnce(Return(&fl_in_2));
  EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_in_2), MockRuntime::OutputFlowOfReadOperation))
    .Times(1).InSequence(s1)
    .WillOnce(Return(&fl_out_2));

  EXPECT_CALL(*mock_runtime, register_use(
    IsUseWithFlows(&fl_in_2, &fl_out_2, use_t::Read, use_t::Read)
  )).Times(1).InSequence(s1)
    .WillOnce(SaveArg<0>(&use_1));


  {
    auto tmp = read_access<int>("hello", version="world");
    create_work([=]{
      std::cout << tmp.get_value();
      FAIL() << "This code block shouldn't be running in this example";
    });

    ASSERT_THAT(use_0, NotNull());

  }

  // this should come after the read_access is released (and shouldn't happen until registered_tasks.clear())
  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_2, &fl_out_2, use_t::Read, use_t::Read)
  )).Times(1).InSequence(s1, s_release_read);

  mock_runtime->registered_tasks.clear();

}

//////////////////////////////////////////////////////////////////////////////////
//
//#if defined(DEBUG) || !defined(NDEBUG)
//TEST_F(TestCreateWork, death_ro_capture_unused) {
//  using namespace ::testing;
//  using namespace darma_runtime;
//  using namespace darma_runtime::keyword_arguments_for_publication;
//
//  EXPECT_DEATH(
//    {
//      mock_runtime->save_tasks = false;
//      auto tmp = initial_access<int>("hello");
//      create_work([=]{
//        std::cout << tmp.get_value();
//        FAIL() << "This code block shouldn't be running in this example";
//      });
//      { create_work(reads(tmp), [=]{ }); }
//    },
//    "handle with key .* declared as read usage, but was actually unused"
//  );
//
//  //mock_runtime->registered_tasks.clear();
//
//}
//#endif
//
//////////////////////////////////////////////////////////////////////////////////
//
//#if defined(DEBUG) || !defined(NDEBUG)
//TEST_F(TestCreateWork, death_ro_capture_MM_unused) {
//  using namespace ::testing;
//  using namespace darma_runtime;
//  using namespace darma_runtime::keyword_arguments_for_publication;
//
//  {
//    EXPECT_DEATH(
//      {
//        mock_runtime->save_tasks = true;
//        auto tmp = initial_access<int>("hello");
//        create_work([=] {
//          create_work(reads(tmp), [=] { });
//        });
//        run_all_tasks();
//      },
//      "handle with key .* declared as read usage, but was actually unused"
//    );
//  }
//
//}
//#endif
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestCreateWork, capture_read_access_2) {
//  using namespace ::testing;
//  using namespace darma_runtime;
//  using namespace darma_runtime::keyword_arguments_for_publication;
//  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;
//
//  mock_runtime->save_tasks = true;
//
//  Sequence s_hm1;
//  auto hm1 = make_same_handle_matcher();
//  expect_handle_life_cycle(hm1, s_hm1, true);
//
//  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
//    in_get_dependencies(hm1), Not(needs_write_of(hm1)), needs_read_of(hm1)
//  ))).Times(Exactly(2));
//
//  {
//    auto tmp = read_access<int>("hello", version="world");
//    create_work([=,&hm1]{
//      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm1.handle));
//    });
//    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm1.handle));
//    create_work([=,&hm1]{
//      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm1.handle));
//    });
//    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(hm1.handle));
//  }
//
//  run_all_tasks();
//
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestCreateWork, mod_capture_MN) {
//  using namespace ::testing;
//  using namespace darma_runtime;
//  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;
//
//  mock_runtime->save_tasks = true;
//
//  // Reverse order of expected usage because of the way expectations work
//  handle_t* h0, *h1, *h2;
//  h0 = h1 = h2 = nullptr;
//  EXPECT_CALL(*mock_runtime, register_handle(_))
//    .Times(Exactly(3))
//    .WillOnce(SaveArg<0>(&h0))
//    .WillOnce(SaveArg<0>(&h1))
//    .WillOnce(SaveArg<0>(&h2));
//
//  {
//    InSequence s;
//    EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
//      handle_in_get_dependencies(h0), needs_write_handle(h0), Not(needs_read_handle(h0))
//    )));
//    EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
//      handle_in_get_dependencies(h1), needs_write_handle(h1), needs_read_handle(h1)
//    )));
//  }
//
//  {
//    auto tmp = initial_access<int>("hello");
//    create_work([=,&h0]{
//      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h0));
//    });
//    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h1));
//    create_work([=,&h1]{
//      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h1));
//    });
//    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h2));
//  }
//
//  run_all_tasks();
//
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestCreateWork, mod_capture_MM) {
//  using namespace ::testing;
//  using namespace darma_runtime;
//  using namespace darma_runtime::keyword_arguments_for_publication;
//  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;
//
//  mock_runtime->save_tasks = true;
//
//  // Reverse order of expected usage because of the way expectations work
//
//  handle_t* h0, *h1, *h2, *h3, *h4;
//  h0 = h1 = h2 = h3 = h4 = nullptr;
//  EXPECT_CALL(*mock_runtime, register_handle(_))
//    .Times(Exactly(5))
//    .WillOnce(SaveArg<0>(&h0))
//    .WillOnce(SaveArg<0>(&h1))
//    .WillOnce(SaveArg<0>(&h2))
//    .WillOnce(SaveArg<0>(&h3))
//    .WillOnce(SaveArg<0>(&h4));
//
//  Sequence s;
//  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
//    handle_in_get_dependencies(h0), needs_write_handle(h0), Not(needs_read_handle(h0))
//  ))).InSequence(s);
//  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
//    handle_in_get_dependencies(h1), needs_write_handle(h1), needs_read_handle(h1)
//  ))).InSequence(s);
//
//  {
//    auto tmp = initial_access<int>("hello");
//    create_work([=,&h0]{
//      // tmp.handle == h0
//      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h0));
//    });
//    // tmp.handle == h1
//    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h1));
//    create_work([=,&h3,&h4,&h1]{
//      // tmp.handle == h1
//      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h1));
//      EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(_)).InSequence(s);
//      // Note that last at version depth shouldn't be called for h1
//      create_work([=,&h3]{
//        // tmp.handle == h3
//        ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h3));
//      });
//      ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h4));
//      // tmp.handle == h4
//    });
//    // tmp.handle == h2
//    ASSERT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h2));
//  }
//
//  run_all_tasks();
//
//  // Note that we can only assert not equal for handles with overlapping lifetimes;
//  // C++ might allocate them in the same place for efficiency
//  ASSERT_THAT(h0, Not(Eq(h1)));
//  ASSERT_THAT(h1, Not(Eq(h2)));
//  ASSERT_THAT(h1, Not(Eq(h3)));
//  ASSERT_THAT(h1, Not(Eq(h4)));
//  ASSERT_THAT(h3, Not(Eq(h4)));
//
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestCreateWork, ro_capture_MM) {
//  using namespace ::testing;
//  using namespace darma_runtime;
//  using namespace darma_runtime::keyword_arguments_for_publication;
//  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;
//
//  mock_runtime->save_tasks = true;
//
//  handle_t* h0, *h1, *h2, *h3;
//  h0 = h1 = h2 = h3 = nullptr;
//  version_t v0, v1, v2, v3;
//
//  EXPECT_CALL(*mock_runtime, register_handle(_))
//    .Times(Exactly(4))
//    .WillOnce(SaveArg<0>(&h0))
//    .WillOnce(SaveArg<0>(&h1))
//    .WillOnce(SaveArg<0>(&h2))
//    .WillOnce(SaveArg<0>(&h3));
//
//  EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h0)))).WillOnce(Assign(&h0, nullptr));
//  EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h1)))).WillOnce(Assign(&h1, nullptr));
//  EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h2)))).WillOnce(Assign(&h2, nullptr));
//  EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h3)))).WillOnce(Assign(&h3, nullptr));
//
//  Sequence s;
//  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
//    handle_in_get_dependencies(h0), needs_write_handle(h0), Not(needs_read_handle(h0))
//  ))).InSequence(s);
//  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
//    handle_in_get_dependencies(h1), needs_write_handle(h1), needs_read_handle(h1)
//  ))).InSequence(s);
//  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
//    handle_in_get_dependencies(h3), Not(needs_write_handle(h3)), needs_read_handle(h3)
//  ))).InSequence(s);
//
//  {
//    auto tmp = initial_access<int>("hello");
//    create_work([=,&h0,&h1,&v0]{
//      // tmp.handle == h0
//      EXPECT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h0));
//      v0 = h0->get_version();
//      EXPECT_THAT(h1, NotNull());
//      EXPECT_EQ(h0->get_key(), h1->get_key());
//    });
//    // tmp.handle == h1
//    EXPECT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h1));
//    create_work([=,&h1,&h3,&v1,&v3]{
//      // tmp.handle == h1
//      EXPECT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h1));
//      v1 = h1->get_version();
//
//      // Note that last at version depth shouldn't be called for hm2, but a
//      // new handle should be created
//      create_work(reads(tmp), [=,&h3,&v3]{
//        // tmp.handle == h3
//        EXPECT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h3));
//        v3 = h3->get_version();
//      });
//      // tmp.handle == h3
//      EXPECT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h3));
//      // Note: tmp should be in state Modify_Read
//    });
//    // tmp.handle == h2
//    EXPECT_THAT(for_AccessHandle::get_dep_handle(tmp), Eq(h2));
//    v2 = h2->get_version();
//    EXPECT_EQ(h1->get_key(), h2->get_key());
//  } // h2 should be released at this point
//
//  run_all_tasks();
//
//  // Assert that the versions are correct
//  ASSERT_EQ(v0, version_t{0});
//  ASSERT_EQ(v1, version_t{1});
//  ASSERT_THAT(v3, Eq(version_t{1,1}));
//  ASSERT_EQ(v2, version_t{2});
//
//  // also assert that the version relationships and increment behavior works
//  v0.pop_subversion();
//  ++v0;
//  ASSERT_EQ(v1, v0);
//  ++v1;
//  ASSERT_EQ(v3, v1);
//  v3.pop_subversion();
//  ++v3;
//  ASSERT_EQ(v3, v2);
//
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestCreateWork, publish_inside_version) {
//  using namespace ::testing;
//  using namespace darma_runtime;
//  using namespace darma_runtime::keyword_arguments_for_publication;
//  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;
//
//  mock_runtime->save_tasks = true;
//
//  {
//    auto tmp = initial_access<double>("data", 0);
//    EXPECT_VERSION_EQ_EXACT(tmp, {0});
//    create_work([=] {
//      EXPECT_VERSION_EQ_EXACT(tmp, {0,0});
//      tmp.publish(n_readers=1);
//      EXPECT_VERSION_EQ_EXACT(tmp, {0,1});
//    });
//    EXPECT_VERSION_EQ_EXACT(tmp, {1});
//  }
//
//  run_all_tasks();
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestCreateWork, publish_outside_version) {
//  using namespace ::testing;
//  using namespace darma_runtime;
//  using namespace darma_runtime::keyword_arguments_for_publication;
//  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;
//
//  mock_runtime->save_tasks = true;
//
//  {
//    auto tmp = initial_access<double>("data", 0);
//    EXPECT_VERSION_EQ_EXACT(tmp, {0});
//    create_work([=] {
//      EXPECT_VERSION_EQ_EXACT(tmp, {0,0});
//    });
//    EXPECT_VERSION_EQ_EXACT(tmp, {1});
//    tmp.publish(n_readers=1);
//    EXPECT_VERSION_EQ_EXACT(tmp, {1});
//  }
//
//  run_all_tasks();
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestCreateWork, publish_inside_register_release) {
//  using namespace ::testing;
//  using namespace darma_runtime;
//  using namespace darma_runtime::keyword_arguments_for_publication;
//  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;
//
//  mock_runtime->save_tasks = true;
//
//  handle_t* h0, *h1, *h2;
//  h0 = h1 = h2 = nullptr;
//
//  {
//    InSequence s;
//    EXPECT_CALL(*mock_runtime, register_handle(_)).WillOnce(SaveArg<0>(&h0));
//    EXPECT_CALL(*mock_runtime, release_read_only_usage(Eq(ByRef(h0))));
//
//    // continuing context handle should be registered before the task
//    EXPECT_CALL(*mock_runtime, register_handle(_)).WillOnce(SaveArg<0>(&h1));
//
//    // Now we expect the task to be registered
//    EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(_));
//
//    // h1 goes away because tmp goes out of scope after the task is registered
//    // but before it is run
//    EXPECT_CALL(*mock_runtime, release_read_only_usage(Eq(ByRef(h1))));
//    EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h1))));
//
//    // Then these should happen once the task runs
//    // Note thate h2 must be registered before h0 is released!
//    EXPECT_CALL(*mock_runtime, register_handle(_)).WillOnce(SaveArg<0>(&h2));
//    // Note thate h0 must be released before h2!
//    EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h0))));
//    // Now the publish should be called
//    EXPECT_CALL(*mock_runtime, publish_handle(Eq(ByRef(h2)), _, _, _));
//    // Then h2 can be released
//    EXPECT_CALL(*mock_runtime, release_read_only_usage(Eq(ByRef(h2))));
//    EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h2))));
//
//  }
//
//  {
//    auto tmp = initial_access<double>("data", 0);
//    ASSERT_VERSION_EQ_EXACT(h0, {0});
//    create_work([=,&h2] {
//      tmp.publish(n_readers=1);
//      ASSERT_VERSION_EQ_EXACT(h2, {0,1});
//    });
//    ASSERT_VERSION_EQ_EXACT(h1, {1});
//  }
//
//  run_all_tasks();
//
//
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//TEST_F(TestCreateWork, publish_inside_register_release_order_2) {
//  // TODO this doesn't actually test the issue that caused publishAndReadAccess to fail
//  using namespace ::testing;
//  using namespace darma_runtime;
//  using namespace darma_runtime::keyword_arguments_for_publication;
//  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;
//
//  mock_runtime->save_tasks = true;
//
//  handle_t* h0, *h1, *h2;
//  h0 = h1 = h2 = nullptr;
//
//  {
//    InSequence s;
//    EXPECT_CALL(*mock_runtime, register_handle(_)).WillOnce(SaveArg<0>(&h0));
//    EXPECT_CALL(*mock_runtime, release_read_only_usage(Eq(ByRef(h0))));
//
//    // continuing context handle should be registered before the task
//    EXPECT_CALL(*mock_runtime, register_handle(_)).WillOnce(SaveArg<0>(&h1));
//
//    // Now we expect the task to be registered
//    EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(_));
//
//    // Then these should happen once the task runs
//    // Note thate h2 must be registered before h0 is released!
//    EXPECT_CALL(*mock_runtime, register_handle(_)).WillOnce(SaveArg<0>(&h2));
//    // Note thate h0 must be released before h2!
//    EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h0))));
//    // Now the publish should be called
//    EXPECT_CALL(*mock_runtime, publish_handle(Eq(ByRef(h2)), _, _, _));
//    // Then h2 can be released
//    EXPECT_CALL(*mock_runtime, release_read_only_usage(Eq(ByRef(h2))));
//    EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h2))));
//
//    // h1 goes away because tmp goes out of scope after the task is registered
//    // but before it is run
//    EXPECT_CALL(*mock_runtime, release_read_only_usage(Eq(ByRef(h1))));
//    EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h1))));
//  }
//
//  {
//    auto tmp = initial_access<double>("data", 0);
//    ASSERT_VERSION_EQ(h0, {0});
//    create_work([=,&h2] {
//      tmp.publish(n_readers=1);
//      ASSERT_VERSION_EQ(h2, {0,1});
//    });
//    run_all_tasks();
//    ASSERT_VERSION_EQ(h1, {1});
//  }
//
//
//
//}
