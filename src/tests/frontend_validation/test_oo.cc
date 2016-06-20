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

using namespace darma_runtime::oo;



DARMA_OO_DEFINE_TAG(larry);
DARMA_OO_DEFINE_TAG(curly);
DARMA_OO_DEFINE_TAG(moe);

DARMA_OO_DEFINE_TAG(bart);
DARMA_OO_DEFINE_TAG(lisa);
DARMA_OO_DEFINE_TAG(marge);
DARMA_OO_DEFINE_TAG(homer);

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
        lisa
      >
    >
{ using darma_class::darma_class; };

template <>
struct Simple_method<bart>
  : darma_method<Simple,
      reads_<larry>,
      modifies_<curly>
    >
{
  using darma_method::darma_method;
  void bart() { }
};

template <>
struct Simple_method<lisa>
  : darma_method<Simple,
      reads_value_<moe>
    >
{
  using darma_method::darma_method;
  void lisa() {
    std::cout << moe << " == " << 42;
  }
};


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

  simple_oo_test::Simple s;
  s.curly = initial_access<std::string>("s curly");
  s.moe = initial_access<double>("s moe");
  //s.bart();
  //s.moe = 3.14;
  s.lisa();
}

