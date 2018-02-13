/*
//@HEADER
// ************************************************************************
//
//                      handle_use_base.h.h
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
    public abstract::frontend::UsePendingRelease,
    public abstract::frontend::PolymorphicSerializableObject<HandleUseBase>
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

    // for use with commutative regions; might not be needed any more?
    std::unique_ptr<types::flow_t> suspended_out_flow_ = nullptr;

    frontend::permissions_t immediate_permissions_ = frontend::Permissions::None;
    frontend::permissions_t scheduling_permissions_ = frontend::Permissions::None;
    frontend::coherence_mode_t coherence_mode_ = frontend::CoherenceMode::Sequential;

    FlowRelationshipImpl in_flow_rel_;
    FlowRelationshipImpl out_flow_rel_;
    FlowRelationshipImpl in_anti_flow_rel_;
    FlowRelationshipImpl out_anti_flow_rel_;

    // For now, at least, is_dependency and is_anti_dependency depend only
    // on the permissions, so the returns are just computed on-the-fly
    //bool is_dependency_ = false;
    //bool is_anti_dependency_ = false;

    bool establishes_alias_ = false;

    // </editor-fold> end Data members }}}1
    //==========================================================================


    //==========================================================================
    // <editor-fold desc="abstract::frontend::Use implementation">

    std::shared_ptr<abstract::frontend::Handle const>
    get_handle() const final {
      return handle_;
    }

    types::flow_t&
    get_in_flow() final {
      return in_flow_;
    }

    types::flow_t&
    get_out_flow() final {
      return out_flow_;
    }

    types::anti_flow_t&
    get_anti_in_flow() final {
      return anti_in_flow_;
    }

    types::anti_flow_t&
    get_anti_out_flow() final {
      return anti_out_flow_;
    }

    frontend::permissions_t
    immediate_permissions() const final {
      return immediate_permissions_;
    }

    frontend::permissions_t
    scheduling_permissions() const final {
      return scheduling_permissions_;
    }

    frontend::coherence_mode_t
    coherence_mode() const final {
      return coherence_mode_;
    }

    void*&
    get_data_pointer_reference() final {
      return data_;
    }

#if _darma_has_feature(create_concurrent_work_owned_by)
    std::size_t
    task_collection_owning_index() const override {
      return collection_owner_;
    }
#endif // _darma_has_feature(create_concurrent_work_owned_by)

    FlowRelationshipImpl const& get_in_flow_relationship() const final {
      return in_flow_rel_;
    }
    FlowRelationshipImpl const& get_out_flow_relationship() const final {
      return out_flow_rel_;
    }

    FlowRelationshipImpl const& get_anti_in_flow_relationship() const final {
      return in_anti_flow_rel_;
    }
    FlowRelationshipImpl const& get_anti_out_flow_relationship() const final {
      return out_anti_flow_rel_;
    }

    bool is_dependency() const final {
      // For now, at least, is_dependency and is_anti_dependency depend only
      // on the permissions, so just put that in here directly
      return (int(immediate_permissions_) & int(frontend::Permissions::Read)) != 0;
    }

    bool is_anti_dependency() const final {
      // For now, at least, is_dependency and is_anti_dependency depend only
      // on the permissions, so just put that in here directly
      return (int(immediate_permissions_) & int(frontend::Permissions::Write)) != 0;
    }

    void set_in_flow(types::flow_t const& new_flow) final {
      // TODO assert that the flow is allowed to be set at this point
      in_flow_ = new_flow;
    }

    void set_out_flow(types::flow_t const& new_flow) final {
      // TODO assert that the flow is allowed to be set at this point
      out_flow_ = new_flow;
    }

    void set_anti_in_flow(types::anti_flow_t const& new_flow) final {
      // TODO assert that the flow is allowed to be set at this point
      anti_in_flow_ = new_flow;
    }

    void set_anti_out_flow(types::anti_flow_t const& new_flow) final {
      // TODO assert that the flow is allowed to be set at this point
      anti_out_flow_ = new_flow;
    }

    bool establishes_alias() const final { return establishes_alias_; }

    // </editor-fold>
    //==========================================================================

    //==========================================================================
    // <editor-fold desc="Ctors and destructor"> {{{1

    /**
     * @internal
     * @brief Initial access (or other non-cloned Use) constructor
     */
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

    /**
     * @internal
     * @brief Cloning constructor
     */
    HandleUseBase(
      HandleUseBase const& clone_from,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      FlowRelationshipImpl&& in_rel,
      FlowRelationshipImpl&& out_rel,
      FlowRelationshipImpl&& anti_in_rel,
      FlowRelationshipImpl&& anti_out_rel,
      frontend::coherence_mode_t coherence_mode
    ) : handle_(clone_from.handle_),
        immediate_permissions_(immediate_permissions),
        scheduling_permissions_(scheduling_permissions),
        in_flow_rel_(std::move(in_rel)),
        out_flow_rel_(std::move(out_rel)),
        in_anti_flow_rel_(std::move(anti_in_rel)),
        out_anti_flow_rel_(std::move(anti_out_rel)),
        coherence_mode_(coherence_mode)
    { }

    /**
     * @internal
     * @brief Pass through to the cloning constructor with coherence mode from
     *         the clone when coherence mode isn't given.
     */
    HandleUseBase(
      HandleUseBase const& clone_from,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      FlowRelationshipImpl&& in_rel,
      FlowRelationshipImpl&& out_rel,
      FlowRelationshipImpl&& anti_in_rel,
      FlowRelationshipImpl&& anti_out_rel
    ) : HandleUseBase(
          clone_from, scheduling_permissions, immediate_permissions,
          std::move(in_rel), std::move(out_rel),
          std::move(anti_in_rel), std::move(anti_out_rel),
          /* if coherence_mode isn't given, get it from the source */
          clone_from.coherence_mode_
        )
    { /* forwarding ctor, must be empty */ }

    /**
     * @internal
     * @brief Unpacking constructor
     *
     * This is the only constructor that uses flow objects directly instead of
     * `FlowRelationship` objects.  Ideally, this will change in the future
     * (for uniformity reasons) with some changes to the backend API.
     */
    HandleUseBase(
      std::shared_ptr<VariableHandleBase> const& handle,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions,
      types::flow_t&& in_flow,
      types::flow_t&& out_flow,
      types::anti_flow_t&& anti_in_flow,
      types::anti_flow_t&& anti_out_flow,
      frontend::CoherenceMode coherence_mode
    ) : handle_(handle),
        immediate_permissions_(immediate_permissions),
        scheduling_permissions_(scheduling_permissions),
        in_flow_(std::move(in_flow)),
        out_flow_(std::move(out_flow)),
        anti_in_flow_(std::move(anti_in_flow)),
        anti_out_flow_(std::move(anti_out_flow)),
        coherence_mode_(coherence_mode)
    { }

    HandleUseBase() = default;
    HandleUseBase(HandleUseBase const&) = delete;
    HandleUseBase(HandleUseBase&&) = default;
    HandleUseBase& operator=(HandleUseBase const&) = delete;
    HandleUseBase& operator=(HandleUseBase&&) = default;

    virtual ~HandleUseBase() = default;

    // </editor-fold> end  }}}1
    //==========================================================================

    template <typename Archive>
    void do_serialize(Archive& ar) {
      ar | scheduling_permissions_ | immediate_permissions_ | coherence_mode_
         | in_flow_ | out_flow_ | anti_in_flow_ | anti_out_flow_;
    }

    void set_handle(std::shared_ptr<VariableHandleBase> const& handle) {
      handle_ = handle;
    }

};


} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_HANDLE_USE_BASE_H_H
