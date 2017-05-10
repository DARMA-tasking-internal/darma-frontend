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

#include <darma/impl/functor_traits.h>

#include <darma/interface/app/create_work.h>
#include <darma/impl/capture.h>
#include <darma/impl/handle.h>
#include <darma/impl/create_work_fwd.h>


// TODO Don't create a task (and generate a warning?) if there are no captures in the if clause

namespace darma_runtime {

namespace detail {

struct _not_a_lambda { };

struct ParsedCaptureOptions {

  using permissions_downgrade_map_t = std::unordered_map<
    types::key_t, HandleUse::permissions_t,
    typename key_traits<types::key_t>::hasher,
    typename key_traits<types::key_t>::key_equal
  >;

  HandleUse::permissions_t default_immediate_permissions;
  HandleUse::permissions_t default_scheduling_permissions;

  permissions_downgrade_map_t scheduling_permissions_downgrades;
  permissions_downgrade_map_t immediate_permissions_downgrades;

  template <typename PermissionsDowngradeT>
  int _process_downgrade_option(
    PermissionsDowngradeT&& downgrade
  ) {
    auto sched_perms = downgrade.get_requested_scheduling_permissions(
      default_scheduling_permissions
    );
    if(sched_perms != default_scheduling_permissions) {
      scheduling_permissions_downgrades.insert(
        std::make_pair(downgrade.handle.get_key(), sched_perms)
      );
    }
    auto immed_perms = downgrade.get_requested_immediate_permissions(
      default_immediate_permissions
    );
    if(immed_perms != default_immediate_permissions) {
      immediate_permissions_downgrades.insert(
        std::make_pair(downgrade.handle.get_key(), immed_perms)
      );
    }
    return 0; // sentinel returned to std::forward_as_tuple
  };

  // Splat any tuples we get...
  template <
    typename... Options,
    size_t... Idxs
  >
  auto _process_option(
    std::tuple<Options...>&& downgrades,
    std::integer_sequence<size_t, Idxs...>
  ) {
    return std::make_tuple( // return value ignored
      this->_process_downgrade_option(
        std::get<Idxs>(std::move(downgrades))
      )...
    );
  }

  template <
    typename... Args,
    size_t... Idxs
  >
  ParsedCaptureOptions(
    HandleUse::permissions_t in_default_immediate_permissions,
    HandleUse::permissions_t in_default_scheduling_permissions,
    std::tuple<Args...>&& args,
    std::integer_sequence<size_t, Idxs...>
  ) : default_scheduling_permissions(in_default_scheduling_permissions),
      default_immediate_permissions(in_default_immediate_permissions)
  {
    // TODO process name=...
    std::make_tuple( // ignored return value
      _process_option(
        std::get<Idxs>(std::move(args)),
        meta::tuple_indices_for(args)
      )...
    );
  }

  ParsedCaptureOptions(ParsedCaptureOptions&&) = default;


  HandleUse::permissions_t
  get_requested_immediate_permissions(
    AccessHandleBase const& handle
  ) const {
    auto found = immediate_permissions_downgrades.find(
      handle.var_handle_base_->get_key()
    );
    if(found != immediate_permissions_downgrades.end()) {
      return found->second;
    }
    else {
      return default_immediate_permissions;
    }
  }

  HandleUse::permissions_t
  get_requested_scheduling_permissions(
    AccessHandleBase const& handle
  ) const {
    auto found = scheduling_permissions_downgrades.find(
      handle.var_handle_base_->get_key()
    );
    if(found != scheduling_permissions_downgrades.end()) {
      return found->second;
    }
    else {
      return default_scheduling_permissions;
    }
  }

};

//==============================================================================
// <editor-fold desc="NestedThenElseLambdaTask"> {{{1

template <typename Lambda>
struct NestedThenElseLambdaTask: public darma_runtime::detail::TaskBase {


  Lambda lambda_;
  ParsedCaptureOptions capture_options_;

  NestedThenElseLambdaTask(NestedThenElseLambdaTask&& other)
    : TaskBase(std::move(other)),
      lambda_(std::move(other.lambda_)),
      capture_options_(std::move(other.capture_options_))
  { }

  template <typename... Args>
  explicit NestedThenElseLambdaTask(
    Lambda&& lambda,
    std::tuple<>&&, // args, unused
    std::tuple<Args...>&& options_args
  ) : lambda_(std::move(lambda)),
      capture_options_(
        HandleUse::Modify, HandleUse::Modify,
        std::move(options_args),
        std::index_sequence_for<Args...>{}
      )
  { }

  void run() override {
    lambda_();
  }
};

template <typename FunctorWrapper, typename StoredArgsTuple>
struct NestedThenElseFunctorTask: public darma_runtime::detail::TaskBase {

  FunctorWrapper wrapper_;
  ParsedCaptureOptions capture_options_;
  StoredArgsTuple args_;

  template <typename Task, typename ForwardedArgsTuple, typename ForwardedOptionsTuple>
  explicit NestedThenElseFunctorTask(
    Task& task,
    ForwardedArgsTuple&& args,
    ForwardedOptionsTuple&& // ignored, for now
  ) : wrapper_(task), // this sets the mode
      capture_options_(
        HandleUse::Modify, HandleUse::Modify,
        std::tuple<>(), std::make_index_sequence<0>{} // for now, don't parse the arguments for downgrades
      ),
      args_(std::forward<ForwardedArgsTuple>(args))
  { }

  template <size_t... Idxs>
  void
  _run_helper(std::integer_sequence<size_t, Idxs...>) {
    using functor_traits_t = functor_call_traits<
      typename FunctorWrapper::functor_t, std::tuple_element_t<Idxs, StoredArgsTuple>...
    >;
    typename FunctorWrapper::functor_t{}(
      typename functor_traits_t::template call_arg_traits<Idxs>{}.get_converted_arg(
        std::get<Idxs>(std::move(args_))
      )...
    );
  }

  void
  run() override {
    _run_helper(
      std::make_index_sequence<std::tuple_size<StoredArgsTuple>::value>{}
    );
  }

