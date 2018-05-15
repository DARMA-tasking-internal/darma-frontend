/*
//@HEADER
// ************************************************************************
//
//                      create_work_while_functor.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMAFRONTEND_CREATE_WORK_WHILE_FUNCTOR_H
#define DARMAFRONTEND_CREATE_WORK_WHILE_FUNCTOR_H

#include <tinympl/vector.hpp>

#include <darma/impl/meta/detection.h>

#include "create_work_while_fwd.h"
#include <darma/impl/create_work/create_if_then.h>

#include <darma/impl/access_handle_base.h>

#include <darma/utility/not_a_type.h>

#include <darma/impl/create_work/record_line_numbers.h>

namespace darma {
namespace detail {

//------------------------------------------------------------------------------
// <editor-fold desc="WhileFunctorTask"> {{{2

template <typename CaptureManagerT, typename Functor, typename... Args>
struct WhileFunctorTask
  : functor_task_with_args_t<Functor, Args...>
{

  using base_t = FunctorTask<Functor, Args...>;

  std::shared_ptr<CaptureManagerT> capture_manager_;

  /**
   *  Ctor called during WhileDoCaptureManager construction
   */
  template <
    typename HelperT,
    size_t... OtherArgsIdxs
  >
  WhileFunctorTask(
    CaptureManagerT* capture_manager,
    HelperT&& helper,
    std::integer_sequence<size_t, OtherArgsIdxs...> /*unused*/
  ) : base_t(
        get_running_task_impl(),
        std::move(helper.while_helper.args_fwd_tup),
        capture_manager->in_while_mode()
      )
  { }

  /**
   *  Ctor called by recapture(); always only used for constructing an inner
   *  nested while task (so never a double copy capture).  Can't be private,
   *  though, since we need to construct it with std::make_unique.
   */
  WhileFunctorTask(
    std::shared_ptr<CaptureManagerT> const& capture_manager,
    WhileFunctorTask const& to_recapture,
    TaskBase* parent_task
  ) : base_t(
        parent_task,
        to_recapture.stored_args_,
        capture_manager->in_while_mode()
        /* TODO propagate other aspects of the task to TaskBase, like name,
         * line numbers, iteration number, etc.
         */
      ),
      capture_manager_(capture_manager)
  {
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    this->copy_context_information_from(to_recapture);
#endif
  }


  static std::unique_ptr<WhileFunctorTask>
  recapture(WhileFunctorTask& from_task) {
    // See note in WhileLambdaTask::recapture

    // sanity check: recapture should only be triggered when the parent while,
    // which we are recapturing from, is running:
    assert(
      (intptr_t)get_running_task_impl()
        == (intptr_t)static_cast<TaskBase*>(&from_task)
    );

    return std::make_unique<WhileFunctorTask>(
      from_task.capture_manager_,
      from_task,
      // since we're reconstructing an inner while, the from_task is the
      // parent task, so we can pass that on to the base constructor
      &from_task
    );

  }


  void run() override {
    if(this->run_functor()) {

      capture_manager_->execute_do_captures();
      auto do_task = std::move(capture_manager_->do_task_);

      capture_manager_->recapture(
        *this, *do_task.get()
      );

      abstract::backend::get_backend_runtime()->register_task(
        std::move(do_task)
      );

      capture_manager_->execute_while_captures(false);
      abstract::backend::get_backend_runtime()->register_task(
        std::move(capture_manager_->while_task_)
      );
    }
    else {
      // Release the do_task so that the capture manager is destroyed properly
      capture_manager_->do_task_ = nullptr;
    }

    // release the capture manager; we're all done with it
    capture_manager_ = nullptr;

  }


};

// </editor-fold> end WhileFunctorTask }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="DoFunctorTask"> {{{2

