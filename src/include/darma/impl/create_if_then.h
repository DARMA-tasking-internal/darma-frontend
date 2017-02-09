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

#include "create_if_then_fwd.h"

#include <darma/interface/app/create_work.h>
#include <darma/impl/capture.h>
#include <darma/impl/handle.h>



// TODO Functor version

namespace darma_runtime {

namespace detail {

template <typename...>
struct _create_work_then_helper;

template <typename...>
struct _create_work_if_helper;

template <typename Lambda>
struct NestedThenElseLambdaTask : public darma_runtime::detail::TaskBase {

  Lambda lambda_;

  NestedThenElseLambdaTask(NestedThenElseLambdaTask&& other)
    : TaskBase(std::move(other)),
      lambda_(std::move(other.lambda_))
  { }

  explicit NestedThenElseLambdaTask(Lambda&& lambda)
    : lambda_(std::move(lambda))
  { }

  void run() override {
    lambda_();
  }

};

template <>
struct NestedThenElseLambdaTask<void> { };

template <typename IfLambda, typename ThenLambda, typename ElseLambda /* =void*/>
struct IfLambdaThenLambdaTask : public darma_runtime::detail::TaskBase {

  public:

    IfLambda if_lambda_;
    using then_task_t = NestedThenElseLambdaTask<ThenLambda>;
    using else_task_t = NestedThenElseLambdaTask<ElseLambda>;
    then_task_t then_task_;
    else_task_t else_task_;

    typedef enum CaptureStage {
      IfCopy,
      ThenCopyForIf,
      ElseCopyForIf,
      ThenCopyForThen,
      ElseCopyForElse
    } capture_stage_t;

    capture_stage_t current_stage;

    struct CaptureDescription {
      AccessHandleBase* captured = nullptr;
      HandleUse::permissions_t requested_schedule_permissions = HandleUse::None;
      HandleUse::permissions_t requested_immediate_permissions = HandleUse::None;
      bool is_implicit_in_if = false;
      bool captured_in_then_or_else = false;
      std::vector<std::shared_ptr<UseHolder>*> uses_to_set;
    };

    std::map<AccessHandleBase const*, CaptureDescription> if_captures_;

    using captured_handle_map_t = std::unordered_map<
      types::key_t, std::shared_ptr<AccessHandleBase>,
      typename key_traits<types::key_t>::hasher,
      typename key_traits<types::key_t>::key_equal
    >;

    captured_handle_map_t implicit_if_captured_handles;
    captured_handle_map_t explicit_if_captured_handles;

    template <typename ThenHelper>
    IfLambdaThenLambdaTask(
      ThenHelper&& helper,
      std::enable_if_t<
        std::is_void<typename std::decay_t<ThenHelper>::else_lambda_t>::value
          and std::is_void<ElseLambda>::value,
        int
      > = 0
    ) : IfLambdaThenLambdaTask(
          std::move(helper.if_helper.func),
          std::move(helper.then_lambda),
          else_task_t{}
        )
    { /* Forwarding constructor, body must be empty */ }

    template <typename ElseHelper>
    IfLambdaThenLambdaTask(
      ElseHelper&& else_helper,
      std::enable_if_t<
        not std::is_void<typename std::decay_t<ElseHelper>::else_lambda_t>::value
          and not std::is_void<ElseLambda>::value,
        int
      > = 0
    ) : IfLambdaThenLambdaTask(
          std::move(else_helper.then_helper.if_helper.func),
          std::move(else_helper.then_helper.then_lambda),
          std::move(else_helper.else_lambda)
        )
    { /* Forwarding constructor, body must be empty */ }

