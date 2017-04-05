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

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(oo_interface)

#include <gtest/gtest.h>

#include "mock_backend.h"
#include "test_frontend.h"
#include "../metatest_helpers.h"

#include <darma/interface/app/access_handle.h>
#include <darma/interface/app/create_work.h>
#include <darma/interface/app/initial_access.h>

#include <darma/interface/app/oo.h>

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
#include <ootag_larry.generated.h>
#include <ootag_curly.generated.h>
#include <ootag_moe.generated.h>
#include <ootag_bart.generated.h>
#include <ootag_lisa.generated.h>
#include <ootag_marge.generated.h>
#include <ootag_homer.generated.h>
#endif

using namespace darma_runtime::oo;

DARMA_OO_DECLARE_CLASS(Simple);

static_assert(is_darma_class<Simple>::value,
  "metafunction is_darma_class should return true_type for class Simple"
);

struct Foo;
static_assert(not darma_runtime::oo::detail::is_complete_darma_class_from_context<Simple, Foo>::value,
  "metafunction is_complete_darma_class_from_context should return false_type for class Simple before definition"
);

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

struct Foo2;
static_assert(darma_runtime::oo::detail::is_complete_darma_class_from_context<Simple, Foo2>::value,
  "metafunction is_complete_darma_class_from_context should return true_type for class Simple after definition"
);


struct Simple_constructors
  : darma_constructors<Simple>
{
  void operator()(std::string const& larry_key_string) {
    larry = initial_access<int>(larry_key_string);
  }
  // example copy constructor
  void operator()(darma_class_instance<Simple> const& other) {
    larry = initial_access<int>(other.larry.get_key(), "copied");
    {
      // This is the weird syntax you have to use to use lambdas in
      // a class context...
      create_work(
        reads(other.larry), [
          larry=larry, other_larry=other.larry
        ] {
          larry.set_value(other_larry.get_value());
        }
      );
    }
  }
};

STATIC_ASSERT_SIZE_IS(Simple,
  sizeof(darma_runtime::AccessHandle<int>)
    + sizeof(darma_runtime::AccessHandle<std::string>)
    + sizeof(darma_runtime::AccessHandle<double>)
);

using namespace darma_runtime::oo::detail;

