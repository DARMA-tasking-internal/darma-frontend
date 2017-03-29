/*
//@HEADER
// ************************************************************************
//
//                      use_collection.h
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

#ifndef DARMA_IMPL_USE_COLLECTION_H
#define DARMA_IMPL_USE_COLLECTION_H

#include <darma/interface/frontend/use_collection.h> // abstract::frontend::UseCollection

#include <darma/impl/index_range/mapping_traits.h>

#include <darma/impl/flow_handling.h> // flow_ptr
#include <darma/impl/handle_use_base.h> // HandleUseBase

namespace darma_runtime {

namespace detail {

//==============================================================================
// <editor-fold desc="MappingManager"> {{{1

//------------------------------------------------------------------------------
// <editor-fold desc="MappingManagerBase"> {{{2

template <typename FrontendHandleIndex>
struct MappingManagerBase
{

  template <typename T>
  using index_iterable = abstract::frontend::UseCollection::index_iterable<T>;

  virtual index_iterable<FrontendHandleIndex>
  local_indices_for(std::size_t backend_task_collection_index) const =0;

  virtual std::size_t
  map_to_task_collection_backend_index(FrontendHandleIndex const& fe_idx) const =0;

  virtual ~MappingManagerBase() = default;

};

// </editor-fold> end MappingManagerBase }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="The MappingManager class template itself"> {{{2

template <typename MappingToTaskCollection, typename FrontendHandleIndex>
struct MappingManager : MappingManagerBase<FrontendHandleIndex>
{

  template <typename T>
  using index_iterable = abstract::frontend::UseCollection::index_iterable<T>;

  // This should be the full mapping from frontend handle collection index to backend task collection index
  MappingToTaskCollection mapping_to_tc_backend_idx;
  using _mapping_traits = indexing::mapping_traits<
    std::decay_t<
      MappingToTaskCollection>>;

  // We can assert this: (at least in part)
  static_assert(
    std::is_same<
      typename _mapping_traits::to_index_type,
      std::size_t // i.e., backend index
    >::value,
    "Backend index incorrectly declared"
  );

  template <
    typename MappingDeduced,
    typename=std::enable_if_t<
      std::is_convertible<
        MappingDeduced&&,
        MappingToTaskCollection
      >::value
    >
  >
  explicit MappingManager(
    MappingDeduced&& mapping
  ) : mapping_to_tc_backend_idx(std::forward<MappingDeduced>(mapping)) {}

  index_iterable<FrontendHandleIndex>
  local_indices_for(std::size_t backend_task_collection_index) const override
  {
    // TODO propagate index range arguments to here?
    return index_iterable<FrontendHandleIndex>{
      _mapping_traits::map_backward(
        mapping_to_tc_backend_idx,
        backend_task_collection_index
      )
    };
  }

  virtual std::size_t
  map_to_task_collection_backend_index(FrontendHandleIndex const& fe_idx) const override
  {
    // TODO propagate index range arguments to here?
    return _mapping_traits::map_forward(
      mapping_to_tc_backend_idx, fe_idx
    );
  }

  ~MappingManager() = default;

};

// </editor-fold> end The MappingManager class template itself }}}2
//------------------------------------------------------------------------------

// </editor-fold> end MappingManager }}}1
//==============================================================================

template <typename FrontendHandleIndex>
class CollectionManagingUseBase
  : public abstract::frontend::UseCollection,
    public HandleUseBase
{
  public:
    // This structure is probably very inefficient, but just to get something working...
    // TODO this should be a std::optional, probably, or something like that, since mapping manager is stateless
    std::unique_ptr<MappingManagerBase<FrontendHandleIndex>> mapping_manager = nullptr;

    using HandleUseBase::HandleUseBase;

    template <typename MappingToTaskCollectionDeduced>
    CollectionManagingUseBase(
      std::shared_ptr<VariableHandleBase> handle,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      abstract::frontend::FlowRelationship::flow_relationship_description_t in_desc,
      types::flow_t* in_rel,
      abstract::frontend::FlowRelationship::flow_relationship_description_t out_desc,
      types::flow_t* out_rel,
      bool out_rel_is_in,
      MappingToTaskCollectionDeduced&& mapping
    ) : HandleUseBase(
          handle, scheduling_permissions, immediate_permissions,
          in_desc, in_rel,
          out_desc, out_rel, out_rel_is_in
        ),
        mapping_manager(
          std::make_unique<
            MappingManager<std::decay_t<MappingToTaskCollectionDeduced>, FrontendHandleIndex>
          >(std::forward<MappingToTaskCollectionDeduced>(mapping))
        )
    { }

    template <typename MappingToTaskCollectionDeduced>
    CollectionManagingUseBase(
      CollectionManagingUseBase&& use_base,
      MappingToTaskCollectionDeduced&& mapping
    ) : HandleUseBase(std::move(use_base)),
        mapping_manager(
          std::make_unique<
            MappingManager<std::decay_t<MappingToTaskCollectionDeduced>, FrontendHandleIndex>
          >(std::forward<MappingToTaskCollectionDeduced>(mapping))
        )
    { }

    template <typename MappingToTaskCollectionDeduced>
    CollectionManagingUseBase(
      std::shared_ptr<VariableHandleBase> handle,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      types::flow_t&& unpacked_in_flow,
      types::flow_t&& unpacked_out_flow,
      MappingToTaskCollectionDeduced&& mapping
    ) : HandleUseBase(
          handle, scheduling_permissions, immediate_permissions,
          std::move(unpacked_in_flow), std::move(unpacked_out_flow)
        ),
        mapping_manager(
          std::make_unique<
            MappingManager<std::decay_t<MappingToTaskCollectionDeduced>, FrontendHandleIndex>
          >(std::forward<MappingToTaskCollectionDeduced>(mapping))
        )
    { }

    CollectionManagingUseBase(CollectionManagingUseBase&&) = default;
    virtual ~CollectionManagingUseBase() = default;
};


template <typename IndexRangeT>
class CollectionManagingUse
  : public CollectionManagingUseBase<typename indexing::index_range_traits<IndexRangeT>::index_type>
{
  public:

    using rng_traits = indexing::index_range_traits<IndexRangeT>;
    using mapping_to_dense_traits = indexing::mapping_traits<
      typename rng_traits::mapping_to_dense_type
    >;

    using base_t = CollectionManagingUseBase<
      typename indexing::index_range_traits<IndexRangeT>::index_type
    >;

    template <typename T>
    using index_iterable = abstract::frontend::UseCollection::index_iterable<T>;

    //--------------------------------------------------------------------------
    // <editor-fold desc="Ctors"> {{{2

    CollectionManagingUse(CollectionManagingUse&&) = default;

    template <
      typename IndexRangeDeduced,
      typename=std::enable_if_t<
        std::is_convertible<IndexRangeDeduced&&, IndexRangeT>::value
      >
    >
    CollectionManagingUse(
      std::shared_ptr<VariableHandleBase> handle,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      abstract::frontend::FlowRelationship::flow_relationship_description_t in_desc,
      types::flow_t* in_rel,
      abstract::frontend::FlowRelationship::flow_relationship_description_t out_desc,
      types::flow_t* out_rel,
      bool out_rel_is_in,
      IndexRangeDeduced&& range
    ) : base_t(
          handle, scheduling_permissions, immediate_permissions,
          in_desc, in_rel, out_desc, out_rel, out_rel_is_in
        ),
        index_range(std::forward<IndexRangeDeduced>(range)),
        mapping_to_dense(rng_traits::mapping_to_dense(index_range))
    { }

    template <typename MappingToTaskCollectionDeduced>
    CollectionManagingUse(
      CollectionManagingUse&& other,
      MappingToTaskCollectionDeduced&& mapping
    ) : base_t(std::move(other)),
        index_range(other.index_range),
        mapping_to_dense(rng_traits::mapping_to_dense(index_range))
    { }

    template <
      typename IndexRangeDeduced, typename MappingToTaskCollectionDeduced,
      typename=std::enable_if_t<
        std::is_convertible<IndexRangeDeduced&&, IndexRangeT>::value
      >
    >
    CollectionManagingUse(
      std::shared_ptr<VariableHandleBase> handle,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      abstract::frontend::FlowRelationship::flow_relationship_description_t in_desc,
      types::flow_t* in_rel,
      abstract::frontend::FlowRelationship::flow_relationship_description_t out_desc,
      types::flow_t* out_rel,
      bool out_rel_is_in,
      IndexRangeDeduced&& range, MappingToTaskCollectionDeduced&& mapping
    ) : base_t(
          handle, scheduling_permissions, immediate_permissions,
          in_desc, in_rel, out_desc, out_rel, out_rel_is_in,
          std::forward<MappingToTaskCollectionDeduced>(mapping)
        ),
        index_range(std::forward<IndexRangeDeduced>(range)),
        mapping_to_dense(rng_traits::mapping_to_dense(index_range))
    { }

    // Should only be used by unpack at this point
    template <
      typename IndexRangeDeduced,
      typename=std::enable_if_t<
        std::is_convertible<IndexRangeDeduced&&, IndexRangeT>::value
      >
    >
    CollectionManagingUse(
      std::shared_ptr<VariableHandleBase> handle,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      types::flow_t&& in_flow,
      types::flow_t&& out_flow,
      IndexRangeDeduced&& range
    ) : base_t(
          handle,
          scheduling_permissions, immediate_permissions,
          std::move(in_flow), std::move(out_flow)
        ),
        index_range(std::forward<IndexRangeDeduced>(range)),
        mapping_to_dense(rng_traits::mapping_to_dense(index_range))
    { }

    // </editor-fold> end Ctors }}}2
    //--------------------------------------------------------------------------

    // Don't change this member order
    IndexRangeT index_range;
    typename rng_traits::mapping_to_dense_type mapping_to_dense;

    std::size_t size() const override { return index_range.size(); }

    bool manages_collection() const override {
      return true;
    }

    abstract::frontend::UseCollection*
    get_managed_collection() override {
      return this;
    }

    index_iterable<std::size_t>
    local_indices_for(std::size_t backend_task_collection_index) const override {
      // Still need to convert these to backend handle collection indices
      // Again, really inefficient...
      assert(base_t::mapping_manager);
      auto fe_idxs = base_t::mapping_manager->local_indices_for(backend_task_collection_index);
      index_iterable<std::size_t> rv;
      rv.reserve(fe_idxs.size());
      for(auto&& fe_idx : fe_idxs) {
        rv.push_back(
          mapping_to_dense_traits::map_forward(
            mapping_to_dense, std::forward<decltype(fe_idx)>(fe_idx),
            index_range
          )
        );
      }
      return rv;
    }

    std::size_t
    task_index_for(std::size_t collection_index) const override {
      return base_t::mapping_manager->map_to_task_collection_backend_index(
        mapping_to_dense_traits::map_backward(
          mapping_to_dense, collection_index, index_range
        )
      );
    }

#if _darma_has_feature(commutative_access_handles)
    bool
    is_mapped() const override {
      return this->mapping_manager.get() != nullptr;
    }
#endif

    ~CollectionManagingUse() = default;

};


} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_USE_COLLECTION_H