template <typename CaptureManagerT, typename Functor, typename... Args>
struct DoFunctorTask
  : functor_task_with_args_t<Functor, Args...>
{

  using base_t = FunctorTask<Functor, Args...>;

  std::shared_ptr<CaptureManagerT> capture_manager_;

  /**
   *  Ctor called during WhileDoCaptureManager construction
   */
  template <
    typename HelperT,
    size_t... OtherArgsIdxs // unused; other args parsed out in FunctorTask
  >
  DoFunctorTask(
    CaptureManagerT* capture_manager,
    HelperT&& helper,
    std::integer_sequence<size_t, OtherArgsIdxs...> /*unused*/
  ) : base_t(
        capture_manager->while_task_.get(), // parent is while block
        std::move(helper.args_fwd_tup),
        capture_manager->in_do_mode(),
        get_running_task_impl() // running is not the same as parent
      )
  { }

  /**
   *  Ctor called by recapture(); always only used for constructing an inner
   *  nested while task (so never a double copy capture).  Can't be private,
   *  though, since we need to construct it with std::make_unique.
   */
  DoFunctorTask(
    std::shared_ptr<CaptureManagerT> const& capture_manager,
    DoFunctorTask const& to_recapture,
    TaskBase* parent_task
  ) : base_t(
        parent_task,
        to_recapture.stored_args_,
        capture_manager->in_do_mode(),
        get_running_task_impl()
        /* TODO propagate other aspects of the task to TaskBase, like name,
         * line numbers, iteration number, etc.
         */
      ),
      capture_manager_(capture_manager)
  {
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    this->copy_context_information_from(to_recapture);
#endif
  }

  static std::unique_ptr<DoFunctorTask>
  recapture(DoFunctorTask const& from_task) {
    // See note in DoLambdaTask::recapture
    return std::make_unique<DoFunctorTask>(
      from_task.capture_manager_,
      from_task,
      // see note about parent task in DoLambdaTask::recapture
      from_task.capture_manager_->while_task_.get()
    );
  }

};


// </editor-fold> end DoFunctorTask }}}2
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// <editor-fold desc="create_work_while_do_helper, functor version"> {{{2

template <
  typename WhileHelper,
  typename Functor,
  typename LastArg,
  typename... Args
>
struct _create_work_while_do_helper<
  WhileHelper,
  Functor,
  tinympl::vector<Args...>,
  LastArg
>
{

  using while_helper_t = WhileHelper;
  using callable_t = Functor;
  using args_fwd_tuple_t = std::tuple<Args&&..., LastArg&&>;
  using task_option_args_tuple_t = std::tuple<>; // unused; FunctorTask does the parsing

  args_fwd_tuple_t args_fwd_tup;
  WhileHelper while_helper;

  static constexpr auto has_lambda_callable = false;

  _create_work_while_do_helper(_create_work_while_do_helper&&) = default;
  _create_work_while_do_helper(_create_work_while_do_helper const&) = delete;

  _create_work_while_do_helper(
    WhileHelper&& while_helper_in,
    Args&&... args,
    LastArg&& last_arg
  ) : while_helper(std::move(while_helper_in)),
      args_fwd_tup(
        std::forward<Args>(args)...,
        std::forward<LastArg>(last_arg)
      )
  {
    auto while_do_mngr = std::make_shared<
      WhileDoCaptureManager<
        typename WhileHelper::callable_t,
        typename WhileHelper::args_fwd_tuple_t,
        WhileHelper::has_lambda_callable,
        callable_t, args_fwd_tuple_t, has_lambda_callable
      >
    >(
      utility::variadic_constructor_tag,
      std::move(*this)
    );
    while_do_mngr->set_capture_managers(while_do_mngr);
    while_do_mngr->register_while_task();
  }
};

// </editor-fold> end create_work_while_do_helper, functor version }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_while_helper, functor version"> {{{2

template <typename Functor, typename LastArg, typename... Args>
struct _create_work_while_helper<
  Functor,
  tinympl::vector<Args...>,
  LastArg
>
{

  using callable_t = Functor;
  using args_fwd_tuple_t = std::tuple<Args&& ..., LastArg&&>;
  // unnecessary; task options are parsed by FunctorTask<...> anyway
  using task_option_args_tuple_t = std::tuple<>;

  static constexpr auto has_lambda_callable = false;

#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
  _create_work_while_creation_context* context_;
#endif

  args_fwd_tuple_t args_fwd_tup;

  _create_work_while_helper(_create_work_while_helper&&) = default;
  _create_work_while_helper(_create_work_while_helper const&) = delete;

  _create_work_while_helper(
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    _create_work_while_creation_context* ctxt,
#endif
    Args&&... args,
    LastArg&& last_arg
  ) : args_fwd_tup(
        std::forward<Args>(args)...,
        std::forward<LastArg>(last_arg)
      )
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
      , context_(ctxt)
#endif
  { }

  template <typename DoFunctor=tinympl::nonesuch, typename... DoArgs>
  auto
  do_(DoArgs&&... args) &&
  {
    return _create_work_while_do_helper<
      _create_work_while_helper, DoFunctor,
      typename tinympl::vector<DoArgs...>::safe_pop_back::type,
      typename tinympl::vector<DoArgs...>::template safe_back<tinympl::nonesuch>::type
    >(
      std::move(*this),
      std::forward<DoArgs>(args)...
    );
  }

};

// </editor-fold> end create_work_while_helper, functor version }}}2
//------------------------------------------------------------------------------

} // end namespace detail
} // end namespace darma

#endif //DARMAFRONTEND_CREATE_WORK_WHILE_FUNCTOR_H
