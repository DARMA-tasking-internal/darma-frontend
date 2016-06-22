/*
//@HEADER
// ************************************************************************
//
//                      test_oo.cc
//                         DARMA
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
#include "../metatest_helpers.h"

#include <darma/interface/app/access_handle.h>
#include <darma/interface/app/create_work.h>
#include <darma/interface/app/initial_access.h>

#include <darma/impl/oo/class.h>
#include <darma/impl/oo/method.h>
#include <darma/impl/oo/macros.h>

using namespace darma_runtime;

////////////////////////////////////////////////////////////////////////////////

class TestOO
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

namespace simple_oo_test {


// Generate these with python for testing/debugging purposes
#ifdef DARMA_CANNOT_GENERATE_TAG_HEADERS
DARMA_OO_DEFINE_TAG(larry);
DARMA_OO_DEFINE_TAG(curly);
DARMA_OO_DEFINE_TAG(moe);

DARMA_OO_DEFINE_TAG(bart);
DARMA_OO_DEFINE_TAG(lisa);
DARMA_OO_DEFINE_TAG(marge);
DARMA_OO_DEFINE_TAG(homer);
#else
#include "ootag_larry.generated.h"
#include "ootag_curly.generated.h"
#include "ootag_moe.generated.h"
#include "ootag_bart.generated.h"
#include "ootag_lisa.generated.h"
#include "ootag_marge.generated.h"
#include "ootag_homer.generated.h"
#endif

using namespace darma_runtime::oo;

DARMA_OO_DECLARE_CLASS(Simple);

struct Simple
  : darma_class<Simple,
      private_fields<
        int, larry
      >,
      public_fields<
        std::string, curly,
        double, moe
      >,
      public_methods<
        bart,
        lisa,
        homer,
        marge
      >
    >
{ using darma_class::darma_class; };
STATIC_ASSERT_SIZE_IS(Simple,
  sizeof(darma_runtime::AccessHandle<int>)
    + sizeof(darma_runtime::AccessHandle<std::string>)
    + sizeof(darma_runtime::AccessHandle<double>)
);

template <>
struct Simple_method<bart>
  : darma_method<Simple,
      reads_value_<moe>
    >
{
  using darma_method::darma_method;
  void operator()() {
    this->immediate::lisa();
  }
};
STATIC_ASSERT_SIZE_IS( Simple_method<bart>, sizeof(double const&) );

template <>
struct Simple_method<lisa>
  : darma_method<Simple,
      reads_value_<moe>
    >
{
  using darma_method::darma_method;
  void operator()() {
    std::cout << moe << " == " << 42;
  }
};
STATIC_ASSERT_SIZE_IS( Simple_method<lisa>, sizeof(double const&) );

template <>
struct Simple_method<homer>
  : darma_method<Simple,
      modifies_value_<moe>
    >
{
  using darma_method::darma_method;
  void operator()() {
    moe = 42;
    // Signal the end of the homer() method (for testing purposes)
    sequence_marker->mark_sequence("homer");
  }

};
STATIC_ASSERT_SIZE_IS( Simple_method<homer>, sizeof(double&) );

template <>
struct Simple_method<marge>
  : darma_method<Simple,
      modifies_<moe>
    >
{
  using darma_method::darma_method;
  void operator()() {
    if( moe.get_value() > 10 ) {
      moe.get_reference() /= 2.0;
      // recurse:
      this->deferred::marge();
      marge();
      deferred::marge();
      // This works too:
      immediate::marge();
    }
    else {
      moe.set_value(3.14);
    }
  }
};
STATIC_ASSERT_SIZE_IS( Simple_method<marge>, sizeof(AccessHandle<double>) );


} // end namespace simple_oo_test

TEST_F(TestOO, static_assertions) {
  using namespace darma_runtime::oo;
  using namespace darma_runtime::oo::detail;

  // Test that ADL is able to get the associated method template specialization
  static_assert_type_eq<
    decltype(
      _darma__get_associated_method_template_specialization(
        std::declval<simple_oo_test::Simple&>(),
        std::declval<simple_oo_test::larry&>()
      )
    ),
    simple_oo_test::Simple_method<simple_oo_test::larry>
  >();

  // Private and public fields extraction
  static_assert_type_eq<
    typename simple_oo_test::Simple::darma_class::helper_t
      ::_private_fields_vector,
    tinympl::vector<
      _private_field_in_chain<AccessHandle<int>, simple_oo_test::larry>
    >
  >();
  static_assert_type_eq<
    typename simple_oo_test::Simple::darma_class::helper_t
      ::_public_fields_vector,
    tinympl::vector<
      _public_field_in_chain<AccessHandle<std::string>, simple_oo_test::curly>,
      _public_field_in_chain<AccessHandle<double>, simple_oo_test::moe>
    >
  >();

  static_assert_type_eq<
    typename simple_oo_test::Simple::darma_class::helper_t
      ::fields_with_types,
    tinympl::vector<
      _field_tag_with_type<std::string, simple_oo_test::curly>,
      _field_tag_with_type<double, simple_oo_test::moe>,
      _field_tag_with_type<int, simple_oo_test::larry>
    >
  >();

  // Make sure the public field works the way we think it does
  static_assert_type_eq<
    decltype( std::declval<simple_oo_test::Simple>().curly ),
    AccessHandle<std::string>
  >;

  // Make sure the public method works like it should
  static_assert_type_eq<
    decltype( std::declval<simple_oo_test::Simple>().bart() ),
    void
  >;

  // Make sure the public method works like it should
  static_assert_type_eq<
    decltype( std::declval<simple_oo_test::Simple>().lisa() ),
    void
  >;

  static_assert_type_eq<
    typename simple_oo_test::Simple_method<simple_oo_test::homer>
      ::darma_method::helper_t
      ::fields,
    tinympl::vector<
      _field_tag_with_type<double&, simple_oo_test::moe>
    >
  >();

  static_assert(
    is_darma_method_of_class<darma_method<simple_oo_test::Simple,
      modifies_value_<simple_oo_test::moe>
    >, simple_oo_test::Simple>::value, ""
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestOO, simple_homer_lisa) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  use_t* use_1, *use_2, *use_3, *use_4;
  MockFlow fl_in_1, fl_out_1;
  MockFlow fl_in_2, fl_out_2;
  MockFlow fl_in_3, fl_out_3;
  MockFlow fl_in_4, fl_out_4;

  Sequence s_reg_captured, s_reg_continuing, s_reg_initial, s_release_initial;

  expect_initial_access(fl_in_1, fl_out_1, use_1, make_key("moe", "s"),
    s_reg_initial, s_release_initial);

  expect_mod_capture_MN_or_MR(
    fl_in_1, fl_out_1, use_1,
    fl_in_2, fl_out_2, use_2,
    fl_in_3, fl_out_3, use_3,
    s_reg_captured, s_reg_continuing
  );

  double data = 0.0;

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(use_2))))
    .Times(1).InSequence(s_reg_captured, s_release_initial)
    .WillOnce(Invoke([&](auto* task){
      use_2->get_data_pointer_reference() = (void*)&data;
    }));


  expect_ro_capture_RN_RR_MN_or_MR(
    fl_in_3, fl_out_3, use_3,
    fl_in_4, fl_out_4, use_4,
    s_reg_continuing
  );

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(use_4))))
    .Times(1).InSequence(s_reg_captured, s_release_initial)
    .WillOnce(Invoke([&](auto* task){
      use_4->get_data_pointer_reference() = (void*)&data;
    }));

  EXPECT_CALL(*mock_runtime, release_use(AllOf(Eq(ByRef(use_3)),
    IsUseWithFlows(&fl_in_3, &fl_out_3, use_t::Modify, use_t::None)
  ))).Times(1).InSequence(s_reg_continuing)
    .WillOnce(Assign(&use_3, nullptr));


  {
    simple_oo_test::Simple s;
    s.moe = initial_access<double>("moe", "s");
    s.homer();
    s.bart(); // makes an "immediate" call to s.lisa();
  }

  // Now expect the releases that have to happen after the tasks start running
  EXPECT_CALL(*sequence_marker, mark_sequence("homer"))
    .Times(1).InSequence(s_reg_captured);

  EXPECT_CALL(*mock_runtime, release_use(AllOf(Eq(ByRef(use_2)),
    IsUseWithFlows(&fl_in_2, &fl_out_2, use_t::Modify, use_t::Modify)
  ))).Times(1).InSequence(s_reg_captured, s_release_initial)
    .WillOnce(Assign(&use_2, nullptr));

  EXPECT_CALL(*mock_runtime, release_use(AllOf(Eq(ByRef(use_4)),
    IsUseWithFlows(&fl_in_4, &fl_out_4, use_t::Read, use_t::Read)
  ))).Times(1).InSequence(s_reg_continuing)
    .WillOnce(Assign(&use_4, nullptr));

  testing::internal::CaptureStdout();

  run_all_tasks();

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "42 == 42"
  );

  ASSERT_EQ(data, 42);

}

