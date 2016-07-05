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

class TestCreateWork_mod_capture_MM_Test;
#define DARMA_TEST_FRONTEND_VALIDATION_CREATE_WORK 1

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

struct TestModCaptureMN
  : TestCreateWork,
    ::testing::WithParamInterface<std::tuple<bool, bool, bool>>
{ };

TEST_P(TestModCaptureMN, mod_capture_MN) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  const bool use_helper = std::get<0>(GetParam());
  const bool helper_does_release = std::get<1>(GetParam());
  const bool use_arrays = std::get<2>(GetParam());

  MockFlow fl_in_1, fl_out_1;
  use_t* use_1, *use_2, *use_3;

  MockFlow fl_in_2, fl_out_2;
  MockFlow fl_in_3, fl_out_3;

  MockFlow fl_init[2], fl_capt[2], fl_cont[2];
  use_t* uses[3];


  Sequence s_reg_captured, s_reg_continuing, s_reg_initial, s_release_initial;

  if(not use_arrays) {
    expect_initial_access(
      fl_in_1, fl_out_1, use_1, make_key("hello"),
      s_reg_initial, s_release_initial
    );
  }
  else {
    expect_initial_access(
      fl_init[0], fl_init[1], uses[0], make_key("hello"),
      s_reg_initial, s_release_initial
    );
  }

  if(use_helper) {

    if(not use_arrays) {

      expect_mod_capture_MN_or_MR(
        fl_in_1, fl_out_1, use_1,
        fl_in_2, fl_out_2, use_2,
        fl_in_3, fl_out_3, use_3,
        s_reg_captured, s_reg_continuing,
        helper_does_release
      );

    }
    else {

      expect_mod_capture_MN_or_MR(
        fl_init, fl_capt, fl_cont, uses,
        s_reg_captured, s_reg_continuing,
        helper_does_release
      );

    }

  }
  else {
    Sequence s1;

    // mod-capture of MN
    //EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_in_1), MockRuntime::Input))
    //  .Times(1).InSequence(s1)
    //  .WillOnce(Return(&fl_in_2));
    EXPECT_CALL(*mock_runtime, make_next_flow(Eq(&fl_in_2), MockRuntime::Output))
      .Times(1).InSequence(s1)
      .WillOnce(Return(&fl_out_2));
    //EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_out_2), MockRuntime::Input))
    //  .Times(1).InSequence(s1)
    //  .WillOnce(Return(&fl_in_3));
    //EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_out_1), MockRuntime::Output))
    //  .Times(1).InSequence(s1, s_reg_continuing)
    //  .WillOnce(Return(&fl_out_3));
    EXPECT_CALL(*mock_runtime, register_use(AllOf(
      IsUseWithFlows(&fl_in_2, &fl_out_2, use_t::Modify, use_t::Modify),
      // expresses the requirement that register of use2 must
      // happen before use1 is released
      UseRefIsNonNull(ByRef(use_1))
    ))).Times(1).InSequence(s1, s_reg_captured)
      .WillOnce(SaveArg<0>(&use_2));

    //EXPECT_CALL(*mock_runtime, register_use(AllOf(
    //  IsUseWithFlows(&fl_in_3, &fl_out_3, use_t::Modify, use_t::None),
    //  // expresses the requirement that register of use3 must
    //  // happen before use1 is released
    //  UseRefIsNonNull(ByRef(use_1))
    //))).Times(1).InSequence(s_reg_continuing)
    //  .WillOnce(SaveArg<0>(&use_3));

  }

  if(not (use_helper and helper_does_release)) {

    EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(
      UseInGetDependencies(ByRef(use_arrays ? uses[1] : use_2))
    )).Times(1).InSequence(s_reg_captured, s_release_initial);

    EXPECT_CALL(*mock_runtime, release_use(
      AllOf(
        Eq(ByRef(use_arrays ? uses[1] : use_2)),
        IsUseWithFlows(
          use_arrays ? &fl_capt[0] : &fl_in_2,
          use_arrays ? &fl_capt[1] : &fl_out_2,
          use_t::Modify, use_t::Modify
        )
      )
    )).Times(1).InSequence(s_reg_captured, s_release_initial)
      .WillOnce(Assign(use_arrays ? &uses[1] : &use_2, nullptr));
    EXPECT_CALL(*mock_runtime, release_use(
      AllOf(
        Eq(ByRef(use_arrays ? uses[2] : use_3)),
        IsUseWithFlows(
          use_arrays ? &fl_cont[0] : &fl_in_3,
          use_arrays ? &fl_cont[1] : &fl_out_3,
          use_t::Modify, use_t::None
        )
      )
    )).Times(1).InSequence(s_reg_continuing)
      .WillOnce(Assign(use_arrays ? &uses[2] : &use_3, nullptr));
  }

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

