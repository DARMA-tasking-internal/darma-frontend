/*
//@HEADER
// ************************************************************************
//
//                        test_functor
//                           DARMA
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include "test_functor.h"


////////////////////////////////////////////////////////////////////////////////

struct SimpleFunctor {
  void
  operator()(
    int arg,
    AccessHandle<int> handle
  ) const {
    // Do nothing for now...
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimpleReadOnlyFunctor {
  void
  operator()(
    int arg,
    ReadAccessHandle<int> handle
  ) const {
    // Do nothing for now...
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimpleReadOnlyFunctorConvert {
  void
  operator()(
    int arg,
    int const& handle
  ) const {
    // Do nothing for now...
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimpleReadOnlyFunctorConvertValue {
  void
  operator()(
    int arg,
    int handle
  ) const {
    // Do nothing for now...
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimpleReadOnlyFunctorConvertLong {
  void
  operator()(
    int arg,
    long handle
  ) const {
    // Do nothing for now...
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimpleFunctorNonConstLvalue {
  void
  operator()(
    int& arg
  ) const {
    // Do nothing for now...
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimplerFunctor {
  void
  operator()() const {
    std::cout << "Hello World" << std::endl;
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimpleFunctorWithDowngradePermissions {

  void
  operator()(AccessHandle<int> handle) {

    EXPECT_DEATH(
      {
        handle.set_value(42);
      },
      "`set_value\\(\\)` performed on AccessHandle"
    );

  }

  static void permissions_downgrades(AccessHandle<int> handle) { 
     darma_runtime::detail::permissions_downgrades(reads(handle));
  }
};


////////////////////////////////////////////////////////////////////////////////

struct SimpleFunctorWithRequiredPermissions {
  
  void
  operator()(AccessHandle<int> handle1, AccessHandle<int> handle2) {
    
    EXPECT_DEATH(
      { 
        handle1.set_value(42);
      },
      "`set_value\\(\\)` performed on AccessHandle"
    );
  
  }
  
  static void required_permissions(AccessHandle<int> handle1, AccessHandle<int> handle2) {
    darma_runtime::detail::required_permissions(reads(handle2));
  }
};


////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simpler) {
  using namespace ::testing;
  testing::internal::CaptureStdout();

  mock_runtime->save_tasks = true;

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(_))
    .Times(1);

  //============================================================================
  // actual code being tested
  {
    create_work<SimplerFunctor>();
  }
  //============================================================================

  run_all_tasks();

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "Hello World\n"
  );
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simpler_named) {
  using namespace ::testing;
  testing::internal::CaptureStdout();
  using namespace darma_runtime::keyword_arguments_for_task_creation;

  mock_runtime->save_tasks = true;

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(HasName(make_key("hello_task"))))
    .Times(1);

  //============================================================================
  // actual code being tested
  {
    create_work<SimplerFunctor>(name = "hello_task");
  }
  //============================================================================

  run_all_tasks();

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "Hello World\n"
  );
}

//////////////////////////////////////////////////////////////////////////////

struct TestFunctorModCaptures
  : TestFunctor,
    ::testing::WithParamInterface<std::string>
{ };

TEST_P(TestFunctorModCaptures, Parametrized) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;

  static_assert(std::is_convertible<meta::any_arg, AccessHandle<int>>::value, "any_arg not convertible!");

  mock_runtime->save_tasks = true;

  std::string test_type = GetParam();

  MockFlow f_initial, f_null, f_task_out;
  use_t* task_use = nullptr;
  use_t* use_initial = nullptr;
  use_t* use_cont = nullptr;

  EXPECT_INITIAL_ACCESS(f_initial, f_null, use_initial, make_key("hello"));

  //--------------------
  // Expect mod capture:

  {
    InSequence s;

    EXPECT_CALL(*mock_runtime, make_next_flow(f_initial))
      .WillOnce(Return(f_task_out));

    darma_runtime::frontend::permissions_t expected_scheduling_permissions;
    if(test_type == "simple_handle") {
      expected_scheduling_permissions = darma_runtime::frontend::Permissions::Modify;
    }
    else {
      expected_scheduling_permissions = darma_runtime::frontend::Permissions::None;
    }

    EXPECT_CALL(*mock_runtime, legacy_register_use(IsUseWithFlows(
      f_initial, f_task_out,
      expected_scheduling_permissions,
      darma_runtime::frontend::Permissions::Modify
    ))).WillOnce(SaveArg<0>(&task_use));
    EXPECT_REGISTER_USE(use_cont, f_task_out, f_null, Modify, None);
    EXPECT_RELEASE_USE(use_initial);

    EXPECT_REGISTER_TASK(task_use);

    EXPECT_FLOW_ALIAS(f_task_out, f_null);
    EXPECT_RELEASE_USE(use_cont);
  }

  // End expect mod capture
  //--------------------


  //============================================================================
  // actual code being tested
  {
    auto tmp = initial_access<int>("hello");
    if(test_type == "simple_handle") {
      create_work<SimpleFunctor>(15, tmp);
    }
    else if(test_type == "convert") {
      create_work<SimpleFunctorNonConstLvalue>(tmp);
    }
    else {
      FAIL() << "unknown test type: " << test_type;
    }
  }
  //============================================================================

  EXPECT_RELEASE_USE(task_use);

  run_all_tasks();

}

INSTANTIATE_TEST_CASE_P(
  Parameterized,
  TestFunctorModCaptures,
  ::testing::Values(
    "simple_handle",
    "convert"
  )
);

////////////////////////////////////////////////////////////////////////////////

struct TestFunctorROCaptures
  : TestFunctor,
    ::testing::WithParamInterface<std::string>
{ };

TEST_P(TestFunctorROCaptures, Parameterized) {
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;


  DECLARE_MOCK_FLOWS(fl_init, fl_null);
  use_t* task_use = nullptr;
  use_t* use_init = nullptr;

  std::string test_type = GetParam();

  EXPECT_INITIAL_ACCESS(fl_init, fl_null, use_init, make_key("hello"));

  //--------------------
  // Expect ro capture:

  darma_runtime::frontend::permissions_t expected_scheduling_permissions;
  if(test_type == "convert" || test_type == "convert_value"
    || test_type == "convert_long" || test_type == "convert_string") {
    expected_scheduling_permissions = darma_runtime::frontend::Permissions::None;
  }
  else {
    expected_scheduling_permissions = darma_runtime::frontend::Permissions::Read;
  }

  int data = 0;

  EXPECT_CALL(*mock_runtime, legacy_register_use(
    IsUseWithFlows(
#if _darma_has_feature(anti_flows)
      fl_init, nullptr,
#else
      fl_init, fl_init,
#endif // _darma_has_feature(anti_flows)
      expected_scheduling_permissions,
      darma_runtime::frontend::Permissions::Read
    )
  )).WillOnce(
    Invoke([&](auto* use) {
      use->get_data_pointer_reference() = (void*)(&data);
      task_use = use;
    })
  );

  EXPECT_REGISTER_TASK(task_use);

  EXPECT_FLOW_ALIAS(fl_init, fl_null);
  EXPECT_RELEASE_USE(use_init);

  // End expect ro capture
  //--------------------

  //============================================================================
  // actual code being tested
  {
    if (test_type == "explicit_read") {
      // TODO reinstate this test
      //auto tmp = initial_access<int>("hello");
      //create_work<SimpleFunctor>(15, reads(tmp));
    }
    else if (test_type == "read_only_handle") {
      // Formal parameter is ReadOnlyAccessHandle<int>
      auto tmp = initial_access<int>("hello");
      create_work<SimpleReadOnlyFunctor>(15, tmp);
    }
    else if (test_type == "convert") {
      // Formal parameter is a const lvalue reference
      auto tmp = initial_access<int>("hello");
      create_work<SimpleReadOnlyFunctorConvert>(15, tmp);
    }
    else if (test_type == "convert_value") {
      // Formal parameter is by value
      auto tmp = initial_access<int>("hello");
      create_work<SimpleReadOnlyFunctorConvertValue>(15, tmp);
    }
    else if (test_type == "convert_long") {
      // Formal parameter is by value and is of type long int
      auto tmp = initial_access<int>("hello");
      create_work<SimpleReadOnlyFunctorConvertLong>(15, tmp);
    }
  }
  //============================================================================

  EXPECT_RELEASE_USE(task_use);

  run_all_tasks();

}

INSTANTIATE_TEST_CASE_P(
  Parameterized,
  TestFunctorROCaptures,
  ::testing::Values(
    //"explicit_read",
    "read_only_handle",
    "convert",
    "convert_value",
    "convert_long"
  )
);

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestFunctor, simple_handle_ref) {
  using namespace ::testing;
  using namespace mock_backend;

  testing::internal::CaptureStdout();

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(f_init, f_task_out, f_null);
  use_t* task_use;
  use_t* use_initial;
  use_t* use_cont;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_initial, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(f_init, f_task_out, task_use, f_null, use_cont);
  EXPECT_RELEASE_USE(use_initial);

  EXPECT_FLOW_ALIAS(f_task_out, f_null);
  EXPECT_RELEASE_USE(use_cont);

  //============================================================================
  // Code to actually be tested
  {

    struct FunctorHandleRef {
      void
      operator()( AccessHandle<int>& ) const {
        std::cout << "Hello World" << std::endl;
      }
    };

    auto tmp = initial_access<int>("hello");

    create_work<FunctorHandleRef>(tmp);

  }
  //============================================================================

  EXPECT_RELEASE_USE(task_use);

  run_all_tasks();

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "Hello World\n"
  );
}

////////////////////////////////////////////////////////////////////////////////

struct dinitialize2 {
  void
  operator()(
    double& a,
    int& b,
    int& c
  ) const {
    //std::cout << "Hello World" << std::endl;
  }
};


TEST_F(TestFunctor, three_refs) {
  using namespace ::testing;
  using namespace mock_backend;

  //testing::internal::CaptureStdout();

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(f_init, f_task_out, f_null);
  DECLARE_MOCK_FLOWS(f_init2, f_task_out2, f_null2);
  DECLARE_MOCK_FLOWS(f_init3, f_task_out3, f_null3);
  use_t* task_use;
  use_t* task_use2;
  use_t* task_use3;
  use_t* use_initial;
  use_t* use_initial2;
  use_t* use_initial3;
  use_t* use_cont;
  use_t* use_cont2;
  use_t* use_cont3;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_initial, make_key("hello"));
  EXPECT_INITIAL_ACCESS(f_init2, f_null2, use_initial2, make_key("hello2"));
  EXPECT_INITIAL_ACCESS(f_init3, f_null3, use_initial3, make_key("hello3"));

  EXPECT_LEAF_MOD_CAPTURE_MN_OR_MR(f_init, f_task_out, task_use, f_null, use_cont);
  EXPECT_RELEASE_USE(use_initial);

  EXPECT_LEAF_MOD_CAPTURE_MN_OR_MR(f_init2, f_task_out2, task_use2, f_null2, use_cont2);
  EXPECT_RELEASE_USE(use_initial2);

  EXPECT_LEAF_MOD_CAPTURE_MN_OR_MR(f_init3, f_task_out3, task_use3, f_null3, use_cont3);
  EXPECT_RELEASE_USE(use_initial3);

  EXPECT_REGISTER_TASK(task_use, task_use2, task_use3);

  EXPECT_FLOW_ALIAS(f_task_out, f_null);
  EXPECT_RELEASE_USE(use_cont);

  EXPECT_FLOW_ALIAS(f_task_out2, f_null2);
  EXPECT_RELEASE_USE(use_cont2);

  EXPECT_FLOW_ALIAS(f_task_out3, f_null3);
  EXPECT_RELEASE_USE(use_cont3);

  //============================================================================
  // Code to actually be tested
  {

    auto tmp = initial_access<double>("hello");
    auto tmp2 = initial_access<int>("hello2");
    auto tmp3 = initial_access<int>("hello3");

    create_work<dinitialize2>(tmp, tmp2, tmp3);

  }
  //============================================================================

  EXPECT_RELEASE_USE(task_use);
  EXPECT_RELEASE_USE(task_use2);
  EXPECT_RELEASE_USE(task_use3);

  run_all_tasks();

  //ASSERT_EQ(testing::internal::GetCapturedStdout(),
  //  "Hello World\n"
  //);
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, lvalue_argument_copy) {
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  //============================================================================
  // Code to actually be tested
  {

    struct ArgumentCopy {
      void operator()(std::vector<int> val) const {
        ASSERT_THAT(val, ElementsAre(1, 2, 3));
      }
    };

    std::vector<int> my_var = { 1, 2, 3 };

    create_work<ArgumentCopy>(my_var);

    // Change something to make sure the copy happens
    my_var[1] = 42;

  }
  //============================================================================

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, schedule_only) {
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(finit, fnull, fout);
  use_t* use_init, *use_cont, *use_capt;

  EXPECT_NEW_INITIAL_ACCESS(finit, fnull, use_init, make_key("hello"));

  EXPECT_NEW_REGISTER_USE(use_capt,
    finit, Same, &finit,
    fout, Next, nullptr, true,
    Modify, None, false
  );

  EXPECT_NEW_REGISTER_USE(use_cont,
    fout, Same, &fout,
    fnull, Same, &fnull, false,
    Modify, None, false
  );

  EXPECT_NEW_RELEASE_USE(use_init, false);

  EXPECT_REGISTER_TASK(use_capt);

  EXPECT_NEW_RELEASE_USE(use_cont, true);

  //============================================================================
  // Code to actually be tested
  {

    struct ScheduleOnlyFunctor {
      void operator()(
        ScheduleOnly<AccessHandle<int>> sched
      ) const {
        // nothing to do here
      }
    };

    auto my_var = initial_access<int>("hello");

    create_work<ScheduleOnlyFunctor>(my_var);

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_NEW_RELEASE_USE(use_capt, true);

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, functor_with_permissions_downgrades) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;

  static_assert(std::is_convertible<meta::any_arg, AccessHandle<int>>::value, "any_arg not convertible!");

  mock_runtime->save_tasks = true;

  MockFlow f_initial, f_null, f_task_out;
  use_t* task_use = nullptr;
  use_t* use_initial = nullptr;
  use_t* use_cont = nullptr;

  EXPECT_INITIAL_ACCESS(f_initial, f_null, use_initial, make_key("hello"));

  // --------------------
  // Expect read capture:

  {
    InSequence s;

    EXPECT_CALL(*mock_runtime, legacy_register_use(IsUseWithFlows(
      f_initial, nullptr, 
      darma_runtime::frontend::Permissions::Read,
      darma_runtime::frontend::Permissions::Read
    ))).WillOnce(SaveArg<0>(&task_use));

    EXPECT_REGISTER_TASK(task_use);

    EXPECT_RELEASE_USE(use_initial);
  }

  // End expect read capture
  // -----------------------


  //============================================================================
  // actual code being tested
  {
    auto tmp = initial_access<int>("hello");
    create_work<SimpleFunctorWithDowngradePermissions>(tmp);
  }
  //============================================================================

  EXPECT_RELEASE_USE(task_use);

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, functor_with_required_permissions) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;

  static_assert(std::is_convertible<meta::any_arg, AccessHandle<int>>::value, "any_arg not convertible!");

  mock_runtime->save_tasks = true;

  MockFlow f_initial_1, f_initial_2, f_null_1, f_null_2, f_task_out;
  use_t* task_use = nullptr;
  use_t* use_initial_1 = nullptr;
  use_t* use_initial_2 = nullptr;
  use_t* use_cont = nullptr;

  EXPECT_INITIAL_ACCESS(f_initial_1, f_null_1, use_initial_1, make_key("hello"));
  EXPECT_INITIAL_ACCESS(f_initial_2, f_null_2, use_initial_2, make_key("hello2"));

  // --------------------
  // Expect read capture:

  {
    InSequence s;

    EXPECT_CALL(*mock_runtime, legacy_register_use(IsUseWithFlows(
      f_initial_2, nullptr,
      darma_runtime::frontend::Permissions::Read,
      darma_runtime::frontend::Permissions::Read
    ))).WillOnce(SaveArg<0>(&task_use));

    EXPECT_REGISTER_TASK(task_use);

    EXPECT_RELEASE_USE(use_initial_2);

    EXPECT_RELEASE_USE(use_initial_1);
  }

  // End expect read capture
  // -----------------------


  //============================================================================
  // actual code being tested
  {
    auto tmp = initial_access<int>("hello");
    auto tmp2 = initial_access<int>("hello2");
    create_work<SimpleFunctorWithRequiredPermissions>(tmp, tmp2);
  }
  //============================================================================

  EXPECT_RELEASE_USE(task_use);

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

// Should fail gracefully at compile time if uncommented
//TEST_F(TestFunctor, lvalue_argument_copy) {
//  using namespace ::testing;
//  using namespace mock_backend;
//
//  {
//    struct MyClassUnserializable {
//      private:
//        int a;
//      public:
//        explicit MyClassUnserializable(int aval) : a(aval) { }
//    };
//
//    struct ArgumentCopy {
//      void operator()(MyClassUnserializable val) const { }
//    };
//
//    MyClassUnserializable unser(42);
//    //std::vector<int> my_var = { 1, 2, 3 };
//
//    create_work<ArgumentCopy>(unser);
//
//    // Change something to make sure the copy happens
//    //my_var[1] = 42;
//
//  }
//  //============================================================================
//
//  run_all_tasks();
//
//}

