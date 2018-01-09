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
    ) : HandleUse(safe_static_cast<HandleUse&&>(std::move(arg)))
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


#ifdef OLD_CODE
template <typename UnderlyingUse>
struct GenericUseHolder : UseHolderBase {
  using held_use_t = managing_ptr<
    std::unique_ptr<UnderlyingUse>, HandleUseBase*
  >;

  held_use_t use;

  GenericUseHolder(GenericUseHolder&&) = delete;
  GenericUseHolder(GenericUseHolder const &) = delete;

  explicit
  GenericUseHolder(UnderlyingUse&& in_use,
    bool do_register_in_ctor=true,
    bool will_be_dep=false
  ) : use(use_base, std::make_unique<UnderlyingUse>(std::move(in_use)))
  {
    use->is_dependency_ = will_be_dep and use_base->immediate_permissions_ != frontend::Permissions::None;
#if _darma_has_feature(anti_flows)
    use->is_anti_dependency_ = use->is_dependency_ and use_base->immediate_permissions_ != frontend::Permissions::Read;

#endif // _darma_has_feature(anti_flows)
    if(do_register_in_ctor) do_register();
  }

  void do_register() {
    if(not is_registered) { // for now
      // TODO remove this hack if statement once I remove the superfluous do_register() calls elsewhere
      assert(!is_registered);
      abstract::backend::get_backend_runtime()->register_use(use_base);
      is_registered = true;
      // Delete the FlowRelationship descriptions so that they don't accidentally get used
      use->in_flow_rel_ = FlowRelationshipImpl();
      use->out_flow_rel_ = FlowRelationshipImpl();
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
    assert(use);
    use_base->establishes_alias_ = could_be_alias;
    abstract::backend::get_backend_runtime()->release_use(use.release_smart_ptr());
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
    using namespace darma_runtime::detail::flow_relationships;

    if(use && use_base->suspended_out_flow_.get() != nullptr) {

      // We ended our lives in commutative mode.  While this doesn't make much
      // sense in most cases, it should work (potentially with a warning message).
      // We'll need to register the use that depends on the output of the
      // commutative region to preserve some desirable invariants

      auto* rt = abstract::backend::get_backend_runtime();

#if _darma_has_feature(anti_flows)
      DARMA_ASSERT_NOT_IMPLEMENTED("new anti-flow semantics with commutative");
#endif

      // TODO @commutative the "collection" version of this needs to work also
      DARMA_ASSERT_MESSAGE(
        not use_base->manages_collection(),
        "Commutative collections released in this way are not yet implemented"
      );

      auto last_use_ptr = std::make_unique<HandleUse>(
        use->handle_,
        frontend::Permissions::None, // This could cause problems for some backends...
        frontend::Permissions::None,
        use->manages_collection() ?
          same_flow(&use->out_flow_).as_collection_relationship()
            : same_flow(&use->out_flow_),
        //FlowRelationship::Same, &(use->out_flow_),
        use->manages_collection() ?
          same_flow(use->suspended_out_flow_.get()).as_collection_relationship()
            : same_flow(use->suspended_out_flow_.get())
        //FlowRelationship::Same, use->suspended_out_flow_.get(), false
#if _darma_has_feature(anti_flows)
        // anti-in
        , insignificant_flow(),
        // anti-out
        use->manages_collection() ?
          anti_next_of_in_flow().as_collection_relationship()
            : anti_next_of_in_flow()
#endif // _darma_has_feature(anti_flows)
      );
      rt->register_use(last_use_ptr.get());

      assert(is_registered);

      do_release();

      last_use_ptr->establishes_alias_ = true;

      rt->release_use(std::move(last_use_ptr));
    }
    else {
      // Normal case...
      if(is_registered) do_release();
    }
  }
};
#endif // OLD_CODE

//using UseHolder = GenericUseHolder<HandleUse>;

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_USE_H
