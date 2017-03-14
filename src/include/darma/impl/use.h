/*
//@HEADER
// ************************************************************************
//
//                      use.h
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

#ifndef DARMA_IMPL_USE_H
#define DARMA_IMPL_USE_H

#include <darma/impl/feature_testing_macros.h>

#include <darma_types.h>


#include <darma/interface/frontend/use.h>
#include <darma/interface/backend/flow.h>
#include <darma/impl/handle.h>
#include <darma/impl/flow_handling.h>
#include <darma/impl/index_range/mapping.h>
#include <darma/impl/index_range/mapping_traits.h>
#include <darma/impl/index_range/index_range_traits.h>
#include <darma/impl/util/managing_ptr.h>

namespace darma_runtime {

namespace detail {

class HandleUseBase
  : public abstract::frontend::Use
{
  public:

    void* data_ = nullptr;

    bool already_captured = false;

    std::shared_ptr<VariableHandleBase> handle_ = nullptr;

    flow_ptr in_flow_;
    flow_ptr out_flow_;
    std::size_t collection_owner_ = std::numeric_limits<std::size_t>::max();

    abstract::frontend::Use::permissions_t immediate_permissions_ = None;
    abstract::frontend::Use::permissions_t scheduling_permissions_ = None;

    ////////////////////////////////////////
    // <editor-fold desc="abstract::frontend::Use implementation">

    std::shared_ptr<abstract::frontend::Handle const>
    get_handle() const override {
      return handle_;
    }

    types::flow_t const&
    get_in_flow() const override {
      return *(in_flow_.get());
    }

    types::flow_t const&
    get_out_flow() const override {
      return *(out_flow_.get());
    }

    void
    set_in_flow(types::flow_t const& flow) {
      in_flow_ = make_flow_ptr(flow);
    }

    void
    set_out_flow(types::flow_t const& flow) {
      out_flow_ = make_flow_ptr(flow);
    }

    abstract::frontend::Use::permissions_t
    immediate_permissions() const override {
      return immediate_permissions_;
    }

    abstract::frontend::Use::permissions_t
    scheduling_permissions() const override {
      return scheduling_permissions_;
    }

    void*&
    get_data_pointer_reference() override {
      return data_;
    }

    std::size_t
    task_collection_owning_index() const override {
      return collection_owner_;
    }

    // </editor-fold>
    ////////////////////////////////////////


    HandleUseBase(
      std::shared_ptr<VariableHandleBase> handle,
      flow_ptr const& in_flow,
      flow_ptr const& out_flow,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions
    ) : handle_(handle),
        immediate_permissions_(immediate_permissions),
        scheduling_permissions_(scheduling_permissions),
        in_flow_(in_flow),
        out_flow_(out_flow)
    { }

    HandleUseBase() = default;
    HandleUseBase(HandleUseBase const&) = default;
    HandleUseBase(HandleUseBase&&) = default;
    HandleUseBase& operator=(HandleUseBase const&) = default;
    HandleUseBase& operator=(HandleUseBase&&) = default;

};

class HandleUse
  : public HandleUseBase
{
  public:

    using HandleUseBase::HandleUseBase;

    bool manages_collection() const override {
      return false;
    }

    abstract::frontend::UseCollection*
    get_managed_collection() override {
      return nullptr;
    }
};

template <typename FrontendHandleIndex>
struct MappingManagerBase {

  template <typename T>
  using index_iterable = abstract::frontend::UseCollection::index_iterable<T>;

  virtual index_iterable<FrontendHandleIndex>
  local_indices_for(std::size_t backend_task_collection_index) const =0;

  virtual bool
  has_same_mapping_as(MappingManagerBase const* other) const =0;

  virtual std::size_t
  map_to_task_collection_backend_index(FrontendHandleIndex const& fe_idx) const =0;

};

template <typename MappingToTaskCollection, typename FrontendHandleIndex>
struct MappingManager : MappingManagerBase<FrontendHandleIndex> {

  template <typename T>
  using index_iterable = abstract::frontend::UseCollection::index_iterable<T>;

  // This should be the full mapping from frontend handle collection index to backend task collection index
  MappingToTaskCollection mapping_to_tc_backend_idx;
  using _mapping_traits = indexing::mapping_traits<std::decay_t<MappingToTaskCollection>>;

  // We can assert this: (at least in part)
  static_assert(
    std::is_same<
      typename _mapping_traits::to_index_type,
      std::size_t // i.e., backend index
    >::value,
    "Backend index incorrectly declared"
  );

  template <typename MappingDeduced,
    typename=std::enable_if_t<std::is_convertible<MappingDeduced&&, MappingToTaskCollection>::value>
  >
  explicit MappingManager(
    MappingDeduced&& mapping
  ) : mapping_to_tc_backend_idx(std::forward<MappingDeduced>(mapping))
  { }

  index_iterable<FrontendHandleIndex>
  local_indices_for(std::size_t backend_task_collection_index) const override {
    // TODO propagate index range arguments to here?
    return index_iterable<FrontendHandleIndex>{
      _mapping_traits::map_backward(
        mapping_to_tc_backend_idx,
        backend_task_collection_index
      )
    };
  }

  bool
  has_same_mapping_as(
    MappingManagerBase<FrontendHandleIndex> const* other
  ) const override {
    // TODO figure out how to do this in a more general way?!?
    auto const* other_cast = dynamic_cast<MappingManager const*>(other);
    if(other_cast) {
      return _mapping_traits::known_same_mapping(
        mapping_to_tc_backend_idx,
        other_cast->mapping_to_tc_backend_idx
      );
    }
    // TODO Maybe some fallback with polymorphic mappings?
    else {
      return false;
    }
  }

  virtual std::size_t
  map_to_task_collection_backend_index(FrontendHandleIndex const& fe_idx) const override {
    // TODO propagate index range arguments to here?
    return _mapping_traits::map_forward(
      mapping_to_tc_backend_idx, fe_idx
    );
  }

};

template <typename FrontendHandleIndex>
class CollectionManagingUseBase
  : public abstract::frontend::UseCollection,
    public HandleUseBase
{
  public:
    // This structure is probably very inefficient, but just to get something working...
    std::unique_ptr<MappingManagerBase<FrontendHandleIndex>> mapping_manager = nullptr;

    using HandleUseBase::HandleUseBase;

    template <typename MappingToTaskCollectionDeduced>
    CollectionManagingUseBase(
      std::shared_ptr<VariableHandleBase> handle,
      flow_ptr const& in_flow,
      flow_ptr const& out_flow,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      MappingToTaskCollectionDeduced&& mapping
    ) : HandleUseBase(handle, in_flow, out_flow, scheduling_permissions, immediate_permissions),
        mapping_manager(
          std::make_unique<
            MappingManager<std::decay_t<MappingToTaskCollectionDeduced>, FrontendHandleIndex>
          >(std::forward<MappingToTaskCollectionDeduced>(mapping))
        )
    { }
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

    template <typename IndexRangeDeduced,
      typename=std::enable_if_t<
        std::is_convertible<IndexRangeDeduced&&, IndexRangeT>::value
      >
    >
    CollectionManagingUse(
      std::shared_ptr<VariableHandleBase> handle,
      flow_ptr const& in_flow,
      flow_ptr const& out_flow,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      IndexRangeDeduced&& range
    ) : base_t(handle, in_flow, out_flow, scheduling_permissions, immediate_permissions),
        index_range(std::forward<IndexRangeDeduced>(range)),
        mapping_to_dense(rng_traits::mapping_to_dense(index_range))
    { }

    template <typename MappingToTaskCollectionDeduced>
    CollectionManagingUse(
      CollectionManagingUse const& other,
      MappingToTaskCollectionDeduced&& mapping
    ) : base_t(
        other.handle_, other.in_flow_, other.out_flow_,
        other.scheduling_permissions_, other.immediate_permissions_,
        std::forward<MappingToTaskCollectionDeduced>(mapping)
      ),
      index_range(other.index_range),
      mapping_to_dense(rng_traits::mapping_to_dense(index_range))
    { }

    template <typename IndexRangeDeduced, typename MappingToTaskCollectionDeduced,
      typename=std::enable_if_t<
        std::is_convertible<IndexRangeDeduced&&, IndexRangeT>::value
      >
    >
    CollectionManagingUse(
      std::shared_ptr<VariableHandleBase> handle,
      flow_ptr const& in_flow,
      flow_ptr const& out_flow,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      IndexRangeDeduced&& range, MappingToTaskCollectionDeduced&& mapping
    ) : base_t(
          handle, in_flow, out_flow, scheduling_permissions, immediate_permissions,
          std::forward<MappingToTaskCollectionDeduced>(mapping)
        ),
        index_range(std::forward<IndexRangeDeduced>(range)),
        mapping_to_dense(rng_traits::mapping_to_dense(index_range))
    { }

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

    bool
    has_same_mapping_as(
      abstract::frontend::UseCollection const* other
    ) const override {
      auto const* other_cast = dynamic_cast<base_t const*>(other);
      if(other_cast) {
        return base_t::mapping_manager->has_same_mapping_as(
          other_cast->mapping_manager.get()
        );
      }
      else {
        return false;
      }
    }

    std::size_t
    task_index_for(std::size_t collection_index) const override {
      return base_t::mapping_manager->map_to_task_collection_backend_index(
        mapping_to_dense_traits::map_backward(
          mapping_to_dense, collection_index, index_range
        )
      );
    }

};


struct migrated_use_arg_t { };
static constexpr migrated_use_arg_t migrated_use_arg = { };

// This is an ugly hack, but it will hold us over for now...
//template <typename UnderlyingUse>
//struct OwningUseWrapper {
//  UnderlyingUse* use_;
//
//  void*& data_;
//  std::shared_ptr<VariableHandleBase>& handle_;
//  flow_ptr& in_flow_;
//  flow_ptr& out_flow_;
//  abstract::frontend::Use::permissions_t& immediate_permissions_;
//  abstract::frontend::Use::permissions_t& scheduling_permissions_;
//
//  OwningUseWrapper(UnderlyingUse&& use)
//    : use_(
//        new (
//          abstract::backend::get_backend_memory_manager()->allocate(
//            sizeof(UnderlyingUse),
//            serialization::detail::DefaultMemoryRequirementDetails{}
//          )
//        ) UnderlyingUse(std::move(use))
//      ),
//      handle_(use_->handle_),
//      data_(use_->data_),
//      in_flow_(use_->in_flow_),
//      out_flow_(use_->out_flow_),
//      immediate_permissions_(use_->immediate_permissions_),
//      scheduling_permissions_(use_->scheduling_permissions_)
//  { }
//
//  OwningUseWrapper(UnderlyingUse* use)
//    : use_(use),
//      handle_(use_->handle_),
//      data_(use_->data_),
//      in_flow_(use_->in_flow_),
//      out_flow_(use_->out_flow_),
//      immediate_permissions_(use_->immediate_permissions_),
//      scheduling_permissions_(use_->scheduling_permissions_)
//  { }
//
//  void*& get_data_pointer_reference() { return data_; }
//
//  UnderlyingUse* operator&() { return use_; }
//  UnderlyingUse const* operator&() const { return use_; }
//
//  operator UnderlyingUse&() {
//    return *use_;
//  }
//
//  operator UnderlyingUse const&() const {
//    return *use_;
//  }
//
//  ~OwningUseWrapper() {
//    use_->~UnderlyingUse();
//    abstract::backend::get_backend_memory_manager()->deallocate(
//      use_, sizeof(UnderlyingUse)
//    );
//
//  }
//
//};

//template <typename UnderlyingUse>
//void swap(
//  OwningUseWrapper<UnderlyingUse>& a,
//  OwningUseWrapper<UnderlyingUse>& b
//) {
//  UnderlyingUse* a_use = a.use_;
//  UnderlyingUse* b_use = b.use_;
//  new (std::addressof(a)) OwningUseWrapper<UnderlyingUse>(b_use);
//  new (std::addressof(b)) OwningUseWrapper<UnderlyingUse>(a_use);
//}

struct UseHolderBase {
  bool is_registered = false;
  bool could_be_alias = false;
  HandleUseBase* use_base = nullptr;
};

// really belongs to AccessHandle, but we can't put this in impl/handle.h
// because of circular header dependencies
template <typename UnderlyingUse>
struct GenericUseHolder : UseHolderBase {
  using held_use_t = managing_ptr<
    std::unique_ptr<UnderlyingUse>, HandleUseBase*
  >;

  held_use_t use;

  GenericUseHolder(GenericUseHolder&&) = delete;
  GenericUseHolder(GenericUseHolder const &) = delete;

  explicit
  GenericUseHolder(UnderlyingUse&& in_use)
    : use(use_base, std::make_unique<UnderlyingUse>(std::move(in_use)))
  {
    do_register();
  }

  void do_register() {
    if(not is_registered) { // for now
      // TODO remove this hack if statement once I remove the superfluous do_register() calls elsewhere
      assert(!is_registered);
      abstract::backend::get_backend_runtime()->register_use(use_base);
      is_registered = true;
    }
  }

  void replace_use(UnderlyingUse&& new_use, bool should_do_register) {
    HandleUseBase* swapped_use_base = nullptr;
    held_use_t new_use_wrapper(
      swapped_use_base,
      std::make_unique<UnderlyingUse>(std::move(new_use))
    );
    swap(use, new_use_wrapper);
    bool old_is_registered = is_registered;
    is_registered = false;
    if(should_do_register) do_register();
    if(old_is_registered) {
      abstract::backend::get_backend_runtime()->release_use(swapped_use_base);
    }
  }

  void do_release() {
    assert(is_registered);
    abstract::backend::get_backend_runtime()->release_use(use_base);
    is_registered = false;
  }

#if _darma_has_feature(task_migration)
  GenericUseHolder(migrated_use_arg_t, UnderlyingUse&& in_use)
    : use(use_base, std::make_unique<UnderlyingUse>(std::move(in_use)))
  {
    abstract::backend::get_backend_runtime()->reregister_migrated_use(use_base);
    is_registered = true;
  }
#endif

  ~GenericUseHolder() {
    if(could_be_alias) {
      // okay, now we know it IS an alias
      abstract::backend::get_backend_runtime()->establish_flow_alias(
        *(use->in_flow_.get()),
        *(use->out_flow_.get())
      );
    }
    if(is_registered) do_release();
  }
};

using UseHolder = GenericUseHolder<HandleUse>;

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_USE_H