template <>
struct Simple_method<bart>
  : darma_method<Simple,
      reads_value_<moe>
    >
{
  using darma_method::darma_method;
  void operator()(int value) {
    this->immediate::lisa(value);
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
  void operator()(int& value) {
    std::cout << moe << " == " << value;
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
  >();

  // Make sure the public method works like it should
  static_assert_type_eq<
    decltype( std::declval<simple_oo_test::Simple>().bart() ),
    void
  >();

  // Make sure the public method works like it should
  static_assert_type_eq<
    decltype( std::declval<simple_oo_test::Simple>().lisa() ),
    void
  >();

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

//////////////////////////////////////////////////////////////////////////////////

DARMA_OO_DEFINE_TAG(some_private_field);
DARMA_OO_DEFINE_TAG(some_public_field);
DARMA_OO_DEFINE_TAG(field_that_is_darma_class);

DARMA_OO_DEFINE_TAG(my_mod_value);
DARMA_OO_DEFINE_TAG(my_mod);
DARMA_OO_DEFINE_TAG(my_read_value);
DARMA_OO_DEFINE_TAG(my_read);

static_assert(darma_runtime::oo::detail::is_oo_name_tag<some_private_field>::value,
  "metafunction is_oo_name_tag should return true_type for class some_private_field"
);

DARMA_OO_DECLARE_CLASS(MyClass);
DARMA_OO_DECLARE_CLASS(MyOtherClass);

using namespace darma_runtime::oo;



struct MyClass
  : darma_class<MyClass,
      private_fields<
        std::string, some_private_field
      >,
      public_fields<
        int, some_public_field
      >,
      public_methods<
        my_mod_value,
        my_mod,
        my_read_value,
        my_read
      >
    >
{ using darma_class::darma_class; };

struct MyOtherClass
  : darma_class<MyOtherClass,
      public_fields<
        MyClass, field_that_is_darma_class
      >,
      public_methods<
        my_mod
      >
    >
{ using darma_class::darma_class; };

//----------------------------------------------------------------------------//

struct MyClass_constructors
  : darma_constructors<MyClass>
{
  using darma_constructors::darma_constructors;
  //void operator()() {
  //  some_private_field = initial_access<std::string>("default_private_key");
  //}
  void operator()(types::key_t const& private_key) {
    some_private_field = initial_access<std::string>(private_key);
  }
  template <typename AccessHandleT>
  std::enable_if_t<darma_runtime::detail::is_access_handle<AccessHandleT>::value>
  operator()(AccessHandleT&& private_handle) {
    // TODO decide if this is allowed
    some_private_field = darma_runtime::darma_copy(std::forward<AccessHandleT>(private_handle));
    std::cout << "hello world" << std::endl;
  }
  void operator()(types::key_t const& private_key, types::key_t const& public_key) {
    some_private_field = initial_access<std::string>(private_key);
    some_public_field = initial_access<int>(public_key);
  }
  // copy ctor
  void operator()(darma_runtime::oo::darma_class_instance<MyClass> const& other) {
    // copy ctor
    FAIL() << "Copy ctor shouldn't be called; it's just defined to make MyClass"
           << " copy constructible for detection test";
  }
};

//----------------------------------------------------------------------------//

template <>
struct MyClass_method<my_mod>
  : darma_method<MyClass,
      modifies_<some_private_field, some_public_field>
    >
{
  using darma_method::darma_method;
  void operator()(){
    some_private_field.set_value("hello");
    some_public_field.set_value(42);
  }
};

//////////////////////////////////////////////////////////////////////////////////


//----------------------------------------------------------------------------//

template <>
struct MyOtherClass_method<my_mod>
  : darma_method<MyOtherClass,
      modifies_<field_that_is_darma_class>::subfield<some_public_field>
    >
{
  using darma_method::darma_method;
  void operator()(){
    field_that_is_darma_class.some_public_field.set_value(42);

  }

};

TEST_F(TestOO, field_slicing_simple) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;

  MockFlow f_init_priv, f_null_priv;
  MockFlow f_init_pub, f_null_pub;

  EXPECT_INITIAL_ACCESS(f_init_priv, f_null_priv, make_key("hello"));
  EXPECT_INITIAL_ACCESS(f_init_pub, f_null_pub, make_key("world"));

  {
    MyOtherClass my;
    my.field_that_is_darma_class = MyClass(make_key("hello"), make_key("world"));
  }

}

TEST_F(TestOO, field_slicing_method) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;

  MockFlow f_init_priv, f_null_priv;
  MockFlow f_init_pub, f_null_pub, f_task_pub;
  use_t* task_use;

  EXPECT_INITIAL_ACCESS(f_init_priv, f_null_priv, make_key("hello"));
  EXPECT_INITIAL_ACCESS(f_init_pub, f_null_pub, make_key("world"));
  EXPECT_MOD_CAPTURE_MN_OR_MR(f_init_pub, f_task_pub, task_use);

  int value = 0;

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(task_use))))
    .WillOnce(Invoke([&](auto* task){
      for(auto&& use : task->get_dependencies()) {
        use->get_data_pointer_reference() = (void*)(&value);
      }
    }));

  EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_task_pub, &f_null_pub));
  EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_init_priv, &f_null_priv));

  {
    MyOtherClass my;
    my.field_that_is_darma_class = MyClass(make_key("hello"), make_key("world"));

    my.my_mod();

  }

  run_all_tasks();

  ASSERT_THAT(value, Eq(42));

}

// Not supported
//TEST_F(TestOO, default_ctor) {
//  using namespace ::testing;
//  using namespace darma_runtime;
//  using namespace mock_backend;
//
//  //static_assert_type_eq<
//  //  typename MyClass::darma_class
//  //    ::template constructor_implementation_type<MyClass>::type,
//  //  MyClass_constructors
//  //>();
//  //static_assert(
//  //  meta::is_detected<_empty_call_op_callable, MyClass_constructors>::value, ""
//  //);
//
//  static_assert(
//    MyClass::darma_class::template default_constructor_implementation_callable<MyClass>::type::value,
//    "MyClass default constructor should be callable"
//  );
//
//  MockFlow f_init, f_null;
//
//  EXPECT_INITIAL_ACCESS(f_init, f_null, make_key("default_private_key"));
//
//  EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_init, &f_null));
//
//  {
//    //MyClass my;
//  }
//
//}

TEST_F(TestOO, private_key_ctor) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;

  MockFlow f_init, f_null;

  EXPECT_INITIAL_ACCESS(f_init, f_null, make_key("hello"));

  EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_init, &f_null));

  {
    MyClass my(make_key("hello"));
  }
}

