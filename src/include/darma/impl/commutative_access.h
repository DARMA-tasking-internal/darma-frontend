/*
//@HEADER
// ************************************************************************
//
//                      commutative_access.h
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

#ifndef DARMA_IMPL_COMMUTATIVE_ACCESS_H
#define DARMA_IMPL_COMMUTATIVE_ACCESS_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(commutative_access_handles)

#include <darma/impl/commutative_access_fwd.h>

#include <darma/impl/meta/detection.h> // nonesuch
#include <darma/impl/keyword_arguments/macros.h>
#include <darma/impl/keyword_arguments/parse.h>
#include <darma/impl/handle.h> // is_access_handle
#include "flow_handling.h"
#include "use.h"


DeclareDarmaTypeTransparentKeyword(commutative_access, to_handle);
DeclareDarmaTypeTransparentKeyword(commutative_access, to_collection);

namespace darma_runtime {

namespace detail {

template <typename T, typename... TraitFlags>
struct _commutative_access_impl {

  using trait_flags_t = std::conditional_t<
    is_access_handle_trait_flag<T>::value,
    tinympl::vector<T, TraitFlags...>,
    tinympl::vector<TraitFlags...>
  >;

  using given_value_type_t = std::conditional_t<
    is_access_handle_trait_flag<T>::value,
    meta::nonesuch,
    T
  >;

  //==============================================================================
  // <editor-fold desc="AccessHandle 'converting' version"> {{{1

  template <typename AccessHandleT, std::size_t... TraitIdxs>
  decltype(auto)
  _handle_version_impl(
    AccessHandleT&& in_handle,
    std::integer_sequence<std::size_t, TraitIdxs...>
  ) const
  {
    using in_handle_t = std::decay_t<AccessHandleT>;
    using out_handle_t = typename in_handle_t::template with_traits<
      typename make_access_handle_traits<
        typename in_handle_t::value_type,
        typename trait_flags_t::template at_t<TraitIdxs>...
      >::template from_traits<
        make_access_handle_traits_t<
          typename in_handle_t::value_type,
          static_scheduling_permissions<AccessHandlePermissions::Commutative>,
          required_scheduling_permissions<AccessHandlePermissions::Commutative>,
          copy_assignable_handle<false>,
          could_be_outermost_scope<true>
        >
      >::type
    >;

    using namespace darma_runtime::abstract::frontend;

    // in handle unless one is given for the new handle, which is not yet
    // supported anyway)
    auto
      new_use_ptr = std::allocate_shared<typename out_handle_t::use_holder_t>(
        in_handle.get_allocator(),
        HandleUse(
          in_handle.var_handle_,
          HandleUse::Commutative, /* scheduling permissions */
          HandleUse::None, /* immediate permissions */
          FlowRelationship::Same, &in_handle.current_use_->use->in_flow_,
          FlowRelationship::Next, nullptr, true
        )
      );
    assert(in_handle.current_use_->use->suspended_out_flow_ == nullptr);
    new_use_ptr->use->suspended_out_flow_ = std::make_unique<types::flow_t>(
      std::move(in_handle.current_use_->use->out_flow_)
    );
    in_handle.current_use_->could_be_alias = false;

    if (in_handle.current_use_->is_registered) {
      new_use_ptr->do_register();
    }

    // Hold the variable handle so that it doesn't expire
    auto var_handle = in_handle.var_handle_;

    // release the use from the expired handle
    in_handle.current_use_ = nullptr;
    in_handle.var_handle_ = nullptr;

    return out_handle_t(
      var_handle, new_use_ptr
    );
  }

  template <typename AccessHandleT>
  decltype(auto)
  operator()(AccessHandleT&& in_handle,
    std::enable_if_t<
      is_access_handle<std::decay_t<AccessHandleT>>::value,
      _not_a_type_numbered<0>
    > = {}
  ) const
  {
    static_assert(
      is_access_handle<std::decay_t<AccessHandleT>>::value
        and std::is_rvalue_reference<AccessHandleT&&>::value,
      "Argument to commutative_access(to_handle=...) must be an rvalue"
        " AccessHandle.  Use std::move() if necessary"
    );
    static_assert(std::is_same<given_value_type_t, meta::nonesuch>::value
        or std::is_same<
          given_value_type_t,
          typename std::decay_t<AccessHandleT>::value_type
        >::value,
      "Don't need to give a template parameter for commutative_access(to_handle=...),"
        " but if you do, it must match the type of the existing parameter"
    );
    static_assert(
      std::decay_t<AccessHandleT>::is_compile_time_scheduling_modifiable,
      "Can't create commutative_access to handle that is marked with scheduling"
        " permissions less than Modify at compile time"
    );
    static_assert(
      std::decay_t<AccessHandleT>::traits::static_scheduling_permissions
        != AccessHandlePermissions::Commutative,
      "Can't create commutative_access to handle that is already marked"
        " commutative at compile time"
    );
    static_assert(
      std::decay_t<AccessHandleT>::traits::static_immediate_permissions
        != AccessHandlePermissions::Commutative,
      "Can't create commutative_access to handle that is already marked"
        " commutative at compile time"
    );
    static_assert(
      std::decay_t<AccessHandleT>::traits::required_scheduling_permissions
        != AccessHandlePermissions::Commutative,
      "Can't create commutative_access to handle that is already marked"
        " commutative at compile time"
    );
    static_assert(
      std::decay_t<AccessHandleT>::traits::required_immediate_permissions
        != AccessHandlePermissions::Commutative,
      "Can't create commutative_access to handle that is already marked"
        " commutative at compile time"
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_.get() != nullptr,
      "commutative_access(to_handle=...) called on uninitialized or released AccessHandle"
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_->use->immediate_permissions_
        == detail::HandleUse::None,
      "commutative_access(to_handle=...) called on use with immediate permissions that aren't None"
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_->use->scheduling_permissions_
        == detail::HandleUse::Modify,
      "commutative_access(to_handle=...) called on use without scheduling modify permissions"
    );
    return _handle_version_impl(
      std::forward<AccessHandleT>(in_handle),
      std::make_index_sequence<trait_flags_t::size>{}
    );
  }

  // </editor-fold> end AccessHandle "converting" version }}}1
  //==============================================================================


  //==============================================================================
  // <editor-fold desc="Collection-converting version"> {{{1

  template <typename AccessHandleCollectionT, std::size_t... TraitIdxs>
  decltype(auto)
  _collection_version_impl(
    AccessHandleCollectionT&& in_handle,
    std::integer_sequence<std::size_t, TraitIdxs...>
  ) const
  {
    using in_handle_t = typename std::decay<AccessHandleCollectionT>::type;
    using out_handle_t = typename in_handle_t::template with_traits<
      typename make_access_handle_collection_traits<
        typename in_handle_t::value_type, typename in_handle_t::index_range_type,
        typename trait_flags_t::template at_t<TraitIdxs>...
      >::template from_traits<
        typename make_access_handle_collection_traits<
          typename in_handle_t::value_type, typename in_handle_t::index_range_type,
          static_scheduling_permissions<AccessHandlePermissions::Commutative>,
          required_scheduling_permissions<AccessHandlePermissions::Commutative>
        >::type
      >::type
    >;

    // TODO use allocator

    using namespace darma_runtime::abstract::frontend;

    // in handle unless one is given for the new handle, which is not yet
    // supported anyway)
    auto new_use_ptr = std::make_shared<typename out_handle_t::use_holder_t>(
      typename out_handle_t::use_t(
        in_handle.var_handle_.get_smart_ptr(),
        HandleUse::Commutative, /* scheduling permissions */
        HandleUse::None, /* immediate permissions */
        FlowRelationship::SameCollection, &in_handle.current_use_->use->in_flow_,
        FlowRelationship::NextCollection, nullptr, true,
        // we probably COULD move the index range from the old use now, since
        // it's going away, but for now, just copy it, since we might want
        // to use it in the destructor of the old use at some point in the
        // future
        in_handle.current_use_->use->index_range
      )
    );
    assert(in_handle.current_use_->use->suspended_out_flow_ == nullptr);
    new_use_ptr->use->suspended_out_flow_ = std::make_unique<types::flow_t>(
      std::move(in_handle.current_use_->use->out_flow_)
    );
    in_handle.current_use_->could_be_alias = false;

    // release the use from the expired handle
    in_handle.current_use_ = nullptr;

    return out_handle_t(
      in_handle.var_handle_.release_smart_ptr(), new_use_ptr
    );
  }

  template <typename AccessHandleCollectionT>
  decltype(auto)
  operator()(
    AccessHandleCollectionT&& in_handle,
    std::enable_if_t<
      is_access_handle_collection<std::decay_t<AccessHandleCollectionT>>::value,
      _not_a_type_numbered<1>
    > = {}
  ) const
  {
    using ahc_t = std::decay_t<AccessHandleCollectionT>;
    static_assert(
      is_access_handle_collection<std::decay_t<AccessHandleCollectionT>>::value
        and std::is_rvalue_reference<AccessHandleCollectionT&&>::value,
      "Argument to commutative_access(to_collection=...) must be an rvalue"
        " AccessHandle.  Use std::move() if necessary"
    );
    static_assert(std::is_same<given_value_type_t, meta::nonesuch>::value
        or std::is_same<
          given_value_type_t,
          typename std::decay_t<AccessHandleCollectionT>::value_type
        >::value,
      "Don't need to give a template parameter for commutative_access(to_collection=...),"
        " but if you do, it must match the type of the existing parameter"
    );
    static_assert(
      handle_permissions_equal_if_given<
        ahc_t::traits::permissions_traits::static_scheduling_permissions,
        AccessHandlePermissions::Modify
      >::value,
      "Can't create commutative_access to collection that is marked with scheduling"
        " permissions less than Modify at compile time"
    );
    static_assert(
      ahc_t::traits::permissions_traits::static_scheduling_permissions
        != AccessHandlePermissions::Commutative,
      "Can't create commutative_access to collection that is already marked"
        " commutative at compile time"
    );
    static_assert(
      ahc_t::traits::permissions_traits::static_immediate_permissions
        != AccessHandlePermissions::Commutative,
      "Can't create commutative_access to collection that is already marked"
        " commutative at compile time"
    );
    static_assert(
      ahc_t::traits::permissions_traits::required_scheduling_permissions
        != AccessHandlePermissions::Commutative,
      "Can't create commutative_access to collection that is already marked"
        " commutative at compile time"
    );
    static_assert(
      ahc_t::traits::permissions_traits::required_immediate_permissions
        != AccessHandlePermissions::Commutative,
      "Can't create commutative_access to collection that is already marked"
        " commutative at compile time"
    );
    static_assert(
      ahc_t::traits::semantic_traits::is_mapped != OptionalBoolean::KnownTrue,
      "Can't create commutative_access to collection that is mapped to a task"
        " collection"
    );
    // TODO check non-static is_mapped
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_.get() != nullptr,
      "commutative_access(to_handle=...) called on uninitialized or released AccessHandle"
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_->use->immediate_permissions_
        == detail::HandleUse::None,
      "commutative_access(to_collection=...) called on use with immediate permissions that aren't None"
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_->use->scheduling_permissions_
        == detail::HandleUse::Modify,
      "commutative_access(to_collection=...) called on use without scheduling modify permissions"
    );
    return _collection_version_impl(
      std::forward<AccessHandleCollectionT>(in_handle),
      std::make_index_sequence<trait_flags_t::size>{}
    );
  }

  // </editor-fold> end Collection-converting version }}}1
  //==============================================================================

  //==============================================================================
  // <editor-fold desc="non-AccessHandle-converting version"> {{{1

  template <typename _Ignored_SFINAE=void,
    typename=std::enable_if_t<std::is_void<_Ignored_SFINAE>::value>
  >
  decltype(auto)
  _impl(types::key_t&& key) const
  {
    auto* rt = abstract::backend::get_backend_runtime();

    static_assert(
      not std::is_same<given_value_type_t, meta::nonesuch>::value,
      "Must give value type template parameter for overloads of"
        " commutative_access(...) that don't take an AccessHandle argument"
    );

    using namespace darma_runtime::abstract::frontend;

    // TODO use allocator

    auto var_h = detail::make_shared<
      detail::VariableHandle<
        given_value_type_t
      >>(std::move(key));

    // TODO This is ugly.  We need a cleaner semantic for this that still preserves some reasonable invariants

    // Effectively, right now we need to create an initial access, register it,
    // then switch to commutative mode so that it has something reasonable to
    // switch back to when it exits commutative mode
    UseHolder initial_use(
      HandleUse(
        var_h,
        HandleUse::Modify,
        HandleUse::None,
        FlowRelationship::Initial, nullptr,
        FlowRelationship::Null, nullptr, false
      )
    );

    return AccessHandle<
      given_value_type_t,
      typename make_access_handle_traits<
        given_value_type_t,
        TraitFlags...
      >::template from_traits<
        make_access_handle_traits_t<
          given_value_type_t,
          static_scheduling_permissions<AccessHandlePermissions::Commutative>,
          required_scheduling_permissions<AccessHandlePermissions::Commutative>,
          copy_assignable_handle<false>,
          could_be_outermost_scope<true>
        >
      >::type
    >(
      var_h,
      std::make_shared<UseHolder>(
        HandleUse(
          var_h,
          HandleUse::Commutative,
          HandleUse::None,
          FlowRelationship::Same, &initial_use.use->in_flow_,
          FlowRelationship::Next, nullptr, true
        )
      ),
      std::make_unique<types::flow_t>(
        std::move(initial_use.use->out_flow_)
      )
    );
    // initial_use released at closing curly brace
  }

  template <
    typename KeyPart1, typename... KeyParts,
    typename=std::enable_if_t<not is_access_handle<KeyPart1>::value>
  >
  decltype(auto)
  operator()(
    variadic_arguments_begin_tag,
    KeyPart1&& kp1, KeyParts&& ... kps
  ) const
  {
    return _impl(darma_runtime::make_key(
      std::forward<KeyPart1>(kp1),
      std::forward<KeyParts>(kps)...
      )
    );
  };

  template <
    typename _Ignored_SFINAE=void,
    typename=std::enable_if_t<std::is_void<_Ignored_SFINAE>::value>
  >
  decltype(auto)
  operator()(
    variadic_arguments_begin_tag
  ) const
  {
    return _impl(
      key_traits<types::key_t>::make_awaiting_backend_assignment_key()
    );
  };

  // </editor-fold> end non-AccessHandle-converting version }}}1
  //==============================================================================

};

