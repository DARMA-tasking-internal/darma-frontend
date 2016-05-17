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
#include <src/include/darma/interface/app/create_work.h>
#include <src/include/darma/interface/app/initial_access.h>

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

struct SimplerFunctor {
  void
  operator()() const {
    std::cout << "Hello World" << std::endl;
  }
};

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simple) {
  using namespace ::testing;

  mock_runtime->save_tasks = true;

  handle_t* h0, *h1;
  h0 = h1 = nullptr;

  InSequence s;

  EXPECT_CALL(*mock_runtime, register_handle(_))
    .Times(2)
    .WillOnce(SaveArg<0>(&h0))
    .WillOnce(SaveArg<0>(&h1));

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    handle_in_get_dependencies(h0), needs_write_handle(h0), Not(needs_read_handle(h0))
  )));

  EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h1))));

  EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h0))));

  {
    auto tmp = initial_access<int>("hello");
    EXPECT_VERSION_EQ(tmp, {0});
    create_work<SimpleFunctor>(15, tmp);
    EXPECT_VERSION_EQ(tmp, {1});
  }

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simpler) {
  using namespace ::testing;
  testing::internal::CaptureStdout();

  mock_runtime->save_tasks = true;

  create_work<SimplerFunctor>();

  run_all_tasks();

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "Hello World\n"
  );
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simple_read) {
  using namespace ::testing;

  mock_runtime->save_tasks = true;

  handle_t* h0, *h1, *h2;
  h0 = h1 = h2 = nullptr;

  InSequence s;

  EXPECT_CALL(*mock_runtime, register_handle(_))
    .Times(2)
    .WillOnce(SaveArg<0>(&h0))
    .WillOnce(SaveArg<0>(&h1));

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    handle_in_get_dependencies(h0), needs_write_handle(h0), Not(needs_read_handle(h0))
  )));
  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    handle_in_get_dependencies(h1), Not(needs_write_handle(h1)), needs_read_handle(h1)
  )));

  EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h0))));

  EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h1))));

  {
    auto tmp = initial_access<int>("hello");
    EXPECT_VERSION_EQ(tmp, {0});
    create_work<SimpleFunctor>(15, tmp);
    EXPECT_VERSION_EQ(tmp, {1});
    create_work<SimpleFunctor>(15, reads(tmp));
    EXPECT_VERSION_EQ(tmp, {1});
  }

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simple_read_only) {
  using namespace ::testing;
  using namespace darma_runtime;

  mock_runtime->save_tasks = true;

  handle_t* h0, *h1, *h2;
  h0 = h1 = h2 = nullptr;

  InSequence s;

  EXPECT_CALL(*mock_runtime, register_handle(_))
    .Times(2)
    .WillOnce(SaveArg<0>(&h0))
    .WillOnce(SaveArg<0>(&h1));

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    handle_in_get_dependencies(h0), needs_write_handle(h0), Not(needs_read_handle(h0))
  )));
  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    handle_in_get_dependencies(h1), Not(needs_write_handle(h1)), needs_read_handle(h1)
  )));

  EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h0))));

  EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h1))));

  static_assert(
    not detail::functor_traits<SimpleReadOnlyFunctor>::template formal_arg_traits<0>::is_access_handle, ""
  );
  static_assert(
    detail::functor_traits<SimpleReadOnlyFunctor>::template formal_arg_traits<1>::is_access_handle, ""
  );
  static_assert(
    not detail::functor_call_traits<SimpleReadOnlyFunctor, int&&, AccessHandle<int>&>
      ::template call_arg_traits<0>::is_access_handle, ""
  );
  static_assert(
    detail::functor_call_traits<SimpleReadOnlyFunctor, int&&, AccessHandle<int>&>
      ::template call_arg_traits<1>::is_access_handle, ""
  );
  static_assert(
    not detail::_functor_traits_impl::decayed_is_compile_time_immediate_modifiable<ReadAccessHandle<int>>::value,
    ""
  );
  static_assert(
    not detail::functor_traits<SimpleReadOnlyFunctor>
      ::template formal_arg_traits<1>::is_compile_time_immediate_modifiable_handle,
    "should not be compile-time immediate modifiable handle"
  );
  static_assert(
    detail::functor_traits<SimpleReadOnlyFunctor>
      ::template formal_arg_traits<1>::is_compile_time_immediate_read_only_handle,
      "Not compile-time immediate read-only handle"
  );
  //static_assert(
  //  not detail::functor_traits<SimpleReadOnlyFunctor>::template arg_traits<1>::is_compile_time_modifiable::value, ""
  //);

  {
    auto tmp = initial_access<int>("hello");
    //static_assert(
    //  detail::functor_traits<SimpleReadOnlyFunctor>
    //    ::template call_arg_traits<decltype(tmp), std::integral_constant<size_t, 1>>::is_read_only_capture, ""
    //);
    EXPECT_VERSION_EQ(tmp, {0});
    create_work<SimpleFunctor>(15, tmp);
    EXPECT_VERSION_EQ(tmp, {1});
    create_work<SimpleReadOnlyFunctor>(15, tmp);
    EXPECT_VERSION_EQ(tmp, {1});
  }

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

