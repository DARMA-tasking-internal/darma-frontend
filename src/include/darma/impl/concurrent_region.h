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
#include <darma/interface/backend/region_context_handle.h>

#include <darma/impl/util/compressed_pair.h>

#include "task.h"

namespace darma_runtime {

namespace detail {

template <typename Callable, typename IndexMappingT, typename... Args>
struct CRTaskRunnable;

} // end namespace detail


template <typename IndexT, typename IndexMappingT>
struct ConcurrentRegionContext {
  private:

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

    friend struct detail::ConcurrentRegionTaskImpl<IndexT, IndexMappingT>;
    template <typename Callable, typename... Args>
    friend struct detail::CRTaskRunnable<Callable, IndexMappingT, Args...>;

};


namespace serialization {

template <typename IndexT, typename IndexMappingT>
struct Serializer<darma_runtime::ConcurrentRegionContext<IndexT, IndexMappingT>> {
  using cr_context_t = darma_runtime::ConcurrentRegionContext<IndexT, IndexMappingT>;

  template <typename ArchiveT>
  std::enable_if_t<std::is_empty<IndexMappingT>::value>
  serialize(cr_context_t& ctxt, ArchiveT& ar) {
    assert(ctxt.context_handle_ == nullptr); // can't move it once context_handle_ is assigned
  }

  template <typename ArchiveT>
  std::enable_if_t<(not std::is_empty<IndexMappingT>::value)
    and detail::serializability_traits<IndexMappingT>
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
  using other_args_vector = typename meta::callable_traits<Callable>::params_vector::pop_front;
  template <typename... Args>
  struct functor_with_args {
    void operator()(Args...) const { DARMA_ASSERT_UNREACHABLE_FAILURE("Something went wrong with metaprogramming"); }
  };
  using type = typename tinympl::splat_to<other_args_vector, functor_with_args>::type;
};

template <typename Callable, typename IndexMappingT, typename... Args>
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

  ConcurrentRegionContext<typename IndexMappingT::from_index_t, IndexMappingT> context_;

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
      static_cast<CRTaskRunnableBase*>(runnable_.get())->set_context_handle(ctxt);
    }

};

template <typename Callable, typename IndexMappingT, typename DataStoreT, typename ArgsTuple>
void _do_register_concurrent_region(
  IndexMappingT&& mapping, DataStoreT&& dstore, ArgsTuple&& args_tup
) {
  detail::TaskBase* parent_task = static_cast<detail::TaskBase* const>(
    abstract::backend::get_backend_context()->get_running_task()
  );
  std::unique_ptr<ConcurrentRegionTaskImpl> task_ptr =
    std::make_unique<ConcurrentRegionTaskImpl>();

  parent_task->current_create_work_context = task_ptr.get();

  template <typename... Args>
  using runnable_t_wrapper = CRTaskRunnable<Callable, IndexMappingT, Args...>;
  using runnable_t = tinympl::splat_to_t<std::decay_t<ArgsTuple>, runnable_t_wrapper>;

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
      std::move(task_ptr), dstore.get_handle()
    );
  }
  else {
    abstract::backend::get_backend_runtime()->register_concurrent_region(
      std::move(task_ptr)
    );
  }

};


template <typename Functor>
struct _create_concurrent_region_impl {
  template <typename... Args>
  void operator()(Args&&... args) const {

    auto mapping = ::darma_runtime::detail::_handle(std::forward<Args>(args)...);

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
