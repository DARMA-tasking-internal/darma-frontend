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

#include <darma_types.h>

#include <darma/interface/frontend/use.h>
#include <darma/interface/backend/flow.h>
#include <darma/impl/handle.h>
#include <darma/impl/flow_handling.h>
#include <darma/impl/index_range/mapping.h>

namespace darma_runtime {

namespace detail {

class HandleUseBase
  : public abstract::frontend::Use
{
  public:

    void* data_ = nullptr;

    std::shared_ptr<VariableHandleBase> handle_ = nullptr;

    flow_ptr in_flow_;
    flow_ptr out_flow_;

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

template <typename IndexRangeT>
class CollectionManagingUseBase
  : public HandleUseBase,
    public abstract::frontend::UseCollection
{
  public:

    IndexRangeT index_range;

    bool manages_collection() const override {
      return true;
    }

    abstract::frontend::UseCollection*
    get_managed_collection() override {
      return this;
    }

};

template <typename IndexRangeT, typename MappingToTaskCollection=NullMapping>
class CollectionManagingUse
  : public CollectionManagingUseBase<IndexRangeT>
{
  private:

    template <typename T>
    using index_iterable = abstract::frontend::UseCollection::index_iterable<T>;

  public:

    MappingToTaskCollection mapping;

    CollectionManagingUse(
      std::shared_ptr<VariableHandleBase> handle,
      flow_ptr const& in_flow,
      flow_ptr const& out_flow,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      IndexRangeT&& range
    ) : HandleUseBase(handle, in_flow, out_flow, scheduling_permissions, HandleUse::None),
        index_range(std::forward<IndexRangeT>(range))
    { }

    CollectionManagingUse(
      std::shared_ptr<VariableHandleBase> handle,
      flow_ptr const& in_flow,
      flow_ptr const& out_flow,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      IndexRangeT&& range, MappingToTaskCollection&& mapping
    ) : HandleUseBase(handle, in_flow, out_flow, scheduling_permissions, HandleUse::None),
        index_range(std::forward<IndexRangeT>(range)),
        mapping(std::forward<MappingToTaskCollection>(mapping))
    { }



    std::size_t size() const override { return index_range.size(); }

    index_iterable<std::size_t>
    local_indices_for(std::size_t backend_task_collection_index) const override {
      // Only one-to-one for now
      // TODO more than just one-to-one mapping
      return index_iterable<std::size_t>{
        mapping.map_backward(backend_task_collection_index)
      };
    }

    bool
    has_same_mapping_as(
      abstract::frontend::UseCollection const* other
    ) const override {
      // TODO implement this
      return false;
    }

};


struct migrated_use_arg_t { };
static constexpr migrated_use_arg_t migrated_use_arg = { };

// really belongs to AccessHandle, but we can't put this in impl/handle.h
// because of circular header dependencies
template <typename UnderlyingUse>
struct GenericUseHolder {
  UnderlyingUse use;
  bool is_registered = false;
  bool could_be_alias = false;
  bool captured_but_not_handled = false;

  GenericUseHolder(GenericUseHolder&&) = delete;
  GenericUseHolder(GenericUseHolder const &) = delete;

  explicit
  GenericUseHolder(UnderlyingUse&& in_use)
    : use(std::move(in_use))
  { }

  void do_register() {
    assert(!is_registered);
    abstract::backend::get_backend_runtime()->register_use(&use);
    is_registered = true;
  }

  void do_release() {
    assert(is_registered);
    abstract::backend::get_backend_runtime()->release_use(&use);
    is_registered = false;
  }

  GenericUseHolder(migrated_use_arg_t const&, UnderlyingUse&& in_use) : use(std::move(in_use)) {
    abstract::backend::get_backend_runtime()->reregister_migrated_use(&use);
    is_registered = true;
  }

  ~GenericUseHolder() {
    if(is_registered) do_release();
    if(could_be_alias) {
      // okay, now we know it IS an alias
      abstract::backend::get_backend_runtime()->establish_flow_alias(
        *(use.in_flow_.get()),
        *(use.out_flow_.get())
      );
    }
  }
};

using UseHolder = GenericUseHolder<HandleUse>;
template <typename... Args>
using UseCollectionManagingHolder = GenericUseHolder<CollectionManagingUse<Args...>>;

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_USE_H
