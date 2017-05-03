/*
//@HEADER
// ************************************************************************
//
//                      handle_use_base.h.h
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

#ifndef DARMA_IMPL_HANDLE_USE_BASE_H_H
#define DARMA_IMPL_HANDLE_USE_BASE_H_H

#include <limits> // std::numeric_limits
#include <memory> // std::shared_ptr

#include <darma_types.h> // types::flow_t

#include <darma/interface/frontend/use.h> // abstract::frontend::Use

#include <darma/impl/handle.h> // VariableHandleBase
#include <darma/impl/flow_handling.h> // flow_ptr
#include <darma/impl/index_range/mapping.h>
#include <darma/impl/index_range/mapping_traits.h>
#include <darma/impl/index_range/index_range_traits.h>
#include <darma/impl/util/managing_ptr.h>

namespace darma_runtime {
namespace detail {

class HandleUseBase
  : public abstract::frontend::DestructibleUse,
    public abstract::frontend::CollectionManagingUse,
    public abstract::frontend::UsePendingRegistration,
    public abstract::frontend::UsePendingRelease
#if _darma_has_feature(create_concurrent_work_owned_by)
    , public abstract::frontend::UniquelyOwnedUse
#endif //_darma_has_feature(create_concurrent_work_owned_by)
{
  public:

    //==============================================================================
    // <editor-fold desc="FlowRelationshipImpl"> {{{1

    struct FlowRelationshipImpl: abstract::frontend::FlowRelationship
    {
      flow_relationship_description_t description_
#if _darma_has_feature(anti_flows)
        = abstract::frontend::FlowRelationship::Insignificant;
#else
      ;
#endif // _darma_has_feature(anti_flows)
      types::flow_t* related_ = nullptr;
      bool related_is_in_ = false;
      types::key_t const* version_key_ = nullptr;
      std::size_t index_ = 0;

#if _darma_has_feature(anti_flows)
      types::anti_flow_t* anti_related_ = nullptr;
      bool related_is_anti_in_ = false;
#endif // _darma_has_feature(anti_flows)

      FlowRelationshipImpl() = default;
      FlowRelationshipImpl(FlowRelationshipImpl&&) = default;
      FlowRelationshipImpl&
      operator=(FlowRelationshipImpl&&) = default;

      FlowRelationshipImpl(
        flow_relationship_description_t out_desc,
        types::flow_t* rel,
        bool rel_is_in = false,
        types::key_t const* version_key = nullptr,
        std::size_t index = 0
#if _darma_has_feature(anti_flows)
        , types::anti_flow_t* anti_rel = nullptr,
        bool anti_rel_is_in = false
#endif // _darma_has_feature(anti_flows)
      ) : description_(out_desc),
          related_(rel),
          related_is_in_(rel_is_in),
          version_key_(version_key),
          index_(index)
#if _darma_has_feature(anti_flows)
          , anti_related_(anti_rel),
          related_is_anti_in_(anti_rel_is_in)
#endif // _darma_has_feature(anti_flows)
      { }

      FlowRelationshipImpl
      as_collection_relationship() const {
        return FlowRelationshipImpl(
          description_ | abstract::frontend::FlowRelationship::Collection,
          related_, related_is_in_, version_key_, index_
#if _darma_has_feature(anti_flows)
          , anti_related_, related_is_anti_in_
#endif // _darma_has_feature(anti_flows)
        );
      }


      flow_relationship_description_t description() const override
      {
        return description_;
      }

      types::flow_t*
      related_flow() const override { return related_; }

#if _darma_has_feature(anti_flows)
      types::anti_flow_t*
      related_anti_flow() const override { return anti_related_; }
#endif // _darma_has_feature(anti_flows)


      types::key_t const* version_key() const override { return version_key_; }

      bool
      use_corresponding_in_flow_as_related() const override
      {
        return related_is_in_;
      }

#if _darma_has_feature(anti_flows)
      bool
      use_corresponding_in_flow_as_anti_related() const override
      {
        return related_is_anti_in_;
      }
#endif // _darma_has_feature(anti_flows)

      std::size_t index() const override { return index_; }

    };

    // </editor-fold> end FlowRelationshipImpl }}}1
    //==============================================================================


    //==========================================================================
    // <editor-fold desc="Data members"> {{{1

    void* data_ = nullptr;

    bool already_captured = false;

    std::shared_ptr<VariableHandleBase> handle_ = nullptr;

    types::flow_t in_flow_;
    types::flow_t out_flow_;
    std::size_t collection_owner_ = std::numeric_limits<std::size_t>::max();

#if _darma_has_feature(anti_flows)
    types::anti_flow_t anti_in_flow_;
    types::anti_flow_t anti_out_flow_;
#endif // _darma_has_feature(anti_flows)

    std::unique_ptr<types::flow_t>
      suspended_out_flow_ = nullptr; // for use with commutative regions

    abstract::frontend::Use::permissions_t immediate_permissions_ = None;
    abstract::frontend::Use::permissions_t scheduling_permissions_ = None;

    FlowRelationshipImpl in_flow_rel_;
    FlowRelationshipImpl out_flow_rel_;
#if _darma_has_feature(anti_flows)
    FlowRelationshipImpl in_anti_flow_rel_;
    FlowRelationshipImpl out_anti_flow_rel_;
#endif // _darma_has_feature(anti_flows)
    bool establishes_alias_ = false;

    bool is_dependency_ = false;

    // </editor-fold> end Data members }}}1
    //==========================================================================

    ////////////////////////////////////////
    // <editor-fold desc="abstract::frontend::Use implementation">

    std::shared_ptr<abstract::frontend::Handle const>
    get_handle() const override {
      return handle_;
    }

    types::flow_t&
    get_in_flow() override {
      return in_flow_;
    }

    types::flow_t&
    get_out_flow() override {
      return out_flow_;
    }

#if _darma_has_feature(anti_flows)
    types::anti_flow_t&
    get_anti_in_flow() override {
      return anti_in_flow_;
    }

    types::anti_flow_t&
    get_anti_out_flow() override {
      return anti_out_flow_;
    }

#endif // _darma_has_feature(anti_flows)

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

#if _darma_has_feature(create_concurrent_work_owned_by)
    std::size_t
    task_collection_owning_index() const override {
      return collection_owner_;
    }
#endif // _darma_has_feature(create_concurrent_work_owned_by)

    FlowRelationshipImpl const& get_in_flow_relationship() const override {
      return in_flow_rel_;
    }
    FlowRelationshipImpl const& get_out_flow_relationship() const override {
      return out_flow_rel_;
    }

#if _darma_has_feature(anti_flows)
    FlowRelationshipImpl const& get_anti_in_flow_relationship() const override {
      return in_anti_flow_rel_;
    }
    FlowRelationshipImpl const& get_anti_out_flow_relationship() const override {
      return out_anti_flow_rel_;
    }
#endif // _darma_has_feature(anti_flows)

    void set_in_flow(types::flow_t const& new_flow) override {
      // TODO assert that the flow is allowed to be set at this point
      in_flow_ = new_flow;
    }

    void set_out_flow(types::flow_t const& new_flow) override {
      // TODO assert that the flow is allowed to be set at this point
      out_flow_ = new_flow;
    }

#if _darma_has_feature(anti_flows)
    void set_anti_in_flow(types::anti_flow_t const& new_flow) override {
      // TODO assert that the flow is allowed to be set at this point
      anti_in_flow_ = new_flow;
    }

    void set_anti_out_flow(types::anti_flow_t const& new_flow) override {
      // TODO assert that the flow is allowed to be set at this point
      anti_out_flow_ = new_flow;
    }
#endif // _darma_has_feature(anti_flows)

    bool will_be_dependency() const override { return is_dependency_; }

    bool was_dependency() const override { return is_dependency_; }


    bool establishes_alias() const override { return establishes_alias_; }

    // </editor-fold>
    ////////////////////////////////////////

    // TODO anti-flow ser/des

//    HandleUseBase(
//      std::shared_ptr<VariableHandleBase> handle,
//      abstract::frontend::Use::permissions_t scheduling_permissions,
//      abstract::frontend::Use::permissions_t immediate_permissions,
//      abstract::frontend::FlowRelationship::flow_relationship_description_t in_desc,
//      types::flow_t* in_rel,
//      abstract::frontend::FlowRelationship::flow_relationship_description_t out_desc,
//      types::flow_t* out_rel,
//      bool out_rel_is_in = false,
//      types::key_t* in_version_key = nullptr,
//      std::size_t in_index = 0,
//      types::key_t* out_version_key = nullptr,
//      std::size_t out_index = 0
//    ) : handle_(handle),
//        immediate_permissions_(immediate_permissions),
//        scheduling_permissions_(scheduling_permissions),
//        in_flow_rel_(
//          in_desc, in_rel, false, in_version_key, in_index
//        ),
//        out_flow_rel_(
//          out_desc, out_rel, out_rel_is_in, out_version_key, out_index
//        )
//    { }

    HandleUseBase(
      std::shared_ptr<VariableHandleBase> handle,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      FlowRelationshipImpl&& in_rel,
      FlowRelationshipImpl&& out_rel
    ) : handle_(handle),
        immediate_permissions_(immediate_permissions),
        scheduling_permissions_(scheduling_permissions),
        in_flow_rel_(std::move(in_rel)),
        out_flow_rel_(std::move(out_rel))
#if _darma_has_feature(anti_flows)
        , in_anti_flow_rel_(), // i.e., insignificant
          out_anti_flow_rel_() // i.e., insignificant
#endif // _darma_has_feature(anti_flows)
    { }

#if _darma_has_feature(anti_flows)
    HandleUseBase(
      std::shared_ptr<VariableHandleBase> handle,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      FlowRelationshipImpl&& in_rel,
      FlowRelationshipImpl&& out_rel,
      FlowRelationshipImpl&& anti_in_rel,
      FlowRelationshipImpl&& anti_out_rel
    ) : handle_(handle),
        immediate_permissions_(immediate_permissions),
        scheduling_permissions_(scheduling_permissions),
        in_flow_rel_(std::move(in_rel)),
        out_flow_rel_(std::move(out_rel)),
        in_anti_flow_rel_(std::move(anti_in_rel)), // i.e., insignificant
        out_anti_flow_rel_(std::move(anti_out_rel)) // i.e., insignificant
    { }
#endif // _darma_has_feature(anti_flows)


    // Should only be used by unpack at this point
    HandleUseBase(
      std::shared_ptr<VariableHandleBase> handle,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      types::flow_t&& in_flow,
      types::flow_t&& out_flow
    ) : handle_(handle),
        immediate_permissions_(immediate_permissions),
        scheduling_permissions_(scheduling_permissions),
        in_flow_(std::move(in_flow)),
        out_flow_(std::move(out_flow))
    { }


    HandleUseBase() = default;
    HandleUseBase(HandleUseBase const&) = delete;
    HandleUseBase(HandleUseBase&&) = default;
    HandleUseBase& operator=(HandleUseBase const&) = delete;
    HandleUseBase& operator=(HandleUseBase&&) = default;

    virtual ~HandleUseBase() = default;

};

namespace flow_relationships {

inline HandleUseBase::FlowRelationshipImpl
initial_flow() {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Initial,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0
#if _darma_has_feature(anti_flows)
    , /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
#endif // _darma_has_feature(anti_flows)
  );
}

inline HandleUseBase::FlowRelationshipImpl
null_flow() {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Null,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0
#if _darma_has_feature(anti_flows)
    , /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
#endif // _darma_has_feature(anti_flows)
  );
}

inline HandleUseBase::FlowRelationshipImpl
same_flow(types::flow_t* rel) {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Same,
    /* related flow = */ rel,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0
