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
#include <darma/impl/serialization/as_pod.h>
#include "mapping_traits.h"

namespace darma_runtime {

namespace detail {

struct not_an_index_range { };

namespace _impl {

struct _not_a_type { };
struct _not_a_type_1 { };
struct _not_a_type_2 { };

} // end namespace _impl


} // end namespace detail

template <
  typename Mapping1, typename Mapping2,
  typename M1ToRange=detail::not_an_index_range,
  typename M2FromRange=detail::not_an_index_range
>
struct CompositeMapping {

  private:

    // TODO make all members into a compressed_pair (/compressed_tuple?)
    // (since they have high probability of being empty)

    Mapping1 m1_;
    Mapping2 m2_;


    M1ToRange m1_to_range_;
    M2FromRange m2_from_range_;


    template <
      typename... Args1,
      typename... Args2,
      size_t... Idxs1,
      size_t... Idxs2
    >
    CompositeMapping(
      std::piecewise_construct_t,
      std::tuple<Args1...>& m1args,
      std::tuple<Args2...>& m2args,
      std::integer_sequence<std::size_t, Idxs1...>,
      std::integer_sequence<std::size_t, Idxs2...>
    ) : m1_(std::forward<Args1>(std::get<Idxs1>(m1args))...),
        m2_(std::forward<Args2>(std::get<Idxs2>(m2args))...)
    { }

    using _traits_1 = indexing::mapping_traits<Mapping1>;
    using _traits_2 = indexing::mapping_traits<Mapping2>;
    using _serdes_traits_1 = serialization::detail::serializability_traits<Mapping1>;
    using _serdes_traits_2 = serialization::detail::serializability_traits<Mapping2>;
    using _serdes_traits_m1to = serialization::detail::serializability_traits<M1ToRange>;
    using _serdes_traits_m2from = serialization::detail::serializability_traits<M2FromRange>;

    template <typename ArchiveT>
    void _serialize(ArchiveT& ar,
      std::false_type, // m1 is given
      std::false_type // m2 is given
    ) {
      ar | m1_ | m2_ | m1_to_range_ | m2_from_range_;
    }

    template <typename ArchiveT>
    void _serialize(ArchiveT& ar,
      std::false_type, // m1 is given
      std::true_type // m2 not given
    ) {
      ar | m1_ | m2_ | m1_to_range_;
    }

    template <typename ArchiveT>
    void _serialize(ArchiveT& ar,
      std::true_type, // m1 not given
      std::false_type // m2 is given
    ) {
      ar | m1_ | m2_ | m1_to_range_;
    }

    template <typename ArchiveT>
    void _serialize(ArchiveT& ar,
      std::true_type, // m1 not given
      std::true_type // m2 not given
    ) {
      ar | m1_ | m2_;
    }

  public:

    template <typename ArchiveT>
    std::enable_if_t<
      _serdes_traits_1::template is_serializable_with_archive<ArchiveT>::value
      and _serdes_traits_2::template is_serializable_with_archive<ArchiveT>::value
      and (
        std::is_same<M1ToRange, detail::not_an_index_range>::value
          or _serdes_traits_m1to::template is_serializable_with_archive<ArchiveT>::value
      )
      and (
        std::is_same<M2FromRange, detail::not_an_index_range>::value
          or _serdes_traits_m2from::template is_serializable_with_archive<ArchiveT>::value
      )
    >
    serialize(ArchiveT& ar) {
      _serialize(ar,
        typename std::is_same<M1ToRange, detail::not_an_index_range>::type{},
        typename std::is_same<M2FromRange, detail::not_an_index_range>::type{}
      );
    }


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

    template <
      typename... Args1,
      typename... Args2
    >
    CompositeMapping(
      std::piecewise_construct_t pc,
      std::tuple<Args1...> m1args,
      std::tuple<Args2...> m2args
    ) : CompositeMapping(
          pc, m1args, m2args,
          std::index_sequence_for<Args1...>{},
          std::index_sequence_for<Args2...>{}
        )
    { }

    // TODO make these play nice with the other constructors

    template <typename M1ToRangeConvertible>
    explicit CompositeMapping(M1ToRangeConvertible&& m1rng,
      std::enable_if_t<
        std::is_same<M2FromRange, detail::not_an_index_range>::value
          and not std::is_same<M1ToRange, detail::not_an_index_range>::value
          and std::is_convertible<M1ToRangeConvertible&&, M1ToRange>::value
          and std::is_default_constructible<Mapping1>::value
          and std::is_default_constructible<Mapping2>::value,
        detail::_impl::_not_a_type_1
      > = { }
    ) : m1_to_range_(std::forward<M1ToRangeConvertible>(m1rng))
    { }

