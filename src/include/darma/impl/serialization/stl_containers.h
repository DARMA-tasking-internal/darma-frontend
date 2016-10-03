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

#include <darma/impl/meta/is_container.h>

#include "nonintrusive.h"
#include "range.h"

namespace darma_runtime {
namespace serialization {
namespace detail {

// A specialization for all stl containers of serializable objects
template <typename C>
struct Serializer_enabled_if<C, std::enable_if_t<meta::is_container<C>::value>> {
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
    using reservable_archetype = decltype( std::declval<T>().reserve(
      std::declval<typename C::size_type>()
    ) );
    template <typename T>
    using has_emplace_back_default_archetype =
      decltype( std::declval<T>().emplace_back() );

    static constexpr auto is_back_insertable = meta::is_detected<
      back_insertable_archetype, C>::value;
    static constexpr auto is_insertable = meta::is_detected<
      insertable_archetype, C>::value;
    static constexpr auto is_reservable = meta::is_detected<
      reservable_archetype , C>::value;
    static constexpr auto has_emplace_back_default = meta::is_detected<
      has_emplace_back_default_archetype, C>::value;

    template <typename _Ignored = void>
    inline std::enable_if_t<is_reservable and std::is_same<_Ignored, void>::value>
    _unpack_prepare(C& val, typename C::size_type to_reserve) const {
      val.reserve(to_reserve);
    };

    template <typename _Ignored = void>
    inline std::enable_if_t<not is_reservable and std::is_same<_Ignored, void>::value>
    _unpack_prepare(C& val, typename C::size_type to_reserve) const {
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
    inline std::enable_if_t<has_allocator and std::is_same<_Ignored, void>::value>
    _deallocate_item(C const& c, void* ptr) const {
      auto alloc = c.get_allocator();
      Alloc_traits::deallocate(alloc,
        static_cast<typename Alloc_traits::pointer>(ptr), 1
      );
    };

    template <typename _Ignored = void>
    inline std::enable_if_t<not has_allocator and std::is_same<_Ignored, void>::value,
      value_type*
    >
    _allocate_item(C const&) const {
      allocator_t alloc;
      return Alloc_traits::allocate(alloc, 1);
    };

    template <typename _Ignored = void>
    inline std::enable_if_t<not has_allocator and std::is_same<_Ignored, void>::value>
    _deallocate_item(C const& c, void* ptr) const {
      allocator_t alloc;
      Alloc_traits::deallocate(alloc, static_cast<typename Alloc_traits::pointer>(ptr), 1);
    };

    template <typename ArchiveT>
    inline std::enable_if_t<has_allocator and not std::is_void<ArchiveT>::value>
    _call_unpack_item(ArchiveT& ar, void* loc, C const& c) const {
      auto alloc = c.get_allocator();
      ar.template unpack_item<value_type>(loc, alloc);
    };

    template <typename ArchiveT>
    inline std::enable_if_t<not has_allocator and std::is_void<ArchiveT>::value>
    _call_unpack_item(ArchiveT& ar, void* loc, C const&) const {
      ar.template unpack_item<value_type>(loc);
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
    std::enable_if_t<value_serdes_traits::template is_sizable_with_archive<ArchiveT>::value>
    compute_size(C const& c, ArchiveT& ar) const {
      assert(ar.is_sizing());
      auto arnew = ar % serialization::range(c.begin(), c.end());
    }

    // </editor-fold>
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // <editor-fold desc="pack()">

    template <typename ArchiveT>
    std::enable_if_t<value_serdes_traits::template is_packable_with_archive<ArchiveT>::value>
    pack(C const& c, ArchiveT& ar) const {
      assert(ar.is_packing());
      ar << serialization::range(c.begin(), c.end());
    }

    // </editor-fold>
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // <editor-fold desc="unpack()">

    template <typename ArchiveT, typename ParentAllocatorT>
    std::enable_if_t<
      value_serdes_traits::template is_unpackable_with_archive<ArchiveT>::value
        and is_back_insertable
    >
    unpack(void* allocated, ArchiveT& ar, ParentAllocatorT&& parent_alloc) const {
      assert(ar.is_unpacking());

      // call default constructor
      std::allocator_traits<std::decay_t<ParentAllocatorT>>::construct(
        parent_alloc, // intentionally not forwarded
        static_cast<C*>(allocated)
      );
      C* c = static_cast<C*>(allocated);

      // and start unpacking
      typename C::size_type n_items = 0;
      ar.unpack_item(n_items);
      _unpack_prepare(*c, n_items);

      auto back_iter = std::back_inserter(*c);
      for(typename C::size_type i = 0; i < n_items; ++i) {
        // Allocate the data for the item
        void *tmp = _allocate_item(*c);
        // unpack into it
        _call_unpack_item(ar, tmp, *c);
        // put it in the container
        back_iter = *(value_type*)tmp;

        _deallocate_item(*c, tmp);
      }
    }

    template <typename ArchiveT, typename ParentAllocatorT>
    std::enable_if_t<
      value_serdes_traits::template is_unpackable_with_archive<ArchiveT>::value
        and is_insertable and not is_back_insertable
    >
    unpack(void* allocated, ArchiveT& ar, ParentAllocatorT&& parent_alloc) const {
      assert(ar.is_unpacking());

      // call default constructor
      std::allocator_traits<std::decay_t<ParentAllocatorT>>::construct(
        parent_alloc, // intentionally not forwarded
        static_cast<C*>(allocated)
      );
      C* c = static_cast<C*>(allocated);

      // and start unpacking
      typename C::size_type n_items = 0;
      ar.unpack_item(n_items);
      _unpack_prepare(*c, n_items);

      std::insert_iterator<C> ins_iter(*c, c->begin());
      for(typename C::size_type i = 0; i < n_items; ++i) {
        // Allocate the data for the item
        void *tmp = _allocate_item(*c);
        // unpack into it
        _call_unpack_item(ar, tmp, *c);
        // put it in the container
        ins_iter = *(value_type*)tmp;

        _deallocate_item(*c, tmp);
      }
    }

    //template <typename ArchiveT>
    //std::enable_if_t<
    //  value_serdes_traits::template is_serializable_with_archive<ArchiveT>::value and
    //    has_emplace_back_default and not is_insertable and not is_back_insertable
    //>
    //unpack(void* allocated, ArchiveT& ar) const {
    //  assert(ar.is_unpacking());
    //  // call default constructor
    //  C* c = new (allocated) C;
    //  // and start unpacking
    //  typename C::size_type n_items = 0;
    //  ar.unpack_item(n_items);
    //  _unpack_prepare(*c, n_items);

    //  for(typename C::size_type i = 0; i < n_items; ++i) {
    //    c->emplace_back();
    //    // unpack into it
    //    ar.unpack_item(c->back());
    //  }
    //}

    // </editor-fold>
    ////////////////////////////////////////////////////////////
};


} // end namespace detail
} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_STL_CONTAINERS_H