INSTANTIATE_TEST_CASE_P(
  WithAndWithoutHelper,
  TestModCaptureMN,
  ::testing::Values(
    std::make_tuple(false, false, false), // no helpers
    std::make_tuple(true, false, false), // helper but no task or release in helper
    std::make_tuple(true, false, true), // helper but no task or release in helper, with arrays
    std::make_tuple(true, true, false), // helper, with task and release in helper
    std::make_tuple(true, true, true) // helper, with task and release in helper, with arrays
  )
);

//////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, mod_capture_MN_vector) {
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


struct TestRoCaptureRN
  : TestCreateWork,
    ::testing::WithParamInterface<bool>
{ };

TEST_P(TestRoCaptureRN, ro_capture_RN) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  Sequence s1, s_release_read;

  MockFlow fl_in_0, fl_out_0;
  MockFlow fl_in_1, fl_out_1;
  use_t* use_0, *use_1;
  expect_read_access(fl_in_0, fl_out_0, use_0, make_key("hello"), make_key("world"), s1, s_release_read);

  bool use_helper = GetParam();

  if(use_helper) {
    expect_ro_capture_RN_RR_MN_or_MR(
      fl_in_0, fl_out_0, use_0,
      fl_in_1, fl_out_1, use_1,
      s1
    );
  }
  else {

    // ro-capture of RN
    //EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_in_0), MockRuntime::Input))
    //  .Times(1).InSequence(s1)
    //  .WillOnce(Return(&fl_in_1));
    //EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_in_1), MockRuntime::OutputFlowOfReadOperation))
    //  .Times(1).InSequence(s1)
    //  .WillOnce(Return(&fl_out_1));

    EXPECT_CALL(*mock_runtime, register_use(
      IsUseWithFlows(&fl_in_1, &fl_out_1, use_t::Read, use_t::Read)
    )).Times(1).InSequence(s1)
      .WillOnce(SaveArg<0>(&use_1));
  }

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(use_1))))
    .Times(1).InSequence(s1);

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
    IsUseWithFlows(&fl_in_1, &fl_out_1, use_t::Read, use_t::Read)
  )).Times(1).InSequence(s1, s_release_read);

  mock_runtime->registered_tasks.clear();

}

INSTANTIATE_TEST_CASE_P(
  WithAndWithoutHelper,
  TestRoCaptureRN,
  ::testing::Bool()
);

//////////////////////////////////////////////////////////////////////////////////

struct TestModCaptureMM
  : TestCreateWork,
    ::testing::WithParamInterface<bool>
{ };

