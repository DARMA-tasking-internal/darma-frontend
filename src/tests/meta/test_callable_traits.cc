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

using namespace darma_runtime::meta;

#define meta_assert(...) static_assert( \
  __VA_ARGS__, \
  "callable_traits metatest failed" \
);

template <typename Callable>
void test_it_1(Callable&& c) {
  meta_assert(callable_traits<Callable>::n_args_min == 8);
  meta_assert(callable_traits<Callable>::template arg_n_matches<std::is_integral, 0>::value);
  meta_assert(callable_traits<Callable>::template arg_n_matches<std::is_integral, 1>::value);
  meta_assert(callable_traits<Callable>::template arg_n_matches<std::is_integral, 2>::value);
  meta_assert(callable_traits<Callable>::template arg_n_matches<std::is_integral, 3>::value);
  meta_assert(not callable_traits<Callable>::template arg_n_matches<std::is_integral, 4>::value);
  meta_assert(not callable_traits<Callable>::template arg_n_matches<std::is_integral, 5>::value);
  meta_assert(not callable_traits<Callable>::template arg_n_matches<std::is_integral, 6>::value);
  meta_assert(not callable_traits<Callable>::template arg_n_matches<std::is_integral, 7>::value);
  meta_assert(not callable_traits<Callable>::template arg_n_matches<std::is_integral, 15>::value);
  meta_assert(not callable_traits<Callable>::template all_args_match<std::is_integral>::value);
  meta_assert(callable_traits<Callable>::template all_args_match<
    tinympl::lambda<
      tinympl::not_equal_to<
        tinympl::find<
          tinympl::vector<int, std::string>,
          std::decay_t<tinympl::placeholders::_>
        >::type,
        std::integral_constant<size_t, 2>
      >
    >::template apply_value
  >::value)
}


void fun1(
  int, int&, int const&, int&&, std::string, std::string&, std::string const&, std::string&&
) { }

struct fun2 {
  void operator()(
    int, int &, int const &, int &&, std::string, std::string &, std::string const &, std::string &&
  ) const { }
};

TEST(TestCallableTraits, static1) {
  test_it_1(fun1);
  test_it_1(fun2());
  test_it_1([&](
    int, int&, int const&, int&&, std::string, std::string&, std::string const&, std::string&&
  ){ });
}
