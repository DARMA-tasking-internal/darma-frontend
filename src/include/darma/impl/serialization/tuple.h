/*
//@HEADER
// ************************************************************************
//
//                      tuple.h
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

#ifndef DARMA_TUPLE_H
#define DARMA_TUPLE_H

#include <type_traits>
#include <tuple>

#include <darma/impl/meta/tuple_for_each.h>

#include "nonintrusive.h"
#include "traits.h"

namespace darma_runtime {
namespace serialization {

////////////////////////////////////////////////////////////////////////////////

template <typename... Args>
struct Serializer<std::tuple<Args...>> {
  private:

    using TupleT = std::tuple<Args...>;

    template <typename ArchiveT>
    struct _make_is_serializable_with_archive {
      template <typename T>
      using apply = typename detail::serializability_traits<T>
        ::template is_serializable_with_archive<ArchiveT>;
    };

    template <typename ArchiveT>
    using serializable_into = std::integral_constant<bool,
      tinympl::variadic::all_of<
        _make_is_serializable_with_archive<ArchiveT>::template apply,
        Args...
      >::value
    >;

    using serializer_tuple = std::tuple<
      typename detail::serializability_traits<Args>::serializer...
    >;

  public:

    template <typename ArchiveT>
    std::enable_if_t<serializable_into<ArchiveT>::value>
    compute_size(TupleT const& tup, ArchiveT& ar) const {
      meta::tuple_for_each(tup, [&](auto const& item) {
        ar.incorporate_size(item);
      });
    }


    template <typename ArchiveT>
    std::enable_if_t<serializable_into<ArchiveT>::value>
    pack(TupleT const& tup, ArchiveT& ar) const {
      meta::tuple_for_each(tup, [&](auto const& item) {
        ar.pack_item(item);
      });
    }

  private:

    template <typename ItemType, typename wrapped_index>
    struct _ser_get_pair_for_item {
      struct type {
        template <typename Tuple>
        decltype(auto) get(Tuple&& tup) {
          return std::get<wrapped_index::value>(std::forward<Tuple>(tup));
        }
        using serializer =
          typename detail::serializability_traits<ItemType>::serializer;
      };
    };

  public:

    template <typename ArchiveT>
    std::enable_if_t<serializable_into<ArchiveT>::value>
    unpack(void* allocated, ArchiveT& ar) const {
      // TODO make sure that it's okay to assume that the tuple constructors
      //      don't do anything but call the constituent constructors. (should
      //      be fine in any implementation I can imagine, but I don't know
      //      if this is standards compliant)
      meta::tuple_for_each(
        typename tinympl::transform2<
          TupleT, std::index_sequence_for<Args...>,
          _ser_get_pair_for_item
        >::type(),
        [&](auto&& item_ser_get) {
          typename std::decay_t<decltype(item_ser_get)>::serializer().unpack(
            // This is safe because std::get on a lvalue reference is
            // specified to return an lvalue reference, which we immediately
            // take the address of and cast to a void* without ever
            // dereferencing anything
            (void*) &(item_ser_get.get(
              static_cast<TupleT&>( *static_cast<TupleT*>(allocated) )
            )), ar
          );
        }
      );
    }
};

////////////////////////////////////////////////////////////////////////////////

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMA_TUPLE_H