// TODO write a test that detects the opposite, i.e., nonconst lvalue reference as Modify request and a corresponding death test when called with a read-only handle
TEST_F(TestFunctor, simple_read_only_convert) {
  using namespace ::testing;
  using namespace darma_runtime;

  mock_runtime->save_tasks = true;

  handle_t* h0, *h1, *h2;
  h0 = h1 = h2 = nullptr;

  int val = 25;

  Sequence shandle;

  mock_backend::MockDataBlock db;

  {

    InSequence s;

    EXPECT_CALL(*mock_runtime, register_handle(_))
      .InSequence(shandle)
      .WillOnce(
        DoAll(
          SaveArg<0>(&h1),
          Invoke([&](auto& h) { h->satisfy_with_data_block(&db); })
        )
      );

    EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
      handle_in_get_dependencies(h1), Not(needs_write_handle(h1)), needs_read_handle(h1)
    )));

    EXPECT_CALL(*mock_runtime, release_handle(Eq(ByRef(h1))));

  }

  EXPECT_CALL(db, get_data())
    .InSequence(shandle)
    .WillRepeatedly(Return((void*)&val));

  {
    auto tmp = initial_access<int>("hello");
//    static_assert(
//      not detail::functor_traits<SimpleReadOnlyFunctorConvert>
//        ::template call_arg_traits<decltype((tmp)), std::integral_constant<size_t, 1>>::is_access_handle,
//      "arg 1 should not be detected as an AccessHandle"
//    );
//    static_assert(
//      detail::functor_traits<SimpleReadOnlyFunctorConvert>
//        ::template call_arg_traits<decltype((tmp)), std::integral_constant<size_t, 1>>::is_convertible_to_value,
//      "arg 1 should be detected as convertible to type wrapped by AccessHandle tmp"
//    );
//    static_assert(
//      not detail::functor_traits<SimpleReadOnlyFunctorConvert>
//        ::template call_arg_traits<decltype((tmp)), std::integral_constant<size_t, 1>>::is_nonconst_lvalue_reference,
//      "arg 1 should not be detected as a non-const lvalue reference"
//    );
//    static_assert(
//      detail::functor_traits<SimpleReadOnlyFunctorConvert>
//        ::template call_arg_traits<decltype((tmp)), std::integral_constant<size_t, 1>>::is_const_conversion_capture,
//      "arg 1 should be detected as a const conversion capture"
//    );
//    static_assert(
//      detail::functor_traits<SimpleReadOnlyFunctorConvert>
//        ::template call_arg_traits<decltype((tmp)), std::integral_constant<size_t, 1>>::is_read_only_capture,
//      "arg 1 should be detected as a read only capture"
//    );
    EXPECT_VERSION_EQ(tmp, {0});
    create_work<SimpleReadOnlyFunctorConvert>(15, tmp);
    EXPECT_VERSION_EQ(tmp, {0});
  }

  run_all_tasks();

}
