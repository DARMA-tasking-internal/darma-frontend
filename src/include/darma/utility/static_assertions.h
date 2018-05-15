/*
//@HEADER
// ************************************************************************
//
//                      static_assertions.h
//                         DARMA
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

#ifndef DARMA_STATIC_ASSERTIONS_H
#define DARMA_STATIC_ASSERTIONS_H

#include <cstdlib>

#include <darma/utility/macros.h>

struct ___________________________expected__________________________________ {};
struct ____________________________size_of__________________________________ {};
struct ___________________________which_is__________________________________ {};
struct ____________________to_be_the_same_type_as___________________________ {};
struct _______________________to_be_the_equal_to____________________________ {};
struct _________________________to_be_less_than_____________________________ {};
struct __________________to_be_less_than_or_equal_to________________________ {};
struct _______________________to_be_greater_than____________________________ {};
struct _________________to_be_greater_than_or_equal_to______________________ {};
struct _____________________________________________________________________ {};
template <std::size_t size>
struct ____size_t_value____ {};
template <typename T, T value>
struct ____constant_value____ {};

template <typename... Args>
struct _darma__static_failure;

//==============================================================================
// <editor-fold desc="static_assert_type_eq">

template <typename T1, typename T2>
struct static_assert_type_eq_ {
  typename _darma__static_failure<
    ___________________________expected__________________________________,
    T1,
    ____________________to_be_the_same_type_as___________________________,
    T2,
    _____________________________________________________________________
  >::type __failed__;
};

template <typename T1>
struct static_assert_type_eq_<T1, T1> {
  enum {
    value = true
  };
};

template <typename T1, typename T2>
bool static_assert_type_eq() {
  (void)static_assert_type_eq_<T1, T2>();
  return true;
}

#define STATIC_ASSERT_TYPE_EQ(...) \
  static constexpr int DARMA_CONCAT_TOKEN_(_type_eq_check_on_line_, __LINE__) = \
    static_assert_type_eq_<__VA_ARGS__>::value;

// </editor-fold> end static_assert_type_eq
//==============================================================================

//==============================================================================
// <editor-fold desc="static_assert_size_is">


template <typename T1, std::size_t size, typename Enable=void>
struct static_assert_size_is_ {
  typename _darma__static_failure<
    ___________________________expected__________________________________,
    ____________________________size_of__________________________________,
    T1,
    ___________________________which_is__________________________________,
    ____size_t_value____<sizeof(T1)>,
    _______________________to_be_the_equal_to____________________________,
    ____size_t_value____<size>,
    _____________________________________________________________________
  >::type __failed__;
};

template <typename T1, std::size_t size>
struct static_assert_size_is_<T1, size, std::enable_if_t<sizeof(T1) == size>> {
  enum {
    value = true
  };
};

#define STATIC_ASSERT_SIZE_IS(...) \
  static constexpr int DARMA_CONCAT_TOKEN_(_type_eq_check_on_line_, __LINE__) = \
    static_assert_size_is_<__VA_ARGS__>::value;

// </editor-fold> end static_assert_size_is
//==============================================================================


//==============================================================================
// <editor-fold desc="static_assert_value_less">

template <typename T, T v1, T v2, typename Enable=void>
struct static_assert_value_less_ {
  typename _darma__static_failure<
    ___________________________expected__________________________________,
    ____constant_value____<T, v1>,
    _________________________to_be_less_than_____________________________,
    ____constant_value____<T, v2>,
    _____________________________________________________________________
  >::type __failed__;
};

template <typename T, T v1, T v2>
struct static_assert_value_less_<T, v1, v2, std::enable_if_t<(v1 < v2)>> {
  enum {
    value = true
  };
};

namespace darma {
namespace utility {
namespace _impl {

template <typename T>
T _get_constant_type(T, T);

template <typename T, typename U>
typename std::common_type<T, U>::type _get_constant_type(T, U);

} // end namespace _impl
} // end namespace utility
} // end namespace darma

#define STATIC_ASSERT_VALUE_LESS(...) \
  static constexpr int DARMA_CONCAT_TOKEN_(_type_eq_check_on_line_, __LINE__) = \
    static_assert_value_less_< \
      decltype(::darma::utility::_impl::_get_constant_type(__VA_ARGS__)), \
      __VA_ARGS__ \
    >::value

// </editor-fold> end static_assert_value_less
//==============================================================================

//==============================================================================
// <editor-fold desc="static_assert_value_less">

template <typename T, T v1, T v2, typename Enable=void>
struct static_assert_value_equal_ {
  typename _darma__static_failure<
    ___________________________expected__________________________________,
    ____constant_value____<T, v1>,
    _______________________to_be_the_equal_to____________________________,
    ____constant_value____<T, v2>,
    _____________________________________________________________________
  >::type __failed__;
};

template <typename T, T v1, T v2>
struct static_assert_value_equal_<T, v1, v2, std::enable_if_t<(v1 == v2)>> {
  enum {
    value = true
  };
};

#define STATIC_ASSERT_VALUE_EQUAL(...) \
  static constexpr int DARMA_CONCAT_TOKEN_(_type_eq_check_on_line_, __LINE__) = \
    static_assert_value_equal_< \
      decltype(::darma::utility::_impl::_get_constant_type(__VA_ARGS__)), \
      __VA_ARGS__ \
    >::value

// </editor-fold> end static_assert_value_less
//==============================================================================


//==============================================================================
// <editor-fold desc="static_assert_value_less_equal">

template <typename T, T v1, T v2, typename Enable=void>
struct static_assert_value_less_equal_ {
  typename _darma__static_failure<
    ___________________________expected__________________________________,
    ____constant_value____<T, v1>,
    __________________to_be_less_than_or_equal_to________________________,
    ____constant_value____<T, v2>,
    _____________________________________________________________________
  >::type __failed__;
};

template <typename T, T v1, T v2>
struct static_assert_value_less_equal_<T, v1, v2, std::enable_if_t<(v1 <= v2)>> {
  enum {
    value = true
  };
};

#define STATIC_ASSERT_VALUE_LESS_EQUAL(...) \
  static constexpr int DARMA_CONCAT_TOKEN_(_type_eq_check_on_line_, __LINE__) = \
    static_assert_value_less_equal_< \
      decltype(::darma::utility::_impl::_get_constant_type(__VA_ARGS__)), \
      __VA_ARGS__ \
    >::value

// </editor-fold> end static_assert_value_less
//==============================================================================


#endif //DARMA_STATIC_ASSERTIONS_H
