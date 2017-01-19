/*
//@HEADER
// ************************************************************************
//
//                      concurrent_region.h
//                         DARMA
//              Copyright (C) 2016 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corpoation,
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

#if 0

#include <darma/interface/app/create_concurrent_region.h>
#include <darma/interface/frontend/concurrent_region_task.h>
#include <darma/impl/array/index_range.h>
#include <darma/impl/data_store.h>
#include <darma/interface/backend/region_context_handle.h>
#include <darma/interface/app/keyword_arguments/index_range.h>


#include <darma/impl/util/compressed_pair.h>

#include "task.h"

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

    std::shared_ptr<abstract::backend::TaskCollectionContextHandle>
      context_handle_ = nullptr;

    mutable detail::compressed_pair<IndexT, IndexMappingT> index_and_mapping_;

    mutable bool index_computed_ = false;


    // TODO use a std::optional (or pre-C++17 equivalent) to store the uninitialized mapping so that
    // ConcurrentRegionContext is still default constructible
    // Stateful mapping ctor
    explicit
    ConcurrentRegionContext(IndexMappingT const& mapping)
      : index_and_mapping_(IndexT(), mapping)
    { }

    // Stateless mapping ctor
    template <typename _Ignored=void,
      typename=std::enable_if_t<
        std::is_void<_Ignored>::value and
        std::is_default_constructible<IndexMappingT>::value
      >
    >
    ConcurrentRegionContext()
      : index_and_mapping_(IndexT(), IndexMappingT())
    { }


  public:

    IndexT const&
    index() const {
      if(not index_computed_) {
        assert(context_handle_);
        index_and_mapping_.first() = index_and_mapping_.second().map_reverse(
          context_handle_->get_backend_index()
        );
        index_computed_ = true;
      }
      return index_and_mapping_.first();
    }

    // not really public, but for now...
    size_t
    get_backend_index(IndexT const& idx) const {
      return index_and_mapping_.second().map_forward(idx);
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
    std::shared_ptr<abstract::backend::TaskCollectionContextHandle> const& ctxt
  ) =0;
};


template <typename Callable, typename RangeT, typename... Args>
struct CRTaskRunnable
  : FunctorLikeRunnableBase<
      typename meta::functor_without_first_param_adapter<Callable>::type,
      Args...
    >,
    CRTaskRunnableBase
{
  using base_t = FunctorLikeRunnableBase<
    typename meta::functor_without_first_param_adapter<Callable>::type,
    Args...
  >;

  using base_t::base_t;

  ConcurrentRegionContext<RangeT> context_;

  template <typename ArchiveT>
  static types::unique_ptr_template<RunnableBase>
  construct_from_archive(ArchiveT& ar) {
    // don't need to worry about reconstructing context_, since it must be null
    typename RangeT::mapping_to_dense_t mapping;
    ar >> mapping;
    auto rv = base_t::template _construct_from_archive<
      ArchiveT, CRTaskRunnable
    >(ar);
    rv->context_.index_and_mapping_.second() = mapping;
    return std::move(rv);
  };

  void pack(void* allocated) const override {
    using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
    serialization::SimplePackUnpackArchive ar;

    ArchiveAccess::start_packing(ar);
    ArchiveAccess::set_buffer(ar, allocated);

    ar << context_.index_and_mapping_.second();

    base_t::pack(ArchiveAccess::get_spot(ar));
  }

  size_t get_packed_size() const override {
    using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
    serialization::SimplePackUnpackArchive ar;

    ArchiveAccess::start_sizing(ar);

    ar % context_.index_and_mapping_.second();

    return base_t::get_packed_size() + ArchiveAccess::get_size(ar);
  }

  void
  set_context_handle(
    std::shared_ptr<abstract::backend::TaskCollectionContextHandle> const& ctxt
  ) override {
    context_.context_handle_ = ctxt;
  }

  template <typename MappingConvertible>
  void set_mapping(MappingConvertible&& m) {
    context_.index_and_mapping_.second() = std::forward<MappingConvertible>(m);
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
      std::shared_ptr<abstract::backend::TaskCollectionContextHandle> const& ctxt
    ) override {
      assert(runnable_);
      // ugly....
      auto* cr_runnable = dynamic_cast<CRTaskRunnableBase*>(runnable_.get());
      cr_runnable->set_context_handle(ctxt);
    }

    std::unique_ptr<abstract::frontend::ConcurrentRegionTask<TaskBase>>
    deep_copy() const override {
      assert(false);
      return nullptr;
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
      auto runnable = std::make_unique<runnable_t>(
        variadic_constructor_arg,
        std::forward<decltype(args)>(args)...
      );
      runnable->set_mapping(mapping);
      task_ptr->set_runnable(std::move(runnable));
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

template <typename T>
using _is_index_range_archetype = typename T::is_index_range_t;
template <typename Arg>
using is_index_range = meta::is_detected<_is_index_range_archetype, Arg>;
template <typename Arg>
using decayed_is_index_range = is_index_range<std::decay_t<Arg>>;

} // end namespace detail

template <typename Functor, typename... Args>
auto
create_concurrent_region(Args&&... args) {
  using namespace darma_runtime::detail;

  using parser = kwarg_parser<
    variadic_positional_overload_description<
      _keyword<parameter_such_that<decayed_is_index_range>,
        keyword_tags_for_create_concurrent_region::index_range
      >,
      _optional_keyword<DataStore,
        keyword_tags_for_create_concurrent_region::data_store
      >
    >
  >;

  // This is on one line for readability of compiler error; don't respace it please!
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

  parser()
    .with_default_generators(
      keyword_arguments_for_create_concurrent_region::data_store=[]{
        return DataStore(DataStore::default_data_store_tag);
      }
    )
    .parse_args(std::forward<Args>(args)...)
    .invoke(
      [](
        auto&& index_range, DataStore dstore,
        variadic_arguments_begin_tag,
        auto&&... args
      ) {
        detail::_do_register_concurrent_region<Functor>(
          std::forward<decltype(index_range)>(index_range),
          get_mapping_to_dense(index_range), // not forwarded because the
                                             // function get_mapping_to_dense
                                             // shouldn't take possession of
                                             // index_range
          dstore,
          std::forward_as_tuple(std::forward<decltype(args)>(args)...)
        );
      }
    );
};

namespace frontend {

inline abstract::backend::runtime_t::concurrent_region_task_unique_ptr
unpack_concurrent_region_task(void* packed_data) {
  return darma_runtime::detail::_unpack_task<
    darma_runtime::detail::ConcurrentRegionTaskImpl
  >(packed_data);
}

} // end namespace frontend

} // end namespace darma_runtime

#endif // 0
#endif //DARMA_IMPL_CONCURRENT_REGION_H

