/*
//@HEADER
// ************************************************************************
//
//                     test_callable_traits.cc
//                         dharma
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

#include <string>
#include <type_traits> // std::is_integral

#include <gtest/gtest.h>

#include <tinympl/lambda.hpp>
#include <tinympl/bind.hpp>
#include <tinympl/find.hpp>
#include <tinympl/not_equal_to.hpp>
#include <tinympl/any_of.hpp>
#include <tinympl/vector.hpp>

#include <darma/impl/meta/callable_traits.h>
#include <tinympl/logical_not.hpp>
#include <tinympl/logical_or.hpp>

using namespace darma_runtime::meta;

#define meta_fail "callable traits metatest failed"

template <typename T>
using decay_is_integral = std::is_integral<std::decay_t<T>>;
template <typename T>
using deref_is_const = std::is_const<std::remove_reference_t<T>>;
template <typename T>
using deref_is_nonconst = std::integral_constant<bool, not std::is_const<std::remove_reference_t<T>>::value>;

template <typename Callable>
void test_it_1(Callable&& c) {
  using tinympl::placeholders::_;
  using tinympl::lambda;
  using tinympl::not_;
  using tinympl::or_;
  static_assert(callable_traits<Callable>::n_args_min == 8, meta_fail);
  //DARMA_TYPE_DISPLAY(typename callable_traits<Callable>::template arg_n<0>::type);
  static_assert(callable_traits<Callable>::template arg_n_matches<decay_is_integral, 0>::value, meta_fail);
  static_assert(callable_traits<Callable>::template arg_n_matches<decay_is_integral, 1>::value, meta_fail);
  static_assert(callable_traits<Callable>::template arg_n_matches<decay_is_integral, 2>::value, meta_fail);
  static_assert(callable_traits<Callable>::template arg_n_matches<decay_is_integral, 3>::value, meta_fail);
  static_assert(not callable_traits<Callable>::template arg_n_matches<decay_is_integral, 4>::value, meta_fail);
  static_assert(not callable_traits<Callable>::template arg_n_matches<decay_is_integral, 5>::value, meta_fail);
  static_assert(not callable_traits<Callable>::template arg_n_matches<decay_is_integral, 6>::value, meta_fail);
  static_assert(not callable_traits<Callable>::template arg_n_matches<decay_is_integral, 7>::value, meta_fail);
  static_assert(not callable_traits<Callable>::template arg_n_matches<decay_is_integral, 15>::value, meta_fail);
  static_assert(not callable_traits<Callable>::template all_args_match<decay_is_integral>::value, meta_fail);
  static_assert(callable_traits<Callable>::template all_args_match<
    lambda<
      or_<
        std::is_same<std::decay<_>, int>,
        std::is_same<std::decay<_>, std::string>
      >
    >::template apply_value
  >::value, meta_fail);
  static_assert(callable_traits<Callable>::template arg_n_matches<deref_is_const, 2>::value, meta_fail);
  static_assert(callable_traits<Callable>::template arg_n_matches<
    lambda< std::is_const< std::remove_reference<_> > >::template apply_value, 2
  >::value, meta_fail);
}

template <typename Callable>
void test_it_by_value(Callable&& c) {
  typedef callable_traits<Callable> traits;
  static_assert(traits::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(traits::template arg_n_is_by_reference<1>::value, meta_fail);
  static_assert(traits::template arg_n_is_by_reference<2>::value, meta_fail);
  // Ignore rvalue references for now
  // static_assert(traits::template arg_n_is_by_value<3>::value, meta_fail);
  static_assert(traits::template arg_n_is_by_value<4>::value, meta_fail);
  static_assert(traits::template arg_n_is_by_reference<5>::value, meta_fail);
  static_assert(traits::template arg_n_is_by_reference<6>::value, meta_fail);
  // Ignore rvalue references for now
  // static_assert(traits::template arg_n_is_by_value<Callable, 7>::value, meta_fail);
}

template <typename Callable>
void test_it_accepts_const_reference(Callable&& c) {
  typedef callable_traits<Callable> traits;
  static_assert(traits::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(not traits::template arg_n_accepts_const_reference<1>::value, meta_fail);
  static_assert(traits::template arg_n_accepts_const_reference<2>::value, meta_fail);
  // Ignore rvalue references for now
  // static_assert(traits::template arg_n_accepts_const_reference<3>::value, meta_fail);
  static_assert(traits::template arg_n_accepts_const_reference<4>::value, meta_fail);
  static_assert(not traits::template arg_n_accepts_const_reference<5>::value, meta_fail);
  static_assert(traits::template arg_n_accepts_const_reference<6>::value, meta_fail);
  // Ignore rvalue references for now
  // static_assert(traits::template arg_n_accepts_const_reference<7>::value, meta_fail);
}

template <typename Callable>
void test_it_is_nonconst_reference(Callable&& c) {
  typedef callable_traits<Callable> traits;
  static_assert(not traits::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(traits::template arg_n_is_nonconst_lvalue_reference<1>::value, meta_fail);
  static_assert(not traits::template arg_n_is_nonconst_lvalue_reference<2>::value, meta_fail);
  // Ignore rvalue references for now
  // static_assert(traits::template arg_n_is_nonconst_lvalue_reference<3>::value, meta_fail);
  static_assert(not traits::template arg_n_is_nonconst_lvalue_reference<4>::value, meta_fail);
  static_assert(traits::template arg_n_is_nonconst_lvalue_reference<5>::value, meta_fail);
  static_assert(not traits::template arg_n_is_nonconst_lvalue_reference<6>::value, meta_fail);
  // Ignore rvalue references for now
  // static_assert(traits::template arg_n_accepts_const_reference<7>::value, meta_fail);
}


void fun1(
  int, int&, int const&, int&&, std::string, std::string&, std::string const&, std::string&&
) { }

struct fun2 {
  void operator()(
    int, int &, int const &, int &&, std::string, std::string &, std::string const &, std::string &&
  ) const { }
};

// TODO re-enable some of these once we have them working with icc
TEST(TestCallableTraits, static1) {
  //test_it_1(fun2());
  test_it_1(fun1);
  //test_it_1([&](
  //  int, int&, int const&, int&&, std::string, std::string&, std::string const&, std::string&&
  //){ });
  test_it_by_value(fun1);
  //test_it_by_value(fun2());
  //test_it_by_value([&](
  //  int, int&, int const&, int&&, std::string, std::string&, std::string const&, std::string&&
  //){ });
  test_it_accepts_const_reference(fun1);
  //test_it_accepts_const_reference(fun2());
  //test_it_accepts_const_reference([&](
  //  int, int&, int const&, int&&, std::string, std::string&, std::string const&, std::string&&
  //){ });
  test_it_is_nonconst_reference(fun1);
  //test_it_is_nonconst_reference(fun2());
  //test_it_is_nonconst_reference([&](
  //  int, int&, int const&, int&&, std::string, std::string&, std::string const&, std::string&&
  //){ });
}


template <typename T>
struct F {
  static void by_value(T);
  static void lvalue_ref(T&);
  static void const_lvalue_ref(const T&);
  static void rvalue_ref(T&&);
};

struct SomeBitField {
  bool i : 1;
  uint8_t j : 6;
  bool k : 1;
};

static_assert(_callable_traits_impl::is_non_functor_callable<decltype(F<int>::by_value)>::value, meta_fail);

TEST(TestCallableTraits, static2) {
  // With int
  using T = int;
  static_assert(callable_traits<decltype(F<T>::by_value)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::by_value)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::by_value)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::by_value)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::by_value)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::by_value)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

  static_assert(not callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

  static_assert(not callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

  static_assert(not callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

}


TEST(TestCallableTraits, static3) {
  // With std::string

  using T = std::string;
  static_assert(callable_traits<decltype(F<T>::by_value)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::by_value)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::by_value)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::by_value)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::by_value)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::by_value)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

  static_assert(not callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

  static_assert(not callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

  static_assert(not callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

}

TEST(TestCallableTraits, static4) {
  using T = SomeBitField;
  static_assert(callable_traits<decltype(F<T>::by_value)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::by_value)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::by_value)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::by_value)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::by_value)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::by_value)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

  static_assert(not callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::lvalue_ref)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

  static_assert(not callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::const_lvalue_ref)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

  static_assert(not callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(F<T>::rvalue_ref)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

}


void with_ptr(int* val);
void with_ptr_ptr(int** val);
void with_ptr_ref(int*& val);
void with_ptr_const_ref(int* const& val);

TEST(TestCallableTraits, static5) {
  static_assert(callable_traits<decltype(with_ptr)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(with_ptr)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(with_ptr)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(with_ptr)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(with_ptr)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(with_ptr)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

  static_assert(callable_traits<decltype(with_ptr_ptr)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(with_ptr_ptr)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(with_ptr_ptr)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(with_ptr_ptr)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(with_ptr_ptr)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(with_ptr_ptr)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

  static_assert(not callable_traits<decltype(with_ptr_ref)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(callable_traits<decltype(with_ptr_ref)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(with_ptr_ref)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(with_ptr_ref)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(with_ptr_ref)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(with_ptr_ref)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);

  static_assert(not callable_traits<decltype(with_ptr_const_ref)>::template arg_n_is_by_value<0>::value, meta_fail);
  static_assert(callable_traits<decltype(with_ptr_const_ref)>::template arg_n_is_by_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(with_ptr_const_ref)>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(with_ptr_const_ref)>::template arg_n_accepts_const_reference<0>::value, meta_fail);
  static_assert(callable_traits<decltype(with_ptr_const_ref)>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
  static_assert(not callable_traits<decltype(with_ptr_const_ref)>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);
  //static_assert(not callable_traits<decltype(with_ptr_const_ref)>::is_templated, meta_fail);
}

// Not currently used
//struct Templated {
//  template <typename T>
//  void operator()(T&& v1) const;
//};
//struct Templated2 {
//  template <typename T, typename U, typename V, typename W>
//  void operator()(T const& t1, U u, V& v, W* const&) const;
//};
//struct NotTemplated {
//  void operator()(int&& v1) const;
//};
//
//TEST(TestCallableTraits, static_template) {
//  static_assert(callable_traits<Templated>::template arg_n_is_nonconst_rvalue_reference<0>::value, meta_fail);
//  static_assert(not callable_traits<Templated>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
//  static_assert(not callable_traits<Templated>::template arg_n_is_nonconst_lvalue_reference<0>::value, meta_fail);
//  static_assert(not callable_traits<Templated>::template arg_n_is_by_value<0>::value, meta_fail);
//  static_assert(callable_traits<Templated>::is_templated, meta_fail);
//
//  static_assert(callable_traits<Templated2>::template arg_n_is_const_lvalue_reference<0>::value, meta_fail);
//  static_assert(callable_traits<Templated2>::template arg_n_is_by_value<1>::value, meta_fail);
//  static_assert(callable_traits<Templated2>::template arg_n_is_nonconst_lvalue_reference<2>::value, meta_fail);
//  static_assert(callable_traits<Templated2>::template arg_n_is_const_lvalue_reference<3>::value, meta_fail);
//  static_assert(callable_traits<Templated2>::is_templated, meta_fail);
//
//  static_assert(not callable_traits<NotTemplated>::is_templated, meta_fail);
//
//}
