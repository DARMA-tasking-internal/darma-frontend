/*
//@HEADER
// ************************************************************************
//
//                      task_collection.h
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

#ifndef DARMA_IMPL_TASK_COLLECTION_TASK_COLLECTION_H
#define DARMA_IMPL_TASK_COLLECTION_TASK_COLLECTION_H

#include <darma/impl/polymorphic_serialization.h>
#include <darma/impl/handle.h>
#include <darma/impl/task_collection/handle_collection.h>
#include <darma/interface/frontend/task_collection.h>
#include <darma/impl/capture.h>
#include <darma/impl/task.h>
#include "task_collection_fwd.h"


namespace darma_runtime {

namespace detail {

namespace _task_collection_impl {


//==============================================================================
// <editor-fold desc="_get_storage_arg_helper for constructing a TaskCollection">

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
      >
  >;
  using return_type = type; // readability

  template <typename TaskCollectionT>
  auto
  operator()(TaskCollectionT& collection, GivenArg&& arg) const {
    auto rv = return_type(
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

  //template <HandleUse::permissions_t sched, HandleUse::permissions_t immed>
  //using _permissions = std::integer_sequence<HandleUse::permissions_t, sched, immed>;
  template <HandleUse::permissions_t p>
  using _permissions = std::integer_sequence<HandleUse::permissions_t, p>;

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
    auto rv = std::decay_t<GivenArg>(
      detail::make_captured_use_holder(
        arg.collection.var_handle_,
        /* Requested Scheduling permissions: */
        required_scheduling_permissions,
        /* Requested Immediate permissions: */
        required_immediate_permissions,
        /* source and continuing use handle */
        arg.current_use_,
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
              std::forward<decltype(flow)>(flow)
            )
          );
        } // end of the custom "next flow maker"
      ) // end arguments to make_captured_use_holder
    );
    collection.add_dependency(&(rv.current_use_->use));
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

// </editor-fold> end _get_storage_arg_helper for constructing a TaskCollection
//==============================================================================

//==============================================================================
// <editor-fold desc="_get_task_stored_arg_helper for creating TaskCollectionTasks">

// Default "regular argument" case
template <
  typename Functor, typename CollectionArg, size_t Position,
  typename Enable /* =void */
>
struct _get_task_stored_arg_helper {
  // TODO decide if this should be allowed to be a reference to the parent (probably not...)
  using type = std::decay_t<CollectionArg>;
  template <typename TaskInstanceT>
  type
  operator()(TaskInstanceT& collection, CollectionArg const& arg) const {
    return arg;
  }
};

// AccessHandle case
template <typename Functor, typename CollectionArg, size_t Position>
struct _get_task_stored_arg_helper<
  Functor, CollectionArg, Position,
  std::enable_if_t<decayed_is_access_handle<CollectionArg>::value>
> {
  using type = CollectionArg;

  template <typename TaskInstanceT>
  type
  operator()(TaskInstanceT& instance, CollectionArg const& arg) const {
    // We still need to create a new use for the task itself...
    auto new_use_holder = std::make_shared<UseHolder>(
      HandleUse(
        arg.var_handle_,
        arg.current_use_->use.in_flow_,
        arg.current_use_->use.out_flow_,
        arg.current_use_->use.scheduling_permissions_, // better be something like Read or less
        arg.current_use_->use.immediate_permissions_  // better be something like Read or less
      )
    );
    new_use_holder->do_register();
    return CollectionArg(new_use_holder);
  }

};

// Mapped HandleCollection case
// TODO non-AccessHandleCollection parameters should probably be deprecated for AccessHandleCollection arguments
template <typename Functor, typename CollectionArg, size_t Position>
struct _get_task_stored_arg_helper<
  Functor, CollectionArg, Position,
  std::enable_if_t<
    tinympl::is_instantiation_of<
      MappedHandleCollection,
      std::decay_t<CollectionArg>>::value
  >