template <typename T, typename... TraitFlags>
struct _noncommutative_access_impl {

  using trait_flags_t = std::conditional_t<
    is_access_handle_trait_flag<T>::value,
    tinympl::vector<T, TraitFlags...>,
    tinympl::vector<TraitFlags...>
  >;

  using given_value_type_t = std::conditional_t<
    is_access_handle_trait_flag<T>::value,
    meta::nonesuch,
    T
  >;

  template <typename AccessHandleT, std::size_t... TraitIdxs>
  decltype(auto)
  _handle_version_impl(
    AccessHandleT&& in_handle,
    std::integer_sequence<std::size_t, TraitIdxs...>
  ) const
  {
    using in_handle_t = std::decay_t<AccessHandleT>;
    using out_handle_t = typename in_handle_t::template with_traits<
      typename make_access_handle_traits<
        typename in_handle_t::value_type,
        typename trait_flags_t::template at_t<TraitIdxs>...
      >::template from_traits<
        make_access_handle_traits_t<
          typename in_handle_t::value_type,
          static_scheduling_permissions<AccessHandlePermissions::Modify>,
          required_scheduling_permissions<AccessHandlePermissions::None>,
          copy_assignable_handle<false>,
          could_be_outermost_scope<true>
        >
      >::type
    >;

    using namespace darma_runtime::abstract::frontend;
    auto* rt = abstract::backend::get_backend_runtime();

    // Use the allocator from the out handle (which is the allocator from the
    // in handle unless one is given for the new handle, which is not yet
    // supported anyway)
    auto suspended_out = std::move(
      *in_handle.current_use_->use->suspended_out_flow_.release()
    );

    in_handle.current_use_->could_be_alias = false;
    auto new_use_ptr =
      std::allocate_shared<typename out_handle_t::use_holder_t>(
        in_handle.get_allocator(),
        HandleUse(
          in_handle.var_handle_,
          HandleUse::Modify, /* scheduling permissions */
          HandleUse::None, /* immediate permissions */
          FlowRelationship::Same, &in_handle.current_use_->use->out_flow_,
          FlowRelationship::Same, &suspended_out
        )
      );

    // Hold the variable handle so that it doesn't expire (shouldn't a
    auto var_handle = in_handle.var_handle_;

    // release the use from the expired handle
    in_handle.current_use_ = nullptr;
    in_handle.var_handle_ = nullptr;

    new_use_ptr->could_be_alias = true;

    return out_handle_t(
      std::move(var_handle), std::move(new_use_ptr)
    );
  }

