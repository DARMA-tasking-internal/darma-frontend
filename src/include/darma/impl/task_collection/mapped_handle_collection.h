/*
//@HEADER
// ************************************************************************
//
//                      mapped_handle_collection.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMA_IMPL_TASK_COLLECTION_MAPPED_HANDLE_COLLECTION_H
#define DARMA_IMPL_TASK_COLLECTION_MAPPED_HANDLE_COLLECTION_H

#include <utility> // std::forward, std::move
#include <cassert>
#include <memory> // std::make_shared

#include <darma_types.h> // types::key_t

#include <darma/interface/backend/runtime.h> // get_backend_runtime()

#include <darma/utility/darma_assert.h>

#include <darma/impl/use.h> // HandleUse, CollectionManagingUse

namespace darma {
namespace detail {

//==============================================================================
// <editor-fold desc="MappedHandleCollection">

template <typename AccessHandleCollectionT, typename Mapping>
struct MappedHandleCollection {
  public:  // all public for now...

    using mapping_t = Mapping;
    using access_handle_collection_t = typename AccessHandleCollectionT
      ::template with_trait_flags<
        advanced::access_handle_collection_traits::internal
          ::is_mapped<OptionalBoolean::KnownTrue>
      >;


    access_handle_collection_t collection;
    Mapping mapping;

    // TODO remove meaningless default ctor once I write serdes stuff
    MappedHandleCollection() = default;

    // TODO seperate copy and move versions, since move implies something different

    template <typename AccessHandleCollectionTDeduced, typename MappingDeduced>
    MappedHandleCollection(
      AccessHandleCollectionTDeduced&& collection,
      MappingDeduced&& mapping
    ) : collection(
          std::forward<AccessHandleCollectionTDeduced>(collection)
        ),
        mapping(std::forward<MappingDeduced>(mapping))
    { }

    template <typename ArchiveT>
    void compute_size(ArchiveT& ar) const {
      ar | mapping;
      ar | collection;
    }

    template <typename ArchiveT>
    void pack(ArchiveT& ar) const {
      ar | mapping;
      ar | collection;
    }

    template <typename ArchiveT>
    static void unpack(void* allocated, ArchiveT& ar) {
      // just for offsets
      auto* rv_uninitialized = reinterpret_cast<MappedHandleCollection*>(allocated);

      // Mapping need not be default constructible, so unpack it here
      ar >> rv_uninitialized->mapping;

      // Collection is default constructible, so just construct it here
      // and unpack it in unpack
      new (&rv_uninitialized->collection) AccessHandleCollectionT();

      ar >> rv_uninitialized->collection;
    }

};

// </editor-fold> end MappedHandleCollection
//==============================================================================


template <typename AccessHandleCollectionT>
struct UnmappedHandleCollection {
  public:

    using access_handle_collection_t = typename AccessHandleCollectionT
      ::template with_trait_flags<
        advanced::access_handle_collection_traits::internal
          ::is_mapped<OptionalBoolean::KnownFalse>
      >;


    access_handle_collection_t collection;

    explicit
    UnmappedHandleCollection(
      AccessHandleCollectionT&& coll
    ) : collection(std::move(coll))
    { }

    UnmappedHandleCollection() = default;
    UnmappedHandleCollection(UnmappedHandleCollection&&) = default;

    // Just the compute_size and pack
    template <typename ArchiveT>
    void serialize(ArchiveT& ar) const {

      ar | collection;

    }
};

} // end namespace detail
} // end namespace darma

#endif //DARMA_IMPL_TASK_COLLECTION_MAPPED_HANDLE_COLLECTION_H
