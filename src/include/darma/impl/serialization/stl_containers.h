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

// TODO Allocator awareness!!!

// A specialization for all stl containers of serializable objects
template <typename C>
struct Serializer<C, std::enable_if_t<meta::is_container<C>::value>> {
  protected:

    typedef meta::is_container<C> container_traits;
    typedef typename container_traits::value_type value_type;
    typedef detail::serializability_traits<value_type> value_serdes_traits;
    typedef typename value_serdes_traits::serializer value_serializer;

    ////////////////////////////////////////////////////////////
    // <editor-fold desc="Special handling if reserve() is available, push_back() or insert(), etc">

    template <typename T>
    using back_insertable_archetype = decltype(
      std::declval<T>().push_back(std::declval<value_type>())
    );

    template <typename T>
    using insertable_archetype = decltype(
      std::declval<T>().insert(
        std::declval<typename container_traits::iterator>(),
        std::declval<value_type>()
      )
    );
    template <typename T>
    using reservable_archetype = decltype( std::declval<T>().reserve( std::declval<size_t>() ) );

    static constexpr auto is_back_insertable = meta::is_detected<
      back_insertable_archetype, C>::value;
    static constexpr auto is_insertable = meta::is_detected<
      insertable_archetype, C>::value;
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

    template <typename T>
    using has_allocator_archetype = decltype( std::declval<const T>().get_allocator() );
    static constexpr auto has_allocator = meta::is_detected<has_allocator_archetype, C>::value;
    using allocator_t =
      meta::detected_or_t<std::allocator<value_type>, has_allocator_archetype, C>;
    using Alloc_traits = std::allocator_traits<allocator_t>;



    template <typename _Ignored = void>
    inline std::enable_if_t<has_allocator and std::is_same<_Ignored, void>::value,
      value_type*
    >
    _allocate_item(C const& c) const {
      auto alloc = c.get_allocator();
      return Alloc_traits::allocate(alloc, 1);
    };

    template <typename _Ignored = void>
    inline std::enable_if_t<not has_allocator and std::is_same<_Ignored, void>::value,
      value_type*
    >
    _allocate_item(C const&) const {
      allocator_t alloc;
      return Alloc_traits::allocate(alloc, 1);
    };

    // UNUSED:
    //template <typename... Args>
    //inline std::enable_if_t<has_allocator>
    //_cosntruct_item(C const& c, void* at, Args&&... args) const {
    //  return Alloc_traits::construct(
    //    c.get_allocator(), at, std::forward<Args>(args)...
    //  );
    //};

    //// UNUSED:
    //template <typename... Args>
    //inline std::enable_if_t<not has_allocator>
    //_construct_item(void* at, Args&&... args) const {
    //  return Alloc_traits::construct(
    //    allocator_t(), at, std::forward<Args>(args)...
    //  );
    //};

    // </editor-fold>
    ////////////////////////////////////////////////////////////

  public:

    ////////////////////////////////////////////////////////////
    // <editor-fold desc="compute_size()">

    template <typename ArchiveT>
    std::enable_if_t<value_serdes_traits::template is_serializable_with_archive<ArchiveT>::value>
    compute_size(C const& c, ArchiveT& ar) const {
      assert(ar.is_sizing());
      ar.incorporate_size(c.size());
      for(auto&& item : c) ar.incorporate_size(item);
    }

    // </editor-fold>
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // <editor-fold desc="pack()">

    template <typename ArchiveT>
    std::enable_if_t<value_serdes_traits::template is_serializable_with_archive<ArchiveT>::value>
    pack(C const& c, ArchiveT& ar) const {
      assert(ar.is_packing());
      ar.pack_item(c.size());
      for(auto&& item : c) ar.pack_item(item);
    }

    // </editor-fold>
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // <editor-fold desc="unpack()">

    // TODO optimization for contiguous containers

    template <typename ArchiveT>
    std::enable_if_t<
      value_serdes_traits::template is_serializable_with_archive<ArchiveT>::value
        and is_back_insertable
    >
    unpack(void* allocated, ArchiveT& ar) const {
      assert(ar.is_unpacking());
      // call default constructor
      C* c = new (allocated) C;

      // and start unpacking
      size_t n_items = 0;
      ar.unpack_item(n_items);
      _unpack_prepare(*c, n_items);

      auto back_iter = std::back_inserter(*c);
      for(typename C::size_type i = 0; i < n_items; ++i) {
        // Allocate the data for the item
        void *tmp = _allocate_item(*c);
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
    unpack(void* allocated, ArchiveT& ar) const {
      assert(ar.is_unpacking());
      // call default constructor
      C* c = new (allocated) C;
      // and start unpacking
      size_t n_items = 0;
      ar.unpack_item(n_items);
      _unpack_prepare(*c, n_items);

      std::insert_iterator<C> ins_iter(*c, c->begin());
      for(typename C::size_type i = 0; i < n_items; ++i) {
        // Allocate the data for the item
        void *tmp = _allocate_item(*c);
        // unpack into it
        ar.unpack_item(*(value_type*)tmp);
        // put it in the container
        ins_iter = *(value_type*)tmp;
      }
    }

    // </editor-fold>
    ////////////////////////////////////////////////////////////
};


} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_STL_CONTAINERS_H
