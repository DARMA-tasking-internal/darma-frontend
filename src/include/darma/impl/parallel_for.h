/*
//@HEADER
// ************************************************************************
//
//                      parallel_for.h
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


#ifndef DARMA_IMPL_PARALLEL_FOR_H
#define DARMA_IMPL_PARALLEL_FOR_H

#include <darma/interface/backend/parallel_for.h>
#include <darma/impl/keyword_arguments/parse.h>
#include <darma/impl/keyword_arguments/macros.h>

DeclareDarmaTypeTransparentKeyword(parallel_for, n_iterations);

namespace darma_runtime {

namespace detail {

template <typename Lambda>
struct ParallelForLambdaRunnable
  : RunnableBase
{
  // Force the double-copy
  ParallelForLambdaRunnable(std::remove_reference_t<Lambda>& lambda, int64_t n_iters)
    : run_this_(lambda), n_iters_(n_iters)
  { }

  bool run() override {
    backend::execute_parallel_for(n_iters_, run_this_);
    return false;
  }

  size_t get_index() const override { DARMA_ASSERT_NOT_IMPLEMENTED(); return 0; }
  virtual size_t get_packed_size() const override {
    DARMA_ASSERT_NOT_IMPLEMENTED();
    return 0; // Unreachable; silence missing return warnings
  }
  virtual void pack(void* allocated) const override { DARMA_ASSERT_NOT_IMPLEMENTED(); }

  int64_t n_iters_;
  std::remove_reference_t<Lambda> run_this_;
};

template <typename Callable, typename... Args>
struct ParallelForFunctorRunnable
  : FunctorLikeRunnableBase<
      typename meta::functor_without_first_param_adapter<Callable>::type,
      Args...
    >
{
  using base_t = FunctorLikeRunnableBase<
    typename meta::functor_without_first_param_adapter<Callable>::type,
    Args...
  >;

  using base_t::base_t;

  void set_n_iters(int64_t n_iters) { n_iters_ = n_iters; }

  bool run() override {
    meta::splat_tuple(
      this->base_t::_get_args_to_splat(),
      [this](auto&&... args) {
        backend::execute_parallel_for(
          n_iters_,
          Callable(),
          std::forward<decltype(args)>(args)...
        );
      }
    );
    return false; // ignored
  }

  // TODO migratability
  size_t get_index() const override { DARMA_ASSERT_NOT_IMPLEMENTED(); return 0; }
  virtual size_t get_packed_size() const override {
    DARMA_ASSERT_NOT_IMPLEMENTED();
    return 0; // Unreachable; silence missing return warnings
  }
  virtual void pack(void* allocated) const override { DARMA_ASSERT_NOT_IMPLEMENTED(); }

  int64_t n_iters_;
};


template <bool is_functor, typename Callable, typename ArgsVector>
struct _do_create_parallel_for;


//==============================================================================
// <editor-fold desc="Lambda version">

// Lambda version

template <typename Callable, typename... Args>
struct _do_create_parallel_for<
  false, Callable, tinympl::vector<Args...>
> {
  void operator()(Args&& ... args, Callable&& c) const {
    using parser = kwarg_parser<
      variadic_positional_overload_description<
        _keyword<size_t, keyword_tags_for_parallel_for::n_iterations>
      >
    >;

    using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

    return parser()
      // For some reason, this doesn't work with zero variadics, so we can give it a
      // random one since they're ignored anyway
      .parse_args(5, std::forward<Args>(args)...)
      .invoke([&](
        size_t n_iters,
        variadic_arguments_begin_tag,
        auto&&... /*ignored*/
      ) {
        auto task = std::make_unique<TaskBase>();
        detail::TaskBase* parent_task = static_cast<detail::TaskBase* const>(
          abstract::backend::get_backend_context()->get_running_task()
        );
        parent_task->current_create_work_context = task.get();

        task->is_double_copy_capture = true;
        task->set_runnable(std::make_unique<
          ParallelForLambdaRunnable<Callable>
        >(
          // Intentionally not forwarded
          c, n_iters
        ));

        parent_task->current_create_work_context = nullptr;

        task->is_parallel_for_task_ = true;

        for (auto&& reg : task->registrations_to_run) {
          reg();
        }
        task->registrations_to_run.clear();

        return abstract::backend::get_backend_runtime()->register_task(
          std::move(task)
        );
      });
  }
};

// </editor-fold> end Lambda version
//==============================================================================

//==============================================================================
// <editor-fold desc="Functor version">

// Lambda version

template <typename Callable, typename... Args>
struct _do_create_parallel_for<
  true, Callable, tinympl::vector<Args...>
> {
  void operator()(Args&& ... args) const {
    using parser = kwarg_parser<
      variadic_positional_overload_description<
        _keyword<size_t, keyword_tags_for_parallel_for::n_iterations>
      >
    >;

    using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

    return parser()
      // For some reason, this doesn't work with zero variadics, so we can give it a
      // random one since they're ignored anyway
      .parse_args(std::forward<Args>(args)...)
      .invoke([&](
        size_t n_iters,
        variadic_arguments_begin_tag,
        auto&&... args_to_fwd
      ) {
        auto task = std::make_unique<TaskBase>();
        detail::TaskBase* parent_task = static_cast<detail::TaskBase* const>(
          abstract::backend::get_backend_context()->get_running_task()
        );
        parent_task->current_create_work_context = task.get();

        auto runnable = std::make_unique<
          ParallelForFunctorRunnable<Callable, decltype(args_to_fwd)...>
        >(
          variadic_constructor_arg,
          std::forward<decltype(args_to_fwd)>(args_to_fwd)...
        );
        runnable->set_n_iters(n_iters);
        task->set_runnable(std::move(runnable));

        parent_task->current_create_work_context = nullptr;

        task->is_parallel_for_task_ = true;

        for (auto&& reg : task->registrations_to_run) {
          reg();
        }
        task->registrations_to_run.clear();

        return abstract::backend::get_backend_runtime()->register_task(
          std::move(task)
        );
      });
  }
};

// </editor-fold> end Lambda version
//==============================================================================


} // end namespace detail

template <typename Functor=void, typename... Args>
void create_parallel_for(
  Args&&... args
) {

  using callable_t = std::conditional_t<
    std::is_void<Functor>::value,
    // Just use Functor in the case of sizeof...(Args) == 0 (should never happen)
    typename tinympl::vector<Functor, Args...>::back::type,
    Functor
  >;
  using args_t = std::conditional_t<
    std::is_void<Functor>::value,
    typename std::conditional_t<
      sizeof...(Args) == 0,
      tinympl::identity<tinympl::vector<>>, /* should never be used; just for safety on the other branch */
      typename tinympl::vector<Args...>::pop_back
    >::type,
    tinympl::vector<Args...>
  >;

  detail::_do_create_parallel_for<
    not std::is_void<Functor>::value, callable_t, args_t
  >()(
    std::forward<Args>(args)...
  );
}

} // end namespace darma_runtime

#endif //DARMA_IMPL_PARALLEL_FOR_H
