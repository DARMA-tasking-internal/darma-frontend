/*
//@HEADER
// ************************************************************************
//
//                      create_work_if_lambda.h
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

#ifndef DARMAFRONTEND_CREATE_WORK_IF_LAMBDA_H
#define DARMAFRONTEND_CREATE_WORK_IF_LAMBDA_H

#include <darma/impl/meta/detection.h>
#include <tinympl/vector.hpp>

#include "create_if_then_fwd.h"

namespace darma_runtime {
namespace detail {


template <typename Lambda, typename CaptureManagerT>
struct IfLambdaTask
  : LambdaTask<Lambda>
{
  public:

    using base_t = LambdaTask<Lambda>;

    std::shared_ptr<CaptureManagerT> capture_manager_;

  private:
    template <
      typename HelperT,
      size_t... OtherArgIdxs
    >
    IfLambdaTask(
      HelperT&& helper,
      CaptureManagerT* capture_manager,
      std::integer_sequence<size_t, OtherArgIdxs...> /*unused*/
    ) : base_t(
          std::move(helper.if_helper.lambda),
          capture_manager->in_if_mode(),
          variadic_arguments_begin_tag{},
          std::get<OtherArgIdxs>(
            std::move(helper.if_helper.task_details_args)
          )...
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

  public:

    template <typename HelperT>
    IfLambdaTask(
      HelperT&& helper,
      CaptureManagerT* capture_manager
    ) : IfLambdaTask::IfLambdaTask(
          std::forward<HelperT>(helper),
          capture_manager,
          std::make_index_sequence<std::tuple_size<
            typename std::decay_t<HelperT>::if_helper_t::task_details_args_tup_t
          >::value>{}
        )
    { /* forwarding ctor, must be empty */ }

    void run() override {

      if(this->callable_()) {
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

template <typename Lambda, typename IsThenType>
struct ThenElseLambdaTask
  : LambdaTask<Lambda>
{
  public:

    using base_t = LambdaTask<Lambda>;

  private:
    template <
      typename HelperT, typename CaptureManagerT,
      size_t... OtherArgIdxs
    >
    ThenElseLambdaTask(
      HelperT&& helper,
      CaptureManagerT* capture_manager,
      std::integer_sequence<size_t, OtherArgIdxs...> /*unused*/
    ) : base_t(
          std::move(helper.lambda),
          IsThenType::value ? capture_manager->in_then_mode() : capture_manager->in_else_mode(),
          variadic_arguments_begin_tag{},
          std::get<OtherArgIdxs>(
            std::move(helper.task_details_args)
          )...
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

  public:

    template <typename HelperT, typename CaptureManagerT>
    ThenElseLambdaTask(
      HelperT&& helper,
      CaptureManagerT* capture_manager
    ) : ThenElseLambdaTask::ThenElseLambdaTask(
          std::forward<HelperT>(helper),
          capture_manager,
          std::make_index_sequence<std::tuple_size<
            typename std::decay_t<HelperT>::task_details_args_tup_t
          >::value>{}
        )
    { /* forwarding ctor, must be empty */ }

};


//------------------------------------------------------------------------------
// <editor-fold desc="create_work_if_helper, lambda version"> {{{2

template <typename Lambda, typename... Args>
struct _create_work_if_helper<
  meta::nonesuch,
  tinympl::vector<Args...>,
  Lambda
> {

  using callable_t = Lambda;
  using args_fwd_tuple_t = std::tuple<>;
  using task_details_args_tup_t = std::tuple<Args&&...>;
  static constexpr auto is_lambda_callable = true;

  Lambda lambda;
  std::tuple<Args&&...> task_details_args;

#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
  _create_work_if_creation_context context_;
#endif


  _create_work_if_helper(
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    _create_work_if_creation_context&& ctxt,
#endif
    Args&&... args,
    Lambda&& f
  ) : lambda(std::forward<Lambda>(f)),
      task_details_args(std::forward<Args>(args)...)
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

// </editor-fold> end create_work_if_helper, lambda version }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_then_helper, lambda version"> {{{2

template <typename IfHelper, typename Lambda, typename... Args>
struct _create_work_then_helper<
  IfHelper, meta::nonesuch, tinympl::vector<Args...>, Lambda
> {

  using if_helper_t = IfHelper;
  using callable_t = Lambda;
  using args_fwd_tuple_t = std::tuple<>;
  using task_details_args_tup_t = std::tuple<Args&&...>;
  static constexpr auto is_lambda_callable = true;
  static constexpr auto is_else_helper = false;

  _create_work_then_helper(
    IfHelper&& if_helper_in,
    Args&& ... args,
    Lambda&& then_lambda_in
  ) : if_helper(std::forward<IfHelper>(if_helper_in)),
      lambda(std::forward<Lambda>(then_lambda_in)),
      task_details_args(std::forward<Args>(args)...),
      then_helper(*this)
  { }

  _create_work_then_helper(_create_work_then_helper&&) = default;
  _create_work_then_helper(_create_work_then_helper const&) = delete;

  IfHelper if_helper;
  _create_work_then_helper& then_helper;

  Lambda lambda;
  task_details_args_tup_t task_details_args;

  bool else_invoked = false;

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
        typename if_helper_t::callable_t,
        typename if_helper_t::args_fwd_tuple_t,
        if_helper_t::is_lambda_callable,
        callable_t, args_fwd_tuple_t, is_lambda_callable,
        _not_a_type, std::tuple<>, false, /* ElseGiven= */ is_else_helper
      >>(
        variadic_constructor_tag,
        std::move(*this)
      );
      capture_manager->finish_construction_and_register_if_task(capture_manager);
    }
  }

};

// </editor-fold> end create_work_then_helper, lambda version }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_else_helper, lambda version"> {{{2

template <typename ThenHelper, typename Lambda, typename... Args>
struct _create_work_else_helper<
  ThenHelper, meta::nonesuch, tinympl::vector<Args...>, Lambda
> {
  public:

    using if_helper_t = typename ThenHelper::if_helper_t;
    using callable_t = Lambda;
    using args_fwd_tup_t = std::tuple<>;
    using task_details_args_tup_t = std::tuple<>;
    static constexpr auto is_lambda_callable = true;
    static constexpr auto is_else_helper = true;

    ThenHelper then_helper;
    if_helper_t& if_helper; // must be after then helper
    Lambda lambda;
    std::tuple<Args&&...> task_details_args;
    std::tuple<> args_fwd_tup;

    _create_work_else_helper(
      ThenHelper&& in_then_helper,
      Args&& ... args,
      Lambda&& else_lambda_in
    ) : then_helper(std::move(in_then_helper)),
        task_details_args(std::forward<Args>(args)...),
        lambda(std::move(else_lambda_in)),
        if_helper(then_helper.if_helper)
    { }

    ~_create_work_else_helper() {
      auto capture_manager = std::make_shared<IfThenElseCaptureManager<
          typename if_helper_t::callable_t,
          typename if_helper_t::args_fwd_tuple_t,
          if_helper_t::is_lambda_callable,
          typename ThenHelper::callable_t,
          typename ThenHelper::args_fwd_tuple_t,
          ThenHelper::is_lambda_callable,
          callable_t, args_fwd_tup_t, true, /* ElseGiven= */ is_else_helper
      >>(
        variadic_constructor_tag,
        std::move(*this)
      );
      capture_manager->finish_construction_and_register_if_task(capture_manager);
    }

};

// </editor-fold> end create_work_else_helper, lambda version }}}2
//------------------------------------------------------------------------------

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_CREATE_WORK_IF_LAMBDA_H
