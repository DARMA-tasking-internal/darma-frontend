/*
//@HEADER
// ************************************************************************
//
//                      create_if_then_functor.h
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

#ifndef DARMAFRONTEND_CREATE_IF_THEN_FUNCTOR_H
#define DARMAFRONTEND_CREATE_IF_THEN_FUNCTOR_H

#include <darma/impl/meta/detection.h>
#include <tinympl/vector.hpp>
#include <darma/impl/task/functor_task.h>

#include "create_if_then_fwd.h"

namespace darma_runtime {
namespace detail {

template <typename CaptureManagerT, typename Functor, typename... Args>
struct IfFunctorTask
  : functor_task_with_args_t<Functor, Args...>
{
  using base_t = FunctorTask<Functor, Args...>;

  std::shared_ptr<CaptureManagerT> capture_manager_;

  /**
   *  Ctor called during WhileDoCaptureManager construction
   */
  template <typename HelperT>
  IfFunctorTask(
    HelperT&& helper,
    CaptureManagerT* capture_manager
  ) : base_t(
        get_running_task_impl(),
        std::move(helper.if_helper.args_fwd_tup),
        capture_manager->in_if_mode()
      )
  {
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    this->set_context_information(
      helper.if_helper.context_.file,
      helper.if_helper.context_.line,
      helper.if_helper.context_.func
    );
#endif
  }

  void run() override {

    if(this->run_functor()) {
      capture_manager_->register_then_task();
    }
    else {
      capture_manager_->register_else_task();
    }
    // clear the implicit captures that now could hold continuation uses of
    // the then or else tasks
    capture_manager_ = nullptr;
  }

  bool is_migratable() const override { return false; }

};

template <typename IsThenType, typename Functor, typename... Args>
struct ThenElseFunctorTask
  : functor_task_with_args_t<Functor, Args...>
{
  using base_t = FunctorTask<Functor, Args...>;

  template <typename HelperT, typename CaptureManagerT>
  ThenElseFunctorTask(
    HelperT&& helper,
    CaptureManagerT* capture_manager
  ) : base_t(
        capture_manager->get_if_task().get(),
        std::move(helper.args_fwd_tup),
        IsThenType::value ? capture_manager->in_then_mode() : capture_manager->in_else_mode(),
        get_running_task_impl()
      )
  {
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    this->set_context_information(
      helper.if_helper.context_.file,
      helper.if_helper.context_.line,
      helper.if_helper.context_.func
    );
#endif
  }

};

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_if_helper, functor version"> {{{2

template <typename Functor, typename LastArg, typename... Args>
struct _create_work_if_helper<
  Functor,
  tinympl::vector<Args...>,
  LastArg
> {

  // TODO handle zero args case

  using callable_t = Functor;
  using args_fwd_tuple_t = std::tuple<Args&&..., LastArg&&>;
  using task_details_args_tup_t = std::tuple<>; // handled in FunctorTask itself
  static constexpr auto is_lambda_callable = false;

  args_fwd_tuple_t args_fwd_tup;

#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
  _create_work_if_creation_context context_;
#endif

  _create_work_if_helper(
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    _create_work_if_creation_context&& ctxt,
#endif
    Args&&... args, LastArg&& last_arg
  ) : args_fwd_tup(
        std::forward<Args>(args)...,
        std::forward<LastArg>(last_arg)
      )
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
      , context_(std::move(ctxt))
#endif
  { }

  template <typename ThenFunctor=meta::nonesuch, typename... ThenArgs>
  auto
  then_(ThenArgs&& ... args)&& {
    return _create_work_then_helper<
      _create_work_if_helper, ThenFunctor,
      typename tinympl::vector<ThenArgs...>::pop_back::type,
      typename tinympl::vector<ThenArgs...>::back::type
    >(
      std::move(*this),
      std::forward<ThenArgs>(args)...
    );
  }
};

// </editor-fold> end create_work_if_helper, functor version }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_then_helper, functor version"> {{{2

template <typename IfHelper, typename Functor, typename LastArg, typename... Args>
struct _create_work_then_helper<
  IfHelper, Functor, tinympl::vector<Args...>, LastArg
