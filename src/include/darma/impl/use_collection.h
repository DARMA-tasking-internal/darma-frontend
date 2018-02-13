/*
//@HEADER
// ************************************************************************
//
//                      use_collection.h
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

#ifndef DARMA_IMPL_USE_COLLECTION_H
#define DARMA_IMPL_USE_COLLECTION_H

#include <darma/interface/frontend/use_collection.h> // abstract::frontend::UseCollection

#include <darma/impl/index_range/index_range_traits.h>
#include <darma/impl/index_range/mapping_traits.h>

#include <darma/impl/flow_handling.h> // flow_ptr
#include <darma/impl/handle_use_base.h> // HandleUseBase

namespace darma_runtime {

namespace detail {

static constexpr struct in_place_ctor_tag_t { } in_place_ctor_tag { };
static constexpr struct cloning_ctor_tag_t { } cloning_ctor_tag { };

template <typename IndexRange>
class BasicUseCollection
  : public abstract::frontend::UseCollection,
    public abstract::frontend::PolymorphicSerializableObject<BasicUseCollection<IndexRange>>
{
  public:  // All public for now, rather than dealing with making friend declarations for AccessHandleCollection and friends

    //--------------------------------------------------------------------------
    // <editor-fold desc="type aliases"> {{{2

    // This is the frontend handle collection's index range
    using index_range_t = IndexRange;
    using index_range_traits_t = indexing::index_range_traits<IndexRange>;
    using mapping_to_dense_t = typename index_range_traits_t::mapping_to_dense_type;
    using mapping_to_dense_traits_t = indexing::mapping_traits<mapping_to_dense_t>;


    // </editor-fold> end type aliases }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="data members"> {{{2

    // Don't change the order of these members; index_range_ is used in the
    // constructor of mapping_to_dense_

    // This is the frontend handle collection's index range
    index_range_t index_range_;

    mapping_to_dense_t mapping_to_dense_;

    // </editor-fold> end data members }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="ctors and destructor"> {{{2

    BasicUseCollection() = default;

    template <typename... IndexRangeArgs>
    explicit
    BasicUseCollection(
      in_place_ctor_tag_t,
      IndexRangeArgs&&... args
    ) : index_range_(
          std::forward<IndexRangeArgs>(args)...
        ),
        mapping_to_dense_(
          index_range_traits_t::mapping_to_dense(index_range_)
        )
    { }

    template <typename OtherBasicUseCollectionT>
    BasicUseCollection(
      cloning_ctor_tag_t,
      OtherBasicUseCollectionT const& clone_from
    ) : index_range_(clone_from.index_range_),
        mapping_to_dense_(
          index_range_traits_t::mapping_to_dense(index_range_)
        )
    { }

    virtual ~BasicUseCollection() = default;

    // </editor-fold> end ctors and destructor }}}2
    //--------------------------------------------------------------------------


  public:

    virtual std::unique_ptr<BasicUseCollection>
    clone() const = 0;

    //--------------------------------------------------------------------------
    // <editor-fold desc="abstract::frontend::UseCollection implementation"> {{{2

    size_t size() const final {
      return index_range_traits_t::size(index_range_);
    }

    // </editor-fold> end abstract::frontend::UseCollection implementation }}}2
    //--------------------------------------------------------------------------

    template <typename Archive>
    void serialize(Archive& ar) {
      ar | index_range_ | mapping_to_dense_;
    }

};



template <typename IndexRange>
class UnmappedUseCollection final /* final for now, at least */
  : public PolymorphicSerializationAdapter<
      UnmappedUseCollection<IndexRange>,
      BasicUseCollection<IndexRange>
    >
{
  public:

    using base_t = PolymorphicSerializationAdapter<
      UnmappedUseCollection<IndexRange>,
      BasicUseCollection<IndexRange>
    >;

    UnmappedUseCollection() = default;

    template <typename... IndexRangeArgs>
    explicit
    UnmappedUseCollection(
      in_place_ctor_tag_t,
      IndexRangeArgs&&... args
    ) : base_t(
          in_place_ctor_tag,
          std::forward<IndexRangeArgs>(args)...
        )
    { }

    template <typename CompatibleUnmappedCollectionT>
    explicit
    UnmappedUseCollection(
      cloning_ctor_tag_t,
      CompatibleUnmappedCollectionT const& clone_from
    ) : base_t(
          cloning_ctor_tag,
          clone_from
        )
    { }

    ~UnmappedUseCollection() final = default;


    std::unique_ptr<BasicUseCollection<IndexRange>>
    clone() const final {
      return std::make_unique<UnmappedUseCollection>(
        cloning_ctor_tag,
        *this
      );
    }

    //--------------------------------------------------------------------------
    // <editor-fold desc="abstract::frontend::UseCollection implementation"> {{{2

    bool is_mapped() const final { return false; }

    abstract::frontend::UseCollection::index_iterable<size_t>
    local_indices_for(size_t task_index) const final {
      assert(!"local_indices_for() called on unmapped UseCollection");
      return { }; // unreachable
    }

    size_t
    task_index_for(size_t collection_index) const final {
      assert(!"task_index_for() called on unmapped UseCollection");
      return 0; // unreachable
    }

    OptionalBoolean
    has_same_mapping_as(
      abstract::frontend::UseCollection const* other
    ) const final {
      assert(!"has_same_mapping_as() called on unmapped UseCollection");
      return OptionalBoolean::Unknown; // unreachable
    }

    // </editor-fold> end abstract::frontend::UseCollection implementation }}}2
    //--------------------------------------------------------------------------

};