> {
  using type = AccessHandle<typename CollectionArg::access_handle_collection_t::value_type>;
  using return_type = type; // readability

  template <typename TaskInstanceT>
  return_type
  operator()(TaskInstanceT& instance, CollectionArg const& arg) const {
    auto* backend_runtime = abstract::backend::get_backend_runtime();

    auto index_range_mapping_to_dense = get_mapping_to_dense(
      arg.collection.current_use_->use.index_range
    );

    // mapping in the collection stored arg already incorporates frontend->backend
    // transformation of the collection index, so we can use it here directly
    // with the backend index.  However, we still need to convert the use collection
    // fronend index to a backend index
    size_t backend_collection_index = index_range_mapping_to_dense.map_forward(
      arg.mapping.map_backward(instance.backend_index_)
    );


    auto local_in_flow = detail::make_flow_ptr(
      backend_runtime->make_indexed_local_flow(
        arg.collection.current_use_.in_flow_,
        backend_collection_index
      )
    );

    auto local_out_flow = detail::make_flow_ptr(
      backend_runtime->make_indexed_local_flow(
        arg.collection.current_use_.out_flow_,
        backend_collection_index
      )
    );

    auto new_use_holder = std::make_shared<UseHolder>(
      HandleUse(
        arg.collection.var_handle_,
        local_in_flow, local_out_flow,
        arg.collection.current_use_->use.scheduling_permissions_,
        arg.collection.current_use_->use.immediate_permissions_
      )
    );
    new_use_holder->do_register();

    auto rv = return_type(new_use_holder);
  }

};


// TODO Mapped HandleCollection -> Mapped HandleCollection case

// </editor-fold> end _get_task_stored_arg_helper for creating TaskCollectionTasks
//==============================================================================

//==============================================================================
// <editor-fold desc="_get_call_arg_helper">

// TODO parameter is a AccessHandleCollection case
// TODO clean this up and generalize it?

template <typename StoredArg, typename ParamTraits, typename Enable=void>
struct _get_call_arg_helper; // TODO better error message for default case

// Normal arg, param by value
template <typename StoredArg, typename ParamTraits>
struct _get_call_arg_helper<StoredArg, ParamTraits,
  std::enable_if_t<
    // Arg is not AccessHandle
    (not is_access_handle<StoredArg>::value)
    // Param is not AccessHandle, is by value
    and (not ParamTraits::template matches<decayed_is_access_handle>::value)
    and ParamTraits::is_by_value
  >
>{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    // Move into the parameter
    return std::move(arg);
  }
};

// Normal arg, param is const ref value
template <typename StoredArg, typename ParamTraits>
struct _get_call_arg_helper<StoredArg, ParamTraits,
  std::enable_if_t<
    // Arg is not AccessHandle
    (not is_access_handle<StoredArg>::value)
    // Param is not AccessHandle, but is const ref
    and (not ParamTraits::template matches<decayed_is_access_handle>::value)
    and ParamTraits::is_const_lvalue_reference
  >
>{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    // Return a reference to the argument in the tuple
    // TODO make sure this works
    return arg;
  }
};

// AccessHandle arg, Param is by value or const lvalue reference
template <typename StoredArg, typename ParamTraits>
struct _get_call_arg_helper<StoredArg, ParamTraits,
  std::enable_if_t<
    // StoredArg is an AccessHandle
    is_access_handle<StoredArg>::value
    // Param is by value or a const lvalue reference
    and (
      ParamTraits::is_by_value
      or ParamTraits::is_const_lvalue_reference
    )
  >

>{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    return arg.get_value();
  }
};

// AccessHandle arg, Param is non-const lvalue reference
template <typename StoredArg, typename ParamTraits>
struct _get_call_arg_helper<StoredArg, ParamTraits,
  std::enable_if_t<
    // StoredArg is an AccessHandle
    is_access_handle<StoredArg>::value
    // Param is non-const lvalue reference
    and ParamTraits::is_nonconst_lvalue_reference
  >

>{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    return arg.get_reference();
  }
};

// AccessHandle arg, Param is AccessHandle (by value)
template <typename StoredArg, typename ParamTraits>
struct _get_call_arg_helper<StoredArg, ParamTraits,
  std::enable_if_t<
    // StoredArg is an AccessHandle
    is_access_handle<StoredArg>::value
    // Param is AccessHandle by value (so we can move into it)
    and (
      ParamTraits::template matches<decayed_is_access_handle>::value
      and ParamTraits::is_by_value
    )
  >