#if _darma_has_feature(anti_flows)
    , /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
#endif // _darma_has_feature(anti_flows)
  );
}

inline HandleUseBase::FlowRelationshipImpl
same_flow_as_in() {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Same,
    /* related flow = */ nullptr,
    /* related_is_in = */ true,
    /* version key = */ nullptr,
    /* index = */ 0
#if _darma_has_feature(anti_flows)
    , /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
#endif // _darma_has_feature(anti_flows)
  );
}

#if _darma_has_feature(anti_flows)
inline HandleUseBase::FlowRelationshipImpl
same_anti_flow(types::anti_flow_t* rel) {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Same,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ rel,
    /* anti_rel_is_in = */ false
  );
}

inline HandleUseBase::FlowRelationshipImpl
initial_anti_flow() {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Initial,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}

inline HandleUseBase::FlowRelationshipImpl
anti_next_of_in_flow() {
return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Next,
    /* related flow = */ nullptr,
    /* related_is_in = */ true,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}
#endif // _darma_has_feature(anti_flows)

inline HandleUseBase::FlowRelationshipImpl
next_flow(types::flow_t* rel) {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Next,
    /* related flow = */ rel,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0
#if _darma_has_feature(anti_flows)
    , /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
#endif // _darma_has_feature(anti_flows)
  );
}

