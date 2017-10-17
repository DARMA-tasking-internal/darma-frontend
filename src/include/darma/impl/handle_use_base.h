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
#include <darma/impl/use/flow_relationship.h>

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

    //==========================================================================
    // <editor-fold desc="Data members"> {{{1

    void* data_ = nullptr;

    bool already_captured = false;

    std::shared_ptr<VariableHandleBase> handle_ = nullptr;

    types::flow_t in_flow_;
    types::flow_t out_flow_;
    types::anti_flow_t anti_in_flow_;
    types::anti_flow_t anti_out_flow_;

    // TODO this shouldn't be here...
    std::size_t collection_owner_ = std::numeric_limits<std::size_t>::max();

    // for use with commutative regions; might not be needed any more?
    std::unique_ptr<types::flow_t> suspended_out_flow_ = nullptr;

    frontend::permissions_t immediate_permissions_ = frontend::Permissions::None;
    frontend::permissions_t scheduling_permissions_ = frontend::Permissions::None;
    frontend::coherence_mode_t coherence_mode_ = frontend::CoherenceMode::Sequential;

    FlowRelationshipImpl in_flow_rel_;
    FlowRelationshipImpl out_flow_rel_;
    FlowRelationshipImpl in_anti_flow_rel_;
    FlowRelationshipImpl out_anti_flow_rel_;

    bool is_dependency_ = false;
    bool is_anti_dependency_ = false;
    bool establishes_alias_ = false;

    // </editor-fold> end Data members }}}1
    //==========================================================================


    //==========================================================================
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

    types::anti_flow_t&
    get_anti_in_flow() override {
      return anti_in_flow_;
    }

    types::anti_flow_t&
    get_anti_out_flow() override {
      return anti_out_flow_;
    }

    frontend::permissions_t
    immediate_permissions() const override {
      return immediate_permissions_;
    }

    frontend::permissions_t
    scheduling_permissions() const override {
      return scheduling_permissions_;
    }

    frontend::coherence_mode_t
    coherence_mode() const override {
      return coherence_mode_;
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

    FlowRelationshipImpl const& get_anti_in_flow_relationship() const override {
      return in_anti_flow_rel_;
    }
    FlowRelationshipImpl const& get_anti_out_flow_relationship() const override {
      return out_anti_flow_rel_;
    }

    bool is_dependency() const override { return is_dependency_; }

    bool is_anti_dependency() const override { return is_anti_dependency_; }

    void set_in_flow(types::flow_t const& new_flow) override {
      // TODO assert that the flow is allowed to be set at this point
      in_flow_ = new_flow;
    }

    void set_out_flow(types::flow_t const& new_flow) override {
      // TODO assert that the flow is allowed to be set at this point
      out_flow_ = new_flow;
    }

    void set_anti_in_flow(types::anti_flow_t const& new_flow) override {
      // TODO assert that the flow is allowed to be set at this point
      anti_in_flow_ = new_flow;
    }

    void set_anti_out_flow(types::anti_flow_t const& new_flow) override {
      // TODO assert that the flow is allowed to be set at this point
      anti_out_flow_ = new_flow;
    }

    bool establishes_alias() const override { return establishes_alias_; }

    // </editor-fold>
    //==========================================================================

    HandleUseBase(
      std::shared_ptr<VariableHandleBase> const& handle,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      FlowRelationshipImpl&& in_rel,
      FlowRelationshipImpl&& out_rel,
      FlowRelationshipImpl&& anti_in_rel,
      FlowRelationshipImpl&& anti_out_rel,
      frontend::coherence_mode_t coherence_mode = frontend::CoherenceMode::Sequential
    ) : handle_(handle),
        immediate_permissions_(immediate_permissions),
        scheduling_permissions_(scheduling_permissions),
        in_flow_rel_(std::move(in_rel)),
        out_flow_rel_(std::move(out_rel)),
        in_anti_flow_rel_(std::move(anti_in_rel)),
        out_anti_flow_rel_(std::move(anti_out_rel)),
        coherence_mode_(coherence_mode)
    { }


    // Should only be used by unpack at this point
    HandleUseBase(
      std::shared_ptr<VariableHandleBase> const& handle,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      types::flow_t&& in_flow,
      types::flow_t&& out_flow,
      types::anti_flow_t&& anti_in_flow,
      types::anti_flow_t&& anti_out_flow
    ) : handle_(handle),
        immediate_permissions_(immediate_permissions),
        scheduling_permissions_(scheduling_permissions),
        in_flow_(std::move(in_flow)),
        out_flow_(std::move(out_flow)),
        anti_in_flow_(std::move(anti_in_flow)),
        anti_out_flow_(std::move(anti_out_flow))
    { }

    HandleUseBase() = default;
    HandleUseBase(HandleUseBase const&) = delete;
    HandleUseBase(HandleUseBase&&) = default;
    HandleUseBase& operator=(HandleUseBase const&) = delete;
    HandleUseBase& operator=(HandleUseBase&&) = default;

    virtual ~HandleUseBase() = default;

};


} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_HANDLE_USE_BASE_H_H
