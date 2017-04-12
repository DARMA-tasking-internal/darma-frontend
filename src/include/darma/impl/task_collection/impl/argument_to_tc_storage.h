/*
//@HEADER
// ************************************************************************
//
//                      argument_to_tc_storage.h
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

#ifndef DARMA_IMPL_TASK_COLLECTION_IMPL_ARGUMENT_TO_TC_STORAGE_H
#define DARMA_IMPL_TASK_COLLECTION_IMPL_ARGUMENT_TO_TC_STORAGE_H


#include <type_traits>

#include <tinympl/switch.hpp>

#include <darma/impl/task_collection/task_collection_fwd.h>

#include <darma/interface/app/access_handle.h>

#include <darma/impl/handle.h>
#include <darma/impl/capture.h>
#include <darma/impl/task_collection/handle_collection.h>
#include <darma/impl/index_range/index_range_traits.h>
#include <darma/impl/index_range/mapping_traits.h>

#include <darma/impl/util/static_assertions.h>

namespace darma_runtime {
namespace detail {
namespace _task_collection_impl {

// TODO more readable errors when none of these work

//==============================================================================
// <editor-fold desc="Default regular argument case"> {{{1

template <
  typename GivenArg, typename ParamTraits, typename CollectionIndexRangeT, typename Enable /* =void */
>
struct _get_storage_arg_helper {
  using type = std::decay_t<GivenArg>;

  static_assert(
    not ParamTraits::is_nonconst_lvalue_reference,
    "Cannot pass \"plain-old\" argument to modify parameter of concurrent work"
      " call.  Use an AccessHandleCollection instead"
  );

  template <typename TaskCollectionT>
  auto
  operator()(TaskCollectionT& collection, GivenArg&& arg) const {
    return arg;
  }
};

// </editor-fold> end Default "regular argument" case }}}1
//==============================================================================


//==============================================================================
// <editor-fold desc="AccessHandle-like"> {{{1


//------------------------------------------------------------------------------
// <editor-fold desc="unique modify case"> {{{2


#if _darma_has_feature(create_concurrent_work_owned_by)
template <
  typename GivenArg,
  typename ParamTraits,
  typename CollectionIndexRangeT
>
struct _get_storage_arg_helper<
  GivenArg, ParamTraits, CollectionIndexRangeT,
  std::enable_if_t<
    // The argument is an access handle
    decayed_is_access_handle<GivenArg>::value
      and is_access_handle_captured_as_unique_modify<std::decay_t<GivenArg>>::value
  >
>
{
  // If the argument is an AccessHandle, the parameter cannot modify unless it is
  // a uniquely-owned capture (in which case the parameter still needs to be an
  // AccessHandle):
  static_assert(
    not(
      ParamTraits::is_nonconst_lvalue_reference
        and not ParamTraits::template matches<decayed_is_access_handle>::value
    ),
    "Cannot pass \"plain-old\" AccessHandle to modify parameter of concurrent work"
      " call.  Use an AccessHandleCollection instead, or pass an access handle with"
      " an owned_by() specifier and use an AccessHandle as the function parameter"
  );

  using type = typename std::decay_t<GivenArg>
  ::template with_traits<
    typename std::decay_t<GivenArg>::traits
  >;
  using return_type = type; // readability

  template <typename TaskCollectionT>
  auto
  operator()(TaskCollectionT& collection, GivenArg&& arg) const
  {
    auto rv = return_type(
      arg.var_handle_,
      detail::make_captured_use_holder(
        arg.var_handle_base_,
        /* Requested Scheduling permissions: */
        // TODO check params(/args?) for reduced scheduling permissions
        HandleUse::Modify,
        /* Requested Immediate permissions: */
        // TODO check params(/args?) for schdule-only permissions request?
        HandleUse::Modify,
        /* source and continuing context use holder */
        arg.current_use_.get()
      )
    );
    using collection_range_traits = typename TaskCollectionT::index_range_traits;
    auto coll_mapping_to_dense =
      collection_range_traits::mapping_to_dense(collection.collection_range_);
    std::size_t backend_owning_index =
      coll_mapping_to_dense.map_forward(arg.owning_index());
    rv.current_use_->use->collection_owner_ = backend_owning_index;
    rv.owning_backend_index() = backend_owning_index;
    collection.add_dependency(rv.current_use_->use.get());
    return rv;
  }
};
#endif // _darma_has_feature(create_concurrent_work_owned_by)