    IfLambdaThenLambdaTask(
      IfLambda&& if_lambda,
      ThenLambda&& then_lambda,
      std::conditional_t<
        std::is_void<ElseLambda>::value,
        else_task_t,
        ElseLambda
      >&& else_lambda

    ) : if_lambda_(std::move(if_lambda)),
        then_task_(std::move(then_lambda)),
        else_task_(std::move(else_lambda))
    {
      auto* parent_task = static_cast<darma_runtime::detail::TaskBase*>(
        abstract::backend::get_backend_context()->get_running_task()
      );
      parent_task->current_create_work_context = this;

      // Setup some variables
      is_double_copy_capture = true;
      current_stage = IfCopy;

      // Invoke the copy constructor of the if lambda
      IfLambda if_lambda_tmp = if_lambda_;

      // Now do a copy of the then lambda and capture the if part as schedule only
      current_stage = ThenCopyForIf;

      // Invoke the copy ctor
      ThenLambda then_lambda_tmp = then_task_.lambda_;

      current_stage = ElseCopyForIf;
      auto else_lambda_tmp = _do_copy_else_lambda();

      // Now loop over the remaining if_captures (those not handle in the Then
      // copy mode) and do the capture on them
      for(auto&& if_cap_pair : if_captures_) {
        auto& desc = if_cap_pair.second;
        auto const* source = if_cap_pair.first;
        desc.captured->current_use_ = make_captured_use_holder(
          source->var_handle_base_,
          desc.requested_schedule_permissions,
          desc.requested_immediate_permissions,
          source->current_use_
        );
        add_dependency(desc.captured->current_use_->use);
        for(auto&& use_to_set : desc.uses_to_set) {
          (*use_to_set) = desc.captured->current_use_;
        }
        if(desc.captured_in_then_or_else and not desc.is_implicit_in_if) {
          auto const& key = desc.captured->var_handle_base_->get_key();
          auto insertion_result = explicit_if_captured_handles.insert(
            std::make_pair(key,
              desc.captured->copy(/* check_context = */ false)
            )
          );
          assert(insertion_result.second); // can't already be in map
        }
      }
      if_captures_.clear();

      // now move the copies back *after* all of the if capture processing is
      // done, since it relies on pointers that the move ctor (might) change
      // or break
      if_lambda_ = std::move(if_lambda_tmp);
      then_task_.lambda_ = std::move(then_lambda_tmp);
      _do_restore_else_lambda(std::move(else_lambda_tmp));

      // Reset stuff
      is_double_copy_capture = false;
      parent_task->current_create_work_context = nullptr;

    }

    struct _not_a_lambda { };

    template <typename _SFINAE_only=void>
    std::enable_if_t<
      std::is_void<_SFINAE_only>::value
      and std::is_void<ElseLambda>::value,
      _not_a_lambda
    >
    _do_copy_else_lambda() { return _not_a_lambda{}; };


    template <typename _SFINAE_only=void>
    std::enable_if_t<
      std::is_void<_SFINAE_only>::value
        and not std::is_void<ElseLambda>::value,
      ElseLambda
    >
    _do_copy_else_lambda() {
      return else_task_.lambda_;
    };

    template <typename _SFINAE_only=void>
    std::enable_if_t<
      std::is_void<_SFINAE_only>::value
        and std::is_void<ElseLambda>::value
    >
    _do_restore_else_lambda(
      _not_a_lambda&&
    ) { };


    template <typename _SFINAE_only=void>
    void
    _do_restore_else_lambda(
      std::enable_if_t<
        std::is_void<_SFINAE_only>::value
          and not std::is_void<ElseLambda>::value,
        ElseLambda
      >&& else_lambda_tmp
    ) {
      else_task_.lambda_ = std::move(else_lambda_tmp);
    };