  template <typename AccessHandleT>
  decltype(auto)
  operator()(AccessHandleT&& in_handle) const
  {
    static_assert(
      is_access_handle<std::decay_t<AccessHandleT>>::value
        and std::is_rvalue_reference<AccessHandleT&&>::value,
      "Argument to noncommutative_access_to_handle(...) must be an rvalue"
        " AccessHandle.  You probably forgot to use std::move()."
    );
    static_assert(std::is_same<given_value_type_t, meta::nonesuch>::value
        or std::is_same<
          given_value_type_t,
          typename std::decay_t<AccessHandleT>::value_type
        >::value,
      "Don't need to give a template parameter for"
        " noncommutative_access_to_handle(...), but if you do, it must match"
        " the type of the existing parameter"
    );
    static_assert(
      std::decay_t<AccessHandleT>::traits::semantic_traits::could_be_outermost_scope,
      "Can't create noncommutative_access to handle that's marked as"
        " noncommutative at compile time"
    );
    static_assert(
      not std::decay_t<AccessHandleT>::traits::static_scheduling_permissions_given
      or std::decay_t<AccessHandleT>::traits::static_scheduling_permissions
        == AccessHandlePermissions::Commutative,
      "Can't create noncommutative_access to handle that's marked as"
        " noncommutative at compile time"
    );
    static_assert(
      not std::decay_t<AccessHandleT>::traits::required_scheduling_permissions_given
      or std::decay_t<AccessHandleT>::traits::required_scheduling_permissions
        == AccessHandlePermissions::Commutative,
      "Can't create noncommutative_access to handle that isn't marked"
        " commutative at compile time"
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.get_is_outermost_scope_dynamic(),
      "Can't create noncommutative_access (releasing commutative_access) at a"
        " more nested scope than the commutative_access handle was created."
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_.get() != nullptr,
      "noncommutative_access_to_handle(...) called on uninitialized or released"
        " AccessHandle"
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_->use->immediate_permissions_
        == detail::HandleUse::None,
      "noncommutative_access_to_handle(...) called on use with immediate"
        " permissions that aren't None"
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_->use->scheduling_permissions_
        == detail::HandleUse::Commutative,
      "noncommutative_access_to_handle(...) called on use without scheduling modify permissions"
    );
    return _handle_version_impl(
      std::forward<AccessHandleT>(in_handle),
      std::make_index_sequence<trait_flags_t::size>{}
    );
  }

};

