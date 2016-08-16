/*
//@HEADER
// ************************************************************************
//
//                      stl_vector.h
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

#ifndef DARMA_IMPL_ARRAY_STL_VECTOR_H
#define DARMA_IMPL_ARRAY_STL_VECTOR_H

#include <vector>

#include "index_decomposition.h"
#include <darma/interface/app/vector_view.h>


namespace darma_runtime {


namespace detail {

template <
  typename vector_t,
  typename subset_object_type,
  typename element_type
>
struct vector_index_decomposition_base {

  size_t n_elements(vector_t const& obj) const {
    return obj.size();
  }

  element_type&
  get_element(vector_t const& obj, size_t i) const {
    return obj[i];
  }

  subset_object_type
  get_element_range(vector_t& obj, size_t offset, size_t size) const {
    DARMA_ASSERT_RELATED_VERBOSE(offset + size, <=, n_elements(obj));
    return subset_object_type(obj, offset, size);
  }

  subset_object_type
  get_element_range(
    void* allocd, vector_t& parent, size_t offset, size_t sz
  ) const {
    DARMA_ASSERT_RELATED_VERBOSE(offset + sz, <=, n_elements(parent));
    new (allocd) subset_object_type(parent, offset, sz);
  }

  bool
  subset_is_deep_copy(vector_t& obj) const {
    return false;
  }

  void
  set_element_range(
    vector_t& obj,
    subset_object_type const& data, size_t offset, size_t size
  ) const {
    DARMA_ASSERT_EQUAL(data.size(), size);
    DARMA_ASSERT_RELATED_VERBOSE(obj.size(), >=, offset + size);
    if(not (
      obj.begin() + offset == data.begin()
        and obj.begin() + offset + size == data.end()
    )) {
      auto change_spot = obj.begin() + offset;
      std::copy(data.begin(), data.end(), change_spot);
    }
    // otherwise, do nothing, since it's a view of the object
  }

  void
  set_element_range(
    vector_t& obj, vector_t const& data, size_t offset, size_t size
  ) const {
    DARMA_ASSERT_EQUAL(data.size(), size);
    DARMA_ASSERT_RELATED_VERBOSE(obj.size(), >=, offset + size);
    auto change_spot = obj.begin() + offset;
    std::copy(data.begin(), data.end(), change_spot);
  }

};

} // end namespace detail


template <typename T, typename Allocator>
class IndexDecomposition<std::vector<T, Allocator>>
  : public detail::vector_index_decomposition_base<
      std::vector<T, Allocator>,
      darma::vector_view<T>,
      T
    >
{

  private:

    using vector_t = std::vector<T, Allocator>;

  public:

    using subset_object_type = darma::vector_view<T>;
    using element_type = T;

};

template <typename T, typename Allocator>
class IndexDecomposition<std::vector<T, Allocator> const>
  : public detail::vector_index_decomposition_base<
      std::vector<T, Allocator> const,
      darma::vector_view<const T>,
      const T
    >
{

};


} // end namespace darma_runtime

#endif //DARMA_IMPL_ARRAY_STL_VECTOR_H
