/*
//@HEADER
// ************************************************************************
//
//                      constexpr_if_else.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_IMPL_UTIL_CONSTEXPR_IF_H
#define DARMAFRONTEND_IMPL_UTIL_CONSTEXPR_IF_H

#include <type_traits>
#include "darma/utility/not_a_type.h"

namespace darma {
namespace utility {

namespace _impl {

template <typename T>
struct _constexpr_if_impl;

template <>
struct _constexpr_if_impl<std::true_type>
{
  template <
    typename GenericCallableIfTrue,
    typename GenericCallableIfFalse,
    typename... Args
  >
  decltype(auto)
  operator()(
    GenericCallableIfTrue&& if_true,
    GenericCallableIfFalse&&,
    Args&&... args
  ) const {
    return std::forward<GenericCallableIfTrue>(if_true)(
      std::forward<Args>(args)...
    );
  }
};

template <>
struct _constexpr_if_impl<std::false_type> {
  template <
    typename GenericCallableIfTrue,
    typename GenericCallableIfFalse,
    typename... Args
  >
  decltype(auto)
  operator()(
    GenericCallableIfTrue&&,
    GenericCallableIfFalse&& if_false,
    Args&&... args
  ) const {
    return std::forward<GenericCallableIfFalse>(if_false)(
      std::forward<Args>(args)...
    );
  }

};

} // end namespace _impl

template <bool Condition,
  typename GenericCallableIfTrue,
  typename GenericCallableIfFalse,
  typename... Args
>
decltype(auto)
constexpr_if_else(
  GenericCallableIfTrue&& if_true,
  GenericCallableIfFalse&& if_false,
  Args&&... args
) {
  _impl::_constexpr_if_impl<std::integral_constant<bool, Condition>>{}(
    std::forward<GenericCallableIfTrue>(if_true),
    std::forward<GenericCallableIfFalse>(if_false),
    std::forward<Args>(args)...
  );
};

template <bool Condition,
  typename GenericCallableIfTrue,
  typename... Args
>
decltype(auto)
darma_constexpr_if(
  GenericCallableIfTrue&& if_true,
  Args&&... args
) {
  return utility::constexpr_if_else<Condition>(
    std::forward<GenericCallableIfTrue>(if_true),
    [](Args&&... args) { },
    std::forward<Args>(args)...
  );
};

} // end namespace utility
} // end namespace darma

#endif //DARMAFRONTEND_IMPL_UTIL_CONSTEXPR_IF_H
