/*
//@HEADER
// ************************************************************************
//
//                          splat_tuple.h
//                         dharma_new
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

#ifndef SRC_DARMA_META_SPLAT_TUPLE_H_
#define SRC_DARMA_META_SPLAT_TUPLE_H_

#include <type_traits>
#include <tuple>

#include <tinympl/tuple_as_sequence.hpp>
#include <tinympl/at.hpp>
#include <tinympl/size.hpp>

namespace darma_runtime { namespace meta {

// Attorney pattern for splatted callables
template <typename To>
struct splat_tuple_access {
  template <typename Callable, typename... Args>
  inline constexpr decltype(auto)
  operator()(Callable&& callable, Args&&... args) const {
    return std::forward<Callable>(callable)(std::forward<Args>(args)...);
  }
};

namespace _splat_tuple_impl {

template <typename AccessTo, size_t... Is, typename Tuple, typename Callable>
constexpr decltype(auto)
_helper(std::index_sequence<Is...>, Tuple&& tup, Callable&& callable) {
  return splat_tuple_access<AccessTo>()(
    std::forward<Callable>(callable),
    std::get<Is>(std::forward<Tuple>(tup))...
  );
};

} // end namespace _splat_tuple_impl

template <typename AccessTo=void, typename Callable, typename Tuple>
inline decltype(auto)
splat_tuple(Tuple&& tup, Callable&& callable) {
  return _splat_tuple_impl::_helper<AccessTo>(
    std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>(),
    std::forward<Tuple>(tup), std::forward<Callable>(callable)
  );
}


}} // end namespace darma_runtime::meta



#endif /* SRC_DARMA_META_SPLAT_TUPLE_H_ */