    void
    do_capture(
      darma_runtime::detail::AccessHandleBase& captured,
      darma_runtime::detail::AccessHandleBase const& source_and_continuing
    ) override {

      switch(current_stage) {
        case IfCopy: {

          // Only do the capture checks on the if copy stage, since source
          // may not be valid in the ThenCopyForThen stage, and since "double copies"
          // should be expected for things captured by both if and then
          do_capture_checks(source_and_continuing);

          // TODO handle things explicitly marked as modify

          // TODO handle actual aliasing

          // this doesn't actually work, since aliasing doesn't involve the same
          // source pointer, but rather the same use pointer
          if(if_captures_.find(&source_and_continuing) == if_captures_.end()) {

            auto& desc = if_captures_[&source_and_continuing];
            desc.captured = &captured;

            // TODO handle the Leaf flag
            DARMA_ASSERT_MESSAGE(
              (source_and_continuing.captured_as_ & AccessHandleBase::Leaf) == 0,
              "Leaf flag handling not yet implemented for create_work_if"
            );

            // TODO handle the ScheduleOnly flag
            DARMA_ASSERT_MESSAGE(
              (source_and_continuing.captured_as_ & AccessHandleBase::ScheduleOnly) == 0,
              "ScheduleOnly flag handling not yet implemented for create_work_if"
            );

            desc.requested_immediate_permissions = HandleUse::Read;
            desc.requested_schedule_permissions = HandleUse::Read;
            desc.is_implicit_in_if = false;

          }

          break;
        }
        case ThenCopyForIf:
        case ElseCopyForIf: {

          // TODO !!! handle things captured in both then and else !!!

          auto& desc = if_captures_[&source_and_continuing];

          desc.requested_schedule_permissions = HandleUse::Modify; // TODO look for permissions downgrades

          //if((source_and_continuing.captured_as_ & AccessHandleBase::ReadOnly) != 0) {
          //  desc.requested_schedule_permissions = HandleUse::Read;
          //}
          //else {
          //  // this is a modify, so request schedule modify permissions here
          //  desc.requested_schedule_permissions = HandleUse::Modify;
          //}

          if(not desc.captured_in_then_or_else) {
            if (desc.captured) {
              // It's explicit in the "if" *and* explicit in the "then"

              // (so something's wrong with my logic if this fails)
              assert(not desc.is_implicit_in_if);


            } else {

              // It's an "implicit" use in the if clause, so we need to store it
              desc.is_implicit_in_if = true;

              auto const & key = source_and_continuing.var_handle_base_->get_key();
              auto insertion_result = implicit_if_captured_handles.insert(
                std::make_pair(key,
                  source_and_continuing.copy(/* check_context = */ false)
                )
              );
              assert(insertion_result.second); // can't already be in map

              desc.captured = insertion_result.first->second.get();

            }
          }

          // Set this for the benefit if the source of the ThenCopyForThen phase
          desc.uses_to_set.push_back(&captured.current_use_);

          desc.captured_in_then_or_else = true;

          break;
        }
        case ThenCopyForThen: {
          // Be careful!!!!! source_and_continuing might not be valid
          // Do this elsewhere so that we don't accidentally access source_and_continuing
          do_capture_then_else_copy_for_then_else(captured, then_task_, std::false_type{});
          break;
        }
        case ElseCopyForElse: {
          do_capture_then_else_copy_for_then_else(captured,
            else_task_,
            typename std::is_void<ElseLambda>::type{}
          );
          break;
        }
      }

      if(current_stage != ThenCopyForThen && current_stage != ElseCopyForElse) {
        // Reset all of the capture flags
        source_and_continuing.captured_as_ = AccessHandleBase::Normal;
      }

    }

    template <typename ThenElseTask>
    void
    do_capture_then_else_copy_for_then_else(
      AccessHandleBase& captured,
      ThenElseTask& then_else_task,
      std::false_type
    ) {
      auto key = captured.var_handle_base_->get_key();

      // TODO !!! make permission downgrades work!!!
      // TODO handle interaction with aliasing

      std::shared_ptr<AccessHandleBase> source_handle = nullptr;
      auto found_explicit = explicit_if_captured_handles.find(key);
      if(found_explicit != explicit_if_captured_handles.end()) {
        source_handle = found_explicit->second;
        // Don't allow multiple captures
        explicit_if_captured_handles.erase(found_explicit);
      }
      else {
        auto found_implicit = implicit_if_captured_handles.find(key);
        if(found_implicit != implicit_if_captured_handles.end()) {
          source_handle = found_implicit->second;
          // Don't allow multiple captures
          implicit_if_captured_handles.erase(found_implicit);
        }
      }

      assert(source_handle.get() != nullptr);

      // For now, assume modify-modify is requested
      captured.current_use_ = make_captured_use_holder<
        /* AllowRegisterContinuation = */ true
      >(
        captured.var_handle_base_,
        HandleUse::Modify,
        HandleUse::Modify,
        source_handle->current_use_
      );

      then_else_task.add_dependency(captured.current_use_->use);

      // But don't register the continuation use.
      // TODO keep this from even generating the continuation flow?!?

      // Tell the source (now continuing) handle to establish an alias
      source_handle->current_use_->could_be_alias = true;

      // Release the handle, triggering release of the use
      source_handle = nullptr;

    }

    template <typename ThenElseTask>
    void
    do_capture_then_else_copy_for_then_else(
      AccessHandleBase& captured,
      ThenElseTask& then_else_task,
      std::true_type
    ) {
      // Do nothing
    }

