/*
//@HEADER
// ************************************************************************
//
//                      make_flattened_tuple.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMA_IMPL_META_MAKE_FLATTENED_TUPLE_H
#define DARMA_IMPL_META_MAKE_FLATTENED_TUPLE_H

#include <tuple>

namespace darma_runtime {
namespace meta {

namespace detail {

template <typename Arg>
decltype(auto)
_make_flattened_tuple_helper(
  Arg&& arg
) {
  return std::make_tuple(
    std::forward<Arg>(arg)
  );
}

template <typename Arg>
decltype(auto)
_make_flattened_tuple_helper(
  std::tuple<Arg>&& arg
) {
  return std::move(arg);
}

template <typename Arg>
decltype(auto)
_make_flattened_tuple_helper(
  std::tuple<Arg>& arg
) {
  return arg;
}

template <typename Arg>
decltype(auto)
_make_flattened_tuple_helper(
  std::tuple<Arg> const& arg
) {
  return arg;
}

} // end namespace detail

template <typename... Args>
auto
make_flattened_tuple(
  Args&&... args
) {
  return std::tuple_cat(
    detail::_make_flattened_tuple_helper(
      std::forward<Args>(args)
    )...
  );
}


} // end namespace meta
} // end namespace darma_runtime

#endif //DARMA_IMPL_META_MAKE_FLATTENED_TUPLE_H