// </editor-fold> end unique modify case }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="shared_read case"> {{{2

template <
  typename GivenArg,
  typename ParamTraits,
  typename CollectionIndexRangeT
>
struct _get_storage_arg_helper<
  GivenArg, ParamTraits, CollectionIndexRangeT,
  std::enable_if_t<
    // The argument is an access handle
    decayed_is_access_handle<GivenArg>::value
      and is_access_handle_captured_as_shared_read<GivenArg>::value
  >
>
{
  // If the argument is an AccessHandle, the parameter cannot modify unless it is
  // a uniquely-owned capture (in which case the parameter still needs to be an
  // AccessHandle):
  static_assert(
    not(
      ParamTraits::is_nonconst_lvalue_reference
        and not ParamTraits::template matches<decayed_is_access_handle>::value
    ),
    "Cannot pass \"plain-old\" AccessHandle to modify parameter of concurrent work"
      " call.  Use an AccessHandleCollection instead, or pass an access handle with"
      " an owned_by() specifier and use an AccessHandle as the function parameter"
  );

  using type = typename std::decay_t<GivenArg>::template with_traits<
    typename std::decay_t<GivenArg>::traits
    ::template with_static_scheduling_permissions<
      ParamTraits::template matches<decayed_is_access_handle>::value ?
        AccessHandlePermissions::Read : AccessHandlePermissions::None
    >::type::template with_static_immediate_permissions<
      // TODO check for a schedule-only AccessHandle parameter
      AccessHandlePermissions::Read
    >::type
  >;
  using return_type = type; // readability

  template <typename TaskCollectionT>
  auto
  operator()(TaskCollectionT& collection, GivenArg&& arg) const
  {
    auto rv = return_type(
      arg.var_handle_,
      detail::make_captured_use_holder(
        arg.var_handle_base_,
        /* Requested Scheduling permissions: */
        return_type::is_compile_time_scheduling_readable ?
          HandleUse::Read : HandleUse::None,
        /* Requested Immediate permissions: */
        // TODO check params(/args?) for schdule-only permissions request
        HandleUse::Read,
        /* source and continuing context use holder */
        arg.current_use_.get()
      )
    );
    collection.add_dependency(rv.current_use_->use.get());
    return rv;
  }
};

// </editor-fold> end shared_read case }}}2
//------------------------------------------------------------------------------

// </editor-fold> end AccessHandle-like }}}1
//==============================================================================


//==============================================================================
// <editor-fold desc="MappedHandleCollection version"> {{{1

template <typename GivenArg, typename ParamTraits, typename CollectionIndexRangeT>
struct _get_storage_arg_helper<
  GivenArg, ParamTraits, CollectionIndexRangeT,
  std::enable_if_t<
    tinympl::is_instantiation_of<
      MappedHandleCollection,
      std::decay_t<GivenArg>>::value
  >