template <typename T, typename... TraitFlags>
struct _noncommutative_collection_access_impl {

  using trait_flags_t = std::conditional_t<
    is_access_handle_trait_flag<T>::value,
    tinympl::vector<T, TraitFlags...>,
    tinympl::vector<TraitFlags...>
  >;

  using given_value_type_t = std::conditional_t<
    is_access_handle_trait_flag<T>::value,
    meta::nonesuch,
    T
  >;

  template <typename AccessHandleCollectionT, std::size_t... TraitIdxs>
  decltype(auto)
  _collection_version_impl(
    AccessHandleCollectionT&& in_handle,
    std::integer_sequence<std::size_t, TraitIdxs...>
  ) const
  {
    using in_handle_t = std::decay_t<AccessHandleCollectionT>;
    using default_traits_t = typename make_access_handle_collection_traits<
      typename in_handle_t::value_type, typename in_handle_t::index_range_type,
      static_scheduling_permissions<AccessHandlePermissions::Modify>,
      required_scheduling_permissions<AccessHandlePermissions::None>
      // TODO track copy assignability of AHC
      // copy_assignable_handle<false>,
      // TODO track outermost scope/captured semantics of AHC
      //could_be_outermost_scope<true>
    >::type;

    using out_handle_t = typename in_handle_t::template with_traits<
      typename make_access_handle_collection_traits<
        typename in_handle_t::value_type, typename in_handle_t::index_range_type,
        typename trait_flags_t::template at_t<TraitIdxs>...
      >::template from_traits<default_traits_t>::type
    >;

    using namespace darma_runtime::abstract::frontend;
    auto* rt = abstract::backend::get_backend_runtime();

    // Use the allocator from the out handle (which is the allocator from the
    // in handle unless one is given for the new handle, which is not yet
    // supported anyway)
    auto suspended_out_flow = std::move(
      // Dereferencing a "dumb" pointer doesn't create an rvalue, so we need
      // to invoke an explicit move so that the stack variable now owns the
      // flow
      *in_handle.current_use_->use->suspended_out_flow_.release()
    );

    auto new_use_ptr = std::make_shared<typename out_handle_t::use_holder_t>(
      typename out_handle_t::use_t(
        in_handle.var_handle_.get_smart_ptr(),
        HandleUse::Commutative, /* scheduling permissions */
        HandleUse::None, /* immediate permissions */
        FlowRelationship::SameCollection, &in_handle.current_use_->use->out_flow_,
        FlowRelationship::SameCollection, &suspended_out_flow, false,
        // we probably COULD move the index range from the old use now, since
        // it's going away, but for now, just copy it, since we might want
        // to use it in the destructor of the old use at some point in the
        // future
        in_handle.current_use_->use->index_range
      )
    );

    // Make sure an alias call doesn't trigger when the commutative use gets
    // released
    in_handle.current_use_->could_be_alias = false;

    // release the use from the expired handle
    in_handle.current_use_ = nullptr;

    new_use_ptr->could_be_alias = true;

    return out_handle_t(
      in_handle.var_handle_.release_smart_ptr(), std::move(new_use_ptr)
    );
  }