  _not_a_lambda lambda_; // for now

};

template <>
struct NestedThenElseLambdaTask<void> {
  NestedThenElseLambdaTask() = default;
  template <typename Ignored>
  explicit NestedThenElseLambdaTask(
    Ignored&&, std::tuple<>&&, std::tuple<>&&
  ) { }
};

// </editor-fold> end NestedThenElseLambdaTask }}}1
//==============================================================================


typedef enum IfThenElseCaptureStage {
  Unknown,
  IfCopy,
  ThenCopyForIf,
  ElseCopyForIf,
  ThenCopyForThen,
  ElseCopyForElse,
  ThenCopyForIfAndThen,
  ElseCopyForIfAndElse
} if_then_else_capture_stage_t;

template <typename Functor, IfThenElseCaptureStage Stage>
struct WrappedFunctorHelper {

  // TODO better argument mismatch error messages

  using functor_t = Functor;

  template <typename TaskT>
  WrappedFunctorHelper(
    TaskT& task
  ) {
    // Basically just do the setup here, so that the copy into arguments
    // knows which task to refer to

    auto* parent_task = static_cast<darma_runtime::detail::TaskBase*>(
      abstract::backend::get_backend_context()->get_running_task()
    );
    parent_task->current_create_work_context = &task;

    task.current_stage = Stage;
    task.is_double_copy_capture = false;
  }

  WrappedFunctorHelper(_not_a_lambda&&) {
    // This is invoked in the post-capture cleanup; nothing to do here
  }

};


//==============================================================================
// <editor-fold desc="IfLambdaThenLambdaTask"> {{{1

template <
  typename IfCallable, typename IfArgsTuple, bool IfIsLambda,
  typename ThenLambda, typename ThenArgsTuple, bool ThenIsLambda,
  typename ElseLambda /* =void*/,
  typename ElseArgsTuple /*=std::tuple<>*/,
  bool ElseIsLambda /* = false */
>
struct IfLambdaThenLambdaTask: public darma_runtime::detail::TaskBase {

  public:

    using then_task_t = typename std::conditional_t<
      ThenIsLambda,
      typename tinympl::lazy<NestedThenElseLambdaTask>
        ::template instantiated_with<ThenLambda>,
      tinympl::lazy<NestedThenElseFunctorTask>
        ::template instantiated_with<ThenLambda, ThenArgsTuple>
    >::type;
    using else_task_t = typename std::conditional_t<
      ElseIsLambda or std::is_void<ElseLambda>::value,
      typename tinympl::lazy<NestedThenElseLambdaTask>
        ::template instantiated_with<ElseLambda>,
      tinympl::lazy<NestedThenElseFunctorTask>
        ::template instantiated_with<ElseLambda, ElseArgsTuple>
    >::type;


    struct CaptureDescription {
      AccessHandleBase const* source_and_continuing = nullptr;
      AccessHandleBase* captured = nullptr;
      HandleUse::permissions_t requested_schedule_permissions = HandleUse::None;
      HandleUse::permissions_t
        requested_immediate_permissions = HandleUse::None;
      bool is_implicit_in_if = false;
      bool captured_in_then_or_else = false;
      std::vector<std::shared_ptr<UseHolder>*> uses_to_set;
    };


    using if_capture_map_t = std::unordered_map<
      types::key_t, CaptureDescription,
      typename key_traits<types::key_t>::hasher,
      typename key_traits<types::key_t>::key_equal
    >;

    // Member order is important here! (for initialization purposes)

    if_then_else_capture_stage_t current_stage = IfThenElseCaptureStage::Unknown;
    std::vector<AccessHandleBase*> delayed_then_capture_handles_;
    std::vector<AccessHandleBase*> delayed_else_capture_handles_;

    if_capture_map_t if_captures_;
    ParsedCaptureOptions if_options_;

    IfCallable if_callable_; // must be before if_args_!!!
    IfArgsTuple if_args_;

    then_task_t then_task_;
    else_task_t else_task_;


    using captured_handle_map_t = std::unordered_map<
      types::key_t, std::shared_ptr<AccessHandleBase>,
      typename key_traits<types::key_t>::hasher,
      typename key_traits<types::key_t>::key_equal
    >;

    // TODO technically, this oversynchronizes, since there may be, e.g., a
    // period of time in between then if and the then where immediate permissions
    // are not needed.  Don't know how to fix this, though
    captured_handle_map_t implicit_if_captured_handles;
    captured_handle_map_t explicit_if_captured_handles;

    template <typename IfCallableIn, typename _SFINAE_only=void>
    std::enable_if_t<
      IfIsLambda and std::is_void<_SFINAE_only>::value,
      IfCallable
    >
    _do_if_capture(IfCallableIn&& if_callable_in, std::true_type) {

      auto* parent_task = static_cast<darma_runtime::detail::TaskBase*>(
        abstract::backend::get_backend_context()->get_running_task()
      );
      parent_task->current_create_work_context = this;

      // Setup some variables
      is_double_copy_capture = true;
      current_stage = IfCopy;

      // trigger copy ctors (by not doing perfect forwarding here)
      return if_callable_in;
    }

    template <typename IfCallableIn, typename _SFINAE_only=void>
    std::enable_if_t<
      not IfIsLambda and std::is_void<_SFINAE_only>::value,
      IfCallable
    >
    _do_if_capture(IfCallableIn&& forward_to_return_value, std::false_type) {

      // The actual capture happened when the if_args_ member is constructed,
      // so there's nothing to do here except forward to the WrappedFunctor
      // ctor that sets up the state

      return std::forward<IfCallableIn>(forward_to_return_value);
    }

    //--------------------------------------------------------------------------
    // <editor-fold desc="Constructors"> {{{2

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // <editor-fold desc="forwarding Ctors"> {{{2

    // If part, IfIsLambda = false
    template <typename ThenHelper, typename _SFINAE_only=void>
    IfLambdaThenLambdaTask(
      ThenHelper&& helper,
      std::enable_if_t<
          not IfIsLambda
            and std::is_void<_SFINAE_only>::value,
        int
      > = 0
    ) : IfLambdaThenLambdaTask(
          *this,
          std::move(helper.then_helper.if_helper_.args_fwd_tup_),
          std::move(helper.then_helper.if_helper_.task_details_args_),
          std::forward<ThenHelper>(helper)
        )
    { /* Forwarding constructor, body must be empty */ }

