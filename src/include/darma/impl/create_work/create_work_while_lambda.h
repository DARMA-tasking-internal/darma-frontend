/*
//@HEADER
// ************************************************************************
//
//                      create_work_while_lambda.h
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

#ifndef DARMAFRONTEND_CREATE_WORK_WHILE_LAMBDA_H
#define DARMAFRONTEND_CREATE_WORK_WHILE_LAMBDA_H

#include <tinympl/vector.hpp>

#include "create_work_while_fwd.h"
#include <darma/impl/create_if_then.h>

namespace darma_runtime {
namespace detail {

//------------------------------------------------------------------------------
// <editor-fold desc="WhileLambdaTask"> {{{2

template <typename Lambda, typename CaptureManagerT>
struct WhileLambdaTask
  : LambdaTask<Lambda>
{
  using base_t = LambdaTask<Lambda>;

  std::shared_ptr<CaptureManagerT> capture_manager_;

  template <
    typename HelperT
  >
  WhileLambdaTask(
    std::shared_ptr<CaptureManagerT> const& capture_manager,
    WhileLambdaTask& to_recapture
  ) : base_t(
        to_recapture.callable_,
        capture_manager->in_while_mode()
      ),
      capture_manager_(capture_manager)
  {

  };

  template <
    typename HelperT
  >
  WhileLambdaTask(
    std::shared_ptr<CaptureManagerT> const& capture_manager,
    HelperT&& helper
  ) : base_t(
        std::move(helper.while_helper.while_lambda),
        capture_manager->in_while_mode()
      ),
      capture_manager_(capture_manager)
  {

  };

  static std::unique_ptr<WhileLambdaTask>
  recapture(WhileLambdaTask& from_task) {
    return std::make_unique<WhileLambdaTask>(
      from_task.capture_manager_,
      from_task
    );
  }

  void run() override {
    if(this->callable_()) {

      auto do_task = std::move(capture_manager_->do_task_);

      capture_manager_->recapture(
        *this, *do_task.get()
      );

      abstract::backend::get_backend_runtime()->register_task(
        std::move(do_task)
      );

      abstract::backend::get_backend_runtime()->register_task(
        std::move(capture_manager_->while_task_)
      );
    }

    // release the capture manager; we're all done with it
    capture_manager_ = nullptr;
  }

};

// </editor-fold> end WhileLambdaTask }}}2
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// <editor-fold desc="DoLambdaTask"> {{{2

template <typename Lambda, typename CaptureManagerT>
struct DoLambdaTask
  : LambdaTask<Lambda>
{

  using base_t = LambdaTask<Lambda>;

  std::shared_ptr<CaptureManagerT> capture_manager_;

  template <typename HelperT>
  DoLambdaTask(
    std::shared_ptr<CaptureManagerT> const& capture_manager,
    HelperT&& helper
  ) : base_t(
        std::move(helper.do_lambda),
        capture_manager->in_do_mode()
      ),
      capture_manager_(capture_manager)
  { };

  template <typename HelperT>
  DoLambdaTask(
    std::shared_ptr<CaptureManagerT> const& capture_manager,
    DoLambdaTask& to_recapture
  ) : base_t(
        to_recapture.callable_,
        capture_manager->in_do_mode()
      ),
      capture_manager_(capture_manager)
  { };

  static std::unique_ptr<DoLambdaTask>
  recapture(DoLambdaTask& from_task) {
    return std::make_unique<DoLambdaTask>(
      from_task.capture_manager_,
      from_task
    );
  }

};

// </editor-fold> end DoLambdaTask }}}2
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// <editor-fold desc="create_work_while_do_helper, lambda version"> {{{2

template <typename WhileHelper, typename Lambda, typename... Args>
struct _create_work_while_do_helper<
  WhileHelper,
  meta::nonesuch,
  tinympl::vector<Args...>,
  Lambda
>
{

  using callable_t = Lambda;
  using args_tuple_t = std::tuple<>;
  using args_fwd_tuple_t = std::tuple<>;
  using task_option_args_tuple_t = decltype(
    std::forward_as_tuple(std::declval<Args&&>()...)
  );

  callable_t do_lambda;
  args_fwd_tuple_t args_fwd_tup;
  task_option_args_tuple_t task_option_args_tup;
  WhileHelper while_helper;

  static constexpr auto has_lambda_callable = true;

  _create_work_while_do_helper(_create_work_while_do_helper&&) = default;
  _create_work_while_do_helper(_create_work_while_do_helper const&) = delete;

  _create_work_while_do_helper(
    WhileHelper&& while_helper_in,
    Args&& ... args,
    std::remove_reference_t<Lambda>&& f // force rvalue reference
  ) : do_lambda(std::move(f)),
      while_helper(std::move(while_helper_in)),
      task_option_args_tup(
        std::forward_as_tuple(std::forward<Args>(args)...)
      ) {}

  ~_create_work_while_do_helper()
  {

    auto while_do_mngr = std::make_shared<
      WhileDoCaptureManager<
          typename WhileHelper::callable_t, typename WhileHelper::args_tuple_t,
          WhileHelper::has_lambda_callable,
          callable_t, args_tuple_t, has_lambda_callable
      >
    >(
      variadic_constructor_tag,
      std::move(*this)
    );

  }
};


// </editor-fold> end create_work_while_do_helper, lambda version }}}2
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// <editor-fold desc="create_work_while_helper, lambda version"> {{{2

template <typename Lambda, typename... Args>
struct _create_work_while_helper<
  meta::nonesuch,
  tinympl::vector<Args...>,
  Lambda
>
{

  using callable_t = Lambda;
  using args_tuple_t = std::tuple<>;
  using args_fwd_tuple_t = std::tuple<>;
  using task_option_args_tuple_t = decltype(
  std::forward_as_tuple(std::declval<Args&&>()...)
  );

  static constexpr auto has_lambda_callable = true;

  callable_t while_lambda;
  args_fwd_tuple_t args_fwd_tup;
  task_option_args_tuple_t task_option_args_tup;

  _create_work_while_helper(_create_work_while_helper&&) = default;
  _create_work_while_helper(_create_work_while_helper const&) = delete;

  _create_work_while_helper(
    Args&& ... args,
    std::remove_reference_t<Lambda>&& f // force rvalue reference<
  ) : while_lambda(std::move(f)),
      task_option_args_tup(
        std::forward_as_tuple(std::forward<Args>(args)...)
      ) {}

  template <typename DoFunctor=meta::nonesuch, typename... DoArgs>
  auto
  do_(DoArgs&& ... args)&&
  {
    return _create_work_while_do_helper<
      _create_work_while_helper, DoFunctor,
      typename tinympl::vector<DoArgs...>::pop_back::type,
      typename tinympl::vector<DoArgs...>::back::type
    >(
      std::move(*this),
      std::forward<DoArgs>(args)...
    );
  }

};

// </editor-fold> end create_work_while_helper, lambda version }}}2
//------------------------------------------------------------------------------

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_CREATE_WORK_WHILE_LAMBDA_H