>{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    return std::move(arg);
  }
};

// AccessHandle arg, Param is AccessHandle (by reference; can't move into it)
template <typename StoredArg, typename ParamTraits>
struct _get_call_arg_helper<StoredArg, ParamTraits,
  std::enable_if_t<
    // StoredArg is an AccessHandle
    is_access_handle<StoredArg>::value
    // Param is AccessHandle is a (potentially const) lvalue reference  (so we can't move into it)
    and (
      ParamTraits::template matches<decayed_is_access_handle>::value
      and (
        ParamTraits::is_nonconst_lvalue_reference
        or ParamTraits::is_const_lvalue_reference
      )
    )
  > // end enable_if_t
>{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    return arg;
  }
};

// </editor-fold> end _get_call_arg_helper
//==============================================================================

} // end namespace _task_collection_impl

//==============================================================================
// <editor-fold desc="TaskCollectionTaskImpl">

template <
  typename Functor, typename Mapping,
  typename... StoredArgs
>
struct TaskCollectionTaskImpl
  : abstract::frontend::TaskCollectionTask<TaskBase>
{
  using base_t = abstract::frontend::TaskCollectionTask<TaskBase>;

  /* TODO implement this */

  using args_tuple_t = std::tuple<StoredArgs...>;

  size_t backend_index_;
  // This is the mapping from frontend index to backend index for the collection itself
  Mapping mapping_;
  args_tuple_t args_;

  template <size_t... Spots>
  decltype(auto)
  _get_call_args_impl(
    std::index_sequence<Spots...>
  ) {
    return std::forward_as_tuple(
      _task_collection_impl::_get_call_arg_helper<
        typename std::tuple_element<Spots, args_tuple_t>::type,
        typename meta::callable_traits<Functor>::template param_n_traits<Spots>
      >{}(
        std::get<Spots>(std::move(args_))
      )...
    );
  }

  template <typename... StoredArgsDeduced>
  TaskCollectionTaskImpl(
    std::size_t backend_index, Mapping const& mapping,
    StoredArgsDeduced&&... args
  ) : backend_index_(backend_index),
      mapping_(mapping),
      args_(std::forward<StoredArgsDeduced>(args)...)
  { }

  void run() override {
    meta::splat_tuple(
      _get_call_args_impl(std::index_sequence_for<StoredArgs...>{}),
      [&](auto&&... args) {
        Functor()(
          mapping_.map_backward(backend_index_),
          std::forward<decltype(args)>(args)...
        );
      }
    );
  }


  bool is_migratable() const override {
    return false;
  }

};

// </editor-fold> end TaskCollectionTaskImpl
//==============================================================================

//==============================================================================
// <editor-fold desc="TaskCollectionImpl">


template <
  typename Functor,
  typename IndexRangeT,
  typename... Args
