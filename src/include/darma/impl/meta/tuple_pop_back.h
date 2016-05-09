/*
//@HEADER
// ************************************************************************
//
//                       tuple_pop_back.hpp
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

#ifndef DARMA_IMPL_META_TUPLE_POP_BACK_H
#define DARMA_IMPL_META_TUPLE_POP_BACK_H

#include <utility>
#include <tuple>
#include <tinympl/at.hpp>
#include <tinympl/insert.hpp>
#include <tinympl/pop_back.hpp>
#include <tinympl/tuple_as_sequence.hpp>
#include <tinympl/splat.hpp>
#include "splat_tuple.h"

namespace darma_runtime {

namespace meta {

namespace m = tinympl;
namespace mv = tinympl::variadic;

//namespace _impl {
//
//template <template <class...> class copy_properties>
//struct _wrapper {
//  template <typename Back, typename... Args>
//  struct _tuple_pop_back_helper {
//    constexpr inline decltype(auto)
//    operator()(typename copy_properties<Args>::type &&... args, typename copy_properties<Back>::type &&) const {
//      return std::forward_as_tuple(std::forward<Args>(args)...);
//    }
//  };
//};

//} // end namespace _impl

//template <typename Tuple>
//decltype(auto)
//tuple_pop_back(Tuple&& tup) {
//  typedef typename m::copy_all_type_properties<Tuple>::template apply<
//    typename m::push_front<
//      typename m::pop_back<std::decay_t<Tuple>>::type,
//      typename m::back<std::decay_t<Tuple>>::type
//    >::type
//  >::type new_tuple_t;
//  typedef typename m::splat_to<new_tuple_t, _impl::_wrapper<
//      m::copy_all_type_properties<Tuple>::template apply
//    >::template _tuple_pop_back_helper>::type helper_t;
//  return splat_tuple(
//    std::forward<Tuple>(tup),
//    helper_t()
//  );
//}

namespace _tuple_pop_back_impl {

template <size_t I, size_t N, typename Enable=void>
struct _impl {
  template <typename Tuple, typename... Args>
  auto operator()(Tuple&& tup, Args&&... args) const {
    return _impl<I + 1, N>()(
      std::forward<Tuple>(tup), std::forward<Args>(args)...,
      std::get<I>(std::forward<Tuple>(tup))
    );
  }
};

template <size_t I, size_t N>
struct _impl<I, N, std::enable_if_t<I+1 == N>> {
  template <typename Tuple, typename... Args>
  auto operator()(Tuple&&, Args&&... args) const {
    return std::forward_as_tuple(std::forward<Args>(args)...);
  }
};

template <size_t N>
struct _impl<N, N>;

} // end namespace impl

template <typename Tuple>
auto
tuple_pop_back(Tuple&& tup) {
  return _tuple_pop_back_impl::_impl<0,
    std::tuple_size<std::remove_reference_t<Tuple>>::value
  >()(std::forward<Tuple>(tup));
}


} // end namespace meta

} // end namespace darma_runtime


#endif //DARMA_IMPL_META_TUPLE_POP_BACK_H