    template <typename M2FromRangeConvertible>
    explicit CompositeMapping(M2FromRangeConvertible&& m2rng,
      std::enable_if_t<
        std::is_same<M1ToRange, detail::not_an_index_range>::value
          and not std::is_same<M2FromRange, detail::not_an_index_range>::value
          and std::is_convertible<M2FromRangeConvertible&&, M2FromRange>::value
          and std::is_default_constructible<Mapping1>::value
          and std::is_default_constructible<Mapping2>::value,
        detail::_impl::_not_a_type_2
      > = { }
    ) : m2_from_range_(std::forward<M2FromRangeConvertible>(m2rng))
    { }

    template <
      typename Mapping1Convertible,
      typename Mapping2Convertible,
      typename M2FromRangeConvertible
    >
    explicit CompositeMapping(
      Mapping1Convertible&& m1,
      Mapping2Convertible&& m2,
      M2FromRangeConvertible&& m2rng,
      std::enable_if_t<
        std::is_same<M1ToRange, detail::not_an_index_range>::value
          and not std::is_same<M2FromRange, detail::not_an_index_range>::value
          and std::is_convertible<M2FromRangeConvertible&&, M2FromRange>::value
          and std::is_convertible<Mapping1Convertible&&, Mapping1>::value
          and std::is_convertible<Mapping2Convertible&&, Mapping2>::value,
        detail::_impl::_not_a_type_2
      > = { }
    ) : m1_(std::forward<Mapping1Convertible>(m1)),
        m2_(std::forward<Mapping2Convertible>(m2)),
        m2_from_range_(std::forward<M2FromRangeConvertible>(m2rng))
    { }

    // For now, only allow composite one-to-one mappings...
    // TODO composite multi-index mappings

    using from_index_type = typename _traits_1::from_index_type;
    using from_multi_index_type = typename _traits_1::from_multi_index_type;
    using to_index_type = typename _traits_2::to_index_type;
    using to_multi_index_type = typename _traits_2::to_multi_index_type;
    using is_index_mapping = std::true_type;

    template <typename FromRange>
    to_multi_index_type map_forward(
      from_index_type const& from, FromRange const& from_range,
      std::enable_if<
        std::is_same<M1ToRange, detail::not_an_index_range>::value
          and std::is_same<M2FromRange, detail::not_an_index_range>::value
          and _traits_2::allows_no_range_map_forward::value
          and _traits_1::template allows_one_range_map_forward<FromRange>::value,
        detail::_impl::_not_a_type
      > = { }
    ) const {
      return _traits_2::map_forward(m2_,
        _traits_1::map_forward(m1_, from, from_range)
      );
    }

    template <typename FromRange>
    to_multi_index_type
    map_forward(
      from_index_type const& from, FromRange const& from_range,
      std::enable_if<
        std::is_same<M1ToRange, detail::not_an_index_range>::value
          and not std::is_same<M2FromRange, detail::not_an_index_range>::value
          and _traits_2::template allows_one_range_map_forward<M2FromRange>::value
          and _traits_1::template allows_one_range_map_forward<FromRange>::value,
        detail::_impl::_not_a_type
      > = { }
    ) const {
      return _traits_2::map_forward(m2_,
        _traits_1::map_forward(m1_, from, from_range), m2_from_range_
      );
    }

    // TODO the rest of the other overloads

    template <typename _for_SFINAE_only=void,
      typename=std::enable_if_t<
        std::is_void<_for_SFINAE_only>::value
          and _traits_1::allows_no_range_map_forward::value
          and _traits_2::allows_no_range_map_forward::value
      >
    >
    to_multi_index_type
    map_forward(from_index_type const& from) const {
      return _traits_2::map_forward(m2_,
        _traits_1::map_forward(m1_, from)
      );
    }

    template <typename FromRange>
    from_multi_index_type map_backward(
      to_index_type const& to, FromRange const& from_range,
      std::enable_if<
        std::is_same<M1ToRange, detail::not_an_index_range>::value
          and std::is_same<M2FromRange, detail::not_an_index_range>::value
          and _traits_2::allows_no_range_map_backward::value
          and _traits_1::template allows_one_range_map_backward<FromRange>::value,
        detail::_impl::_not_a_type
      > = { }
    ) const {
      return _traits_1::map_backward(m1_,
        _traits_2::map_backward(m2_, to), from_range
      );
    }

