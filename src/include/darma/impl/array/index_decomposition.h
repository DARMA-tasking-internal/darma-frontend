/*
//@HEADER
// ************************************************************************
//
//                      index_decomposition.h
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

#ifndef DARMA_IMPL_ARRAY_INDEX_DECOMPOSITION_H
#define DARMA_IMPL_ARRAY_INDEX_DECOMPOSITION_H

#include <cstdlib>
#include <cassert>
#include <type_traits>
#include <iterator>

#include <darma/impl/meta/is_iterable.h>

namespace darma_runtime {

namespace detail {

template <typename T, typename Enable=void>
struct IndexDecomposition_enabled_if {
  // Trivial index decomposition case

  using subset_object_type = T;
  using element_type = T;

  // TODO this only works for copy-constructible types

  size_t n_elements(T const& obj) const {
    return 1;
  }

  T get_element(T& obj, size_t i) const {
    assert(i == 0);
    return obj;
  }

  T get_element_range(T const& obj, size_t offset, size_t size) const {
    assert(offset == 0 and size == 1);
    return obj;
  }

  void
  get_element_range(void* allocd, T& parent, size_t offset, size_t sz) const {
    assert(offset + sz < n_elements(parent));
    new (allocd) T(parent);
  }

  // TODO make this not a deep copy if possible
  // TODO generalization to types that don't deep copy their data
  bool
  is_deep_copy(T& obj) const {
    return true;
  }

  void
  set_element_range(T& obj, T const& data, size_t offset, size_t size) const {
    assert(offset == 0 and size == 1);
    obj = data;
  }

};

template <typename T>
struct IndexDecomposition_enabled_if<T,
  std::enable_if_t<
    meta::iterable_traits<T>::is_random_access_iterable
    and meta::iterable_traits<T>::has_iterator_constructor
  >
> {

  using subset_object_type = T;
  using element_type = typename meta::iterable_traits<T>::element_type;

  size_t n_elements(T const& obj) const {
    return end(obj) - begin(obj);
  }

  element_type&
  get_element(T& obj, size_t i) const {
    assert(i < n_elements(obj));
    return *(begin(obj) + i);
  }

  T
  get_element_range(T& obj, size_t offset, size_t size) const {
    assert(offset + size < n_elements(obj));
    return T(begin(obj) + offset, begin(obj) + offset + size);
  }

  void
  get_element_range(void* allocd, T& parent, size_t offset, size_t sz) const {
    assert(offset + sz < n_elements(parent));
    new (allocd) T(begin(parent) + offset, begin(parent) + offset + sz);
  }

  bool
  is_deep_copy(T& obj) const {
    return true;
  }

  void
  set_element_range(T& obj, T const& data, size_t offset, size_t size) const {
    assert(offset == 0 and size == 1);
    obj = data;
  }

};

} // end namespace detail

template <typename T>
struct IndexDecomposition
  : detail::IndexDecomposition_enabled_if<T, void>
{ };

} // end namespace darma_runtime

#endif //DARMA_IMPL_ARRAY_INDEX_DECOMPOSITION_H