>
struct TaskCollectionImpl
  : PolymorphicSerializationAdapter<
      TaskCollectionImpl<Functor, IndexRangeT, Args...>,
      abstract::frontend::TaskCollection
    >
{
  public:
    using index_range_t = IndexRangeT;

  protected:

    template <typename ArgsForwardedTuple, size_t... Spots>
    decltype(auto)
    _get_args_stored_impl(
      ArgsForwardedTuple&& args_fwd,
      std::index_sequence<Spots...>
    ) {
      return std::forward_as_tuple(
        _task_collection_impl::_get_storage_arg_helper<
          decltype(std::get<Spots>(std::forward<ArgsForwardedTuple>(args_fwd))),
          typename meta::callable_traits<Functor>::template param_n_traits<Spots>
        >{}(
          *this, std::get<Spots>(std::forward<ArgsForwardedTuple>(args_fwd))
        )...
      );
    }

    template <size_t... Spots>
    auto _make_task_impl(
      std::size_t index,
      std::index_sequence<Spots...>
    ) {
      return std::make_unique<
        TaskCollectionTaskImpl<
          Functor, mapping_to_dense_t<index_range_t>,
          typename _task_collection_impl::_get_task_stored_arg_helper<
            Functor, decltype(std::get<Spots>(args_stored_)), Spots
          >::type...
        >>(
          index, get_mapping_to_dense(collection_range_),
          _task_collection_impl::_get_task_stored_arg_helper<
              Functor, decltype(std::get<Spots>(args_stored_)), Spots
            >{}(*this, std::get<Spots>(args_stored_))...
      );
    }


  public:

    // Leave this member declaration order the same; construction of args_stored_
    // depends on collection_range_ being initialized already

    types::handle_container_template<abstract::frontend::Use*> dependencies_;
    IndexRangeT collection_range_;
    std::tuple<Args...> args_stored_;

    template <typename Archive>
    void serialize(Archive& ar) {
      /* TODO write this!!! */
    }

    void add_dependency(abstract::frontend::Use* dep) {
      dependencies_.insert(dep);
    }

    //==========================================================================
    // Ctors

    TaskCollectionImpl() = default;

    template <typename IndexRangeDeduced, typename... ArgsForwarded>
    TaskCollectionImpl(
      IndexRangeDeduced&& collection_range,
      ArgsForwarded&& ... args_forwarded
    ) : collection_range_(std::forward<IndexRangeDeduced>(collection_range)),
        args_stored_(
          _get_args_stored_impl(
            std::forward_as_tuple(std::forward<ArgsForwarded>(args_forwarded)...),
            std::index_sequence_for<ArgsForwarded...>()
          )
        )
    { }


    size_t size() const override { return collection_range_.size(); }

    std::unique_ptr<types::task_collection_task_t>
    create_task_for_index(std::size_t index) override {

      return _make_task_impl(
        index, std::make_index_sequence<sizeof...(Args)>()
      );
    }

    // This should really return something ternary like "known false, known true, or unknown"
    bool
    all_mappings_same_as(abstract::frontend::TaskCollection const* other) const override {
      /* TODO */
      return false;
    }

    types::handle_container_template<abstract::frontend::Use*> const&
    get_dependencies() const override {
      return dependencies_;
    }

    template <typename, typename, typename>
    friend struct _task_collection_impl::_get_storage_arg_helper;

    template <typename, typename, size_t, typename>
    friend struct _task_collection_impl::_get_task_stored_arg_helper;

};

template <typename Functor, typename IndexRangeT, typename... Args>
struct make_task_collection_impl_t {
  private:

    using arg_vector_t = tinympl::vector<Args...>;

    template <typename T> struct _helper;

    template <size_t... Idxs>
    struct _helper<std::index_sequence<Idxs...>> {
      using type = TaskCollectionImpl<
        Functor, IndexRangeT,
        typename detail::_task_collection_impl::_get_storage_arg_helper<
          tinympl::at_t<Idxs, arg_vector_t>,
          typename meta::callable_traits<Functor>::template param_n_traits<Idxs>
        >::type...
      >;
    };

  public:

    using type = typename _helper<std::index_sequence_for<Args...>>::type;

};

// </editor-fold> end TaskCollectionImpl
//==============================================================================


} // end namespace detail


template <typename Functor, typename... Args>
void create_concurrent_work(Args&&... args) {
  using namespace darma_runtime::detail;
  using darma_runtime::keyword_tags_for_create_concurrent_region::index_range;
  using parser = kwarg_parser<
    variadic_positional_overload_description<
      _keyword<deduced_parameter, index_range>
    >
    // TODO other overloads
  >;

  // This is on one line for readability of compiler error; don't respace it please!
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

  parser()
    .parse_args(std::forward<Args>(args)...)
    .invoke([](
      auto&& index_range,
      variadic_arguments_begin_tag,
      auto&&... args
    ){
      using task_collection_impl_t = typename detail::make_task_collection_impl_t<
        Functor, std::decay_t<decltype(index_range)>, decltype(args)...
      >::type;

      auto task_collection = std::make_unique<task_collection_impl_t>(
        std::forward<decltype(index_range)>(index_range),
        std::forward<decltype(args)>(args)...
      );

      auto* backend_runtime = abstract::backend::get_backend_runtime();
      backend_runtime->register_task_collection(
        std::move(task_collection)
      );

    });

}

} // end namespace darma_runtime

#endif //DARMA_IMPL_TASK_COLLECTION_TASK_COLLECTION_H
