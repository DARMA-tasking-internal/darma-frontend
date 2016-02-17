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

#include <tinympl/tuple_as_sequence.hpp>
#include <tinympl/at.hpp>
#include <tinympl/size.hpp>

namespace darma_runtime { namespace meta {

namespace _splat_tuple_impl {

namespace m = tinympl;
namespace mv = tinympl::variadic;

template <size_t Spot, size_t Size, typename Callable, typename Tuple, typename... Args>
struct helper;

template <size_t Spot, size_t Size, typename Callable, typename... Ts, typename... Args>
struct helper<Spot, Size, Callable, std::tuple<Ts...>, Args...> {
  private:
    typedef helper<
        Spot+1, Size, Callable, std::tuple<Ts...>,
        Args..., typename mv::at<Spot, Ts...>::type
    > _next_t;
  public:
    typedef decltype(std::declval<Callable>()(std::declval<Ts>()...)) return_t;
    static_assert(std::is_same<return_t, typename _next_t::return_t>::value, "return type mismatch");

    template <typename ForwardedTuple, typename... ForwardedArgs>
    inline constexpr return_t
    operator()(Callable&& callable, ForwardedTuple&& ftup, ForwardedArgs&&... args) const {
      return _next_t()(
        std::forward<Callable>(callable),
        std::forward<ForwardedTuple>(ftup),
        std::forward<ForwardedArgs>(args)...,
        std::get<Spot>(std::forward<ForwardedTuple>(ftup))
      );
    }
};

template <size_t Size, typename Callable, typename... Ts, typename... Args>
struct helper<Size, Size, Callable, std::tuple<Ts...>, Args...> {
  typedef decltype(std::declval<Callable>()(std::declval<Args>()...)) return_t;

  template <typename ForwardedTuple, typename... ForwardedArgs>
  inline constexpr return_t
  operator()(Callable&& callable, ForwardedTuple&& ftup, ForwardedArgs&&... args) const {
    static_assert(
      std::is_same<return_t, decltype(callable(std::forward<ForwardedArgs>(args)...))>::value,
      "return type mismatch"
    );
    return callable(std::forward<ForwardedArgs>(args)...);
  }

};


} // end namespace _splat_tuple_impl

template <typename Callable, typename Tuple>
typename _splat_tuple_impl::helper<0, tinympl::size<Tuple>::value, Callable, std::decay_t<Tuple>>::return_t
splat_tuple(Tuple&& tuple, Callable&& callable) {
  return _splat_tuple_impl::helper<0, tinympl::size<Tuple>::value, Callable, std::decay_t<Tuple>>()(
    std::forward<Callable>(callable),
    std::forward<Tuple>(tuple)
  );
}


}} // end namespace darma_runtime::meta



#endif /* SRC_DARMA_META_SPLAT_TUPLE_H_ */