    // If part, IfIsLambda = true
    template <typename ThenHelper, typename _SFINAE_only=void>
    IfLambdaThenLambdaTask(
      ThenHelper&& helper,
      std::enable_if_t<
        IfIsLambda
          and std::is_void<_SFINAE_only>::value,
        int
      > = 0
    ) : IfLambdaThenLambdaTask(
          std::move(helper.then_helper.if_helper_.func),
          std::move(helper.then_helper.if_helper_.args_fwd_tup_),
          std::move(helper.then_helper.if_helper_.task_details_args_),
          std::forward<ThenHelper>(helper)
        )
    { /* Forwarding constructor, body must be empty */ }

    // Then part, ThenIsLambda = false
    template <
      typename IfCallableIn, typename IfArgsForwarded,
      typename ThenHelper, typename _SFINAE_only=void,
      typename... IfTaskOptions
    >
    IfLambdaThenLambdaTask(
      IfCallableIn&& if_lambda, IfArgsForwarded&& if_args,
      std::tuple<IfTaskOptions...>&& if_capture_options,
      ThenHelper&& helper,
      std::enable_if_t<
          not ThenIsLambda
            and std::is_void<_SFINAE_only>::value,
        int
      > = 0
    ) : IfLambdaThenLambdaTask(
          std::forward<IfCallableIn>(if_lambda),
          std::forward<IfArgsForwarded>(if_args),
          std::move(if_capture_options),
          *this,
          std::move(helper.then_helper.args_fwd_tup_),
          std::move(helper.then_helper.task_details_args_),
          std::forward<ThenHelper>(helper)
        )
    { /* Forwarding constructor, body must be empty */ }

    // Then part, ThenIsLambda = true
    template <
      typename IfCallableIn, typename IfArgsForwarded,
      typename ThenHelper, typename _SFINAE_only=void,
      typename... IfTaskOptions
    >
    IfLambdaThenLambdaTask(
      IfCallableIn&& if_lambda, IfArgsForwarded&& if_args,
      std::tuple<IfTaskOptions...>&& if_capture_options,
      ThenHelper&& helper,
      std::enable_if_t<
        ThenIsLambda
          and std::is_void<_SFINAE_only>::value,
        int
      > = 0
    ) : IfLambdaThenLambdaTask(
          std::forward<IfCallableIn>(if_lambda),
          std::forward<IfArgsForwarded>(if_args),
          std::move(if_capture_options),
          std::move(helper.then_helper.then_lambda),
          std::move(helper.then_helper.args_fwd_tup_),
          std::move(helper.then_helper.task_details_args_),
          std::forward<ThenHelper>(helper)
        )
    { /* Forwarding constructor, body must be empty */ }


    // Else is void
    template <
      typename IfCallableIn, typename IfArgsForwarded,
      typename ThenCallableIn, typename ThenArgsForwarded,
      typename ThenTaskOptionsTuple, typename Helper,
      typename _SFINAE_only = void,
      typename... IfTaskOptions
    >
    IfLambdaThenLambdaTask(
      IfCallableIn&& if_lambda, IfArgsForwarded&& if_args,
      std::tuple<IfTaskOptions...>&& if_capture_options,
      ThenCallableIn&& then_lambda, ThenArgsForwarded&& then_args,
      ThenTaskOptionsTuple&& then_task_options_tup,
      Helper&&,
      std::enable_if_t<
        std::is_void<ElseLambda>::value
          and std::is_void<_SFINAE_only>::value,
        int
      > = 0
    ) : IfLambdaThenLambdaTask(
          std::forward<IfCallableIn>(if_lambda),
          std::forward<IfArgsForwarded>(if_args),
          std::move(if_capture_options),
          std::forward<ThenCallableIn>(then_lambda),
          std::forward<ThenArgsForwarded>(then_args),
          std::forward<ThenTaskOptionsTuple>(then_task_options_tup),
          else_task_t{}, std::tuple<>{}, std::tuple<>{}
        )
    { /* Forwarding constructor, body must be empty */ }

    // Else is not void, but is a lambda
    template <
      typename IfCallableIn, typename IfArgsForwarded,
      typename ThenCallableIn, typename ThenArgsForwarded,
      typename ThenTaskOptionsTuple, typename Helper,
      typename _SFINAE_only = void,
      typename... IfTaskOptions
    >
    IfLambdaThenLambdaTask(
      IfCallableIn&& if_lambda, IfArgsForwarded&& if_args,
      std::tuple<IfTaskOptions...>&& if_capture_options,
      ThenCallableIn&& then_lambda, ThenArgsForwarded&& then_args,
      ThenTaskOptionsTuple&& then_task_options_tup,
      Helper&& helper,
      std::enable_if_t<
        not std::is_void<ElseLambda>::value
          and std::is_void<_SFINAE_only>::value
          and ElseIsLambda,
        int
      > = 0
    ) : IfLambdaThenLambdaTask(
          std::forward<IfCallableIn>(if_lambda),
          std::forward<IfArgsForwarded>(if_args),
          std::move(if_capture_options),
          std::forward<ThenCallableIn>(then_lambda),
          std::forward<ThenArgsForwarded>(then_args),
          std::forward<ThenTaskOptionsTuple>(then_task_options_tup),
          std::move(helper.else_lambda),
          std::tuple<>{},
          std::move(helper.task_details_args_)
        )
    { /* Forwarding constructor, body must be empty */ }

