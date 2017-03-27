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
  : public abstract::frontend::Use
{
  public:

    void* data_ = nullptr;

    bool already_captured = false;

    std::shared_ptr<VariableHandleBase> handle_ = nullptr;

    flow_ptr in_flow_;
    flow_ptr out_flow_;
    std::size_t collection_owner_ = std::numeric_limits<std::size_t>::max();

    flow_ptr suspended_out_flow_ = nullptr; // for use with commutative regions

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

    virtual ~HandleUseBase() = default;

};

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_HANDLE_USE_BASE_H_H
