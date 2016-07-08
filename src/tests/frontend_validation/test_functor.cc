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

#include <gtest/gtest.h>

#include "mock_backend.h"
#include "test_frontend.h"

#include <darma/interface/app/access_handle.h>
#include <darma/interface/app/create_work.h>
#include <darma/interface/app/initial_access.h>

using namespace darma_runtime;

////////////////////////////////////////////////////////////////////////////////

class TestFunctor
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

TEST_F(TestFunctor, simple) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;

  static_assert(std::is_convertible<meta::any_arg, AccessHandle<int>>::value, "any_arg not convertible!");

  mock_runtime->save_tasks = true;

  MockFlow fl_init[2], fl_cap[2], fl_con[2];
  use_t* uses[3];

  expect_initial_access(fl_init[0], fl_init[1], uses[0], make_key("hello"));
  expect_mod_capture_MN_or_MR(
    fl_init, fl_cap, fl_con, uses
  );

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(uses[1]))))
    .Times(1);

  EXPECT_CALL(*mock_runtime, release_use(AllOf(Eq(ByRef(uses[1])),
    IsUseWithFlows(&fl_cap[0], &fl_cap[1], use_t::Modify, use_t::Modify)
  ))).Times(1)
    .WillOnce(Assign(&uses[1], nullptr));
  EXPECT_CALL(*mock_runtime, release_use(AllOf(Eq(ByRef(uses[2])),
    IsUseWithFlows(&fl_con[0], &fl_con[1], use_t::Modify, use_t::None)
  ))).Times(1)
    .WillOnce(Assign(&uses[2], nullptr));

  {
    auto tmp = initial_access<int>("hello");
    create_work<SimpleFunctor>(15, tmp);
  }

  run_all_tasks();

}

//////////////////////////////////////////////////////////////////////////////

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

TEST_F(TestFunctor, simple_read) {
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  Sequence s_register_cap;

  MockFlow fl_init[2], fl_cap[2];
  use_t* uses[2];

  expect_initial_access(fl_init[0], fl_init[1], uses[0], make_key("hello"), s_register_cap);
  expect_ro_capture_RN_RR_MN_or_MR(fl_init, fl_cap, uses, s_register_cap);

  {
    auto tmp = initial_access<int>("hello");
    create_work<SimpleFunctor>(15, reads(tmp));
  }

  EXPECT_CALL(*mock_runtime, release_use(AllOf(Eq(ByRef(uses[1])),
    IsUseWithFlows(&fl_cap[0], &fl_cap[1], use_t::Read, use_t::Read)
  ))).Times(1).InSequence(s_register_cap);

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simple_read_only) {
  using namespace ::testing;
  using namespace mock_backend;
  using namespace darma_runtime;
  using namespace darma_runtime::detail;

  mock_runtime->save_tasks = true;

  Sequence s_register_cap;

  MockFlow fl_init[2], fl_cap[2];
  use_t* uses[2];

  expect_initial_access(fl_init[0], fl_init[1], uses[0], make_key("hello"), s_register_cap);
  expect_ro_capture_RN_RR_MN_or_MR(fl_init, fl_cap, uses, s_register_cap);


  {
    auto tmp = initial_access<int>("hello");

    // Static assert that the correct arg_tuple_entry is deduced
    StaticAssertTypeEq<
      typename functor_call_traits<SimpleReadOnlyFunctor, decltype((15)), decltype((tmp))>
        ::template call_arg_traits<1>::args_tuple_entry,
      ReadAccessHandle<int>
    >();

    create_work<SimpleReadOnlyFunctor>(15, tmp);
  }

  EXPECT_CALL(*mock_runtime, release_use(AllOf(Eq(ByRef(uses[1])),
    IsUseWithFlows(&fl_cap[0], &fl_cap[1], use_t::Read, use_t::Read)
  ))).Times(1).InSequence(s_register_cap);

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simple_read_only_convert) {
  using namespace ::testing;
  using namespace mock_backend;
  using namespace darma_runtime;
  using namespace darma_runtime::detail;

  mock_runtime->save_tasks = true;

  Sequence s_register_cap;

  MockFlow fl_init[2], fl_cap[2];
  use_t* uses[2];

  expect_initial_access(fl_init[0], fl_init[1], uses[0], make_key("hello"), s_register_cap);
  expect_ro_capture_RN_RR_MN_or_MR(fl_init, fl_cap, uses, s_register_cap);

  {
    auto tmp = initial_access<int>("hello");

    // Static assert that the correct arg_tuple_entry is deduced
    StaticAssertTypeEq<
      typename functor_call_traits<SimpleReadOnlyFunctorConvert, decltype((15)), decltype((tmp))>
      ::template call_arg_traits<1>::args_tuple_entry,
      typename ReadAccessHandle<int>::template with_traits<
        // Also a leaf...
        typename ReadAccessHandle<int>::traits
          ::template with_min_schedule_permissions<AccessHandlePermissions::None>::type
          ::template with_max_schedule_permissions<AccessHandlePermissions::None>::type
      >
    >();

    create_work<SimpleReadOnlyFunctorConvert>(15, tmp);
  }

  EXPECT_CALL(*mock_runtime, release_use(AllOf(Eq(ByRef(uses[1])),
    IsUseWithFlows(&fl_cap[0], &fl_cap[1], use_t::Read, use_t::Read)
  ))).Times(1).InSequence(s_register_cap);

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simple_mod_convert) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow fl_init[2], fl_cap[2], fl_con[2];
  use_t* uses[3];

  expect_initial_access(fl_init[0], fl_init[1], uses[0], make_key("hello"));
  expect_mod_capture_MN_or_MR(
    fl_init, fl_cap, fl_con, uses
  );

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(uses[1]))))
    .Times(1);

  EXPECT_CALL(*mock_runtime, release_use(AllOf(Eq(ByRef(uses[1])),
    IsUseWithFlows(&fl_cap[0], &fl_cap[1], use_t::Modify, use_t::Modify)
  ))).Times(1)
    .WillOnce(Assign(&uses[1], nullptr));
  EXPECT_CALL(*mock_runtime, release_use(AllOf(Eq(ByRef(uses[2])),
    IsUseWithFlows(&fl_con[0], &fl_con[1], use_t::Modify, use_t::None)
  ))).Times(1)
    .WillOnce(Assign(&uses[2], nullptr));

  {
    auto tmp = initial_access<int>("hello");
    create_work<SimpleFunctorNonConstLvalue>(tmp);
  }

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

// TODO test that replecates the problem that caused this to not compile
//#include <darma.h>
//using namespace darma_runtime;
//
//// this works fine
//struct storeMessage{
//  void operator()(AccessHandle<std::string> h) const{
//    h.set_value("hello world!");
//  }
//};
//// this works fine
//struct storeMessage2{
//  void operator()(std::string & mess) const{
//    mess = "hello world!";
//  }
//};
//
//// this work fine
//struct printMessage{
//  void operator()(ReadAccessHandle<std::string> h) const{
//    std::cout << h.get_value() << std::endl;
//  }
//};
//// this gives compile time error that is attached
//struct printMessage2{
//  void operator()(std::string mess) const{
//    std::cout << mess << std::endl;
//  }
//};
//
//TEST_F(TestFunctor, francesco)
//{
//  using namespace darma_runtime;
//
//
//  // create handle to string variable
//  auto greeting = initial_access<std::string>("myName", 42);
//  create_work<storeMessage>(greeting);    // ok
//  create_work<storeMessage2>(greeting);    // ok
//  create_work<printMessage>(greeting);    // ok
//  create_work<printMessage2>(greeting);  // compile time error
//
//}