  template <typename AccessHandleCollectionT>
  decltype(auto)
  operator()(AccessHandleCollectionT&& in_handle) const
  {
    using ahc_t = std::decay_t<AccessHandleCollectionT>;

    //--------------------------------------------------------------------------
    // <editor-fold desc="check that we were actually given an AccessHandleCollection rvalue"> {{{2

    static_assert(
      is_access_handle_collection<std::decay_t<AccessHandleCollectionT>>::value
        and std::is_rvalue_reference<AccessHandleCollectionT&&>::value,
      "Argument to noncommutative_access_to_collection(...) must be an rvalue"
        " AccessHandleCollection.  You probably forgot to use std::move()."
    );
    static_assert(std::is_same<given_value_type_t, meta::nonesuch>::value
        or std::is_same<
          given_value_type_t,
          typename std::decay_t<AccessHandleCollectionT>::value_type
        >::value,
      "Don't need to give a template parameter for noncommutative_access_to_collection(...),"
        " but if you do, it must match the type of the existing value_type parameter"
    );

    // </editor-fold> end check that we were actually given an AccessHandleCollection rvalue }}}2
    //--------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    // <editor-fold desc="semantic trait checks"> {{{2

    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_.get() != nullptr,
      "noncommutative_access_to_collection(...) called on uninitialized or"
        " released AccessHandleCollection"
    );