TEST_P(TestModCaptureMM, mod_capture_MM) {

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;
  using task_t = abstract::backend::runtime_t::task_t;
  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;

  mock_runtime->save_tasks = true;

  MockFlow fl_in_init, fl_out_init;
  MockFlow fl_in_cap_1, fl_out_cap_1;
  MockFlow fl_in_con_1, fl_out_con_1;
  MockFlow fl_in_cap_2, fl_out_cap_2;
  MockFlow fl_in_con_2, fl_out_con_2;
  use_t* use_init, *use_cap_1, *use_con_1, *use_cap_2, *use_con_2;

  Sequence s0, s1;

  expect_initial_access(fl_in_init, fl_out_init, use_init, make_key("hello"), s0);

  expect_mod_capture_MN_or_MR(
    fl_in_init, fl_out_init, use_init,
    fl_in_cap_1, fl_out_cap_1, use_cap_1,
    fl_in_con_1, fl_out_con_1, use_con_1,
    s0
  );

  int value = 42;

  task_t* outer, *inner;

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(use_cap_1))))
    .Times(1).InSequence(s0).WillOnce(SaveArg<0>(&outer));

  EXPECT_CALL(*mock_runtime, release_use(AllOf(Eq(ByRef(use_con_1)),
    IsUseWithFlows(&fl_in_con_1, &fl_out_con_1, use_t::Modify, use_t::None)
  ))).Times(1).InSequence(s0, s1);


  EXPECT_CALL(*mock_runtime, make_forwarding_flow(Eq(&fl_in_cap_1), Eq(MockRuntime::ForwardingChanges)))
    .Times(1).InSequence(s0).WillOnce(Return(&fl_in_cap_2));
  EXPECT_CALL(*mock_runtime, make_next_flow(Eq(&fl_in_cap_2), MockRuntime::Output))
    .Times(1).InSequence(s0).WillOnce(Return(&fl_out_cap_2));
  //EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_out_cap_2), MockRuntime::Input))
  //  .Times(1).InSequence(s0).WillOnce(Return(&fl_in_con_2));
  //EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_out_cap_1), MockRuntime::Output))
  //  .Times(1).InSequence(s1).WillOnce(Return(&fl_out_con_2));

  EXPECT_CALL(*mock_runtime, register_use(
    IsUseWithFlows(&fl_in_cap_2, &fl_out_cap_2, use_t::Modify, use_t::Modify)
  )).Times(1).InSequence(s0, s1).WillOnce(Invoke([&](use_t* u) {
    u->get_data_pointer_reference() = (void*)(&value);
    use_cap_2 = u;
  }));


  //EXPECT_CALL(*mock_runtime, register_use(
  //  IsUseWithFlows(&fl_in_con_2, &fl_out_con_2, use_t::Modify, use_t::None)
  //)).Times(1).InSequence(s0).WillOnce(SaveArg<0>(&use_con_2));

  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_cap_1, &fl_out_cap_1, use_t::Modify, use_t::Modify)
  )).Times(1).InSequence(s0);

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(use_cap_2))))
    .Times(1).InSequence(s0).WillOnce(SaveArg<0>(&inner));

  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_con_2, &fl_out_con_2, use_t::Modify, use_t::None)
  )).Times(1).InSequence(s0);

  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_cap_2, &fl_out_cap_2, use_t::Modify, use_t::Modify)
  )).Times(1).InSequence(s0);

  bool use_vector = GetParam();

  ////////////////////////////////////////////////////////////////////////////////

  if(use_vector) {

    std::vector<AccessHandle<int>> tmp;
    tmp.push_back(initial_access<int>("hello"));

    create_work([=] {
      create_work([=] {
        ASSERT_THAT(tmp[0].get_value(), Eq(42));
      });
    });

  }

  ////////////////////////////////////////////////////////////////////////////////

  else {

    auto tmp = initial_access<int>("hello");

    create_work([=]{
      create_work([=]{
        ASSERT_THAT(tmp.get_value(), Eq(42));
      });
    });

  }

  ////////////////////////////////////////////////////////////////////////////////

  ON_CALL(*mock_runtime, get_running_task())
    .WillByDefault(Return(ByRef(outer)));

  run_all_tasks();

}

INSTANTIATE_TEST_CASE_P(
  WithAndWithoutVector,
  TestModCaptureMM,
  ::testing::Bool()
);

