/*
//@HEADER
// ************************************************************************
//
//                          assert.h
//                         darma_new
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

#ifndef SRC_DARMA_ASSERT_H_
#define SRC_DARMA_ASSERT_H_

#include <cassert>
#include <iostream>

namespace darma_runtime {

namespace detail {

template <typename T>
struct value_dumper {
  void
  operator()(T const& val, std::ostream& o) {
    o << val;
  }
};

template <typename T, typename U>
struct value_dumper<std::pair<T, U>> {
  void
  operator()(std::pair<T, U> const& val, std::ostream& o) {
    o << "{ " << val.first << ", " << val.second << "}";
  }
};

template <typename Container, typename ValueDumper = value_dumper<typename Container::value_type>>
struct container_dumper {
  void
  operator()(Container const& c, std::ostream& o = std::cout, const char* indent = "    ") {
    o << indent << "{" << std::endl;
    ValueDumper vd;
    for(auto&& val : c) {
      o << indent << "  ";
      vd(val, o);
      o << std::endl;
    }
    o << indent << "}";
  }
};

template <typename Key, typename Container>
bool _check_contains(const Key& k, const Container& c) {
  return c.find(k) != c.end();
}


} // end namespace detail

} // end namespace darma_runtime


#ifndef DARMA_ASSERTION_BEGIN
#define DARMA_ASSERTION_BEGIN std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl
#endif
#ifndef DARMA_ASSERTION_END
#define DARMA_ASSERTION_END std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl
#endif

#define DARMA_ASSERT_RELATED_VERBOSE(lhs, op, rhs) assert(                                         \
  (lhs op rhs) ||                                                                                   \
  ((                                                                                                \
    DARMA_ASSERTION_BEGIN,                                                             \
    std::cerr << "DARMA assertion failed:" << std::endl                                            \
              << "  " << #lhs << " " << #op << " " << #rhs << std::endl                             \
              << "  " << lhs << " " << #op << " " << rhs << std::endl,                              \
    DARMA_ASSERTION_END                                                               \
  ), false)                                                                                         \
)

#define DARMA_ASSERT_NOT_NULL_VERBOSE(...) assert(                                                  \
  ((__VA_ARGS__) != nullptr) ||                                                                     \
  ((                                                                                                \
    DARMA_ASSERTION_BEGIN,                                                             \
    std::cerr << "DARMA assertion failed:" << std::endl                                             \
              << "  Expression was null that should be non-null:"                                   \
              << "    " << #__VA_ARGS__ << std::endl                                                \
              << "    (evalutated as true for == with nullptr)" << std::endl,                                     \
    DARMA_ASSERTION_END                                                               \
  ), false)                                                                                         \
)

#define DARMA_ASSERT_EQUAL_VERBOSE(lhs, rhs) DARMA_ASSERT_RELATED_VERBOSE(lhs, ==, rhs)

#define DARMA_ASSERT_EQUAL(lhs, rhs) DARMA_ASSERT_EQUAL_VERBOSE(lhs, rhs)

#define DARMA_ASSERT_NOT_NULL(...) DARMA_ASSERT_NOT_NULL_VERBOSE(__VA_ARGS__)


#define DARMA_ASSERT_MESSAGE(expr, ...) assert(                                                     \
  (expr) ||                                                                                         \
  ((                                                                                                \
    DARMA_ASSERTION_BEGIN,                                                              \
    std::cerr << "DARMA assertion failed:" << std::endl                                             \
              << "  Expression should be true: " << #expr << std::endl                              \
              << "  Assertion details:" << std::endl                                                \
              << "    File: " << __FILE__ << std::endl                                                \
              << "    Line: " << __LINE__ << std::endl                                                \
              << "    " << __VA_ARGS__ << std::endl,                                                 \
    DARMA_ASSERTION_END                                                               \
  ), false)                                                                                         \
)

#define DARMA_ASSERT_FAILURE(...) DARMA_ASSERT_MESSAGE( \
  false, __VA_ARGS__ \
)

#define DARMA_ASSERT_UNREACHABLE_FAILURE(...) DARMA_ASSERT_FAILURE( \
  "DARMA encountered a failing piece of code that should be unreachable," \
  << " most likely because an earlier error should have been thrown for the same reason.  " \
  __VA_ARGS__ \
)

#define DARMA_ASSERT_NOT_IMPLEMENTED(...) DARMA_ASSERT_FAILURE( \
  "DARMA encountered a branch or feature that is not implemented in the current version" \
  << " of the translation layer.  " \
  __VA_ARGS__ \
)

#endif /* SRC_DARMA_ASSERT_H_ */
