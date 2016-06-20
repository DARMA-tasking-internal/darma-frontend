/*
//@HEADER
// ************************************************************************
//
//                       metatest_helpers.h
//                         darma_mockup
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

#ifndef META_TINYMPL_TEST_METATEST_HELPERS_H_
#define META_TINYMPL_TEST_METATEST_HELPERS_H_

#define meta_assert(...) \
  static_assert(__VA_ARGS__, \
      "Metafunction test case failed, expression did not evaluate to true:\n   " #__VA_ARGS__ "\n" \
  )
#define meta_assert_same(...) \
  static_assert(std::is_same<__VA_ARGS__>::value, \
      "Metafunction test case failed, types are not the same:\n    " #__VA_ARGS__ "\n" \
  )

#include <tuple>

namespace tinympl { namespace test {

// Utility types for ensuring short-circuit actually happens

template <typename... T>
struct invalid_for_int {
  typedef typename std::tuple_element<0, std::tuple<T...>>::type type;
};

template <typename... T>
struct invalid_for_int<int, T...>;

template <typename... T>
struct never_evaluate_predicate
{
  typedef typename invalid_for_int<int, T...>::type type;
  static constexpr bool value = type::value; // shouldn't be usable
};

}} // end namespace tinympl::test

struct ___________________________expected__________________________________ {};
struct ____________________to_be_the_same_type_as___________________________ {};
struct _____________________________________________________________________ {};

template <typename... Args>
struct static_failure;

template <typename T1, typename T2>
struct static_assert_type_eq_ {
  typename static_failure<
    ___________________________expected__________________________________,
    T1,
    ____________________to_be_the_same_type_as___________________________,
    T2,
    _____________________________________________________________________
  >::type __failed__;
};

template <typename T1>
struct static_assert_type_eq_<T1, T1> { using type = void; };

template <typename T1, typename T2>
bool static_assert_type_eq() {
  (void)typename static_assert_type_eq_<T1, T2>::type();
  return true;
}


#endif /* META_TINYMPL_TEST_METATEST_HELPERS_H_ */