//////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWork, ro_capture_MM) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;
  using task_t = abstract::backend::runtime_t::task_t;
  using darma_runtime::detail::create_work_attorneys::for_AccessHandle;

  mock_runtime->save_tasks = true;

  MockFlow fl_in_init, fl_out_init;
  MockFlow fl_in_cap_1, fl_out_cap_1;
  MockFlow fl_in_con_1, fl_out_con_1;
  MockFlow fl_in_cap_2, fl_out_cap_2;
  MockFlow fl_in_con_2, fl_out_con_2;
  use_t* use_init, *use_cap_1, *use_con_1, *use_cap_2, *use_con_2;

  Sequence s0, s1, s2;

  expect_initial_access(fl_in_init, fl_out_init, use_init, make_key("hello"), s0);

  expect_mod_capture_MN_or_MR(
    fl_in_init, fl_out_init, use_init,
    fl_in_cap_1, fl_out_cap_1, use_cap_1,
    fl_in_con_1, fl_out_con_1, use_con_1,
    s0
  );

  int value = 42;

  task_t* outer, *inner;

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(use_cap_1))))
    .Times(1).InSequence(s0).WillOnce(SaveArg<0>(&outer));

  EXPECT_CALL(*mock_runtime, release_use(AllOf(Eq(ByRef(use_con_1)),
    IsUseWithFlows(&fl_in_con_1, &fl_out_con_1, use_t::Modify, use_t::None)
  ))).Times(1).InSequence(s0, s1);


  EXPECT_CALL(*mock_runtime, make_forwarding_flow(Eq(&fl_in_cap_1), Eq(MockRuntime::ForwardingChanges)))
    .Times(1).InSequence(s0, s1).WillOnce(Return(&fl_in_cap_2));
  //EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_in_cap_2), MockRuntime::OutputFlowOfReadOperation))
  //  .Times(1).InSequence(s0).WillOnce(Return(&fl_out_cap_2));
  //EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_in_cap_2), MockRuntime::Input))
  //  .Times(1).InSequence(s1).WillOnce(Return(&fl_in_con_2));
  //EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_out_cap_1), MockRuntime::Output))
  //  .Times(1).InSequence(s2).WillOnce(Return(&fl_out_con_2));

  EXPECT_CALL(*mock_runtime, register_use(
    IsUseWithFlows(&fl_in_cap_2, &fl_out_cap_2, use_t::Read, use_t::Read)
  )).Times(1).InSequence(s0, s1, s2).WillOnce(Invoke([&](use_t* u) {
    u->get_data_pointer_reference() = (void*)(&value);
    use_cap_2 = u;
  }));


  EXPECT_CALL(*mock_runtime, register_use(
    IsUseWithFlows(&fl_in_con_2, &fl_out_con_2, use_t::Modify, use_t::Read)
  )).Times(1).InSequence(s0).WillOnce(SaveArg<0>(&use_con_2));

  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_cap_1, &fl_out_cap_1, use_t::Modify, use_t::Modify)
  )).Times(1).InSequence(s0);

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(use_cap_2))))
    .Times(1).InSequence(s1).WillOnce(SaveArg<0>(&inner));

  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_con_2, &fl_out_con_2, use_t::Modify, use_t::Read)
  )).Times(1).InSequence(s0, s1);

  EXPECT_CALL(*mock_runtime, release_use(
    IsUseWithFlows(&fl_in_cap_2, &fl_out_cap_2, use_t::Read, use_t::Read)
  )).Times(1).InSequence(s0);

  {
    auto tmp = initial_access<int>("hello");

    create_work([=]{
      create_work(reads(tmp), [=]{
        ASSERT_THAT(tmp.get_value(), Eq(42));
      });
    });
  }

  ON_CALL(*mock_runtime, get_running_task())
    .WillByDefault(Return(ByRef(outer)));

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

#if defined(DEBUG) || !defined(NDEBUG)
TEST_F(TestCreateWork, death_ro_capture_unused) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  EXPECT_DEATH(
    {
      mock_runtime->save_tasks = false;
      auto tmp = initial_access<int>("hello");
      create_work([=]{
        std::cout << tmp.get_value();
        FAIL() << "This code block shouldn't be running in this example";
      });
      { create_work(reads(tmp), [=]{ }); }
    },
    "handle with key .* declared as read usage, but was actually unused"
  );

  //mock_runtime->registered_tasks.clear();

}
#endif

////////////////////////////////////////////////////////////////////////////////

#if defined(DEBUG) || !defined(NDEBUG)
TEST_F(TestCreateWork, death_ro_capture_MM_unused) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  {
    EXPECT_DEATH(
      {
        mock_runtime->save_tasks = true;
        auto tmp = initial_access<int>("hello");
        create_work([=] {
          create_work(reads(tmp), [=] { });
        });
        run_all_tasks();
      },
      "handle with key .* declared as read usage, but was actually unused"
    );
  }

}
#endif

////////////////////////////////////////////////////////////////////////////////

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