    // Else is not void, but is not a lambda
    template <
      typename IfCallableIn, typename IfArgsForwarded,
      typename ThenCallableIn, typename ThenArgsForwarded,
      typename ThenTaskOptionsTuple, typename Helper,
      typename _SFINAE_only = void,
      typename... IfTaskOptions
    >
    IfLambdaThenLambdaTask(
      IfCallableIn&& if_lambda, IfArgsForwarded&& if_args,
      std::tuple<IfTaskOptions...>&& if_capture_options,
      ThenCallableIn&& then_lambda, ThenArgsForwarded&& then_args,
      ThenTaskOptionsTuple&& then_task_options_tup,
      Helper&& helper,
      std::enable_if_t<
        not std::is_void<ElseLambda>::value
          and std::is_void<_SFINAE_only>::value
          and not ElseIsLambda,
        int
      > = 0
    ) : IfLambdaThenLambdaTask(
          std::forward<IfCallableIn>(if_lambda),
          std::forward<IfArgsForwarded>(if_args),
          std::move(if_capture_options),
          std::forward<ThenCallableIn>(then_lambda),
          std::forward<ThenArgsForwarded>(then_args),
          std::forward<ThenTaskOptionsTuple>(then_task_options_tup),
          *this,
          std::move(helper.args_fwd_tup_),
          std::move(helper.task_details_args_)
        )
    { /* Forwarding constructor, body must be empty */ }


//    template <typename ElseHelper>
//    IfLambdaThenLambdaTask(
//      ElseHelper&& else_helper,
//      std::enable_if_t<
//        not std::is_void<typename std::decay_t<ElseHelper>::else_lambda_t>::value
//          and not std::is_void<ElseLambda>::value,
//        int
//      > = 0
//    ) : IfLambdaThenLambdaTask(
//          std::move(else_helper.then_helper.if_helper.func),
//          std::move(else_helper.then_helper.if_helper.args_fwd_tup_),
//          std::move(else_helper.then_helper.if_helper.task_details_args_),
//          std::move(else_helper.then_helper.then_lambda),
//          std::move(else_helper.then_helper.args_fwd_tup_),
//          std::move(else_helper.then_helper.task_details_args_),
//          std::move(else_helper.else_lambda),
//          std::move(else_helper.task_details_args_)
//        )
//    { /* Forwarding constructor, body must be empty */ }