TEST_F(TestOO, handle) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;

  testing::internal::CaptureStdout();

  MockFlow f_init, f_null;

  EXPECT_INITIAL_ACCESS(f_init, f_null, make_key("hello"));
  {
    auto tmp = initial_access<std::string>("hello");
    MyClass my(tmp);
  }

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "hello world\n"
  );

}


TEST_F(TestOO, both_keys) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;

  MockFlow f_init_priv, f_null_priv;
  MockFlow f_init_pub, f_null_pub;

  EXPECT_INITIAL_ACCESS(f_init_priv, f_null_priv, make_key("hello"));
  EXPECT_INITIAL_ACCESS(f_init_pub, f_null_pub, make_key("world"));

  {
    MyClass my(make_key("hello"), make_key("world"));
  }

}

//////////////////////////////////////////////////////////////////////////////////

using namespace darma_runtime::oo::detail;
using namespace simple_oo_test;
typedef typename reads_<larry>::template subfield<curly>::template subfield<moe> subfield_desc;
STATIC_ASSERT_TYPE_EQ(
  subfield_desc,
  subfields_path<outermost_decorator<reads_>,
    tinympl::vector<larry>,
    tinympl::vector<curly>,
    tinympl::vector<moe>
  >
);

//////////////////////////////////////////////////////////////////////////////////

static_assert(
  std::is_convertible<darma_runtime::meta::any_arg, MyClass>::value,
  "any_arg should be convertible to a DARMA class type MyClass"
);
static_assert(
  std::is_convertible<darma_runtime::meta::any_arg, MyOtherClass>::value,
  "any_arg should be convertible to a DARMA class type MyOtherClass"
);

//////////////////////////////////////////////////////////////////////////////////

TEST_F(TestOO, simple_copy_ctor) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace mock_backend;

  MockFlow finit_s, fnull_s;
  MockFlow finit_t, fnull_t, f_task_out_t;
  use_t* s_read_use, *t_mod_use;

  /* expectation of things happening in string constructor */
  EXPECT_INITIAL_ACCESS(finit_s, fnull_s, make_key("larry"));

  /* expectations in copy constructor */
  EXPECT_INITIAL_ACCESS(finit_t, fnull_t, make_key("larry", "copied"));

  EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR(finit_s, s_read_use);
  EXPECT_MOD_CAPTURE_MN_OR_MR(finit_t, f_task_out_t, t_mod_use);

  int larry_value = 73;
  int larry_copy_value = 0;

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(AllOf(
    UseInGetDependencies(ByRef(s_read_use)),
    UseInGetDependencies(ByRef(t_mod_use))
  ))).WillOnce(Invoke([&](auto&&...) {
    s_read_use->get_data_pointer_reference() = &larry_value;
    t_mod_use->get_data_pointer_reference() = &larry_copy_value;
  }));

  /* end expectations in copy constructor */

  {
    simple_oo_test::Simple s("larry");
    simple_oo_test::Simple t(s);
  }

  // only expect releases after tasks run
  EXPECT_RELEASE_USE(s_read_use);
  EXPECT_RELEASE_USE(t_mod_use);

  run_all_tasks();

  ASSERT_EQ(larry_copy_value, 73);
}

//////////////////////////////////////////////////////////////////////////////////


DARMA_OO_DECLARE_CLASS(darma_vector);

DARMA_OO_DEFINE_TAG(myData_);
DARMA_OO_DEFINE_TAG(initialize);
DARMA_OO_DEFINE_TAG(print);
DARMA_OO_DEFINE_TAG(scalarMultiply);
DARMA_OO_DEFINE_TAG(plusEquals);

using namespace darma_runtime;
using namespace darma_runtime::oo;

struct darma_vector : darma_class<darma_vector,
  public_fields<std::vector<double>, myData_>,
  public_methods<initialize>,
  public_methods<print>,
  public_methods<scalarMultiply>,
  public_methods<plusEquals>
>
{ using darma_class::darma_class; };

struct darma_vector_constructors
  : darma_constructors<darma_vector>
{
  using darma_constructors::darma_constructors;
  void operator()(int me) {
    myData_ = initial_access<std::vector<double>>(me, "v_my data");
  }
  void operator()(darma_class_instance<darma_vector> const& other) {
    myData_ = other.myData_;
  }
};

template <>
struct darma_vector_method<initialize>
  : darma_method<darma_vector,
      modifies_value_<myData_>
    >
{
  using darma_method::darma_method;
  void operator()(int size, double last_value) {
    myData_ = std::vector<double>(size, 0.0);
    myData_[size-1] = last_value;
  }
};

