/*
//@HEADER
// ************************************************************************
//
//                      mapped_handle_collection.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMA_IMPL_TASK_COLLECTION_MAPPED_HANDLE_COLLECTION_H
#define DARMA_IMPL_TASK_COLLECTION_MAPPED_HANDLE_COLLECTION_H

#include <utility> // std::forward, std::move
#include <cassert>
#include <memory> // std::make_shared

#include <darma_types.h> // types::key_t

#include <darma/interface/backend/runtime.h> // get_backend_runtime()

#include <darma/impl/darma_assert.h>

#include <darma/impl/use.h> // HandleUse, CollectionManagingUse

#include <darma/impl/serialization/serializer_attorneys.h>

namespace darma_runtime {
namespace detail {

//==============================================================================
// <editor-fold desc="MappedHandleCollection">

template <typename AccessHandleCollectionT, typename Mapping>
struct MappedHandleCollection {
  public:  // all public for now...

    AccessHandleCollectionT collection;
    Mapping mapping;

    using mapping_t = Mapping;
    using access_handle_collection_t = AccessHandleCollectionT;

    // TODO remove meaningless default ctor once I write serdes stuff
    MappedHandleCollection() = default;

    template <typename AccessHandleCollectionTDeduced, typename MappingDeduced>
    MappedHandleCollection(
      AccessHandleCollectionTDeduced&& collection,
      MappingDeduced&& mapping
    ) : collection(std::forward<AccessHandleCollectionTDeduced>(collection)),
        mapping(std::forward<MappingDeduced>(mapping))
    { }

    // Just the compute_size and pack
    template <typename ArchiveT>
    void serialize(ArchiveT& ar) const {
      assert(not ar.is_unpacking());

      ar | mapping;

      ar | collection.var_handle_->get_key();

      ar | collection.get_index_range();

      ar | collection.current_use_->use->scheduling_permissions_;
      ar | collection.current_use_->use->immediate_permissions_;
      DARMA_ASSERT_MESSAGE(
        collection.mapped_backend_index_ == collection.unknown_backend_index,
        "Can't migrate a handle collection after it has been mapped to a task"
      );
      DARMA_ASSERT_MESSAGE(
        collection.local_use_holders_.empty(),
        "Can't migrate a handle collection after it has been mapped to a task"
      );
      auto* backend_runtime = abstract::backend::get_backend_runtime();
      if(ar.is_sizing()) {
        ar.add_to_size_indirect(
          backend_runtime->get_packed_flow_size(
            collection.current_use_->use->in_flow_
          )
        );
        ar.add_to_size_indirect(
          backend_runtime->get_packed_flow_size(
            collection.current_use_->use->out_flow_
          )
        );
      }
      else { // ar.is_packing()
        assert(ar.is_packing());
        using serialization::Serializer_attorneys::ArchiveAccess;
        backend_runtime->pack_flow(
          collection.current_use_->use->in_flow_,
          reinterpret_cast<void*&>(ArchiveAccess::spot(ar))
        );
        backend_runtime->pack_flow(
          collection.current_use_->use->out_flow_,
          reinterpret_cast<void*&>(ArchiveAccess::spot(ar))
        );
      }

    }

    template <typename ArchiveT>
    static MappedHandleCollection&
    reconstruct(void* allocated, ArchiveT& ar) {
      // just for offsets
      auto* rv_uninitialized = reinterpret_cast<MappedHandleCollection*>(allocated);

      // Mapping need not be default constructible, so unpack it here
      ar >> rv_uninitialized->mapping;

      // Collection is default constructible, so just construct it here
      // and unpack it in unpack
      new (&rv_uninitialized->collection) AccessHandleCollectionT();

      return *rv_uninitialized;
    }

    template <typename ArchiveT>
    void unpack(ArchiveT& ar) {
      // Mapping already unpacked in reconstruct()

      // Set up the access handle collection here, though
      types::key_t key = darma_runtime::make_key();
      ar >> key;
      auto var_handle = std::make_shared<
        detail::VariableHandle<typename AccessHandleCollectionT::value_type>
      >(key);
      collection.var_handle_ = var_handle;

      using handle_range_t = typename AccessHandleCollectionT::index_range_type;
      using handle_range_traits = indexing::index_range_traits<handle_range_t>;
      using handle_range_serdes_traits = serialization::detail::serializability_traits<handle_range_t>;

      // Unpack index range of the handle itself
      char hr_buffer[sizeof(handle_range_t)];
      handle_range_serdes_traits::unpack(reinterpret_cast<void*>(hr_buffer), ar);
      auto& hr = *reinterpret_cast<handle_range_t*>(hr_buffer);

      // unpack permissions
      HandleUse::permissions_t sched_perm, immed_perm;
      ar >> sched_perm >> immed_perm;

      // unpack the flows
      using serialization::Serializer_attorneys::ArchiveAccess;
      auto* backend_runtime = abstract::backend::get_backend_runtime();
      char const*& archive_spot = const_cast<char const*&>(
        ArchiveAccess::spot(ar)
      );
      auto inflow = backend_runtime->make_unpacked_flow(
        reinterpret_cast<void const*&>(archive_spot)
      );
      auto outflow = backend_runtime->make_unpacked_flow(
        reinterpret_cast<void const*&>(archive_spot)
      );


      // remake the use:
      collection.current_use_ = std::make_shared<
        GenericUseHolder<CollectionManagingUse<handle_range_t>>
      >(
        CollectionManagingUse<handle_range_t>(
          var_handle, sched_perm, immed_perm,
          std::move(inflow), std::move(outflow), std::move(hr)
          // the mapping will be re-set up in the task collection unpack,
          // so don't worry about it here
        ),
          /* do_register_in_ctor= */ false // MappedHandleCollection unpack handles this
      );

      // the use will be reregistered after the mapping is added back in, so
      // don't worry about it here

    }

};

// </editor-fold> end MappedHandleCollection
//==============================================================================

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_TASK_COLLECTION_MAPPED_HANDLE_COLLECTION_H