> {

  using if_helper_t = IfHelper;
  using callable_t = Functor;
  using args_fwd_tuple_t = std::tuple<Args&&..., LastArg&&>;
  using task_details_args_tup_t = std::tuple<>; // handled in FunctorTask itself
  static constexpr auto is_lambda_callable = false;
  static constexpr auto is_else_helper = false;

  IfHelper if_helper;
  _create_work_then_helper& then_helper;
  args_fwd_tuple_t args_fwd_tup;

  bool else_invoked = false;

  _create_work_then_helper(
    IfHelper&& if_helper_in,
    Args&&... args,
    LastArg&& last_arg
  ) : if_helper(std::forward<IfHelper>(if_helper_in)),
      then_helper(*this),
      args_fwd_tup(
        std::forward<Args>(args)...,
        std::forward<LastArg>(last_arg)
      )
  { }

  _create_work_then_helper(_create_work_then_helper&&) = default;
  _create_work_then_helper(_create_work_then_helper const&) = delete;

  template <typename ElseFunctor=meta::nonesuch, typename... ElseArgs>
  void else_(ElseArgs&& ... args)&& {
    else_invoked = true;
    _create_work_else_helper<
      _create_work_then_helper,
      ElseFunctor,
      typename tinympl::vector<ElseArgs...>::pop_back::type,
      typename tinympl::vector<ElseArgs...>::back::type
    >(std::move(*this), std::forward<ElseArgs>(args)...);
  }

  ~_create_work_then_helper() {
    if (not else_invoked) {
      auto capture_manager = std::make_shared<IfThenElseCaptureManager<
        typename IfHelper::callable_t,
        typename IfHelper::args_fwd_tuple_t,
        IfHelper::is_lambda_callable,
        callable_t, args_fwd_tuple_t, false,
        _not_a_type, std::tuple<>, false, /* ElseGiven= */ false
      >>(
        variadic_constructor_tag,
        std::move(*this)
      );
      capture_manager->finish_construction_and_register_if_task(capture_manager);
    }
  }

};

// </editor-fold> end create_work_then_helper, functor version }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_else_helper, functor version"> {{{2

template <typename ThenHelper, typename Functor, typename LastArg, typename... Args>
struct _create_work_else_helper<
  ThenHelper, Functor, tinympl::vector<Args...>, LastArg
> {
  public:

    using if_helper_t = typename ThenHelper::if_helper_t;
    using callable_t = Functor;
    using args_fwd_tuple_t = std::tuple<Args&&..., LastArg&&>;
    using task_details_args_tup_t = std::tuple<>; // handled in FunctorTask itself
    static constexpr auto is_lambda_callable = false;
    static constexpr auto is_else_helper = true;

    ThenHelper then_helper;
    if_helper_t& if_helper;
    args_fwd_tuple_t args_fwd_tup;

    _create_work_else_helper(
      ThenHelper&& then_helper_in,
      Args&& ... args,
      LastArg&& last_arg
    ) : then_helper(std::forward<ThenHelper>(then_helper_in)),
        if_helper(then_helper.if_helper),
        args_fwd_tup(
          std::forward<Args>(args)...,
          std::forward<LastArg>(last_arg)
        )
    { }

    ~_create_work_else_helper() {
      auto capture_manager = std::make_shared<IfThenElseCaptureManager<
        typename if_helper_t::callable_t,
        typename if_helper_t::args_fwd_tuple_t,
        if_helper_t::is_lambda_callable,
        typename ThenHelper::callable_t,
        typename ThenHelper::args_fwd_tuple_t,
        ThenHelper::is_lambda_callable,
        callable_t, args_fwd_tuple_t, false, /* ElseGiven= */ true
      >>(
        variadic_constructor_tag,
        std::move(*this)
      );
      capture_manager->finish_construction_and_register_if_task(capture_manager);
    }

};

// </editor-fold> end create_work_else_helper, functor version }}}2
//------------------------------------------------------------------------------

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_CREATE_IF_THEN_FUNCTOR_H