template <>
struct darma_vector_method<print>
  : darma_method<darma_vector,
      reads_value_<myData_>
    >
{
  using darma_method::darma_method;
  void operator()(int index) {
    std::cout << index << " : " << std::setprecision(3) << myData_[index]
              << std::endl;
  }
};

template <>
struct darma_vector_method<scalarMultiply>
  : darma_method<darma_vector,
    modifies_value_<myData_>
  >
{
  using darma_method::darma_method;
  void operator()(double scale) {
    for(auto& val : myData_) {
      val *= scale;
    }
  }
};

template <>
struct darma_vector_method<plusEquals>
  : darma_method<darma_vector,
    modifies_value_<myData_>
  >
{
  using darma_method::darma_method;
  void operator()(darma_vector other) {
    int i = 0;
    for(auto& val : myData_) {
      val += other.myData_.get_value()[i++];
    }
  }
};

//////////////////////////////////////////////////////////////////////////////////

TEST_F(TestOO, use_darma_vector) {
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  ::testing::internal::CaptureStdout();

  MockFlow finit, fnull, f_initialize_out, f_scale_out;
  use_t* initialize_use, *print_use, *scale_use, *print_2_use;
  initialize_use = print_use = scale_use = print_2_use = nullptr;

  Sequence s1;


  EXPECT_INITIAL_ACCESS(finit, fnull, make_key(5, "v_my data"));

  std::vector<double> my_data_value;

  EXPECT_MOD_CAPTURE_MN_OR_MR(finit, f_initialize_out, initialize_use);

  EXPECT_REGISTER_TASK(initialize_use)
    .InSequence(s1)
    .WillOnce(Invoke([&](auto&&...){
      initialize_use->get_data_pointer_reference() = &my_data_value;
    }));

  EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR(f_initialize_out, print_use).InSequence(s1);

  EXPECT_REGISTER_TASK(print_use)
    .InSequence(s1)
    .WillOnce(Invoke([&](auto&&...){
      print_use->get_data_pointer_reference() = &my_data_value;
    }));

  EXPECT_MOD_CAPTURE_MN_OR_MR(f_initialize_out, f_scale_out, scale_use);

  EXPECT_REGISTER_TASK(scale_use)
    .InSequence(s1)
    .WillOnce(Invoke([&](auto&&...){
      scale_use->get_data_pointer_reference() = &my_data_value;
    }));

  EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR(f_scale_out, print_2_use).InSequence(s1);

  EXPECT_REGISTER_TASK(print_2_use)
    .InSequence(s1)
    .WillOnce(Invoke([&](auto&&...){
      print_2_use->get_data_pointer_reference() = &my_data_value;
    }));

  EXPECT_FLOW_ALIAS(f_scale_out, fnull).InSequence(s1);

  EXPECT_RELEASE_FLOW(fnull).InSequence(s1);

  //============================================================================
  // Code being tested
  {

    darma_vector vect(5);

    vect.initialize(20, 3.14);

    vect.print(19);

    vect.scalarMultiply(2.0);

    vect.print(19);

    darma_vector v2 = vect;
  }
  //============================================================================

  {
    InSequence seq;

    EXPECT_RELEASE_USE(initialize_use);
    EXPECT_RELEASE_FLOW(finit);

    EXPECT_RELEASE_USE(print_use);

    EXPECT_RELEASE_USE(scale_use);

    EXPECT_RELEASE_FLOW(f_initialize_out);

    EXPECT_RELEASE_USE(print_2_use);

    EXPECT_RELEASE_FLOW(f_scale_out);
  }

  run_all_tasks();

  EXPECT_THAT(my_data_value.size(), Eq(20));
  EXPECT_THAT(my_data_value[19], Eq(double(3.14) * double(2.0)));

  ASSERT_EQ(::testing::internal::GetCapturedStdout(),
    "19 : 3.14\n"
    "19 : 6.28\n"
  );

}

