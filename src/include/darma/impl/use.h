/*
//@HEADER
// ************************************************************************
//
//                      use.h
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
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
#include <darma/impl/handle_use_base.h>
#include "polymorphic_serialization.h"

namespace darma_runtime {

namespace detail {

class HandleUse
  : public PolymorphicSerializationAdapter<HandleUse, HandleUseBase>
{
  private:

    using base_t = PolymorphicSerializationAdapter<HandleUse, HandleUseBase>;

  public:

    //--------------------------------------------------------------------------
    // <editor-fold desc="Ctors and destructor"> {{{2

    HandleUse() = default;
    HandleUse(HandleUse&&) = default;

    // This is for the purposes of reconstruction during migration
    // TODO put an unpack_ctor_tag or something here?!?
    HandleUse(
      HandleUseBase&& arg
    ) : HandleUse(std::move(*utility::safe_static_cast<HandleUse*>(&arg)))
    { }

    /**
     * @internal
     * @brief Passes through to the initial access (or other non-cloned Use)
     *        and unpacking constructors.
     */
    template <
      typename... PassthroughArgs
    >
    explicit /* should never be only one argument anyway... */
    HandleUse(
      std::shared_ptr<VariableHandleBase> const& handle,
      PassthroughArgs&&... passthrough_args
    ) : base_t(
          handle,
          std::forward<PassthroughArgs>(passthrough_args)...
        )
    { }

    /**
     * @internal
     * @brief Passes through to the cloning ctor
     *
     * For now, there isn't much interesting to clone from the old Use here,
     * but I bet we'll need it in the future.
     */
    template <
      typename... PassthroughArgs
    >
    explicit /* should never be only one argument anyway... */
    HandleUse(
      HandleUse const& clone_from,
      PassthroughArgs&&... passthrough_args
    ) : base_t(
          clone_from,
          std::forward<PassthroughArgs>(passthrough_args)...
        )
    { }

    ~HandleUse() override = default;

    // </editor-fold> end Ctors and destructor }}}2
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // <editor-fold desc="cloning helper methods"> {{{2

    /**
     * @internal
     * @brief Constructs a cloned Use with same_flow relationships for all of
     *        the flows/anti-flows but with different permissions
     *
     * @param different_scheduling_permissions Scheduling permissions of the new Use to be created
     * @param different_immediate_permissions Immediate permissions of the new Use to be created
     * @return A new HandleUse cloned from `this` but with different permissions
     */
    HandleUse
    with_different_permissions(
      frontend::Permissions different_scheduling_permissions,
      frontend::Permissions different_immediate_permissions
    ) {
      using namespace flow_relationships;
      // Call the cloning ctor
      auto rv = HandleUse(
        *this,
        different_scheduling_permissions,
        different_immediate_permissions,
        same_flow(&in_flow_),
        same_flow(&out_flow_),
        same_anti_flow(&anti_in_flow_),
        same_anti_flow(&anti_out_flow_)
      );
      rv.establishes_alias_ = establishes_alias_;
      rv.data_ = data_;
      return std::move(rv);
    }

    // </editor-fold> end cloning helper methods }}}2
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // <editor-fold desc="abstract::frontend::Use method implementations"> {{{2

    bool manages_collection() const override { return false; }

    abstract::frontend::UseCollection*
    get_managed_collection() override { return nullptr; }

    // </editor-fold> end abstract::frontend::Use method implementations }}}2
    //--------------------------------------------------------------------------

    template <typename Archive>
    void serialize(Archive& ar) {
      this->HandleUseBase::do_serialize(ar);
    }
};

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_USE_H
