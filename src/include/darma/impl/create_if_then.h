/*
//@HEADER
// ************************************************************************
//
//                      create_if_then.h
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

#ifndef DARMA_CREATE_IF_THEN_H
#define DARMA_CREATE_IF_THEN_H

#include <tinympl/vector.hpp>

#include <darma/interface/app/create_work.h>
#include <darma/impl/capture.h>
#include <darma/impl/handle.h>



// TODO Functor version

namespace darma_runtime {

namespace experimental {

namespace detail {

template <typename...>
struct _create_work_then_helper;

template <typename...>
struct _create_work_if_helper;

template <typename IfLambda, typename ThenLambda>
struct IfLambdaThenLambdaTask : public darma_runtime::detail::TaskBase {

  public:

    struct ThenLambdaTask : public darma_runtime::detail::TaskBase {

      ThenLambda then_lambda_;

      ThenLambdaTask(ThenLambdaTask&& other)
        : then_lambda_(std::move(other.then_lambda_))
      { }

      explicit ThenLambdaTask(ThenLambda&& then_lambda)
        : then_lambda_(std::move(then_lambda))
      { }

      void run() override {
        then_lambda_();
      }

    };

    IfLambda if_lambda_;
    ThenLambdaTask then_task_;
    bool is_doing_then_copy_ = false;

    std::vector<std::shared_ptr<darma_runtime::detail::UseHolder>> schedule_uses;

    template <typename ThenHelper>
    IfLambdaThenLambdaTask(
      ThenHelper&& helper
    ) : if_lambda_(std::move(helper.if_helper.func)),
        then_task_(std::move(helper.then_lambda))
    {
      auto* parent_task = static_cast<darma_runtime::detail::TaskBase*>(
        abstract::backend::get_backend_context()->get_running_task()
      );
      parent_task->current_create_work_context = this;


      is_double_copy_capture = true;
      is_doing_then_copy_ = false;

      default_capture_as_info |= darma_runtime::detail::AccessHandleBase::CapturedAsInfo::ReadOnly;

      // Invoke the copy constructor of the if lambda
      IfLambda if_lambda_tmp = if_lambda_;
      // now move the copy back
      if_lambda_ = std::move(if_lambda_tmp);


      // Now do a copy of the then lambda and capture the if part as schedule only
      is_doing_then_copy_ = true;
      default_capture_as_info &= ~darma_runtime::detail::AccessHandleBase::CapturedAsInfo::ReadOnly;

      ThenLambda then_lambda_tmp = then_task_.then_lambda_;
      // now move the copy back
      then_task_.then_lambda_ = std::move(then_lambda_tmp);

      is_doing_then_copy_ = false;

      parent_task->current_create_work_context = nullptr;

    }


    void do_capture_uses(
      darma_runtime::detail::HandleUse::permissions_t requested_schedule_permissions,
      darma_runtime::detail::HandleUse::permissions_t requested_immediate_permissions,
      std::shared_ptr<darma_runtime::detail::VariableHandleBase> const& var_handle,
      std::shared_ptr<darma_runtime::detail::UseHolder>& captured_current_use,
      std::shared_ptr<darma_runtime::detail::UseHolder>& source_and_continuing_current_use
    ) override {
      if(not is_doing_then_copy_) {
        // We're handling the if capture
        captured_current_use = darma_runtime::detail::make_captured_use_holder(
          var_handle,
          requested_schedule_permissions, // TODO !!! need to upgrade this if then part does writes...
          requested_immediate_permissions,
          source_and_continuing_current_use
        );

        add_dependency(captured_current_use->use);
      }
      else {
        // We're handling the "then" capture, but also need to put stuff in the "if"
        // TODO !!! look to see if this was already captured in the "if" with these permissions
        // Only if it's not already captured....
        auto if_nested_use = darma_runtime::detail::make_captured_use_holder(
          var_handle,
          requested_schedule_permissions,
          darma_runtime::detail::HandleUse::None,
          source_and_continuing_current_use
        );

        // TODO !!! only if not already captured...
        // TODO this appears to be the problem...
        schedule_uses.push_back(if_nested_use);

        captured_current_use = darma_runtime::detail::make_captured_use_holder(
          var_handle,
          requested_schedule_permissions,
          requested_immediate_permissions,
          if_nested_use
        );


        add_dependency(if_nested_use->use);
        then_task_.add_dependency(captured_current_use->use);
      }

    }

    void run() override {
      if(if_lambda_()) {
        std::unique_ptr<abstract::frontend::Task> then_task_ptr(
          std::make_unique<ThenLambdaTask>(std::move(then_task_))
        );
        abstract::backend::get_backend_runtime()->register_task(
          std::move(then_task_ptr)
        );
        schedule_uses.clear();
      }
    }


};


template <typename IfHelper, typename Lambda, typename... Args>
struct _create_work_then_helper<
  IfHelper, meta::nonesuch, tinympl::vector<Args...>, Lambda
> {

  _create_work_then_helper(
    IfHelper&& if_helper_in,
    Args&&... args,
    Lambda&& then_lambda_in
  ) : if_helper(std::forward<IfHelper>(if_helper_in)),
      then_lambda(std::forward<Lambda>(then_lambda_in))
  { }

  IfHelper if_helper;
  Lambda then_lambda;
  bool else_invoked = false;

  template <typename... ElseArgs>
  auto
  else_(ElseArgs&&... args) && {
    else_invoked = true;
    // TODO implement this
  }

  ~_create_work_then_helper() {
    if(not else_invoked) {
      std::unique_ptr<abstract::frontend::Task> if_then_task = std::make_unique<
        IfLambdaThenLambdaTask<typename IfHelper::lambda_t, Lambda>
      >(
        std::move(*this)
      );
      abstract::backend::get_backend_runtime()->register_task(
        std::move(if_then_task)
      );
    }
  }

};

template <typename Lambda, typename... Args>
struct _create_work_if_helper<meta::nonesuch, tinympl::vector<Args...>, Lambda> {

  Lambda func;
  using lambda_t = Lambda;
  using args_tuple_t = std::tuple<Args...>;

  _create_work_if_helper(
    Args&&..., /* modifiers ignored/processed elsewhere */
    Lambda&& f
  ) : func(std::forward<Lambda>(f))
  { }

  template <typename... ThenArgs>
  auto
  then_(ThenArgs&&... args) && {
    return _create_work_then_helper<
      _create_work_if_helper, meta::nonesuch,
      typename tinympl::vector<ThenArgs...>::pop_back::type,
      typename tinympl::vector<ThenArgs...>::back::type
    >(
      std::move(*this),
      std::forward<ThenArgs>(args)...
    );
  }
};


} // end namespace detail

template <typename Functor=meta::nonesuch, typename... Args>
auto
create_work_if(Args&&... args) {
  return detail::_create_work_if_helper<
    Functor,
    typename tinympl::vector<Args...>::safe_pop_back::type,
    typename tinympl::vector<Args...>::template safe_back<meta::nonesuch>::type
  >(
    std::forward<Args>(args)...
  );
}

} // end namespace experimental

} // end namespace darma_runtime

#endif //DARMA_CREATE_IF_THEN_H