// TODO not working yet
////////////////////////////////////////////////////////////////////////////////////
//
//static_assert(is_darma_class<darma_vector>::value, "");
//
//TEST_F(TestOO, darma_vector_plus_equals) {
//  using namespace ::testing;
//  using namespace mock_backend;
//
//  mock_runtime->save_tasks = true;
//
//  ::testing::internal::CaptureStdout();
//
//  MockFlow finit, fnull, finit2, fnull2, f_init_out1, f_init_out2, fplus1, fplus2;
//  use_t* initialize_use, *initialize_use2;
//  use_t* plus_use, *plus_use2;
//  initialize_use = initialize_use2 = plus_use = plus_use2 = nullptr;
//
//  Sequence s1;
//
//  EXPECT_INITIAL_ACCESS(finit, fnull, make_key(5, "v_my data"));
//  EXPECT_INITIAL_ACCESS(finit2, fnull2, make_key(6, "v_my data"));
//
//  std::vector<double> my_data_value;
//  std::vector<double> my_data_value_2;
//
//  EXPECT_MOD_CAPTURE_MN_OR_MR(finit, f_init_out1, initialize_use);
//  EXPECT_MOD_CAPTURE_MN_OR_MR(finit2, f_init_out2, initialize_use2);
//
//  EXPECT_REGISTER_TASK(initialize_use)
//    .WillOnce(Invoke([&](auto&&...){
//      initialize_use->get_data_pointer_reference() = &my_data_value;
//    }));
//  EXPECT_REGISTER_TASK(initialize_use2)
//    .WillOnce(Invoke([&](auto&&...){
//      initialize_use2->get_data_pointer_reference() = &my_data_value_2;
//    }));
//
//  EXPECT_MOD_CAPTURE_MN_OR_MR(f_init_out1, fplus1, plus_use);
//  EXPECT_MOD_CAPTURE_MN_OR_MR(f_init_out2, fplus2, plus_use2);
//  EXPECT_REGISTER_TASK(plus_use)
//    .WillOnce(Invoke([&](auto&&...){
//      plus_use->get_data_pointer_reference() = &my_data_value;
//    }));
//  EXPECT_REGISTER_TASK(plus_use2)
//    .WillOnce(Invoke([&](auto&&...){
//      plus_use2->get_data_pointer_reference() = &my_data_value_2;
//    }));
//
//  //EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR(f_initialize_out, print_use).InSequence(s1);
//
//  //EXPECT_REGISTER_TASK(print_use)
//  //  .InSequence(s1)
//  //  .WillOnce(Invoke([&](auto&&...){
//  //    print_use->get_data_pointer_reference() = &my_data_value;
//  //  }));
//
//  //EXPECT_MOD_CAPTURE_MN_OR_MR(f_initialize_out, f_scale_out, scale_use);
//
//  //EXPECT_REGISTER_TASK(scale_use)
//  //  .InSequence(s1)
//  //  .WillOnce(Invoke([&](auto&&...){
//  //    scale_use->get_data_pointer_reference() = &my_data_value;
//  //  }));
//
//  //EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR(f_scale_out, print_2_use).InSequence(s1);
//
//  //EXPECT_REGISTER_TASK(print_2_use)
//  //  .InSequence(s1)
//  //  .WillOnce(Invoke([&](auto&&...){
//  //    print_2_use->get_data_pointer_reference() = &my_data_value;
//  //  }));
//
//  EXPECT_FLOW_ALIAS(fplus1, fnull);
//  EXPECT_FLOW_ALIAS(fplus2, fnull2);
//
//  EXPECT_RELEASE_FLOW(fnull);
//  EXPECT_RELEASE_FLOW(fnull2);
//
//  //============================================================================
//  // Code being tested
//  {
//
//    darma_vector vect(5);
//    darma_vector v2(6);
//
//    vect.initialize(20, 3.5);
//    v2.initialize(20, 2.3);
//
//    v2.plusEquals(vect);
//
//
//    //v2.print(19);
//
//  }
//  //============================================================================
//
//  //{
//  //  InSequence seq;
//
//  //  EXPECT_RELEASE_USE(initialize_use);
//  //  EXPECT_RELEASE_FLOW(finit);
//
//  //  EXPECT_RELEASE_USE(print_use);
//
//  //  EXPECT_RELEASE_USE(scale_use);
//
//  //  EXPECT_RELEASE_FLOW(f_initialize_out);
//
//  //  EXPECT_RELEASE_USE(print_2_use);
//
//  //  EXPECT_RELEASE_FLOW(f_scale_out);
//  //}
//
//  run_all_tasks();
//
//  EXPECT_THAT(my_data_value.size(), Eq(20));
//  EXPECT_THAT(my_data_value_2[19], Eq(double(3.5) + double(2.3)));
//
//  ASSERT_EQ(::testing::internal::GetCapturedStdout(),
//    "19 : 3.14\n"
//      "19 : 6.28\n"
//  );
//
//}

#endif // _darma_has_feature(oo_interface)