template <typename... Args>
auto
make_unmapped_use_collection(Args&&... args) {
  return std::make_unique<UnmappedUseCollection<
    indexing::index_range_type_from_arguments_t<Args...>
  >>(
    in_place_ctor_tag,
    std::forward<Args>(args)...
  );
}


template <typename IndexRange, typename Mapping>
class MappedUseCollection final /* final for now, at least */
  : public PolymorphicSerializationAdapter<
      MappedUseCollection<IndexRange, Mapping>,
      BasicUseCollection<IndexRange>
    >
{

  public:

    //--------------------------------------------------------------------------
    // <editor-fold desc="type aliases and data members"> {{{2

    // This should be the full mapping from frontend handle collection index to
    // backend task collection index; by the time it gets here it has been
    // formed from a combination of the user's mapping and the mapping to dense
    // of the task collection
    using mapping_t = Mapping;
    mapping_t mapping_fe_handle_to_be_task_;

    using mapping_traits_t = darma_runtime::indexing::mapping_traits<Mapping>;
    using base_t = PolymorphicSerializationAdapter<
      MappedUseCollection<IndexRange, Mapping>,
      BasicUseCollection<IndexRange>
    >;

    // </editor-fold> end type aliases and data members }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="ctors and destructor"> {{{2

    MappedUseCollection() = default;

    template <
      typename UnmappedUseCollectionT,
      typename... MappingArgs
    >
    explicit
    MappedUseCollection(
      UnmappedUseCollectionT&& clone_from,
      in_place_ctor_tag_t,
      MappingArgs&&... mapping_args
    ) : base_t(
          cloning_ctor_tag,
          clone_from
        ),
        mapping_fe_handle_to_be_task_(
          std::forward<MappingArgs>(mapping_args)...
        )

    { }

    template <
      typename CompatibleMappedCollectionT
    >
    explicit
    MappedUseCollection(
      cloning_ctor_tag_t,
      CompatibleMappedCollectionT const& clone_from
    ) : base_t(
          cloning_ctor_tag,
          clone_from
        ),
        mapping_fe_handle_to_be_task_(
          clone_from.mapping_fe_handle_to_be_task_
        )
    { }

    ~MappedUseCollection() final = default;

    // </editor-fold> end ctors and destructor }}}2
    //--------------------------------------------------------------------------


    std::unique_ptr<BasicUseCollection<IndexRange>>
    clone() const final {
      return std::make_unique<MappedUseCollection>(
        cloning_ctor_tag,
        *this
      );
    }

    //--------------------------------------------------------------------------
    // <editor-fold desc="abstract::frontend::UseCollection implementation"> {{{2

    bool is_mapped() const override { return true; }

    abstract::frontend::UseCollection::index_iterable<size_t>
    local_indices_for(size_t backend_task_index) const override {
      // TODO cache this?
      // TODO short buffer optimization for the return type?
      // Map from backend task index to frontend handle index first:
      // TODO do this more generally, rather than using index_iterable?
      auto fe_handle_idxs = abstract::frontend::UseCollection::index_iterable<
        typename mapping_traits_t::from_index_type
      >{
        mapping_traits_t::map_backward(
          /* mapping= */ mapping_fe_handle_to_be_task_,
          backend_task_index,
          this->index_range_
        )
      };
      // Now map the frontend handle indices to dense, backend handle indices:
      abstract::frontend::UseCollection::index_iterable<std::size_t> rv;
      rv.reserve(fe_handle_idxs.size());
      for(auto&& fe_idx : fe_handle_idxs) {
        rv.push_back(
          base_t::mapping_to_dense_traits_t::map_forward(
            /* mapping= */ this->mapping_to_dense_,
            /* from= */ fe_idx,
            /* from_range= */ this->index_range_
          )
        );
      }
      return rv;
    }

    size_t
    task_index_for(size_t backend_handle_index) const override {
      // It's fine to make this a single element since a mapping_to_dense has
      // to be a bijection
      auto fe_handle_idx = base_t::mapping_to_dense_traits_t::map_backward(
        /* mapping= */ this->mapping_to_dense_,
        /* to= */ backend_handle_index,
        /* from_range= */ this->index_range_
      );
      // Now map the frontend handle index to the backend dense task collection
      // indices (this also has to be a surjection, at least for now, since
      // handle collection entries must be surjectively mapped onto task collection
      // indices
      return mapping_traits_t::map_forward(
        /* mapping= */ mapping_fe_handle_to_be_task_,
        /* from= */ fe_handle_idx,
        /* from_range= */ this->index_range_
      );
    }

    OptionalBoolean
    has_same_mapping_as(
      abstract::frontend::UseCollection const* other
    ) const override {
      auto* other_cast = utility::try_dynamic_cast<MappedUseCollection const*>(other);
      if(other_cast) {
        if(mapping_traits_t::known_same_mapping(
          mapping_fe_handle_to_be_task_, *other_cast
        )) {
          return OptionalBoolean::KnownTrue;
        }
        else {
          return OptionalBoolean::Unknown;
        }
      }
      else {
        return OptionalBoolean::Unknown;
      }
    }

    // </editor-fold> end abstract::frontend::UseCollection implementation }}}2
    //--------------------------------------------------------------------------

    template <typename Archive>
    void serialize(Archive& ar) {
      ar | mapping_fe_handle_to_be_task_;
      this->BasicUseCollection<IndexRange>::serialize(ar);
    }


};