    void run() override {
      if(if_lambda_()) {

        auto* parent_task = static_cast<darma_runtime::detail::TaskBase*>(
          abstract::backend::get_backend_context()->get_running_task()
        );
        parent_task->current_create_work_context = this;

        current_stage = ThenCopyForThen;
        is_double_copy_capture = false; // since source context may no longer be valid

        {
          // Invoke the copy ctor
          ThenLambda then_lambda_tmp = then_task_.lambda_;

          // Then move it back
          then_task_.lambda_ = std::move(then_lambda_tmp);
        }

        parent_task->current_create_work_context = nullptr;

        {
          // Delete the if_lambda by moving it and deleting the move destination
          IfLambda if_lambda_delete_tmp = std::move(if_lambda_);

          // Delete the else task by moving it...
          else_task_t else_task_delete_tmp = std::move(else_task_);
        }

        explicit_if_captured_handles.clear();
        implicit_if_captured_handles.clear();

        abstract::backend::get_backend_runtime()->register_task(
          std::make_unique<then_task_t>(std::move(then_task_))
        );
      }
      else {
        _run_else_part(typename std::is_void<ElseLambda>::type{});
      }
    }

    template <typename _SFINAE_only=void>
    std::enable_if_t<
      std::is_void<_SFINAE_only>::value
      and not std::is_void<ElseLambda>::value
    >
    _run_else_part(std::false_type) {

      auto* parent_task = static_cast<darma_runtime::detail::TaskBase*>(
        abstract::backend::get_backend_context()->get_running_task()
      );
      parent_task->current_create_work_context = this;

      current_stage = ElseCopyForElse;
      is_double_copy_capture = false; // since source context may no longer be valid

      // Invoke the copy ctor
      auto else_lambda_tmp = _do_copy_else_lambda();

      // Then move it back
      _do_restore_else_lambda(std::move(else_lambda_tmp));

      parent_task->current_create_work_context = nullptr;

      {
        // Delete the if_lambda by moving it and deleting the move destination
        IfLambda if_lambda_delete_tmp = std::move(if_lambda_);
        // Delete the then task by moving it...
        then_task_t then_task_delete_tmp = std::move(then_task_);
      }

      explicit_if_captured_handles.clear();
      implicit_if_captured_handles.clear();

      abstract::backend::get_backend_runtime()->register_task(
        std::make_unique<else_task_t>(std::move(else_task_))
      );

    }

    void _run_else_part(std::true_type) {
      // Do nothing
    }

};

template <typename...>
struct _create_work_else_helper;

template <typename ThenHelper, typename Lambda, typename... Args>
struct _create_work_else_helper<
  ThenHelper, meta::nonesuch, tinympl::vector<Args...>, Lambda
> {
  public:

    _create_work_else_helper(
      ThenHelper&& then_helper,
      Args&&... args,
      Lambda&& else_lambda_in
    ) : then_helper(std::move(then_helper)),
        else_lambda(std::move(else_lambda_in))
    { }

    using then_lambda_t = typename ThenHelper::then_lambda_t;
    using else_lambda_t = Lambda;
    using if_helper_t = typename ThenHelper::if_helper_t;

    ThenHelper then_helper;
    Lambda else_lambda;

    ~_create_work_else_helper() {
      std::unique_ptr<abstract::frontend::Task> if_then_else_task = std::make_unique<
        IfLambdaThenLambdaTask<typename if_helper_t::lambda_t, then_lambda_t, Lambda>
      >(
        std::move(*this)
      );
      abstract::backend::get_backend_runtime()->register_task(
        std::move(if_then_else_task)
      );
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

  _create_work_then_helper(_create_work_then_helper&&) = default;
  _create_work_then_helper(_create_work_then_helper const&) = delete;

  IfHelper if_helper;
  Lambda then_lambda;
  bool else_invoked = false;

  using if_helper_t = IfHelper;
  using then_lambda_t = Lambda;
  using else_lambda_t = void;

  template <typename... ElseArgs>
  void else_(ElseArgs&&... args) && {
    else_invoked = true;
    _create_work_else_helper<_create_work_then_helper,
      meta::nonesuch,
      typename tinympl::vector<ElseArgs...>::pop_back::type,
      typename tinympl::vector<ElseArgs...>::back::type
    >(std::move(*this), std::forward<ElseArgs>(args)...);
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

namespace experimental {

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
