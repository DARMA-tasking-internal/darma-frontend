/*
//@HEADER
// ************************************************************************
//
//                      concurrent_region.h
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

#ifndef DARMA_IMPL_CONCURRENT_REGION_H
#define DARMA_IMPL_CONCURRENT_REGION_H

#include <darma/interface/app/create_concurrent_region.h>
#include <darma/interface/frontend/concurrent_region_task.h>
#include <darma/impl/array/index_range.h>
#include <darma/impl/data_store.h>
#include <darma/interface/backend/region_context_handle.h>

#include <darma/impl/util/compressed_pair.h>

#include "task.h"

DeclareDarmaTypeTransparentKeyword(create_concurrent_region, index_range);
DeclareDarmaTypeTransparentKeyword(create_concurrent_region, data_store);

namespace darma_runtime {

namespace detail {

template <typename Callable, typename IndexMappingT, typename... Args>
struct CRTaskRunnable;

} // end namespace detail


template <typename RangeT>
struct ConcurrentRegionContext {
  private:

    using IndexT = typename RangeT::index_t;
    using IndexMappingT = typename RangeT::mapping_to_dense_t;

    std::shared_ptr<abstract::backend::ConcurrentRegionContextHandle>
      context_handle_ = nullptr;

    detail::compressed_pair<IndexT, IndexMappingT> index_and_mapping_;

    bool index_computed_ = false;


    // Stateful mapping ctor
    explicit
    ConcurrentRegionContext(IndexMappingT const& mapping)
      : index_and_mapping_(IndexT(), mapping)
    { }

    // Stateless mapping ctor
    ConcurrentRegionContext()
      : index_and_mapping_(IndexT(), IndexMappingT())
    { }

    size_t
    get_backend_index(IndexT const& idx) {
      return index_and_mapping_.second().map_forward(idx);
    }

  public:

    IndexT const&
    index() {
      if(not index_computed_) {
        assert(context_handle_);
        index_and_mapping_.first() = index_and_mapping_.second().map_backward(
          context_handle_->get_backend_index()
        );
        index_computed_ = true;
      }
      return index_and_mapping_.first();
    }

  private:

    template <typename Callable, typename IndexMappingFriendT, typename... Args>
    friend struct detail::CRTaskRunnable;

};


namespace serialization {

template <typename RangeT>
struct Serializer<darma_runtime::ConcurrentRegionContext<RangeT>> {
  using cr_context_t = darma_runtime::ConcurrentRegionContext<RangeT>;

  template <typename ArchiveT>
  std::enable_if_t<std::is_empty<typename cr_context_t::IndexMappingT>::value>
  serialize(cr_context_t& ctxt, ArchiveT& ar) {
    assert(ctxt.context_handle_ == nullptr); // can't move it once context_handle_ is assigned
  }

  template <typename ArchiveT>
  std::enable_if_t<(not std::is_empty<typename cr_context_t::IndexMappingT>::value)
    and detail::serializability_traits<typename RangeT::mapping_to_dense_t>
      ::template is_serializable_with_archive<ArchiveT>::value
  >
  serialize(cr_context_t& ctxt, ArchiveT& ar) {
    assert(ctxt.context_handle_ == nullptr); // can't move it once context_handle_ is assigned
    ar | ctxt.index_and_mapping_.second();
  }

};


} // end namespace serialization


namespace detail {

struct CRTaskRunnableBase {
  virtual void
  set_context_handle(
    std::shared_ptr<abstract::backend::ConcurrentRegionContextHandle> const& ctxt
  ) =0;
};

// Trick the detection into thinking the first parameter isn't part of the formal parameters
// (for code reuse purposes)
template <typename Callable>
struct functor_without_first_param_adapter {
  using other_args_vector = typename meta::callable_traits<Callable>::params_vector::pop_front::type;
  template <typename... Args>
  struct functor_with_args {
    void operator()(Args...) const { DARMA_ASSERT_UNREACHABLE_FAILURE("Something went wrong with metaprogramming"); }
  };
  using type = typename tinympl::splat_to<other_args_vector, functor_with_args>::type;
};

template <typename Callable, typename RangeT, typename... Args>
struct CRTaskRunnable
  : FunctorLikeRunnableBase<
      typename functor_without_first_param_adapter<Callable>::type,
      Args...
    >,
    CRTaskRunnableBase
{
  using base_t = FunctorLikeRunnableBase<
    typename functor_without_first_param_adapter<Callable>::type,
    Args...
  >;

  using base_t::base_t;

  ConcurrentRegionContext<RangeT> context_;

  template <typename ArchiveT>
  static types::unique_ptr_template<RunnableBase>
  construct_from_archive(ArchiveT& ar) {
    // don't need to worry about reconstructing context_, since it must be null
    return base_t::template _construct_from_archive<
      ArchiveT, CRTaskRunnable
    >(ar);
  };

  void
  set_context_handle(
    std::shared_ptr<abstract::backend::ConcurrentRegionContextHandle> const& ctxt
  ) override {
    context_.context_handle_ = ctxt;
  }


  bool run() override {
    meta::splat_tuple(
      this->base_t::_get_args_to_splat(),
      [this](auto&&... args) {
        Callable()(context_, std::forward<decltype(args)>(args)...);
      }
    );
    return false; // ignored
  }

  size_t get_index() const override { return register_runnable<CRTaskRunnable>(); }

};

struct ConcurrentRegionTaskImpl
  : abstract::frontend::ConcurrentRegionTask<TaskBase>
{
  public:


    using base_t = abstract::frontend::ConcurrentRegionTask<TaskBase>;
    using base_t::base_t;

    void set_region_context(
      std::shared_ptr<abstract::backend::ConcurrentRegionContextHandle> const& ctxt
    ) override {
      assert(runnable_);
      // ugly....
      auto* cr_runnable = dynamic_cast<CRTaskRunnableBase*>(runnable_.get());
      cr_runnable->set_context_handle(ctxt);
    }

};

template <typename Callable, typename RangeT>
struct _make_runnable_t_wrapper {
  template <typename... Args>
  using apply_t = CRTaskRunnable<Callable, RangeT, Args...>;

};

template <typename Callable, typename RangeT, typename DataStoreT, typename ArgsTuple>
void _do_register_concurrent_region(
  RangeT&& range,
  typename std::decay_t<RangeT>::mapping_to_dense_t mapping, DataStoreT&& dstore, ArgsTuple&& args_tup
) {
  detail::TaskBase* parent_task = static_cast<detail::TaskBase* const>(
    abstract::backend::get_backend_context()->get_running_task()
  );
  std::unique_ptr<ConcurrentRegionTaskImpl> task_ptr =
    std::make_unique<ConcurrentRegionTaskImpl>();

  parent_task->current_create_work_context = task_ptr.get();

  using runnable_t = tinympl::splat_to_t<std::decay_t<ArgsTuple>,
    _make_runnable_t_wrapper<Callable, RangeT>::template apply_t
  >;

  meta::splat_tuple(
    std::forward<ArgsTuple>(args_tup),
    [&](auto&&... args){
      task_ptr->set_runnable(std::make_unique<runnable_t>(
        variadic_constructor_arg,
        std::forward<decltype(args)>(args)...
      ));
    }
  );

  parent_task->current_create_work_context = nullptr;

  for (auto&& reg : task_ptr->registrations_to_run) {
    reg();
  }
  task_ptr->registrations_to_run.clear();

  for (auto&& post_reg_op : task_ptr->post_registration_ops) {
    post_reg_op();
  }
  task_ptr->post_registration_ops.clear();

  if(not dstore.is_default()) {
    abstract::backend::get_backend_runtime()->register_concurrent_region(
      std::move(task_ptr), range.size(), detail::DataStoreAttorney::get_handle(dstore)
    );
  }
  else {
    abstract::backend::get_backend_runtime()->register_concurrent_region(
      std::move(task_ptr), range.size()
    );
  }

};


template <typename Functor>
struct _create_concurrent_region_impl {

  template <typename InArg>
  struct parse_for_mapping_argument {
    using Arg = std::decay_t<InArg>;
    using index_range_kwarg = std::decay_t<
      typename is_kwarg_expression_with_tag<Arg,
        darma_runtime::keyword_tags_for_create_concurrent_region::index_range
      >::argument_type
    >;

    template <typename T>
    using _is_index_range_archetype = typename T::is_index_range_t;
    static constexpr auto pos_arg_is_index_range = meta::detected_or_t<std::false_type,
      _is_index_range_archetype, Arg
    >::value;
    static constexpr auto kwarg_is_index_range = meta::detected_or_t<std::false_type,
      _is_index_range_archetype, index_range_kwarg
    >::value;

    template <typename T>
    using _mapping_to_dense_archetype = decltype( get_mapping_to_dense(std::declval<T>()) );
    template <typename T>
    using _is_index_mapping_archetype = typename T::is_index_mapping_t;
    using pos_arg_mapping_to_dense = meta::detected_t<
      _mapping_to_dense_archetype, Arg
    >;
    //meta::detected_t<_is_index_mapping_archetype, Arg>,
    using kwarg_mapping_to_dense = meta::detected_t<
      _mapping_to_dense_archetype, index_range_kwarg
    >;
    //meta::detected_t<_is_index_mapping_archetype, index_range_kwarg>,
    static constexpr auto pos_arg_has_mapping_to_dense = not
      std::is_same<pos_arg_mapping_to_dense, meta::nonesuch>::value;
    static constexpr auto kwarg_has_mapping_to_dense = not
      std::is_same<kwarg_mapping_to_dense, meta::nonesuch>::value;

    static constexpr auto value =
      (pos_arg_is_index_range and pos_arg_has_mapping_to_dense)
      or (kwarg_is_index_range and kwarg_has_mapping_to_dense);
    using type = tinympl::bool_<value>;


    static constexpr auto from_positional_arg =
      (pos_arg_is_index_range and pos_arg_has_mapping_to_dense);

    using index_range_t = tinympl::select_first_t<
      //------------------------------
      tinympl::bool_<(pos_arg_is_index_range and pos_arg_has_mapping_to_dense)>,
      Arg,
      //------------------------------
      tinympl::bool_<(kwarg_is_index_range and kwarg_has_mapping_to_dense)>,
      index_range_kwarg,
      //------------------------------
      std::true_type,
      meta::nonesuch
    >;

    static_assert(not (
        (pos_arg_is_index_range and pos_arg_has_mapping_to_dense)
        and (kwarg_is_index_range and kwarg_has_mapping_to_dense)
      ), "Metaprogramming error"
    );

    template <typename ArgForwarded>
    std::enable_if_t<
      std::is_same<std::decay_t<ArgForwarded>, Arg>::value
        and (pos_arg_is_index_range and pos_arg_has_mapping_to_dense),
      index_range_t
    >
    get_range(ArgForwarded&& arg) {
      return std::forward<ArgForwarded>(arg);
    }


    template <typename ArgForwarded>
    std::enable_if_t<
      std::is_same<std::decay_t<ArgForwarded>, Arg>::value
        and (kwarg_is_index_range and kwarg_has_mapping_to_dense),
      index_range_t
    >
    get_range(ArgForwarded&& arg) {
      return arg.value();
    }

    template <typename ArgForwarded>
    std::enable_if_t<
      std::is_same<std::decay_t<ArgForwarded>, Arg>::value
        and (pos_arg_is_index_range and pos_arg_has_mapping_to_dense),
      pos_arg_mapping_to_dense
    >
    get_mapping(ArgForwarded&& arg) {
      return get_mapping_to_dense(std::forward<ArgForwarded>(arg));
    }


    template <typename ArgForwarded>
    std::enable_if_t<
      std::is_same<std::decay_t<ArgForwarded>, Arg>::value
        and (kwarg_is_index_range and kwarg_has_mapping_to_dense),
      kwarg_mapping_to_dense
    >
    get_mapping(ArgForwarded&& arg) {
      return get_mapping_to_dense(
        std::forward<ArgForwarded>(arg.value())
      );
    }
  };


  template <typename... Args>
  auto _extract_range(Args&&... args) const {
    static constexpr auto arg_idx_mapping_derived_from = tinympl::variadic::find_if<
      parse_for_mapping_argument, Args...
    >::value;
    static_assert(arg_idx_mapping_derived_from < sizeof...(Args),
      "Cannot determine IndexRange over which to create_concurrent_region and/or"
        " the mapping of that IndexRange to dense indices"
    );
    using found_arg_or_kwarg_t = tinympl::variadic::at_t<arg_idx_mapping_derived_from, Args...>;
    // TODO check that the range is valid if the range keyword argument is given
    static_assert(arg_idx_mapping_derived_from == 0 or
        not parse_for_mapping_argument<found_arg_or_kwarg_t>::from_positional_arg,
      "create_concurrent_region IndexRange must be given as first positional argument"
        " or as the keyword argument"
        " darma_runtime::keyword_arguments_for_create_concurrent_region::index_range"
    );
    return parse_for_mapping_argument<found_arg_or_kwarg_t>().get_range(
      std::get<arg_idx_mapping_derived_from>(
        std::forward_as_tuple(std::forward<Args>(args)...)
      )
    );
  }

  template <typename... Args>
  auto _extract_mapping(Args&&... args) const {
    static constexpr auto arg_idx_mapping_derived_from = tinympl::variadic::find_if<
      parse_for_mapping_argument, Args...
    >::value;
    static_assert(arg_idx_mapping_derived_from < sizeof...(Args),
      "Cannot determine IndexRange over which to create_concurrent_region and/or"
        " the mapping of that IndexRange to dense indices"
    );
    using found_arg_or_kwarg_t = tinympl::variadic::at_t<arg_idx_mapping_derived_from, Args...>;
    // TODO check that the range is valid if the range keyword argument is given
    static_assert(arg_idx_mapping_derived_from == 0 or
      not parse_for_mapping_argument<found_arg_or_kwarg_t>::from_positional_arg,
      "create_concurrent_region IndexRange must be given as first positional argument"
        " or as the keyword argument"
        " darma_runtime::keyword_arguments_for_create_concurrent_region::index_range"
    );
    return parse_for_mapping_argument<found_arg_or_kwarg_t>().get_mapping(
      std::get<arg_idx_mapping_derived_from>(
        std::forward_as_tuple(std::forward<Args>(args)...)
      )
    );
  }

  template <typename... Args>
  auto _extract_dstore(Args&&... args) const {
    return get_typeless_kwarg_with_default_as<
      darma_runtime::keyword_tags_for_create_concurrent_region::data_store,
      DataStore
    >(
      DataStore(DataStore::default_data_store_tag),
      std::forward<Args>(args)...
    );
  }

  template <size_t... Idxs, typename... Args>
  auto _extract_other_helper(
    std::integer_sequence<size_t, Idxs...>,
    Args&&... args
  ) const {
    return std::tuple<tinympl::variadic::at_t<Idxs, Args...>...>(
      std::get<Idxs>(std::forward_as_tuple(std::forward<Args>(args)...))...
    );
  };

  template <typename T>
  using _is_other_arg = tinympl::and_<
    tinympl::not_<parse_for_mapping_argument<T>>,
    tinympl::not_<
      is_kwarg_expression_with_tag<std::decay_t<T>,
        darma_runtime::keyword_tags_for_create_concurrent_region::data_store
      >
    >
  >;

  template <typename... Args>
  auto _extract_remaining_args_tuple(Args&&... args) const {
    using other_idxs_t = typename tinympl::variadic::find_all_if<_is_other_arg, Args...>::type;
    return _extract_other_helper(other_idxs_t(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  void operator()(Args&&... args) const {
    _do_register_concurrent_region<Functor> (
      _extract_range(std::forward<Args>(args)...),
      _extract_mapping(std::forward<Args>(args)...),
      _extract_dstore(std::forward<Args>(args)...),
      _extract_remaining_args_tuple(std::forward<Args>(args)...)
    );
  }
};

template <>
struct _create_concurrent_region_impl<void> {
  template <typename Return, typename... FuncParams, typename... Args>
  void operator()(Return (*func)(FuncParams...), Args&&... args) const {
    DARMA_ASSERT_NOT_IMPLEMENTED("function arguments to create_concurrent region");

  }
};

} // end namespace detail

template <typename Functor, typename... Args>
auto
create_concurrent_region(Args&&... args) {
  return detail::_create_concurrent_region_impl<Functor>()(
    std::forward<Args>(args)...
  );
};

} // end namespace darma_runtime

#endif //DARMA_IMPL_CONCURRENT_REGION_H
