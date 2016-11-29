/*
//@HEADER
// ************************************************************************
//
//                      mapping.h
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

#ifndef DARMA_IMPL_INDEX_RANGE_MAPPING_H
#define DARMA_IMPL_INDEX_RANGE_MAPPING_H

#include <utility> // std::declval

#include <tinympl/variadic/at.hpp>

#include <darma/impl/meta/detection.h>

namespace darma_runtime {

namespace detail {

namespace _impl {

template <typename IndexRangeT>
using _has_mapping_to_dense_archetype = decltype(
  get_mapping_to_dense(std::declval<IndexRangeT>())
);

} // end namespace _impl

template <typename T>
using has_mapping_to_dense = meta::is_detected<
  _impl::_has_mapping_to_dense_archetype, T
>;

template <typename IndexRangeT>
using mapping_to_dense_t = meta::detected_t<
  _impl::_has_mapping_to_dense_archetype, IndexRangeT
>;





} // end namespace detail

template <typename... Mappings>
struct CompositeMapping;

template <typename Mapping1, typename Mapping2>
struct CompositeMapping<Mapping1, Mapping2> {

  private:

    Mapping1 m1_;
    Mapping2 m2_;


  public:

    template <
      typename _IgnoredSFINAE = void,
      typename = std::enable_if_t<
        std::is_void<_IgnoredSFINAE>::value
          and std::is_default_constructible<Mapping1>::value
          and std::is_default_constructible<Mapping2>::value
      >
    >
    CompositeMapping()
      : m1_(), m2_()
    { }

    template <
      typename Mapping1Convertible,
      typename Mapping2Convertible
    >
    CompositeMapping(
      Mapping1Convertible&& m1,
      Mapping2Convertible&& m2
    ) : m1_(std::forward<Mapping1Convertible>(m1)),
        m2_(std::forward<Mapping2Convertible>(m2))
    { }


    // TODO other ctors, including pass-through


    using from_index_t = typename Mapping1::from_index_t;
    using to_index_t = typename Mapping2::to_index_t;
    using is_index_mapping_t = std::true_type;

    to_index_t map_forward(from_index_t const& from) const {
      return m2_.map_forward(m1_.map_forward(from));
    }

    from_index_t map_backward(to_index_t const& to) const {
      return m1_.map_backward(m2_.map_backward(to));
    }
};

template <typename Mapping1, typename Mapping2, typename Mapping3, typename... Mappings>
struct CompositeMapping<Mapping1, Mapping2, Mapping3, Mappings...>
  : CompositeMapping<Mapping1,
    CompositeMapping<Mapping2, Mapping3, Mappings...>
  >
{ };

template <typename Mapping>
struct ReverseMapping {
  private:

    Mapping m1_;

  public:

    template <typename... Args,
      typename=std::enable_if_t<
        not std::is_same<std::decay_t<
          tinympl::variadic::at_or_t<meta::nonesuch, 0, Args...>
        >, ReverseMapping>::value
      >
    >
    ReverseMapping(Args&&... args)
      : m1_(std::forward<Args>(args)...)
    { }

    using from_index_t = typename Mapping::to_index_t;
    using to_index_t = typename Mapping::from_index_t;
    using is_index_mapping_t = std::true_type;

    to_index_t map_forward(from_index_t const& from) const {
      return m1_.map_backward(from);
    }

    from_index_t map_backward(to_index_t const& to) const {
      return m1_.map_forward(to);
    }

};

template <typename IndexT>
struct IdentityMapping {

  public:

    using from_index_t = IndexT;
    using to_index_t = IndexT;
    using is_index_mapping_t = std::true_type;

    to_index_t map_forward(from_index_t const& from) const {
      return from;
    }

    from_index_t map_backward(to_index_t const& to) const {
      return to;
    }
};

} // end namespace darma_runtime

#endif //DARMA_IMPL_INDEX_RANGE_MAPPING_H