    // TODO outer scope check
    //static_assert(
    //  std::decay_t<AccessHandleT>::traits::semantic_traits::could_be_outermost_scope,
    //  "Can't create noncommutative_access to handle that's marked as"
    //    " noncommutative at compile time"
    //);

    static_assert(
      ahc_t::traits::semantic_traits::is_mapped != OptionalBoolean::KnownTrue,
      "Can't release commutative_access to collection that is mapped to a task"
        " collection"
    );

    // TODO non-static is_mapped

    // </editor-fold> end semantic trait checks }}}2
    //------------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // <editor-fold desc="scheduling permissions checks"> {{{2
    static_assert(
      handle_permissions_equal_if_given<
        ahc_t::traits::permissions_traits::static_scheduling_permissions,
        AccessHandlePermissions::Commutative
      >::value,
      "Can't create noncommutative_access to handle that's marked as"
        " not commutative at compile time"
    );
    static_assert(
      handle_permissions_equal_if_given<
        ahc_t::traits::permissions_traits::required_scheduling_permissions,
        AccessHandlePermissions::Commutative
      >::value,
      "Can't create noncommutative_access to handle that's marked as"
        " not commutative at compile time"
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_->use->scheduling_permissions_
        == detail::HandleUse::Commutative,
      "noncommutative_access_to_collection(...) called on use without scheduling"
        " commutative permissions"
    );
    // </editor-fold> end scheduling permissions }}}2
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // <editor-fold desc="immediate permissions checks"> {{{2

