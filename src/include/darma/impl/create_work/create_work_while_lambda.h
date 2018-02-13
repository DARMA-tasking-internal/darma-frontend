/*
//@HEADER
// ************************************************************************
//
//                      create_work_while_lambda.h
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

#ifndef DARMAFRONTEND_CREATE_WORK_WHILE_LAMBDA_H
#define DARMAFRONTEND_CREATE_WORK_WHILE_LAMBDA_H

#include <tinympl/vector.hpp>

#include "create_work_while_fwd.h"
#include <darma/impl/create_work/create_if_then.h>
#include <darma/impl/runtime.h>
#include <darma/impl/task/lambda_task.h>

namespace darma_runtime {
namespace detail {

//------------------------------------------------------------------------------
// <editor-fold desc="WhileLambdaTask"> {{{2

template <typename Lambda, typename CaptureManagerT>
struct WhileLambdaTask
  : LambdaTask<Lambda>
{
  public:
    using base_t = LambdaTask<Lambda>;

    std::shared_ptr<CaptureManagerT> capture_manager_;

    /**
     *  Ctor called during WhileDoCaptureManager construction
     */
    template <
      typename HelperT,
      size_t... OtherArgIdxs
    >
    WhileLambdaTask(
      CaptureManagerT* capture_manager,
      HelperT&& helper,
      std::integer_sequence<size_t, OtherArgIdxs...>
    ) : base_t(
          std::move(helper.while_helper.while_lambda),
          capture_manager->in_while_mode(),
          variadic_arguments_begin_tag{},
          std::get<OtherArgIdxs>(std::move(helper.while_helper.task_option_args_tup))...
        )
    { }

    /**
     *  Ctor called by recapture(); always only used for constructing an inner
     *  nested while task (so never a double copy capture).  Can't be private,
     *  though, since we need to construct it with std::make_unique.
     */
    WhileLambdaTask(
      std::shared_ptr<CaptureManagerT> const& capture_manager,
      WhileLambdaTask const& to_recapture,
      TaskBase* parent_task
    ) : base_t(
          LambdaTask_tags::no_double_copy_capture_tag,
          to_recapture.callable_,
          parent_task,
          capture_manager->in_while_mode(),
          variadic_arguments_begin_tag{}
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

    static std::unique_ptr<WhileLambdaTask>
    recapture(WhileLambdaTask& from_task) {
      // Note: the above is non-const because parent_task needs to be non-const
      // (see TaskBase ctors).  Disregard this previous note:
      // Old note (no longer applicable): it might be possible that in the
      // future this argument needs to be non-const, but for now I'm using const
      // since it's consistent with (most aspects of) the task object being
      // immutible once it's owned by the runtime system, which is the case with
      // from_task

      // sanity check: recapture should only be triggered when the parent while,
      // which we are recapturing from, is running:
      assert(
        (intptr_t)get_running_task_impl()
          == (intptr_t)static_cast<TaskBase*>(&from_task)
      );

      return std::make_unique<WhileLambdaTask>(
        from_task.capture_manager_,
        from_task,
        // since we're reconstructing an inner while, the from_task is the
        // parent task, so we can pass that on to the base constructor
        &from_task
      );
    }

    void run() override {
      if(this->callable_()) {

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

// </editor-fold> end WhileLambdaTask }}}2
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// <editor-fold desc="DoLambdaTask"> {{{2

template <typename Lambda, typename CaptureManagerT>
struct DoLambdaTask
  : LambdaTask<Lambda>
{

  public:

    using base_t = LambdaTask<Lambda>;

    std::shared_ptr<CaptureManagerT> capture_manager_;

    /**
     *  Ctor called during WhileDoCaptureManager construction
     */
    template <
      typename HelperT,
      size_t... OtherArgIdxs
    >
    DoLambdaTask(
      CaptureManagerT* capture_manager,
      HelperT&& helper,
      std::integer_sequence<size_t, OtherArgIdxs...>
    ) : base_t(
          std::move(helper.do_lambda),
          capture_manager->in_do_mode(),
          variadic_arguments_begin_tag{},
          std::get<OtherArgIdxs>(std::move(helper.task_option_args_tup))...
        )
    { }

    /**
     *  Ctor called by recapture(); always only used for constructing an inner
     *  nested do task (so never a double copy capture).  Can't be private,
     *  though, since we need to construct it with std::make_unique.
     */
    DoLambdaTask(
      std::shared_ptr<CaptureManagerT> const& capture_manager,
      DoLambdaTask const& to_recapture,
      TaskBase* parent_task
    ) : base_t(
          LambdaTask_tags::no_double_copy_capture_tag,
          to_recapture.callable_,
          parent_task,
          get_running_task_impl(),
          capture_manager->in_do_mode(),
          variadic_arguments_begin_tag{}
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

    static std::unique_ptr<DoLambdaTask>
    recapture(DoLambdaTask const& from_task) {
      // See note about argument constness in WhileLambdaTask::recapture(),
      // except this time we can still use const since we get the parent task
      // from elsewhere

      return std::make_unique<DoLambdaTask>(
        from_task.capture_manager_,
        from_task,
        // since we're reconstructing an inner do, and since this must be called
        // after an inner while has been recaptured in the capture managers's
        // recapture() method, the parent task will always be the current
        // while_task in the capture_manager.  Note that it will *not* be the
        // currently running task!
        from_task.capture_manager_->while_task_.get()
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
  tinympl::nonesuch,
  tinympl::vector<Args...>,
  Lambda
>
{

  using while_helper_t = WhileHelper;
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
      )
  {
    auto while_do_mngr = std::make_shared<
      WhileDoCaptureManager<
        typename WhileHelper::callable_t,
        typename WhileHelper::args_fwd_tuple_t,
        WhileHelper::has_lambda_callable,
        callable_t, args_tuple_t, has_lambda_callable
      >
    >(
      utility::variadic_constructor_tag,
      std::move(*this)
    );
    while_do_mngr->set_capture_managers(while_do_mngr);
    while_do_mngr->register_while_task();
  }

};


// </editor-fold> end create_work_while_do_helper, lambda version }}}2
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// <editor-fold desc="create_work_while_helper, lambda version"> {{{2

template <typename Lambda, typename... Args>
struct _create_work_while_helper<
  tinympl::nonesuch,
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

#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
  _create_work_while_creation_context* context_;
#endif

  static constexpr auto has_lambda_callable = true;

  callable_t while_lambda;
  args_fwd_tuple_t args_fwd_tup;
  task_option_args_tuple_t task_option_args_tup;

  _create_work_while_helper(_create_work_while_helper&&) = default;
  _create_work_while_helper(_create_work_while_helper const&) = delete;

  _create_work_while_helper(
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    _create_work_while_creation_context* ctxt,
#endif
    Args&& ... args,
    std::remove_reference_t<Lambda>&& f // force rvalue reference<
  ) : while_lambda(std::move(f)),
      task_option_args_tup(
        std::forward_as_tuple(std::forward<Args>(args)...)
      )
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
      , context_(ctxt)
#endif
  { }

  template <typename DoFunctor=tinympl::nonesuch, typename... DoArgs>
  auto
  do_(DoArgs&&... args)&&
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
