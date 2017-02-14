/*
//@HEADER
// ************************************************************************
//
//                        test_functor
//                           DARMA
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

TEST_F(TestFunctor, simpler) {
  using namespace ::testing;
  testing::internal::CaptureStdout();

  mock_runtime->save_tasks = true;

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(_))
    .Times(1);

  create_work<SimplerFunctor>();

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

  create_work<SimplerFunctor>(name="hello_task");

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

  EXPECT_INITIAL_ACCESS(f_initial, f_null, make_key("hello"));

  //--------------------
  // Expect mod capture:

  EXPECT_CALL(*mock_runtime, make_next_flow(&f_initial))
    .WillOnce(Return(&f_task_out));

  use_t::permissions_t expected_scheduling_permissions;
  if(test_type == "simple_handle") {
    expected_scheduling_permissions = use_t::Modify;
  }
  else {
    expected_scheduling_permissions = use_t::None;
  }

  EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows(
    &f_initial, &f_task_out,
    expected_scheduling_permissions,
    use_t::Modify
  ))).WillOnce(SaveArg<0>(&task_use));

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(
    UseInGetDependencies(ByRef(task_use))
  ));

  EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_task_out, &f_null));

  // End expect mod capture
  //--------------------

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

  EXPECT_CALL(*mock_runtime, release_use(task_use));

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


  MockFlow fl_init, fl_null;
  use_t* task_use = nullptr;

  std::string test_type = GetParam();

  Sequence s1;

  EXPECT_INITIAL_ACCESS(fl_init, fl_null, make_key("hello"));

  //--------------------
  // Expect ro capture:

  use_t::permissions_t expected_scheduling_permissions;
  if(test_type == "convert" || test_type == "convert_value"
    || test_type == "convert_long" || test_type == "convert_string") {
    expected_scheduling_permissions = use_t::None;
  }
  else {
    expected_scheduling_permissions = use_t::Read;
  }

  int data = 0;

  EXPECT_CALL(*mock_runtime, register_use(
    IsUseWithFlows(
      &fl_init, &fl_init,
      expected_scheduling_permissions,
      use_t::Read
    )
  )).InSequence(s1).WillOnce(
    Invoke([&](auto* use) {
      use->get_data_pointer_reference() = (void*)(&data);
      task_use = use;
    })
  );


  EXPECT_CALL(*mock_runtime,
    register_task_gmock_proxy(UseInGetDependencies(ByRef(task_use)))
  ).InSequence(s1);

  EXPECT_CALL(*mock_runtime, establish_flow_alias(&fl_init, &fl_null))
    .InSequence(s1);

  // End expect ro capture
  //--------------------

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

  EXPECT_CALL(*mock_runtime, release_use(Eq(ByRef(task_use)))).InSequence(s1);

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

  MockFlow f_init, f_task_out, f_null;
  use_t* task_use;

  EXPECT_INITIAL_ACCESS(f_init, f_null, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(f_init, f_task_out, task_use);

  EXPECT_FLOW_ALIAS(f_task_out, f_null);

  EXPECT_RELEASE_FLOW(f_null);

  //============================================================================
  // Code to actually be tested
  {
    auto tmp = initial_access<int>("hello");

    struct FunctorHandleRef {
      void
      operator()( AccessHandle<int>& ) const {
        std::cout << "Hello World" << std::endl;
      }
    };

    create_work<FunctorHandleRef>(tmp);

  }
  //============================================================================

  EXPECT_RELEASE_USE(task_use);
  EXPECT_RELEASE_FLOW(f_init);
  EXPECT_RELEASE_FLOW(f_task_out);

  run_all_tasks();

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "Hello World\n"
  );
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