    static_assert(
      handle_permissions_equal_if_given<
        ahc_t::traits::permissions_traits::required_immediate_permissions,
        AccessHandlePermissions::None
      >::value,
      "Can't create noncommutative_access to handle that's marked as"
        " requiring immediate permissions other than None at compile time at"
        " compile time"
    );
    static_assert(
      handle_permissions_equal_if_given<
        ahc_t::traits::permissions_traits::static_immediate_permissions,
        AccessHandlePermissions::None
      >::value,
      "Can't create noncommutative_access to handle that's marked as"
        " starting with immediate permissions other than None at compile time at"
        " compile time"
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_->use->immediate_permissions_
        == detail::HandleUse::None,
      "noncommutative_access_to_collection(...) called on use with immediate"
        " permissions that aren't None"
    );

    // </editor-fold> end immediate permissions checks }}}2
    //--------------------------------------------------------------------------

    return _collection_version_impl(
      std::forward<AccessHandleCollectionT>(in_handle),
      std::make_index_sequence<trait_flags_t::size>{}
    );
  }

};

} // end namespace detail


template <typename T=meta::nonesuch,
  typename... TraitFlags,
  typename... Args
>
auto
commutative_access(Args&&... args) {

  // TODO allocator kwarg

  using namespace darma_runtime::detail;
  using parser = detail::kwarg_parser<
    overload_description<
      // for converting an existing handle into commutative mode:
      _keyword<
        deduced_parameter, keyword_tags_for_commutative_access::to_handle
      >
    >,
    overload_description<
      // for converting an existing handle collection into commutative mode:
      _keyword<
        deduced_parameter, keyword_tags_for_commutative_access::to_collection
      >
    >,
    // for creating a new parameter in commutative mode:
    variadic_positional_overload_description<>
  >;
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

  return parser()
    .parse_args(std::forward<Args>(args)...)
    .invoke(
      detail::_commutative_access_impl<T, TraitFlags...>{}
    );

};

template <
  typename T=meta::nonesuch,
  typename... TraitFlags,
  typename Arg1,
  typename... Args
>
auto
commutative_access_to_handle(Arg1&& a1, Args&&... args) {
  return commutative_access<T, TraitFlags...>(
    darma_runtime::keyword_arguments_for_commutative_access::to_handle
      = std::forward<Arg1>(a1),
    std::forward<Args>(args)...
  );
};

template <
  typename T=meta::nonesuch,
  typename... TraitFlags,
  typename... Args
>
auto
noncommutative_access_to_handle(Args&&... args) {

  using namespace darma_runtime::detail;
  using parser = detail::kwarg_parser<
    overload_description<
      // for converting an existing handle into commutative mode:
      _positional_or_keyword<
        deduced_parameter, keyword_tags_for_commutative_access::to_handle
      >
    >
  >;
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

  return parser()
    .parse_args(std::forward<Args>(args)...)
    .invoke(detail::_noncommutative_access_impl<T, TraitFlags...>{});
};

template <
  typename T=meta::nonesuch,
  typename... TraitFlags,
  typename... Args
>
auto
noncommutative_access_to_collection(Args&&... args) {

  using namespace darma_runtime::detail;
  using parser = detail::kwarg_parser<
    overload_description<
      // for converting an existing handle into commutative mode:
      _positional_or_keyword<
        deduced_parameter, keyword_tags_for_commutative_access::to_collection
      >
    >
  >;
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

  return parser()
    .parse_args(std::forward<Args>(args)...)
    .invoke(detail::_noncommutative_collection_access_impl<T, TraitFlags...>{});
};



} // end namespace darma_runtime

#endif // _darma_has_feature(commutative_access_handles)

#endif //DARMA_IMPL_COMMUTATIVE_ACCESS_H