>
{
  using type = std::decay_t<GivenArg>;
  using return_type = type; // readability

  template <HandleUse::permissions_t p>
  using _permissions = std::integral_constant<HandleUse::permissions_t, p>;

  // TODO static schedule-only permissions

  template <typename AHC>
  using _required_immediate_permissions_archetype = typename std::integral_constant<
    access_handle_permissions_t, AHC::traits_t::permissions_traits::required_immediate_permissions
  >;

  template <typename AHC>
  using _required_immediate_permissions = meta::detected_or_t<
    std::integral_constant<access_handle_permissions_t, AccessHandlePermissions::NotGiven>,
    _required_immediate_permissions_archetype, AHC
  >;

  template <typename AHC>
  using _required_scheduling_permissions_archetype = typename std::integral_constant<
    access_handle_permissions_t, AHC::traits_t::permissions_traits::required_scheduling_permissions
  >;

  template <typename AHC>
  using _required_scheduling_permissions = meta::detected_or_t<
    std::integral_constant<access_handle_permissions_t, AccessHandlePermissions::NotGiven>,
    _required_scheduling_permissions_archetype, AHC
  >;

  //----------------------------------------------------------------------------
  // <editor-fold desc="required_immediate_permissions"> {{{2

  static constexpr auto required_immediate_permissions = tinympl::select_first<
    // TODO read-only AccessHandleCollection parameters (and other static permissions, including schedule-only)
    tinympl::bool_<
      (
        // Parameter is an AccessHandleCollection
        ParamTraits::template matches<decayed_is_access_handle_collection>::value
      )
    >, /* => */ typename tinympl::switch_value_<
      access_handle_permissions_t, _required_immediate_permissions<
        std::decay_t<typename ParamTraits::type>
      >::value
    > ::template case_<AccessHandlePermissions::Modify, _permissions<HandleUse::Modify>>
      ::template case_<AccessHandlePermissions::Read, _permissions<HandleUse::Read>>
      ::template case_<AccessHandlePermissions::None, _permissions<HandleUse::None>>
      ::template default_<_permissions<HandleUse::Modify>>
    ::type,
    /*----------------------------------------*/
    // // TODO reinstate support for single-entry-in-collection parameters when it's implemented elsewhere?
    // tinympl::bool_<
    //   (
    //     // Parameter is an AccessHandle
    //     ParamTraits::template matches<decayed_is_access_handle>::value
    //       // Parameter is compile-time immediate read-only
    //       and ParamTraits::template matches<
    //         compile_time_immediate_read_only_if_access_handle
    //       >::value
    //   )
    // >, /* => */ _permissions<HandleUse::Read>,
    // /*----------------------------------------*/
    // tinympl::bool_<
    //   (
    //     // Parameter is an AccessHandle
    //     ParamTraits::template matches<decayed_is_access_handle>::value
    //       // Parameter is compile-time immediate read-only
    //       and not ParamTraits::template matches<
    //         compile_time_immediate_read_only_if_access_handle
    //       >::value
    //   )
    // >, /* => */ _permissions<HandleUse::Modify>,
    // /*----------------------------------------*/
    // tinympl::bool_<
    //   (
    //     // Parameter is an not AccessHandle
    //     (not ParamTraits::template matches<decayed_is_access_handle>::value)
    //       // Parameter is by value or by const reference
    //       and (
    //         ParamTraits::is_by_value
    //           or ParamTraits::is_const_lvalue_reference
    //       )
    //   )
    // >, /* => */ _permissions<HandleUse::Read>,
    // /*----------------------------------------*/
    // tinympl::bool_<
    //   (
    //     // Parameter is an not AccessHandle
    //     (not ParamTraits::template matches<decayed_is_access_handle>::value)
    //       // Parameter is a nonconst lvalue reference
    //       and ParamTraits::is_nonconst_lvalue_reference
    //   )
    // >, /* => */ _permissions<HandleUse::Modify>
    /*----------------------------------------*/
    std::true_type, /* => */ _darma__static_failure<
      GivenArg,
      _____________________________________________________________________,
      ParamTraits,
      _____________________________________________________________________,
      CollectionIndexRangeT
      // TODO better error message in the failure case
    >
  >::type::value;

  // </editor-fold> end required_immediate_permissions }}}2
  //----------------------------------------------------------------------------

  //----------------------------------------------------------------------------
  // <editor-fold desc="required_scheduling_permissions"> {{{2

  static constexpr auto required_scheduling_permissions = tinympl::select_first<
    // TODO read-only AccessHandleCollection parameters (and other static permissions, including schedule-only)
    tinympl::bool_<
      // Parameter is an AccessHandleCollection
      ParamTraits::template matches<decayed_is_access_handle_collection>::value
    >, /* => */ typename tinympl::switch_value_<
      access_handle_permissions_t, _required_scheduling_permissions<
        std::decay_t<typename ParamTraits::type>
      >::value
    > ::template case_<AccessHandlePermissions::Modify, _permissions<HandleUse::Modify>>
      ::template case_<AccessHandlePermissions::Read, _permissions<HandleUse::Read>>
      ::template case_<AccessHandlePermissions::None, _permissions<HandleUse::None>>
      ::template case_<AccessHandlePermissions::Commutative, _permissions<HandleUse::Commutative>>
      ::template default_<_permissions<HandleUse::Modify>>
    ::type,
    /*----------------------------------------*/
    // TODO check the actual scheduling permissions static flags rather than relying on the immediate flags
    // TODO reinstate support for single-entry-in-collection parameters when it's implemented elsewhere?
    //tinympl::bool_<
    //  (
    //    // Parameter is an AccessHandle
    //    ParamTraits::template matches<decayed_is_access_handle>::value
    //      // Parameter is compile-time immediate read-only
    //      and ParamTraits::template matches<
    //        compile_time_immediate_read_only_if_access_handle
    //      >::value
    //  )
    //>, /* => */ _permissions<HandleUse::Read>,
    /*----------------------------------------*/
    //tinympl::bool_<
    //  (
    //    // Parameter is an AccessHandle
    //    ParamTraits::template matches<decayed_is_access_handle>::value
    //      // Parameter is compile-time immediate read-only
    //      and not ParamTraits::template matches<
    //        compile_time_immediate_read_only_if_access_handle
    //      >::value
    //  )
    //>, /* => */ _permissions<HandleUse::Modify>,
    /*----------------------------------------*/
    //tinympl::bool_<
    //  (
    //    // Parameter is not an AccessHandle
    //    (not ParamTraits::template matches<decayed_is_access_handle>::value)
    //  )
    //>, /* => */ _permissions<HandleUse::None>
    /*----------------------------------------*/
    std::true_type, /* => */ _darma__static_failure<
      GivenArg,
      _____________________________________________________________________,
      ParamTraits,
      _____________________________________________________________________,
      CollectionIndexRangeT
      // TODO better error message in the failure case
    >
  >::type::value;

  // </editor-fold> end  }}}2
  //----------------------------------------------------------------------------

  template <typename TaskCollectionT>
  return_type
  operator()(TaskCollectionT& collection, GivenArg&& arg) const {

    using mapped_handle_t = std::decay_t<GivenArg>;
    using handle_collection_t = typename mapped_handle_t::access_handle_collection_t;
    using handle_range_t = typename handle_collection_t::index_range_type;

    // This is the user's mapping type.  It maps from handle frontend index to
    // collection frontend index.
    // To get a mapping from frontend index to backend index, we need to composite it
    // with the task collection's mapping-to-dense
    using user_mapping_t = std::decay_t<typename mapped_handle_t::mapping_t>;
    using user_mapping_traits = indexing::mapping_traits<user_mapping_t>;
    using task_collection_t = std::decay_t<TaskCollectionT>;
    using tc_index_range = typename task_collection_t::index_range_t;
    using tc_index_range_traits = typename task_collection_t::index_range_traits;

    // mapping all the way from frontend handle index to backend tc index
    using full_mapping_t = CompositeMapping<
      user_mapping_t,
      typename tc_index_range_traits::mapping_to_dense_type
      //, detail::not_an_index_range /* for now; make this handle_range_t later once implemented in composite mapping */
      //, tc_index_range // second mapping "from" range type
    >;
    using full_mapping_traits = indexing::mapping_traits<full_mapping_t>;

    // Custom create use holder callable for the captured use holder
    auto captured_use_holder_maker = [&](
      auto handle,
      auto scheduling_permissions,
      auto immediate_permissions,
      auto&& in_rel,
      auto&& out_rel
#if _darma_has_feature(anti_flows)
      , auto&& anti_in_rel,
      auto&& anti_out_rel
#endif // _darma_has_feature(anti_flows)
    ) {
      return std::make_shared<
        GenericUseHolder<CollectionManagingUse<handle_range_t>>
      >(
        CollectionManagingUse<handle_range_t>(
          handle,
          scheduling_permissions, immediate_permissions,
          std::move(in_rel).as_collection_relationship(),
          std::move(out_rel).as_collection_relationship(),
#if _darma_has_feature(anti_flows)
          std::move(anti_in_rel).as_collection_relationship(),
          std::move(anti_out_rel).as_collection_relationship(),
#endif // _darma_has_feature(anti_flows)
          arg.collection.get_index_range(),
          full_mapping_t(
            arg.mapping,
            tc_index_range_traits::mapping_to_dense(collection.collection_range_)
            //, collection.collection_range_
          )
        )
      );
    };

    // Custom create use holder callable for the continuation use
    auto continuation_use_holder_maker = [&](
      auto handle,
      auto scheduling_permissions,
      auto immediate_permissions,
      auto&& in_rel,
      auto&& out_rel
#if _darma_has_feature(anti_flows)
      , auto&& anti_in_rel,
      auto&& anti_out_rel
#endif // _darma_has_feature(anti_flows)
    ) {
      return CollectionManagingUse<handle_range_t>(
        handle,
        scheduling_permissions, immediate_permissions,
        std::move(in_rel).as_collection_relationship(),
        std::move(out_rel).as_collection_relationship(),
#if _darma_has_feature(anti_flows)
        std::move(anti_in_rel).as_collection_relationship(),
        std::move(anti_out_rel).as_collection_relationship(),
#endif // _darma_has_feature(anti_flows)
        arg.collection.get_index_range()
      );
    };

    // Finally, make the return type...
    auto rv = return_type(
      handle_collection_t(
        arg.collection.var_handle_.get_smart_ptr(),
        darma_runtime::detail::make_captured_use_holder(
          arg.collection.var_handle_base_,
          /* Requested Scheduling permissions: */
          required_scheduling_permissions,
          /* Requested Immediate permissions: */
          required_immediate_permissions,
          /* source and continuing use handle */
          arg.collection.current_use_.get(),
          // Customization functors:
          captured_use_holder_maker,
          continuation_use_holder_maker
        ) // end arguments to make_captured_use_holder
      ),
      arg.mapping
    );
    collection.add_dependency(rv.collection.current_use_->use.get());
    return rv;
  }

};