    // </editor-fold> end forwarding Ctors }}}2
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    template <
      typename IfCallableIn, typename IfArgsForwarded,
      typename ThenCallableIn, typename ThenArgsForwarded, typename ThenTaskOptionsTuple,
      typename ElseCallableIn, typename ElseArgsForwarded, typename ElseTaskOptionsTuple,
      typename... IfTaskOptions
    >
    IfLambdaThenLambdaTask(
      IfCallableIn&& if_lambda, IfArgsForwarded&& if_args,
      std::tuple<IfTaskOptions...>&& if_capture_options,
      ThenCallableIn&& then_lambda, ThenArgsForwarded&& then_args,
      ThenTaskOptionsTuple&& then_task_options_tup,
      ElseCallableIn&& else_lambda, ElseArgsForwarded&& else_args,
      ElseTaskOptionsTuple&& else_task_options_tup
    ) : if_options_(
          HandleUse::Read,
          HandleUse::Read,
          std::move(if_capture_options),
          std::index_sequence_for<IfTaskOptions...>{}
        ),
        if_callable_(
          _do_if_capture(
            std::forward<IfCallableIn>(if_lambda),
            std::integral_constant<bool, IfIsLambda>{}
          )
        ),
        if_args_(std::forward<IfArgsForwarded>(if_args)),
        then_task_(
          std::forward<ThenCallableIn>(then_lambda),
          std::forward<ThenArgsForwarded>(then_args),
          std::forward<ThenTaskOptionsTuple>(then_task_options_tup)
        ),
        else_task_(
          std::forward<ElseCallableIn>(else_lambda),
          std::forward<ElseArgsForwarded>(else_args),
          std::forward<ElseTaskOptionsTuple>(else_task_options_tup)
        )
    {

      // Now do a copy of the then lambda and capture the if part as schedule only
      current_stage = ThenCopyForIf;
      is_double_copy_capture = true; // since the functor version does nothing
      auto then_lambda_tmp = _do_copy_then_lambda();
      is_double_copy_capture = false; // reset the flag as soon as possible

      current_stage = ElseCopyForIf;
      is_double_copy_capture = true; // since the functor version does nothing
      auto else_lambda_tmp = _do_copy_else_lambda();
      is_double_copy_capture = false; // reset the flag as soon as possible

      // Now loop over the remaining if_captures (those not handle in the Then
      // copy mode) and do the capture on them
      for (auto&& if_cap_pair : if_captures_) {
        auto& desc = if_cap_pair.second;
        auto* source = desc.source_and_continuing;
        desc.captured->call_make_captured_use_holder(
          source->var_handle_base_,
          desc.requested_schedule_permissions,
          desc.requested_immediate_permissions,
          *source
        );
        source->current_use_base_->use_base->already_captured = false;
        desc.captured->call_add_dependency(this);
        if (desc.captured_in_then_or_else and not desc.is_implicit_in_if) {
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
      _do_restore_then_lambda(std::move(then_lambda_tmp));
      _do_restore_else_lambda(std::move(else_lambda_tmp));

      // Reset stuff
      auto* parent_task = static_cast<darma_runtime::detail::TaskBase*>(
        abstract::backend::get_backend_context()->get_running_task()
      );
      parent_task->current_create_work_context = nullptr;

    }

    // </editor-fold> end Constructors }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="else lambda copy/restore"> {{{2


    template <typename _SFINAE_only=void>
    std::enable_if_t<
      std::is_void<_SFINAE_only>::value
        and not ThenIsLambda,
      _not_a_lambda
    >
    _do_copy_then_lambda() { return _not_a_lambda{}; };


    template <typename _SFINAE_only=void>
    std::enable_if_t<
      std::is_void<_SFINAE_only>::value
        and ThenIsLambda,
      ThenLambda
    >
    _do_copy_then_lambda() {
      return then_task_.lambda_;
    };

    template <typename _SFINAE_only=void>
    std::enable_if_t<
      std::is_void<_SFINAE_only>::value
        and not ThenIsLambda
    >
    _do_restore_then_lambda(
      _not_a_lambda&&
    ) {};

    template <typename _SFINAE_only=void>
    void
    _do_restore_then_lambda(
      std::enable_if_t<
        std::is_void<_SFINAE_only>::value
          and ThenIsLambda,
        ThenLambda
      >&& then_lambda_tmp
    ) {
      then_task_.lambda_.~ThenLambda();
      new(&then_task_.lambda_) ThenLambda(std::move(then_lambda_tmp));
    };

    template <typename _SFINAE_only=void>
    std::enable_if_t<
      std::is_void<_SFINAE_only>::value
        and not ElseIsLambda,
      _not_a_lambda
    >
    _do_copy_else_lambda() { return _not_a_lambda{}; };


    template <typename _SFINAE_only=void>
    std::enable_if_t<
      std::is_void<_SFINAE_only>::value
        and ElseIsLambda,
      ElseLambda
    >
    _do_copy_else_lambda() {
      return else_task_.lambda_;
    };

    template <typename _SFINAE_only=void>
    std::enable_if_t<
      std::is_void<_SFINAE_only>::value
        and not ElseIsLambda
    >
    _do_restore_else_lambda(
      _not_a_lambda&&
    ) {};


    template <typename _SFINAE_only=void>
    void
    _do_restore_else_lambda(
      std::enable_if_t<
        std::is_void<_SFINAE_only>::value
          and ElseIsLambda,
        ElseLambda
      >&& else_lambda_tmp
    ) {
      else_task_.lambda_.~ElseLambda();
      new(&else_task_.lambda_) ElseLambda(std::move(else_lambda_tmp));
    };

    // </editor-fold> end else lambda copy/restore }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="do_capture() and helpers"> {{{2

    void
    do_capture(
      darma_runtime::detail::AccessHandleBase& captured,
      darma_runtime::detail::AccessHandleBase const& source_and_continuing
    ) override {

      switch (current_stage) {
        case IfThenElseCaptureStage::IfCopy: {

          // Only do the capture checks on the if copy stage, since source
          // may not be valid in the ThenCopyForThen stage, and since "double copies"
          // should be expected for things captured by both if and then
          do_capture_checks(source_and_continuing);

          // TODO handle things explicitly marked as modify

          // TODO handle actual aliasing

          auto& key = source_and_continuing.var_handle_base_->get_key();

          // this doesn't actually work, since aliasing doesn't involve the same
          // source pointer, but rather the same use pointer
          if (if_captures_.find(key) == if_captures_.end()) {

            auto& desc = if_captures_[key];
            desc.source_and_continuing = &source_and_continuing;
            desc.captured = &captured;

            desc.requested_immediate_permissions =
              if_options_.get_requested_immediate_permissions(source_and_continuing);

            if((source_and_continuing.captured_as_ & AccessHandleBase::Leaf) != 0) {
              // This could still be upgraded by the then or else clauses, though
              desc.requested_schedule_permissions = HandleUse::None;
            }
            else {
              desc.requested_schedule_permissions =
                if_options_.get_requested_immediate_permissions(source_and_continuing);
            }

            desc.is_implicit_in_if = false;

          }
          else {
            DARMA_ASSERT_NOT_IMPLEMENTED("Aliasing with create_work_if");
          }

          break;
        }
        case IfThenElseCaptureStage::ThenCopyForIfAndThen:
        case IfThenElseCaptureStage::ElseCopyForIfAndElse:
        case IfThenElseCaptureStage::ThenCopyForIf:
        case IfThenElseCaptureStage::ElseCopyForIf: {

          auto const& key = source_and_continuing.var_handle_base_->get_key();
          auto& desc = if_captures_[key];

          desc.source_and_continuing = &source_and_continuing;

          HandleUse::permissions_t req_sched = HandleUse::None;
          if(current_stage == IfThenElseCaptureStage::ThenCopyForIf
            or current_stage == IfThenElseCaptureStage::ThenCopyForIfAndThen
          ) {
            req_sched = then_task_.capture_options_.get_requested_scheduling_permissions(
              source_and_continuing
            );
            auto req_immed = then_task_.capture_options_.get_requested_immediate_permissions(
              source_and_continuing
            );
            if(req_immed > req_sched) { req_sched = req_immed; }
          }
          else {
            assert(
              current_stage == IfThenElseCaptureStage::ElseCopyForIf
              or current_stage == IfThenElseCaptureStage::ElseCopyForIfAndElse
            );
            req_sched = _get_else_required_permissions_for_if(
              source_and_continuing,
              std::is_void<ElseLambda>{}
            );
          }

          if(req_sched > desc.requested_schedule_permissions) {
            desc.requested_schedule_permissions = req_sched;
          }

          if (not desc.captured_in_then_or_else) {
            if (desc.captured) {
              // It's explicit in the "if" *and* explicit in the "then"

              // (so something's wrong with my logic if this fails)
              assert(not desc.is_implicit_in_if);

            } else {

              // It's an "implicit" use in the if clause, so we need to store it
              desc.is_implicit_in_if = true;

              auto insertion_result = implicit_if_captured_handles.insert(
                { key, source_and_continuing.copy(/* check_context = */ false) }
              );
              assert(insertion_result.second); // can't already be in map

              desc.captured = insertion_result.first->second.get();

            }
          }

          desc.captured_in_then_or_else = true;

          if(current_stage == IfThenElseCaptureStage::ThenCopyForIfAndThen) {
            delayed_then_capture_handles_.push_back(&captured);
          }
          else if(current_stage == IfThenElseCaptureStage::ElseCopyForIfAndElse) {
            delayed_else_capture_handles_.push_back(&captured);
          }

          if(current_stage == IfThenElseCaptureStage::ThenCopyForIf
            or current_stage == IfThenElseCaptureStage::ElseCopyForIf) {
            // It's a double-copy capture, but we don't want to hold the use
            // in the captured (since we'll make another one when we do the
            // copy for the then or else block
            // TODO we should probably instead put the implicit capture use here
            captured.release_current_use();
          }

          break;
        }
        case IfThenElseCaptureStage::ThenCopyForThen: {
          // Be careful!!!!! source_and_continuing might not be valid
          // Do this elsewhere so that we don't accidentally access source_and_continuing
          do_capture_then_else_copy_for_then_else(captured,
            then_task_,
            std::false_type{}
          );
          break;
        }
        case IfThenElseCaptureStage::ElseCopyForElse: {
          do_capture_then_else_copy_for_then_else(captured,
            else_task_,
            typename std::is_void<ElseLambda>::type{}
          );
          break;
        }
        case IfThenElseCaptureStage::Unknown: {
          DARMA_ASSERT_FAILURE("Unexpected capture in if_then_else_task");      // LCOV_EXCL_LINE
          break;
        }
      }

      if (current_stage != ThenCopyForThen
        && current_stage != ElseCopyForElse) {
        // Reset all of the capture flags
        source_and_continuing.captured_as_ = AccessHandleBase::Normal;
      }

    }

    template <typename _SFINAE_only = void>
    std::enable_if_t<
      std::is_void<ElseLambda>::value
        and std::is_void<_SFINAE_only>::value,
      HandleUse::permissions_t
    >
    _get_else_required_permissions_for_if(
      AccessHandleBase const&, std::true_type
    ) const {
      // Ignored
      return HandleUse::None;
    }

    template <typename _SFINAE_only = void>
    std::enable_if_t<
      not std::is_void<ElseLambda>::value
        and std::is_void<_SFINAE_only>::value,
      HandleUse::permissions_t
    >
    _get_else_required_permissions_for_if(
      AccessHandleBase const& source_and_continuing,
      std::false_type
    ) const {
      auto req_sched = else_task_.capture_options_.get_requested_scheduling_permissions(
        source_and_continuing
      );
      auto req_immed = else_task_.capture_options_.get_requested_immediate_permissions(
        source_and_continuing
      );
      if(req_immed > req_sched) { req_sched = req_immed; }
      return req_sched;
    }

    template <typename ThenElseTask>
    void
    do_capture_then_else_copy_for_then_else(
      AccessHandleBase& captured,
      ThenElseTask& then_else_task,
      std::false_type // ElseLambda is NOT void (or this is the ThenLambda version)
    ) {
      auto key = captured.var_handle_base_->get_key();

      // TODO handle interaction with aliasing

      std::shared_ptr<AccessHandleBase> source_handle = nullptr;
      auto found_explicit = explicit_if_captured_handles.find(key);
      if (found_explicit != explicit_if_captured_handles.end()) {
        source_handle = found_explicit->second;
        // Don't allow multiple captures
        explicit_if_captured_handles.erase(found_explicit);
      } else {
        auto found_implicit = implicit_if_captured_handles.find(key);
        if (found_implicit != implicit_if_captured_handles.end()) {
          source_handle = found_implicit->second;
          // Don't allow multiple captures
          implicit_if_captured_handles.erase(found_implicit);
        }
      }

      assert(source_handle.get() != nullptr);

      captured.call_make_captured_use_holder(
        captured.var_handle_base_,
        then_else_task.capture_options_.get_requested_scheduling_permissions(
          captured /* defaults to modify */
        ),
        then_else_task.capture_options_.get_requested_immediate_permissions(
          captured /* defaults to modify */
        ),
        *source_handle
      );

      captured.call_add_dependency(&then_else_task);

      // TODO keep this from even generating the continuation flow?!?

      // Release the handle, triggering release of the use
      source_handle = nullptr;

    }

    template <typename ThenElseTask>
    void
    do_capture_then_else_copy_for_then_else(
      AccessHandleBase& captured,
      ThenElseTask& then_else_task,
      std::true_type // ElseLambda is void
    ) {
      // Do nothing
    }

    // </editor-fold> end do_capture() and helpers }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="run() and helpers"> {{{2

    template <typename _SFINAE_only=void>
    std::enable_if_t<
      IfIsLambda and std::is_void<_SFINAE_only>::value,
      bool
    >
    _do_if_call(std::true_type) {
      return if_callable_();
    };

    template <std::size_t... Idxs>
    bool
    _do_if_call_functor_helper(
      std::integer_sequence<std::size_t, Idxs...>
    ) {
      using functor_traits_t = functor_call_traits<
        typename IfCallable::functor_t,
        std::tuple_element_t<Idxs, IfArgsTuple>...
      >;
      return typename IfCallable::functor_t{}(
        typename functor_traits_t::template call_arg_traits<Idxs>{}.get_converted_arg(
          std::get<Idxs>(std::move(if_args_))
        )...
      );
    }

    template <typename _SFINAE_only=void>
    std::enable_if_t<
      not IfIsLambda and std::is_void<_SFINAE_only>::value,
      bool
    >
    _do_if_call(std::false_type) {
      return _do_if_call_functor_helper(
        std::make_index_sequence<std::tuple_size<IfArgsTuple>::value>{}
      );
    };

    template <typename _SFINAE_only=void>
    std::enable_if_t<
      ThenIsLambda and std::is_void<_SFINAE_only>::value
    >
    _trigger_then_capture(std::true_type) {
      current_stage = ThenCopyForThen;
      is_double_copy_capture =
        false; // since source context may no longer be valid

      // Invoke the copy ctor
      ThenLambda then_lambda_tmp = then_task_.lambda_;

      // don't need clean up the uses to unmark already captured while the
      // source is still valid since all of those uses should be nullptr
      // but still clear the list
      uses_to_unmark_already_captured.clear();

      // Then move it back
      then_task_.lambda_.~ThenLambda();
      new(&then_task_.lambda_) ThenLambda(std::move(then_lambda_tmp));
    }

    template <typename _SFINAE_only=void>
    std::enable_if_t<
      not ThenIsLambda and std::is_void<_SFINAE_only>::value
    >
    _trigger_then_capture(std::false_type) {
      for(auto* cap_handle : delayed_then_capture_handles_) {
        do_capture_then_else_copy_for_then_else(
          *cap_handle,
          then_task_,
          std::false_type{} // then callable is not void
        );
      }
      delayed_then_capture_handles_.clear();
    }

    void run() override {
      if (_do_if_call(std::integral_constant<bool, IfIsLambda>{})) {

        auto* parent_task = static_cast<darma_runtime::detail::TaskBase*>(
          abstract::backend::get_backend_context()->get_running_task()
        );
        parent_task->current_create_work_context = this;

        _trigger_then_capture(std::integral_constant<bool, ThenIsLambda>{});

        parent_task->current_create_work_context = nullptr;

        {
          // Delete the if_lambda by moving it and deleting the move destination
          IfCallable if_lambda_delete_tmp = std::move(if_callable_);

          // Delete the else task by moving it...
          else_task_t else_task_delete_tmp = std::move(else_task_);
        }

        explicit_if_captured_handles.clear();
        implicit_if_captured_handles.clear();

        abstract::backend::get_backend_runtime()->register_task(
          std::make_unique<then_task_t>(std::move(then_task_))
        );
      } else {
        _run_else_part(typename std::is_void<ElseLambda>::type{});
      }
    }

    template <typename _SFINAE_only=void>
    std::enable_if_t<
      ElseIsLambda and std::is_void<_SFINAE_only>::value
    >
    _trigger_else_capture(std::true_type) {
      current_stage = ElseCopyForElse;
      is_double_copy_capture = false; // since source context may no longer be valid

      // Invoke the copy ctor
      auto else_lambda_tmp = _do_copy_else_lambda();

      // Then move it back
      _do_restore_else_lambda(std::move(else_lambda_tmp));
    }

    template <typename _SFINAE_only=void>
    std::enable_if_t<
      not ElseIsLambda
        and not std::is_void<ElseLambda>::value
        and std::is_void<_SFINAE_only>::value
    >
    _trigger_else_capture(std::false_type) {
      for(auto* cap_handle : delayed_else_capture_handles_) {
        do_capture_then_else_copy_for_then_else(
          *cap_handle,
          else_task_,
          std::false_type{} // else callable is not void
        );
      }
      delayed_else_capture_handles_.clear();
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

      _trigger_else_capture(std::integral_constant<bool, ElseIsLambda>{});

      parent_task->current_create_work_context = nullptr;

      {
        // Delete the if_lambda by moving it and deleting the move destination
        IfCallable if_lambda_delete_tmp = std::move(if_callable_);
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

    // </editor-fold> end run() and helpers }}}2
    //--------------------------------------------------------------------------

#if _darma_has_feature(task_migration)
    bool is_migratable() const override {
      return false; // TODO if/then/else task migration
    }
#endif


    template <typename, IfThenElseCaptureStage>
    friend struct WrappedFunctorHelper;
};

// </editor-fold> end IfLambdaThenLambdaTask }}}1
//==============================================================================


//==============================================================================
// <editor-fold desc="Task creation helpers in the form of proxy return objects"> {{{1

template <typename...>
struct _create_work_else_helper;

template <typename...>
struct _create_work_then_helper;

template <typename...>
struct _create_work_if_helper;

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_else_helper, lambda version"> {{{2

template <typename ThenHelper, typename Lambda, typename... Args>
struct _create_work_else_helper<
  ThenHelper, meta::nonesuch, tinympl::vector<Args...>, Lambda
> {
  public:

    _create_work_else_helper(
      ThenHelper&& then_helper,
      Args&& ... args,
      Lambda&& else_lambda_in
    ) : then_helper(std::move(then_helper)),
        task_details_args_(std::forward<Args>(args)...),
        else_lambda(std::move(else_lambda_in))
    { }

    using then_lambda_t = typename ThenHelper::then_lambda_t;
    using else_lambda_t = Lambda;
    using if_helper_t = typename ThenHelper::if_helper_t;

    ThenHelper then_helper;
    Lambda else_lambda;
    std::tuple<Args&&...> task_details_args_;
    std::tuple<> args_fwd_tup_;

    ~_create_work_else_helper() {
      std::unique_ptr<abstract::frontend::Task> if_then_else_task = std::make_unique<
        IfLambdaThenLambdaTask<
          typename if_helper_t::lambda_t,
          typename if_helper_t::args_tuple_t,
          if_helper_t::is_lambda_callable,
          then_lambda_t,
          typename ThenHelper::args_tuple_t,
          ThenHelper::is_lambda_callable,
          Lambda, std::tuple<>, true
        >
      >(
        std::move(*this)
      );
      abstract::backend::get_backend_runtime()->register_task(
        std::move(if_then_else_task)
      );
    }

};

// </editor-fold> end create_work_else_helper, lambda version }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_else_helper, functor version"> {{{2

template <typename ThenHelper, typename Functor, typename LastArg, typename... Args>
struct _create_work_else_helper<
  ThenHelper, Functor, tinympl::vector<Args...>, LastArg
> {
  public:

    _create_work_else_helper(
      ThenHelper&& then_helper_in,
      Args&& ... args,
      LastArg&& last_arg
    ) : then_helper(std::forward<ThenHelper>(then_helper_in)),
        args_fwd_tup_(
          std::forward<Args>(args)...,
          std::forward<LastArg>(last_arg)
        ),
        task_details_args_()
    { }

    using functor_call_traits_t = functor_call_traits<Functor, Args&&..., LastArg&&>;
    using args_tuple_t = typename functor_call_traits_t::args_tuple_t;
    using args_fwd_tuple_t = std::tuple<Args&&..., LastArg&&>;

    using then_lambda_t = typename ThenHelper::then_lambda_t;
    using if_helper_t = typename ThenHelper::if_helper_t;
    using else_lambda_t = WrappedFunctorHelper<Functor,
      IfThenElseCaptureStage::ElseCopyForIfAndElse
    >;

    ThenHelper then_helper;
    std::tuple<> task_details_args_;
    args_fwd_tuple_t args_fwd_tup_;

    ~_create_work_else_helper() {
      std::unique_ptr<abstract::frontend::Task> if_then_else_task = std::make_unique<
        IfLambdaThenLambdaTask<
          typename if_helper_t::lambda_t,
          typename if_helper_t::args_tuple_t,
          if_helper_t::is_lambda_callable,
          then_lambda_t,
          typename ThenHelper::args_tuple_t,
          ThenHelper::is_lambda_callable,
          else_lambda_t, args_tuple_t, false
        >
      >(
        std::move(*this)
      );
      abstract::backend::get_backend_runtime()->register_task(
        std::move(if_then_else_task)
      );
    }

};

// </editor-fold> end create_work_else_helper, functor version }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_then_helper, lambda version"> {{{2

template <typename IfHelper, typename Lambda, typename... Args>
struct _create_work_then_helper<
  IfHelper, meta::nonesuch, tinympl::vector<Args...>, Lambda
> {

  _create_work_then_helper(
    IfHelper&& if_helper_in,
    Args&& ... args,
    Lambda&& then_lambda_in
  ) : if_helper_(std::forward<IfHelper>(if_helper_in)),
      then_lambda(std::forward<Lambda>(then_lambda_in)),
      task_details_args_(std::forward<Args>(args)...),
      then_helper(*this)
  { }

  _create_work_then_helper(_create_work_then_helper&&) = default;
  _create_work_then_helper(_create_work_then_helper const&) = delete;

  _create_work_then_helper& then_helper;
  IfHelper if_helper_;
  Lambda then_lambda;
  std::tuple<Args&&...> task_details_args_;
  std::tuple<> args_fwd_tup_;
  bool else_invoked = false;

  static constexpr auto is_lambda_callable = true;

  using if_helper_t = IfHelper;
  using then_lambda_t = Lambda;
  using else_lambda_t = void;
  using task_details_args_t = std::tuple<Args&&...>;
  using args_tuple_t = std::tuple<>;
  using args_fwd_tuple_t = std::tuple<>;

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
      std::unique_ptr<abstract::frontend::Task> if_then_task = std::make_unique<
        IfLambdaThenLambdaTask<
          typename IfHelper::lambda_t,
          typename IfHelper::args_tuple_t, IfHelper::is_lambda_callable,
          Lambda, std::tuple<>, true
        >
      >(
        std::move(*this)
      );
      abstract::backend::get_backend_runtime()->register_task(
        std::move(if_then_task)
      );
    }
  }

};

// </editor-fold> end create_work_then_helper, lambda version }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_then_helper, functor version"> {{{2

template <typename IfHelper, typename Functor, typename LastArg, typename... Args>
struct _create_work_then_helper<
  IfHelper, Functor, tinympl::vector<Args...>, LastArg
> {

  using if_helper_t = IfHelper;
  using then_lambda_t = WrappedFunctorHelper<Functor,
    IfThenElseCaptureStage::ThenCopyForIfAndThen
  >;
  using functor_call_traits_t = functor_call_traits<Functor, Args&&..., LastArg&&>;
  using args_tuple_t = typename functor_call_traits_t::args_tuple_t;
  using args_fwd_tuple_t = std::tuple<Args&&..., LastArg&&>;
  using else_lambda_t = void;

  _create_work_then_helper& then_helper;
  IfHelper if_helper_;
  args_fwd_tuple_t args_fwd_tup_;
  using then_task_details_args_t = std::tuple<>;
  then_task_details_args_t task_details_args_; // for now

  bool else_invoked = false;

  static constexpr auto is_lambda_callable = false;

  _create_work_then_helper(
    IfHelper&& if_helper_in,
    Args&& ... args,
    LastArg&& last_arg
  ) : if_helper_(std::forward<IfHelper>(if_helper_in)),
      args_fwd_tup_(
        std::forward<Args>(args)...,
        std::forward<LastArg>(last_arg)
      ),
      task_details_args_(),
      then_helper(*this)
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
      std::unique_ptr<abstract::frontend::Task> if_then_task = std::make_unique<
        IfLambdaThenLambdaTask<
          typename IfHelper::lambda_t,
          typename IfHelper::args_tuple_t, IfHelper::is_lambda_callable,
          then_lambda_t, args_tuple_t, false
        >
      >(
        std::move(*this)
      );
      abstract::backend::get_backend_runtime()->register_task(
        std::move(if_then_task)
      );
    }
  }

};

// </editor-fold> end create_work_then_helper, functor version }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_if_helper, lambda version"> {{{2

template <typename Lambda, typename... Args>
struct _create_work_if_helper<
  meta::nonesuch,
  tinympl::vector<Args...>,
  Lambda
> {

  using lambda_t = Lambda;
  using args_tuple_t = std::tuple<>;
  static constexpr auto is_lambda_callable = true;

  Lambda func;
  std::tuple<> args_fwd_tup_;
  std::tuple<Args...> task_details_args_;

  _create_work_if_helper(
    Args&&... args,
    Lambda&& f
  ) : func(std::forward<Lambda>(f)),
      task_details_args_(std::forward<Args>(args)...)
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
// <editor-fold desc="create_work_if_helper, functor version"> {{{2

template <typename Functor, typename LastArg, typename... Args>
struct _create_work_if_helper<
  Functor,
  tinympl::vector<Args...>,
  LastArg
> {

  // TODO handle zero args case

  using lambda_t = WrappedFunctorHelper<Functor, IfThenElseCaptureStage::IfCopy>;
  using functor_call_traits_t = functor_call_traits<Functor, Args&&..., LastArg&&>;
  using args_tuple_t = typename functor_call_traits_t::args_tuple_t;
  static constexpr auto is_lambda_callable = false;

  Functor func;
  std::tuple<Args&&..., LastArg&&> args_fwd_tup_;
  std::tuple<> task_details_args_; // for now

  _create_work_if_helper(
    Args&&... args, LastArg&& last_arg
  ) : args_fwd_tup_(
        std::forward<Args>(args)...,
        std::forward<LastArg>(last_arg)
      )
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

// </editor-fold> end Task creation helpers in the form of proxy return objects }}}1
//==============================================================================


} // end namespace detail


//==============================================================================
// <editor-fold desc="The create_work_if function itself"> {{{1

namespace experimental {

template <typename Functor=meta::nonesuch, typename... Args>
auto
create_work_if(Args&& ... args) {
  return detail::_create_work_if_helper<
    Functor,
    typename tinympl::vector<Args...>::safe_pop_back::type,
    typename tinympl::vector<Args...>::template safe_back<meta::nonesuch>::type
  >(
    std::forward<Args>(args)...
  );
}

} // end namespace experimental

// </editor-fold> end The create_work_if function itself }}}1
//==============================================================================


} // end namespace darma_runtime

#endif //DARMA_CREATE_IF_THEN_H
