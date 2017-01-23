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
#include <darma/impl/index_range/mapping_traits.h>
#include <darma/impl/index_range/index_range_traits.h>
#include "task_collection_fwd.h"

#include "task_collection_task.h"

#include "impl/argument_to_tc_storage.h"
#include "impl/tc_storage_to_task_storage.h"

namespace darma_runtime {

namespace detail {

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
    using index_range_traits = indexing::index_range_traits<index_range_t>;

  protected:

    template <typename ArgsForwardedTuple, size_t... Spots>
    decltype(auto)
    _get_args_stored_impl(
      ArgsForwardedTuple&& args_fwd,
      std::index_sequence<Spots...>
    ) {
      return std::make_tuple(
        _task_collection_impl::_get_storage_arg_helper<
          decltype(std::get<Spots>(std::forward<ArgsForwardedTuple>(args_fwd))),
          // offset by 1 to incorporate the index parameter
          typename meta::callable_traits<Functor>::template param_n_traits<Spots+1>,
          IndexRangeT
        >{}(
          *this, std::get<Spots>(std::forward<ArgsForwardedTuple>(args_fwd))
        )...
      );
    }

    template <size_t... Spots>
    auto _make_task_impl(
      std::size_t index,
      std::index_sequence<Spots...> seq
    ) {
      return std::make_unique<
        TaskCollectionTaskImpl<
          Functor, typename index_range_traits::mapping_to_dense_type,
          typename _task_collection_impl::_get_task_stored_arg_helper<
            Functor, Args, Spots
          >::type...
        >
      >(
        index, index_range_traits::mapping_to_dense(collection_range_),
        *this, seq, args_stored_
      );
    }

    using args_tuple_t = std::tuple<Args...>;

    TaskCollectionImpl() = default;

    types::key_t name_ = make_key(); // TODO change this!!!

  public:

    types::key_t const&
    get_name() const override { return name_; }

    void
    set_name(types::key_t const& name) override { name_ = name; }

    // Leave this member declaration order the same; construction of args_stored_
    // depends on collection_range_ being initialized already

    using dependencies_container_t = types::handle_container_template<abstract::frontend::Use*>;
    dependencies_container_t dependencies_;
    IndexRangeT collection_range_;
    args_tuple_t args_stored_;

    template <typename Archive>
    static TaskCollectionImpl&
    reconstruct(void* allocated, Archive& ar) {
      using args_tuple_traits = darma_runtime::serialization::detail
        ::serializability_traits<args_tuple_t>;
      using range_serdes_traits = darma_runtime::serialization::detail
        ::serializability_traits<IndexRangeT>;

      // Need to call ctor to initialize vtable
      auto* rv_ptr = new (allocated) TaskCollectionImpl();

      // TODO figure out (yet another) way around the default constructibility issue

      // No default constructibility requirement on index range, so unpack it here...
      ar >> rv_ptr->collection_range_;

      // Some arguments might not be default constructible either...
      ar >> rv_ptr->args_stored_;

      // for dependencies_, just reconstruct directly; it was never packed
      new (&rv_ptr->dependencies_) dependencies_container_t();

      return *rv_ptr;
    }

    template <typename Archive>
    void serialize(Archive& ar) {
      if(not ar.is_unpacking()) {
        ar | collection_range_;
        ar | args_stored_;
        // nothing to pack for dependencies.  They'll be handled later
      }
      else {
        // Unpacking.
        // collection_range_ already unpacked in reconstruct
        // args_stored_ already unpacked in reconstruct

        // need to set up dependencies here...
        _unpack_deps(std::index_sequence_for<Args...>{});
      }
    }

  private:

    struct _do_unpack_deps {
      template <typename TaskCollectionT,
        typename MappedHandleCollectionT
      >
      std::enable_if_t<
        tinympl::is_instantiation_of<
          MappedHandleCollection,
          std::decay_t<MappedHandleCollectionT>
        >::value,
        int
      >
      operator()(TaskCollectionT& tc, MappedHandleCollectionT&& mcoll) const {
        using tc_index_range_traits = typename TaskCollectionT::index_range_traits;
        using handle_index_range_t = typename std::decay_t<MappedHandleCollectionT>
          ::access_handle_collection_t::index_range_type;
        using full_mapping_t = CompositeMapping<
          typename std::decay_t<MappedHandleCollectionT>::mapping_t,
          typename tc_index_range_traits::mapping_to_dense_type
        >;
        assert(not mcoll.collection.current_use_->is_registered);
        mcoll.collection.current_use_ = std::make_shared<
          GenericUseHolder<CollectionManagingUse<handle_index_range_t>>
        >(
          migrated_use_arg,
          CollectionManagingUse<handle_index_range_t>(
            mcoll.collection.current_use_->use,
            full_mapping_t(
              mcoll.mapping,
              tc_index_range_traits::mapping_to_dense(tc.collection_range_)
            )
          )
        );
        tc.add_dependency(&(mcoll.collection.current_use_->use));
        return 0;
      }

      template <typename TaskCollectionT,
        typename AccessHandleT
      >
      std::enable_if_t<
        decayed_is_access_handle<AccessHandleT>::value,
        int
      >
      operator()(TaskCollectionT& tc, AccessHandleT&& ah) const {
        tc.add_dependency(&(ah.current_use_->use));
        return 0;
      }

      template <typename TaskCollectionT, typename T>
      std::enable_if_t<
        not tinympl::is_instantiation_of<
          MappedHandleCollection,
          std::decay_t<T>
        >::value
        and not decayed_is_access_handle<T>::value,
        int
      >
      operator()(TaskCollectionT& tc, T&& ah) const {
        // Nothing to do...
        return 0;
      }
    };

    template <size_t... Spots>
    void _unpack_deps(
      std::index_sequence<Spots...>
    ) {
      std::make_tuple(
        _do_unpack_deps()(
          *this,
          std::get<Spots>(args_stored_)
        )...
      ); // return value ignored
    }

  public:


    void add_dependency(abstract::frontend::Use* dep) {
      dependencies_.insert(dep);
    }

    //==========================================================================
    // Ctors


    ~TaskCollectionImpl() { }

    template <typename IndexRangeDeduced, typename... ArgsForwarded>
    TaskCollectionImpl(
      IndexRangeDeduced&& collection_range,
      ArgsForwarded&& ... args_forwarded
    ) : collection_range_(std::forward<IndexRangeDeduced>(collection_range)),
        args_stored_(
          _get_args_stored_impl(
            std::forward_as_tuple(std::forward<ArgsForwarded>(args_forwarded)...),
            std::index_sequence_for<ArgsForwarded...>{}
          )
        )
    { }

    size_t size() const override { return collection_range_.size(); }

    std::unique_ptr<types::task_collection_task_t>
    create_task_for_index(std::size_t index) override {

      return _make_task_impl(
        index, std::make_index_sequence<sizeof...(Args)>{}
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

    template <typename, typename, typename, typename>
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
          // offset by 1 to incorporate the index parameter
          typename meta::callable_traits<Functor>::template param_n_traits<Idxs+1>,
          IndexRangeT
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
  using darma_runtime::keyword_tags_for_create_concurrent_work::index_range;
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