// </editor-fold> end MappedHandleCollection version }}}1
//==============================================================================


//==============================================================================
// <editor-fold desc="AccessHandleCollection version"> {{{1

template <typename GivenArg, typename ParamTraits, typename CollectionIndexRangeT>
struct _get_storage_arg_helper<
  GivenArg, ParamTraits, CollectionIndexRangeT,
  std::enable_if_t<
    detail::decayed_is_access_handle_collection<GivenArg>::value
  >
>
{
  //----------------------------------------------------------------------------
  // <editor-fold desc="type aliases and traits"> {{{2

  // TODO use a less circuitous mapping in the one-to-one case?

  using collection_range_t = CollectionIndexRangeT;
  using collection_range_traits =
    indexing::index_range_traits<CollectionIndexRangeT>;

  using handle_range_t = typename std::decay_t<GivenArg>::index_range_type;
  using handle_range_traits = indexing::index_range_traits<handle_range_t>;

  using _default_mapping_t = CompositeMapping<
    typename handle_range_traits::mapping_to_dense_type,
    ReverseMapping<typename collection_range_traits::mapping_to_dense_type>
  >;

  // See if it needs to be mapped (i.e., if it needs immediate permissions)
  static constexpr auto needs_mapping = std::decay_t<typename ParamTraits::type>::traits
    ::permissions_traits::required_immediate_permissions
      != AccessHandlePermissions::None;

  template <typename ArgNeedingMapping>
  using _default_mapped_helper_t = _get_storage_arg_helper<
    // Note that this type gets decayed in the companion helper, so no reason
    // to also do so here
    decltype(std::declval<ArgNeedingMapping>().mapped_with(
      _default_mapping_t{
        std::declval<typename handle_range_traits::mapping_to_dense_type>(),
        std::declval<
          decltype(
            make_reverse_mapping(
              std::declval<typename handle_range_traits::mapping_to_dense_type>()
            )
          )
        >()
      }
    )),
    ParamTraits, CollectionIndexRangeT
  >;

  using type = typename std::conditional_t<
    needs_mapping,
    typename tinympl::lazy<_default_mapped_helper_t>::template applied_to<GivenArg>,
    tinympl::identity<UnmappedHandleCollection<std::decay_t<GivenArg>>>
  >::type;
  using return_type = type; // readability alias

  // </editor-fold> end type aliases and traits }}}2
  //----------------------------------------------------------------------------

  //----------------------------------------------------------------------------
  // <editor-fold desc="default mapped version"> {{{2

  template <typename TaskCollectionT>
  return_type
  operator()(
    TaskCollectionT& collection, GivenArg&& arg,
    std::enable_if_t<
      not std::is_void<TaskCollectionT>::value // should be always true
        and needs_mapping,
      _not_a_type
    > = {}
  ) const {

    // First, check that the identity mapping is valid...
    DARMA_ASSERT_EQUAL_VERBOSE(
      arg.get_index_range().size(), collection.collection_range_.size()
    );

    // This default should probably be:
    // Composite of mapping_to_dense(handle_collection) -> reverse(mapping_to_dense(task_collection))
    return _default_mapped_helper_t<GivenArg>{}(
      collection, std::forward<GivenArg>(arg).mapped_with(_default_mapping_t{
          handle_range_traits::mapping_to_dense(arg.get_index_range()),
          // Intentionally leave this as ADL; user could want to override it
          make_reverse_mapping(
            collection_range_traits::mapping_to_dense(collection.collection_range_)
          )
        }
      )
    );
  }

  // </editor-fold> end default mapped version }}}2
  //----------------------------------------------------------------------------

  //----------------------------------------------------------------------------
  // <editor-fold desc="unmapped version"> {{{2

  template <typename TaskCollectionT>
  return_type
  operator()(
    TaskCollectionT& collection, GivenArg&& arg,
    std::enable_if_t<
      not std::is_void<TaskCollectionT>::value // should be always true
        and not needs_mapping,
      _not_a_type
    > = { }
  ) const {

    using handle_collection_t = typename return_type::access_handle_collection_t;
    using handle_range_t = typename handle_collection_t::index_range_type;

    // Custom create use holder callable for the captured use holder
    auto captured_use_holder_maker = [&](
      auto handle,
      auto scheduling_permissions,
      auto immediate_permissions,
      auto&& in_rel,
      auto&& out_rel
#if _darma_has_feature(anti_flows)
      , auto&& anti_in_rel,
      auto&& anti_out_rel
#endif // _darma_has_feature(anti_flows)
    ) {
      return std::make_shared<
        GenericUseHolder<CollectionManagingUse<handle_range_t>>
      >(
        CollectionManagingUse<handle_range_t>(
          handle,
          scheduling_permissions, immediate_permissions,
          std::move(in_rel).as_collection_relationship(),
          std::move(out_rel).as_collection_relationship(),
#if _darma_has_feature(anti_flows)
          std::move(anti_in_rel).as_collection_relationship(),
          std::move(anti_out_rel).as_collection_relationship(),
#endif // _darma_has_feature(anti_flows)
          arg.get_index_range()
        ), true, true
      );
    };

    // Custom create use holder callable for the continuation use
    auto continuation_use_holder_maker = [&](
      auto handle,
      auto scheduling_permissions,
      auto immediate_permissions,
      auto&& in_rel,
      auto&& out_rel
#if _darma_has_feature(anti_flows)
      , auto&& anti_in_rel,
      auto&& anti_out_rel
#endif // _darma_has_feature(anti_flows)
    ) {
      return CollectionManagingUse<handle_range_t>(
        handle,
        scheduling_permissions, immediate_permissions,
        std::move(in_rel).as_collection_relationship(),
        std::move(out_rel).as_collection_relationship(),
#if _darma_has_feature(anti_flows)
        std::move(anti_in_rel).as_collection_relationship(),
        std::move(anti_out_rel).as_collection_relationship(),
#endif // _darma_has_feature(anti_flows)
        arg.get_index_range()
      );
    };

    // Finally, make the return type...
    auto rv = return_type(
      handle_collection_t(
        arg.var_handle_.get_smart_ptr(),
        darma_runtime::detail::make_captured_use_holder(
          arg.var_handle_base_,
          /* Requested Scheduling permissions: */
          HandleUse::Commutative,
          /* Requested Immediate permissions: */
          HandleUse::None,
          /* source and continuing use handle */
          arg.current_use_.get(),
          // Customization functors:
          captured_use_holder_maker,
          continuation_use_holder_maker
        ) // end arguments to make_captured_use_holder
      )
    );
    collection.add_dependency(rv.collection.current_use_->use.get());

    return std::move(rv);
  }

  // </editor-fold> end unmapped version }}}2
  //----------------------------------------------------------------------------
};

// </editor-fold> end AccessHandleCollection version }}}1
//==============================================================================

} // end namespace _task_collection_impl
} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_TASK_COLLECTION_IMPL_ARGUMENT_TO_TC_STORAGE_H
