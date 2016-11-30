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

#include <darma/interface/app/access_handle.h>

#include <darma/impl/handle.h>
#include <darma/impl/capture.h>
#include <darma/impl/task_collection/handle_collection.h>


namespace darma_runtime {
namespace detail {
namespace _task_collection_impl {

// TODO more readable errors when none of these work

// Default "regular argument" case
template <
  typename GivenArg, typename ParamTraits, typename Enable /* =void */
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



// AccessHandle-like
template <typename GivenArg, typename ParamTraits>
struct _get_storage_arg_helper<
  GivenArg, ParamTraits,
  std::enable_if_t<
    // The argument is an access handle
    decayed_is_access_handle<GivenArg>::value
>
> {
// If the argument is an AccessHandle, the parameter cannot modify:
static_assert(
  not ParamTraits::is_nonconst_lvalue_reference,
  "Cannot pass \"plain-old\" AccessHandle to modify parameter of concurrent work"
    " call.  Use an AccessHandleCollection instead"
);

using type = AccessHandle<
  typename std::decay_t<GivenArg>::value_type,
  typename access_handle_traits<>
  ::template with_max_schedule_permissions<
    ParamTraits::template matches<decayed_is_access_handle>::value ?
      AccessHandlePermissions::Read : AccessHandlePermissions::None
  >::type::template with_max_immediate_permissions<
    // TODO check for a schedule-only AccessHandle parameter
    AccessHandlePermissions::Read
  >::type
>;
using return_type = type; // readability

template <typename TaskCollectionT>
auto
operator()(TaskCollectionT& collection, GivenArg&& arg) const {
  auto rv = return_type(
    arg.var_handle_,
    detail::make_captured_use_holder(
      arg.var_handle_,
      /* Requested Scheduling permissions: */
      return_type::is_compile_time_schedule_readable ?
        HandleUse::Read : HandleUse::None,
      /* Requested Immediate permissions: */
      // TODO check params(/args?) for schdule-only permissions request
      HandleUse::Read,
      /* source and continuing context use holder */
      arg.current_use_
    )
  );
  collection.add_dependency(&(rv.current_use_->use));
  return rv;
}
};

// MappedHandleCollection
template <typename GivenArg, typename ParamTraits>
struct _get_storage_arg_helper<
  GivenArg, ParamTraits,
  std::enable_if_t<
    tinympl::is_instantiation_of<
    MappedHandleCollection,
  std::decay_t<GivenArg>>::value
>
> {
using type = std::decay_t<GivenArg>;
using return_type = type; // readability

//template <HandleUse::permissions_t sched, HandleUse::permissions_t immed>
//using _permissions = std::integer_sequence<HandleUse::permissions_t, sched, immed>;
template <HandleUse::permissions_t p>
using _permissions = std::integral_constant<HandleUse::permissions_t, p>;

template <typename T>
using _compile_time_immediate_read_only_archetype = tinympl::bool_<
  std::decay_t<T>::is_compile_time_immediate_read_only
>;
template <typename T>
using compile_time_immediate_read_only_if_access_handle = meta::detected_or_t<std::false_type,
  _compile_time_immediate_read_only_archetype, T
>;

// TODO static schedule-only permissions
// TODO non-AccessHandleCollection parameters should probably be deprecated

static constexpr auto required_immediate_permissions = tinympl::select_first<
  tinympl::bool_<(
  // Parameter is an AccessHandle
  ParamTraits::template matches<decayed_is_access_handle>::value
    // Parameter is compile-time immediate read-only
    and ParamTraits::template matches<compile_time_immediate_read_only_if_access_handle>::value
)>, /* => */ _permissions<HandleUse::Read>,
/*----------------------------------------*/
  tinympl::bool_<(
  // Parameter is an AccessHandle
  ParamTraits::template matches<decayed_is_access_handle>::value
    // Parameter is compile-time immediate read-only
    and not ParamTraits::template matches<compile_time_immediate_read_only_if_access_handle>::value
)>, /* => */ _permissions<HandleUse::Modify>,
/*----------------------------------------*/
  tinympl::bool_<(
  // Parameter is an not AccessHandle
  (not ParamTraits::template matches<decayed_is_access_handle>::value)
    // Parameter is by value or by const reference
    and (
      ParamTraits::is_by_value
        or ParamTraits::is_const_lvalue_reference
    )
)>, /* => */ _permissions<HandleUse::Read>,
/*----------------------------------------*/
// TODO read-only AccessHandleCollection parameters (or other static permissions?)
  tinympl::bool_<(
  // Parameter is an AccessHandleCollection
  (not ParamTraits::template matches<decayed_is_access_handle_collection>::value)
)>, /* => */ _permissions<HandleUse::Modify>,
/*----------------------------------------*/
  tinympl::bool_<(
  // Parameter is an not AccessHandle
  (not ParamTraits::template matches<decayed_is_access_handle>::value)
    // Parameter is a nonconst lvalue reference
    and ParamTraits::is_nonconst_lvalue_reference
)>, /* => */ _permissions<HandleUse::Modify>
/*----------------------------------------*/
// TODO better error message in the failure case
>::type::value;

static constexpr auto required_scheduling_permissions = tinympl::select_first<
  // TODO check the actual scheduling permissions static flags rather than relying on the immediate flags
  tinympl::bool_<(
  // Parameter is an AccessHandle
  ParamTraits::template matches<decayed_is_access_handle>::value
    // Parameter is compile-time immediate read-only
    and ParamTraits::template matches<compile_time_immediate_read_only_if_access_handle>::value
)>, /* => */ _permissions<HandleUse::Read>,
/*----------------------------------------*/
  tinympl::bool_<(
  // Parameter is an AccessHandle
  ParamTraits::template matches<decayed_is_access_handle>::value
    // Parameter is compile-time immediate read-only
    and not ParamTraits::template matches<compile_time_immediate_read_only_if_access_handle>::value
)>, /* => */ _permissions<HandleUse::Modify>,
/*----------------------------------------*/
// TODO read-only AccessHandleCollection parameters (or other static permissions?)
  tinympl::bool_<(
  // Parameter is an AccessHandleCollection
  (not ParamTraits::template matches<decayed_is_access_handle_collection>::value)
)>, /* => */ _permissions<HandleUse::Modify>,
/*----------------------------------------*/
  tinympl::bool_<(
  // Parameter is not an AccessHandle
  (not ParamTraits::template matches<decayed_is_access_handle>::value)
)>, /* => */ _permissions<HandleUse::None>
/*----------------------------------------*/
// TODO better error message in the failure case
>::type::value;

template <typename TaskCollectionT>
auto
operator()(TaskCollectionT& collection, GivenArg&& arg) const {
  using mapped_t = std::decay_t<GivenArg>;
  // This is the user's mapping type.  It maps from frontend index to frontend index.
  // To get a mapping from frontend index to backend index, we need to composite it
  // with the task collection's mapping-to-dense
  using user_mapping_t = std::decay_t<typename mapped_t::mapping_t>;
  using mapping_t = CompositeMapping<
  user_mapping_t,
  mapping_to_dense_t<typename TaskCollectionT::index_range_t>
  >;
  auto rv = return_type(typename return_type::access_handle_collection_t(
    arg.collection.var_handle_,
    detail::make_captured_use_holder<
      UseCollectionManagingHolder<
        typename mapped_t::access_handle_collection_t::index_range_type,
      mapping_t
    >
      >(
        arg.collection.var_handle_,
          /* Requested Scheduling permissions: */
          required_scheduling_permissions,
          /* Requested Immediate permissions: */
          required_immediate_permissions,
          /* source and continuing use handle */
          arg.collection.current_use_,
          // Custom create use holder callable
          [&](
            auto handle,
            auto const& in_flow, auto const& out_flow,
            auto scheduling_permissions,
            auto immediate_permissions
          ) {
            return std::make_shared<
              UseCollectionManagingHolder<
              typename mapped_t::access_handle_collection_t::index_range_type,
              mapping_t
                >>(
                  handle, in_flow, out_flow,
                    scheduling_permissions, immediate_permissions,
                    arg.collection.current_use_->use.index_range,
                    mapping_t(
                      arg.mapping,
                      get_mapping_to_dense(collection.collection_range_)
                    )
                );
          }, // end custom create use holder callable
          // Custom "next flow maker"
          [](auto&& flow, auto* backend_runtime) {
            return make_flow_ptr(
              backend_runtime->make_next_flow_collection(
                *std::forward<decltype(flow)>(flow).get()
              )
            );
          }, // end of the custom "next flow maker"
          // Custom create use holder callable for the continuation use
          [&](
            auto handle,
            auto const& in_flow, auto const& out_flow,
            auto scheduling_permissions,
            auto immediate_permissions
          ) {
            return std::make_shared<
              UseCollectionManagingHolder<
              typename mapped_t::access_handle_collection_t::index_range_type
                >>(
                  handle, in_flow, out_flow,
                    scheduling_permissions, immediate_permissions,
                    arg.collection.current_use_->use.index_range,
                    arg.mapping
                );
          } // end custom create use holder callable for continuation use
      ) // end arguments to make_captured_use_holder
  ), arg.mapping);
  collection.add_dependency(&(rv.collection.current_use_->use));
  return rv;
}

};

// AccessHandleCollection version
template <typename GivenArg, typename ParamTraits>
struct _get_storage_arg_helper<
  GivenArg, ParamTraits,
  std::enable_if_t<
    detail::decayed_is_access_handle_collection<GivenArg>::value
>
> {
// TODO use a less circuitous mapping
using _identity_mapping_t = IdentityMapping<typename std::decay_t<GivenArg>::index_range_type::index_t>;
using _identity_mapped_version_t = _get_storage_arg_helper<
  decltype(std::declval<GivenArg>().mapped_with(
    // If we've gotten here, the indices better at least be the same type
    _identity_mapping_t{}
  )), ParamTraits
>;

using type = typename _identity_mapped_version_t::type;

template <typename TaskCollectionT>
auto
operator()(TaskCollectionT& collection, GivenArg&& arg) const {
  return _identity_mapped_version_t()(
    collection, arg.mapped_with(_identity_mapping_t{})
  );
}

};

} // end namespace _task_collection_impl
} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_TASK_COLLECTION_IMPL_ARGUMENT_TO_TC_STORAGE_H
