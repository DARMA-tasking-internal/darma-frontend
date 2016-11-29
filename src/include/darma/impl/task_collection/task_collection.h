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

namespace darma_runtime {

namespace detail {

//==============================================================================
// <editor-fold desc="_get_storage_arg_helper for constructing a TaskCollection">

// Default "regular argument" case
template <
  typename Functor, typename GivenArg, size_t Position, typename Enable=void
>
struct _get_storage_arg_helper {
  using type = std::decay_t<GivenArg>;
  template <typename TaskCollectionT>
  auto
  operator()(TaskCollectionT& collection, GivenArg&& arg) const {
    return arg;
  }
};

// AccessHandle-like
template <typename Functor, typename GivenArg, size_t Position>
struct _get_storage_arg_helper<
  Functor, GivenArg, Position,
  std::enable_if_t<decayed_is_access_handle<GivenArg>::value>
> {
  // TODO make this AccessHandle have static read-only permissions
  using type = std::decay_t<GivenArg>;

  template <typename TaskCollectionT>
  auto
  operator()(TaskCollectionT& collection, GivenArg&& arg) const {
    auto rv = std::decay_t<GivenArg>(
      detail::make_captured_use_holder(
        arg.var_handle_,
        /* Requested Scheduling permissions: */
        HandleUse::Read, // TODO check params for leaf-ness
        /* Requested Immediate permissions: */
        HandleUse::Read, // TODO check params for schdule-only permissions request
        arg.current_use_
      )
    );
    collection.add_dependency(&(rv.current_use_->use));
    return rv;
  }
};

// MappedHandleCollection
template <typename Functor, typename GivenArg, size_t Position>
struct _get_storage_arg_helper<
  Functor, GivenArg, Position,
  std::enable_if_t<
    tinympl::is_instantiation_of<
      MappedHandleCollection,
      std::decay_t<GivenArg>>::value
  >
> {
  using type = std::decay_t<GivenArg>;

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
        HandleUse::Read, // TODO CHANGE THIS BASED ON THE PARAMETER!!!
        /* Requested Immediate permissions: */
        HandleUse::None, // TODO change this based on the parameter?
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
            handle,
            in_flow,
            out_flow,
            scheduling_permissions,
            immediate_permissions,
            arg.collection.current_use_->use.index_range,
            mapping_t(
              arg.mapping,
              get_mapping_to_dense(collection.collection_range_)
            )
          );
        }
      )
    );
    collection.add_dependency(&(rv.current_use_->use));
    return rv;
  }

};

// AccessHandleCollection version
template <typename Functor, typename GivenArg, size_t Position>
struct _get_storage_arg_helper<
  Functor, GivenArg, Position,
  std::enable_if_t<
    detail::decayed_is_access_handle_collection<GivenArg>::value
  >
> {
  // TODO use a less circuitous mapping
  using _identity_mapping_t = IdentityMapping<typename std::decay_t<GivenArg>::index_range_type::index_t>;
  using _identity_mapped_version_t = typename _get_storage_arg_helper<
    Functor, decltype(std::declval<GivenArg>().mapped_with(
      // If we've gotten here, the indices better at least be the same type
      identity_mapping_t()
    )), Position
  >;

  using type = typename _identity_mapped_version_t::type;

  template <typename TaskCollectionT>
  auto
  operator()(TaskCollectionT& collection, GivenArg&& arg) const {
    return _identity_mapped_version_t()(
      collection, arg.mapped_with(identity_mapping_t())
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
  typename Enable=void
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
  operator()(TaskInstanceT& collection, CollectionArg const& arg) const {
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

// TODO !!!MappedHandleCollection case

// </editor-fold> end _get_task_stored_arg_helper for creating TaskCollectionTasks
//==============================================================================

//==============================================================================
// <editor-fold desc="_get_call_arg_helper">

// TODO clean this up and generalize it?

template <typename StoredArg, typename Param, typename Enable=void>
struct _get_call_arg_helper; // TODO better error message for default case

// Normal arg, param by value
template <typename StoredArg, typename Param>
struct _get_call_arg_helper<StoredArg, Param,
  std::enable_if_t<
    std::is_same<std::decay_t<Param>, Param>::value
    and not is_access_handle<StoredArg>::value
  >
>{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    // Move into the parameter
    return std::move(arg);
  }
};

// Normal arg, param is const ref value
template <typename StoredArg, typename Param>
struct _get_call_arg_helper<StoredArg, Param,
  std::enable_if_t<
    std::is_lvalue_reference<Param>::value
      and std::is_const<std::remove_reference_t<Param>>::value
      and not is_access_handle<StoredArg>::value
  >
>{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    // Return a reference to the argument in the tuple
    // TODO make sure this works
    return arg;
  }
};

// AccessHandle arg, Param is not non-const lvalue reference
template <typename StoredArg, typename Param>
struct _get_call_arg_helper<StoredArg, Param,
  std::enable_if_t<
    (
      // const lvalue ref
      (std::is_lvalue_reference<Param>::value
        and std::is_const<std::remove_reference_t<Param>>::value)
      // or by value
      or std::is_same<std::decay_t<Param>, Param>::value
    )
    // and StoredArg is an AccessHandle
    and not is_access_handle<StoredArg>::value
  >

>{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    return arg.get_value();
  }
};

// AccessHandle arg, Param is non-const lvalue reference
template <typename StoredArg, typename Param>
struct _get_call_arg_helper<StoredArg, Param,
  std::enable_if_t<
    // param is non-const lvalue ref
    (
      std::is_lvalue_reference<Param>::value
      and not std::is_const<std::remove_reference_t<Param>>::value
    )
    // and StoredArg is an AccessHandle
    and not is_access_handle<StoredArg>::value
  >

>{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    return arg.get_reference();
  }
};