inline HandleUseBase::FlowRelationshipImpl
next_of_in_flow() {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Next,
    /* related flow = */ nullptr,
    /* related_is_in = */ true,
    /* version key = */ nullptr,
    /* index = */ 0
#if _darma_has_feature(anti_flows)
    , /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
#endif // _darma_has_feature(anti_flows)
  );
}

inline HandleUseBase::FlowRelationshipImpl
forwarding_flow(types::flow_t* rel) {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Forwarding,
    /* related flow = */ rel,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0
#if _darma_has_feature(anti_flows)
    , /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
#endif // _darma_has_feature(anti_flows)
  );
}

#if _darma_has_feature(anti_flows)
inline HandleUseBase::FlowRelationshipImpl
insignificant_flow() {
  // The default constructed flow relationship is the insignificant flow
  return HandleUseBase::FlowRelationshipImpl();
}
#endif // _darma_has_feature(anti_flows)

inline HandleUseBase::FlowRelationshipImpl
indexed_fetching_flow(
  types::flow_t* rel,
  types::key_t const* version_key,
  std::size_t backend_index
) {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::IndexedFetching,
    /* related flow = */ rel,
    /* related_is_in = */ false,
    /* version key = */ version_key,
    /* index = */ backend_index
#if _darma_has_feature(anti_flows)
    , /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
#endif // _darma_has_feature(anti_flows)
  );
}

