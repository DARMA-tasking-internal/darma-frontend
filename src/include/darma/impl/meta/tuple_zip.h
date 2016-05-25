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

#include <algorithm>

#include <tinympl/zip.hpp>
#include <tinympl/tuple_as_sequence.hpp>
#include <tinympl/min_element.hpp>

namespace darma_runtime {

namespace meta {

namespace _impl {

// Credit for this solution that works around libc++ bug #22806 goes to
// the author of the answer to http://stackoverflow.com/a/37259996/152060

template<size_t I, typename... Tuples>
constexpr auto
tuple_zip_one(Tuples&&... tuples) {
  return std::forward_as_tuple(
    std::get<I>(std::forward<Tuples>(tuples))...
  );
}

template<size_t...Is, typename... Tuples>
constexpr auto
tuple_zip_helper(std::index_sequence<Is...>, Tuples&&... tuples) {
  return std::make_tuple(
    tuple_zip_one<Is>( std::forward<Tuples>(tuples)... )...
  );
}

} // end namespace _impl


template <typename... Tuples>
auto tuple_zip(Tuples&&... tuples) {
  //static constexpr size_t min_size = tinympl::min<
  //  std::tuple_size<std::decay_t<Tuples>>...
  //>::value;
  static constexpr size_t min_size = std::min({
    std::tuple_size<std::decay_t<Tuples>>::value...
  });
  return _impl::tuple_zip_helper(
    std::make_index_sequence<min_size>(),
    std::forward<Tuples>(tuples)...
  );
}

} // end namespace meta

} // end namespace darma_runtime


#endif //DARMA_IMPL_META_TUPLE_ZIP_H_