    template <typename FromRange>
    from_multi_index_type map_backward(
      to_index_type const& to, FromRange const& from_range,
      std::enable_if<
        std::is_same<M1ToRange, detail::not_an_index_range>::value
          and not std::is_same<M2FromRange, detail::not_an_index_range>::value
          and _traits_2::template allows_one_range_map_backward<M2FromRange>::value
          and _traits_1::template allows_one_range_map_backward<FromRange>::value,
        detail::_impl::_not_a_type
      > = { }
    ) const {
      return _traits_1::map_backward(m1_,
        _traits_2::map_backward(m2_, to, m2_from_range_), from_range
      );
    }

    template <typename _for_SFINAE_only_1=void>
    from_multi_index_type
    map_backward(to_index_type const& to,
      std::enable_if_t<
        std::is_void<_for_SFINAE_only_1>::value
          //and _traits_1::allows_no_range_map_backward::value
          //and _traits_2::allows_no_range_map_backward::value
          and std::is_same<M1ToRange, detail::not_an_index_range>::value
          and std::is_same<M2FromRange, detail::not_an_index_range>::value,
        detail::_impl::_not_a_type
      > = { }
    ) const {
      return _traits_1::map_backward(m1_,
        _traits_2::map_backward(m2_, to)
      );
    }

    template <typename _for_SFINAE_only_2=void>
    from_multi_index_type
    map_backward(
      to_index_type const& to,
      std::enable_if_t<
        std::is_void<_for_SFINAE_only_2>::value
          and _traits_1::allows_no_range_map_backward::value
          and _traits_2::template allows_one_range_map_backward<M2FromRange>::value
          and std::is_same<M1ToRange, detail::not_an_index_range>::value
          and not std::is_same<M2FromRange, detail::not_an_index_range>::value,
        detail::_impl::_not_a_type
      > = { }
    ) const {
      return _traits_1::map_backward(m1_,
        _traits_2::map_backward(m2_, to, m2_from_range_)
      );
    }

    // TODO the rest of the other overloads

    bool is_same(CompositeMapping const& other) const {
      return _traits_1::known_same_mapping(m1_, other.m1_) and _traits_2::known_same_mapping(m2_, other.m2_);
    }
};


template <typename Mapping>
struct ReverseMapping {
  private:

    Mapping m1_;

    using _m_traits = indexing::mapping_traits<Mapping>;

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

    using from_index_type = typename _m_traits::to_index_type;
    using to_index_type = typename _m_traits::from_index_type;
    using is_index_mapping = std::true_type;

    to_index_type map_forward(from_index_type const& from) const {
      return m1_.map_backward(from);
    }

    from_index_type map_backward(to_index_type const& to) const {
      return m1_.map_forward(to);
    }

    template <typename ArchiveT>
    std::enable_if_t<
      serialization::detail::serializability_traits<Mapping>
        ::template is_serializable_with_archive<ArchiveT>::value
    >
    serialize(ArchiveT& ar) {
      ar | m1_;
    }

};

template <typename MappingT>
auto
make_reverse_mapping(
  MappingT&& mapping
) {
  return ReverseMapping<std::decay_t<MappingT>>(
    std::forward<MappingT>(mapping)
  );
}

template <typename FromIndexT, typename ToIndexT=FromIndexT>
struct IdentityMapping {

  public:

    using from_index_type = FromIndexT;
    using to_index_type = ToIndexT;
    using is_index_mapping = std::true_type;

    template <typename FromRange, typename ToRange>
    to_index_type
    map_forward(from_index_type const& from, FromRange const&, ToRange const&) const {
      return from;
    }

    template <typename FromRange>
    to_index_type
    map_forward(from_index_type const& from, FromRange const&) const {
      return from;
    }

    to_index_type map_forward(from_index_type const& from) const {
      return from;
    }

    template <typename FromRange, typename ToRange>
    from_index_type map_backward(to_index_type const& to, FromRange const&, ToRange const&) const {
      return to;
    }

    template <typename FromRange>
    from_index_type map_backward(to_index_type const& to, FromRange const&) const {
      return to;
    }

    from_index_type map_backward(to_index_type const& to) const {
      return to;
    }

    bool is_same(IdentityMapping const&) const {
      return true;
    }

};

// Just a sentinel that can't have map_forward or map_backward called
struct NullMapping {
  using is_index_mapping = std::true_type;
};


namespace serialization {

template <typename T>
struct serialize_as_pod_if<T,
  std::enable_if_t<
    indexing::mapping_traits<T>::is_index_mapping
    and std::is_empty<T>::value
  >
> : std::true_type { };

} // end namespace serialization


} // end namespace darma_runtime

#endif //DARMA_IMPL_INDEX_RANGE_MAPPING_H
