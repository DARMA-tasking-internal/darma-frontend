/*
//@HEADER
// ************************************************************************
//
//                      stl_containers.h
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

#ifndef DARMA_IMPL_SERIALIZATION_STL_CONTAINERS_H
#define DARMA_IMPL_SERIALIZATION_STL_CONTAINERS_H

#include <type_traits>
#include <iterator>

#include "nonintrusive.h"
#include <darma/impl/meta/is_container.h>

namespace darma_runtime {
namespace serialization {


// A specialization for all stl containers of serializable objects
template <typename C>
struct Serializer<C, std::enable_if_t<meta::is_container<C>::value>> {
  private:
    typedef meta::is_container<C> container_traits;
    typedef typename container_traits::value_type value_type;
    typedef detail::serializability_traits<value_type> value_serdes_traits;
    typedef typename value_serdes_traits::serializer value_serializer;

    template <typename T>
    using back_inserter_valid_archetype = decltype( std::back_inserter( std::declval<T&>() ) );
    template <typename T>
    using inserter_valid_archetype = decltype( std::inserter(
      std::declval<T&>(), std::declval<typename T::iterator>()
    ) );
    template <typename T>
    using reservable_archetype = decltype( std::declval<T>().reserve( std::declval<size_t>() ) );

    static constexpr auto is_back_insertable = meta::is_detected<
      back_inserter_valid_archetype, C>::value;
    static constexpr auto is_insertable = meta::is_detected<
      inserter_valid_archetype, C>::value;
    static constexpr auto is_reservable = meta::is_detected<
      reservable_archetype , C>::value;

    template <typename _Ignored = void>
    inline std::enable_if_t<is_reservable and std::is_same<_Ignored, void>::value>
    _unpack_prepare(C& val, size_t to_reserve) const {
      val.reserve(to_reserve);
    };

    template <typename _Ignored = void>
    inline std::enable_if_t<not is_reservable and std::is_same<_Ignored, void>::value>
    _unpack_prepare(C& val, size_t to_reserve) const {
      /* do nothing */
    };

  public:
    template <typename ArchiveT>
    std::enable_if_t<value_serdes_traits::template is_serializable_with_archive<ArchiveT>::value>
    get_packed_size(C const& c, ArchiveT& ar) const {
      ar.incorporate_size(c.size());
      for(auto&& item : c) ar.incorporate_size(item);
    }

    template <typename ArchiveT>
    std::enable_if_t<value_serdes_traits::template is_serializable_with_archive<ArchiveT>::value>
    pack(C const& c, ArchiveT& ar) const {
      ar.pack_item(c.size());
      for(auto&& item : c) ar.pack_item(item);
    }

    template <typename ArchiveT>
    std::enable_if_t<
      value_serdes_traits::template is_serializable_with_archive<ArchiveT>::value
        and is_back_insertable
    >
    unpack(C& c, ArchiveT& ar) const {
      // call default constructor
      new (&c) C;

      // and start unpacking
      size_t n_items = 0;
      ar.unpack_item(n_items);
      _unpack_prepare(c, n_items);

      auto back_iter = std::back_inserter(c);
      for(typename C::size_type i = 0; i < n_items; ++i) {
        // Allocate the data for the item
        void* tmp = operator new(sizeof(value_type));
        // unpack into it
        ar.unpack_item(*(value_type*)tmp);
        // put it in the container
        back_iter = *(value_type*)tmp;
      }
    }

    template <typename ArchiveT>
    std::enable_if_t<
      value_serdes_traits::template is_serializable_with_archive<ArchiveT>::value
        and is_insertable and not is_back_insertable
    >
    unpack(C& c, ArchiveT& ar) const {
      // call default constructor
      new (&c) C;
      // and start unpacking
      size_t n_items = 0;
      ar.unpack_item(n_items);
      _unpack_prepare(c, n_items);

      std::insert_iterator<C> ins_iter(c, c.begin());
      for(typename C::size_type i = 0; i < n_items; ++i) {
        // Allocate the data for the item
        void *tmp = operator new(sizeof(value_type));
        // unpack into it
        ar.unpack_item(*(C*)tmp);
        // put it in the container
        ins_iter = *(C*)tmp;
      }
    }
};


} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_STL_CONTAINERS_H