template <typename UnmappedCollectionT, typename... Args>
auto
make_mapped_use_collection(UnmappedCollectionT&& clone_from, Args&&... args) {
  return std::make_unique<MappedUseCollection<
    typename std::decay_t<UnmappedCollectionT>::index_range_t,
    indexing::mapping_type_from_arguments_t<Args...>
  >>(
    std::forward<UnmappedCollectionT>(clone_from),
    in_place_ctor_tag,
    std::forward<Args>(args)...
  );
}


template <typename IndexRange>
class BasicCollectionManagingUse final /* final for now, at least */
  : public PolymorphicSerializationAdapter<
      BasicCollectionManagingUse<IndexRange>, HandleUseBase
    >
{

  public:

    using base_t = PolymorphicSerializationAdapter<
      BasicCollectionManagingUse<IndexRange>, HandleUseBase
    >;

    //--------------------------------------------------------------------------
    // <editor-fold desc="data members"> {{{2

    std::unique_ptr<BasicUseCollection<IndexRange>> collection_;

    // </editor-fold> end data members }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="Constructors and Destructor"> {{{2

    BasicCollectionManagingUse() = default;
    BasicCollectionManagingUse(BasicCollectionManagingUse const&) = delete;
    BasicCollectionManagingUse(BasicCollectionManagingUse&&) = default;

    /**
     * @internal
     * @brief non-cloning constructor (mostly for initial Uses)
     *
     * Passes through to the initial Use (or other non-cloned Use) constructor.
     */
    template <
      typename UseCollectionUniquePtr,
      typename... PassthroughArgs
    >
    BasicCollectionManagingUse(
      UseCollectionUniquePtr&& use_collection,
      std::shared_ptr<VariableHandleBase> const& handle,
      PassthroughArgs&&... passthrough_args
    ) : base_t(
          handle,
          std::forward<PassthroughArgs>(passthrough_args)...
        ),
        collection_(std::forward<UseCollectionUniquePtr>(use_collection))
    { }


    /**
     * @internal
     * @brief creates a cloned use with a different collection
     *
     * (Passes through the other arguments to the base cloning constructor)
     */
    template <
      typename UseCollectionUniquePtr,
      typename... PassthroughArgs
    >
    explicit
    BasicCollectionManagingUse(
      UseCollectionUniquePtr&& new_collection,
      BasicCollectionManagingUse const& clone_from,
      frontend::Permissions scheduling,
      frontend::Permissions immediate,
      FlowRelationshipImpl&& in_rel,
      FlowRelationshipImpl&& out_rel,
      FlowRelationshipImpl&& anti_in_rel,
      FlowRelationshipImpl&& anti_out_rel,
      PassthroughArgs&&... passthrough_args
    ) : base_t(
          clone_from, scheduling, immediate,
          std::move(in_rel).as_collection_relationship(),
          std::move(out_rel).as_collection_relationship(),
          std::move(anti_in_rel).as_collection_relationship(),
          std::move(anti_out_rel).as_collection_relationship(),
          std::forward<PassthroughArgs>(passthrough_args)...
        ),
        collection_(
          std::forward<UseCollectionUniquePtr>(new_collection)
        )
    { }

    /**
     * @internal
     * @brief cloning constructor.
     *
     * Uses a clone of the collection instance from `clone_from` and passes
     * through to the base cloning constructor
     */
    template <
      typename... PassthroughArgs
    >
    BasicCollectionManagingUse(
      BasicCollectionManagingUse const& clone_from,
      PassthroughArgs&&... passthrough_args
    ) : BasicCollectionManagingUse(
          clone_from.collection_->clone(), clone_from,
          std::forward<PassthroughArgs>(passthrough_args)...
        )
    { /* forwarding ctor, must be empty */}

    // This is for the purposes of reconstruction during migration
    // TODO put an unpack_ctor_tag or something here?!?
    BasicCollectionManagingUse(
      HandleUseBase&& arg
    ) : BasicCollectionManagingUse(
          std::move(*utility::safe_static_cast<BasicCollectionManagingUse*>(&arg))
        )
    { }

    virtual ~BasicCollectionManagingUse() final = default;

    // </editor-fold> end Constructors and Destructor }}}2
    //--------------------------------------------------------------------------


    /**
     * @internal
     * @warning Only call this before the Use is registered; otherwise it won't
     *          convey the information to the backend
     * @param new_collection
     */
    void set_collection(
      std::unique_ptr<BasicUseCollection<IndexRange>> new_collection
    ) {
      collection_ = std::move(new_collection);
    }

    //--------------------------------------------------------------------------
    // <editor-fold desc="abstract::frontend::Use implementation"> {{{2

    bool manages_collection() const final { return true; }

    BasicUseCollection<IndexRange>*
    get_managed_collection() final { return collection_.get(); }

    // </editor-fold> end abstract::frontend::Use implementation }}}2
    //--------------------------------------------------------------------------

    template <typename Archive>
    void compute_size(Archive& ar) const {
      ar.add_to_size_raw(collection_->get_packed_size());
      const_cast<BasicCollectionManagingUse*>(this)->HandleUseBase::do_serialize(ar);
    }

    template <typename Archive>
    void pack(Archive& ar) const {
      auto ptr_ar = serialization::PointerReferenceSerializationHandler<>::make_packing_archive_referencing(ar);
      collection_->pack(*reinterpret_cast<char**>(&ar.data_pointer_reference()));
      const_cast<BasicCollectionManagingUse*>(this)->HandleUseBase::do_serialize(ar);
    }

    template <typename Archive>
    static void unpack(void* allocated, Archive& ar) {
      auto* rv = new (allocated) BasicCollectionManagingUse();
      auto ptr_ar = serialization::PointerReferenceSerializationHandler<>::make_unpacking_archive_referencing(ar);
      rv->collection_ = abstract::frontend::PolymorphicSerializableObject<BasicUseCollection<IndexRange>>
        ::unpack(*reinterpret_cast<char const**>(&ar.data_pointer_reference()));
      rv->HandleUseBase::do_serialize(ar);
    }
};

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_USE_COLLECTION_H
