/*
//@HEADER
// ************************************************************************
//
//                      tuple_zip.h
//                         DARMA
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

#ifndef DARMA_IMPL_META_TUPLE_ZIP_H_
#define DARMA_IMPL_META_TUPLE_ZIP_H_

#include <tinympl/zip.hpp>
#include <tinympl/tuple_as_sequence.hpp>
#include <tinympl/min_element.hpp>

namespace darma_runtime {

namespace meta {

namespace _impl {


template <size_t I, size_t N, typename... Tuples>
struct tuple_zip_helper {
  typedef tuple_zip_helper<I+1, N, Tuples...> next_helper_t;
  inline constexpr auto
  operator()(Tuples&&... tuples) const {
    return std::tuple_cat(
      std::forward_as_tuple(
        std::forward_as_tuple(std::get<I>(std::forward<Tuples>(tuples))...)
      ),
      next_helper_t()(std::forward<Tuples>(tuples)...)
    );
  }
};

template <size_t N, typename... Tuples>
struct tuple_zip_helper<N, N, Tuples...> {
  inline constexpr auto
  operator()(Tuples&&...) const {
    return std::forward_as_tuple();
  }
};

} // end namespace _impl

template <typename... Tuples>
auto
tuple_zip(Tuples&&... tuples) {
  static constexpr size_t min_size =
    tinympl::min<std::tuple_size<Tuples>...>::value;
  typedef _impl::tuple_zip_helper<0, min_size, Tuples...> helper_t;

  return helper_t()(std::forward<Tuples>(tuples)...);
}

} // end namespace meta

} // end namespace darma_runtime


#endif //DARMA_IMPL_META_TUPLE_ZIP_H_