#if _darma_has_feature(anti_flows)
inline HandleUseBase::FlowRelationshipImpl
indexed_fetching_anti_flow(
  types::flow_t* rel,
  types::key_t const* version_key,
  std::size_t backend_index
) {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::AntiIndexedFetching,
    /* related flow = */ rel,
    /* related_is_in = */ false,
    /* version key = */ version_key,
    /* index = */ backend_index,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}
#endif // _darma_has_feature(anti_flows)

inline HandleUseBase::FlowRelationshipImpl
indexed_local_flow(
  types::flow_t* rel,
  std::size_t backend_index
) {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::IndexedLocal,
    /* related flow = */ rel,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ backend_index
#if _darma_has_feature(anti_flows)
    , /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
#endif // _darma_has_feature(anti_flows)
  );
}

#if _darma_has_feature(anti_flows)
inline HandleUseBase::FlowRelationshipImpl
indexed_local_anti_flow(
  types::anti_flow_t* rel,
  std::size_t backend_index
) {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::AntiIndexedLocal,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ backend_index,
    /* anti_related = */ rel,
    /* anti_rel_is_in = */ false
  );
}
#endif // _darma_has_feature(anti_flows)

inline HandleUseBase::FlowRelationshipImpl
indexed_flow(
  types::flow_t* rel,
  std::size_t backend_index
) {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Indexed,
    /* related flow = */ rel,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ backend_index
#if _darma_has_feature(anti_flows)
    , /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
#endif // _darma_has_feature(anti_flows)
  );
}

#if _darma_has_feature(anti_flows)
inline HandleUseBase::FlowRelationshipImpl
indexed_anti_flow(
  types::anti_flow_t* rel,
  std::size_t backend_index
) {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Indexed,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ backend_index,
    /* anti_related = */ rel,
    /* anti_rel_is_in = */ false
  );
}

inline HandleUseBase::FlowRelationshipImpl
forwarding_anti_flow(
  types::anti_flow_t* rel
) {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::AntiForwarding,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ rel,
    /* anti_rel_is_in = */ false
  );
}

inline HandleUseBase::FlowRelationshipImpl
next_anti_flow(
  types::anti_flow_t* rel
) {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Next,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ rel,
    /* anti_rel_is_in = */ false
  );
}

inline HandleUseBase::FlowRelationshipImpl
next_anti_flow_of_anti_in() {
  return HandleUseBase::FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Next,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ true
  );
}
#endif

} // end namespace flow_relationships

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_HANDLE_USE_BASE_H_H