// </editor-fold> end _get_call_arg_helper
//==============================================================================

//==============================================================================
// <editor-fold desc="TaskCollectionTaskImpl">

template <
  typename Functor, typename IndexConvertible,
  typename... StoredArgs
>
struct TaskCollectionTaskImpl
  : abstract::frontend::TaskCollectionTask<TaskBase>
{
  using base_t = abstract::frontend::TaskCollectionTask<TaskBase>;

  /* TODO implement this */

  using args_tuple_t = std::tuple<StoredArgs>;

  IndexConvertible idx_;
  args_tuple_t args_;

  template <size_t... Spots>
  decltype(auto)
  _get_call_args_impl(
    std::index_sequence<Spots...>
  ) {
    return std::forward_as_tuple(
      detail::_get_call_args_helper<
        typename std::tuple_element<Spots, args_tuple_t>::type,
        typename meta::callable_traits<Functor>::template param_n<Spots>::type
      >()(
        std::get<Spots>(std::move(args_))
      )...
    );
  }

  template <typename... StoredArgsDeduced>
  TaskCollectionTaskImpl(
    IndexConvertible idx,
    StoredArgsDeduced&&... args
  ) : idx_(idx),
      args_(std::forward<StoredArgsDeduced>(args)...)
  { }

  void run() override {
    meta::splat_tuple(
      _get_call_args_impl(std::index_sequence_for<StoredArgs...>()),
      [&](auto&&... args) {
        Functor()(
          idx_,
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



template <
  typename Functor,
  typename IndexRangeT,
  typename... Args
>
struct TaskCollectionImpl
  : PolymorphicSerializationAdapter<
      TaskCollectionImpl<IndexRangeT, Args...>,
      abstract::frontend::TaskCollection
    > {
  public:
    using index_range_t = IndexRangeT;

  protected:

    template <typename ArgsForwardedTuple, size_t... Spots>
    auto _get_args_stored_impl(
      ArgsForwardedTuple&& args_fwd,
      std::index_sequence<Spots...>
    ) {
      return std::forward_as_tuple(
        detail::_get_storage_arg_helper<
          Functor,
          decltype(std::get<Spots>(std::forward<ArgsForwardedTuple>(args_fwd))),
          Spots
        >()(
          *this, std::get<Spots>(std::forward<ArgsForwardedTuple>(args_fwd))
        )...
      );
    }

    template <size_t... Spots>
    auto _make_task_impl(
      std::size_t index,
      std::index_sequence<Spots...>
    ) {
      return std::make_unique<TaskCollectionTaskImpl<
        Functor, typename index_range_t::index_t,
        typename _get_task_stored_arg_helper<
          Functor, decltype(std::get<Spots>(args_stored_)), Spots
        >::type...
      >>(
        detail::_get_task_stored_arg_helper<
          Functor, decltype(std::get<Spots>(args_stored_)), Spots
        >()(*this, std::get<Spots>(args_stored_))...
      );
    }

  public:

    std::tuple<Args...> args_stored_;
    IndexRangeT collection_range_;

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
    {}


    size_t size() const override { return collection_range_.size(); }

    std::unique_ptr<types::task_collection_task_t>
    create_task_for_index(std::size_t index) override {

      return _make_task_impl(
        index, std::make_index_sequence<sizeof...(Args)>()
      );
    }


    template <typename Functor, typename GivenArg, size_t Position, typename Enable>
    friend struct detail::_get_storage_arg_helper;

    template < typename Functor, typename CollectionArg, size_t Position >
    friend struct detail::_get_task_stored_arg_helper;

};


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
    .invokee([](
      auto&& index_range,
      variadic_arguments_begin_tag,
      auto&&... args
    ){
      using task_collection_impl_t = detail::TaskCollectionImpl<
        Functor, std::decay_t<decltype(index_range)>,
        typename _get_storage_arg_helper<decltype(args)>::type...
      >;

      auto task_collection = std::make_unique<task_collection_impl_t>(
        std::forward<decltype(index_range)>(index_range),
        std::forward<decltype(args)>(args)...
      );

      auto* backend_runtime = abstract::backend::get_backend_runtime()->register_task_collection(
        std::move(task_collection)
      );

    });

}

} // end namespace darma_runtime

#endif //DARMA_IMPL_TASK_COLLECTION_TASK_COLLECTION_H
