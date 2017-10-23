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

#include <darma/impl/capture/functor_traits.h>

#include <darma/interface/app/create_work.h>
#include <darma/impl/capture.h>
#include <darma/impl/handle.h>
#include <darma/impl/create_work/create_work_fwd.h>

#include <darma/impl/create_work/create_if_then_fwd.h>
#include <darma/impl/create_work/create_if_then_lambda.h>
#include <darma/impl/create_work/create_if_then_functor.h>

// TODO Don't create a task (and generate a warning?) if there are no captures in the if clause

namespace darma_runtime {

namespace detail {

struct _not_a_lambda { };


struct IfThenElseCaptureManagerSetupHelper {

//  struct DeferredCapture {
//    AccessHandleBase const* source_and_continuing = nullptr;
//    AccessHandleBase* captured = nullptr;
//    int req_sched_perms = (int)frontend::Permissions::None;
//    int req_immed_perms = (int)frontend::Permissions::None;
//  };

  // We put these in a base class since they need to be constructed before other
  // members of WhileDoCaptureManager
  std::map<types::key_t, std::unique_ptr<CaptureDescriptionBase>> if_captures_;
  std::map<types::key_t, std::unique_ptr<CaptureDescriptionBase>> then_captures_;
  std::map<types::key_t, std::unique_ptr<CaptureDescriptionBase>> else_captures_;

  std::map<types::key_t, std::shared_ptr<detail::AccessHandleBase>> if_implicit_captures_;

};

template <
  typename IfCallableT, typename... IfArgsT, bool IfIsLambda,
  typename ThenCallableT, typename... ThenArgsT, bool ThenIsLambda,
  typename ElseCallableT, typename... ElseArgsT, bool ElseIsLambda,
  bool ElseGiven
>
struct IfThenElseCaptureManager<
  IfCallableT, std::tuple<IfArgsT...>, IfIsLambda,
  ThenCallableT, std::tuple<ThenArgsT...>, ThenIsLambda,
  ElseCallableT, std::tuple<ElseArgsT...>, ElseIsLambda,
  ElseGiven
> : public CaptureManager, public IfThenElseCaptureManagerSetupHelper
{
  private:

    using if_task_t = typename std::conditional_t<
      IfIsLambda,
      tinympl::lazy<IfLambdaTask>::template instantiated_with<
        IfCallableT, IfThenElseCaptureManager
      >,
      tinympl::lazy<IfFunctorTask>::template instantiated_with<
         IfThenElseCaptureManager, IfCallableT, IfArgsT...
      >
    >::type;

    using then_task_t = typename std::conditional_t<
      ThenIsLambda,
      tinympl::lazy<ThenElseLambdaTask>::template instantiated_with<
        ThenCallableT, std::true_type
      >,
      tinympl::lazy<ThenElseFunctorTask>::template instantiated_with<
        std::true_type, ThenCallableT, ThenArgsT...
      >
    >::type;

    using else_task_t = typename std::conditional_t<
      ElseGiven,
      std::conditional_t<
        ElseIsLambda,
        tinympl::lazy<ThenElseLambdaTask>::template instantiated_with<
          ElseCallableT, std::false_type
        >,
        tinympl::lazy<ThenElseFunctorTask>::template instantiated_with<
          std::false_type, ElseCallableT, ElseArgsT...
        >
      >,
      tinympl::identity<_not_a_lambda>
    >::type;


    std::unique_ptr<if_task_t> if_task_;
    std::unique_ptr<then_task_t> then_task_;
    std::unique_ptr<else_task_t> else_task_;

    enum struct CaptureMode { If, Then, Else, None };
    CaptureMode current_capturing_mode_ = CaptureMode::None;

    template <typename TaskPtrT>
    void _execute_captures(
      std::map<types::key_t, std::unique_ptr<CaptureDescriptionBase>>& captures,
      TaskPtrT& task
    ) {
      // See notes in WhileDoCaptureManager::execute_while_captures
      for(auto& pair : captures) {
        auto const& key = pair.first;
        auto& details = pair.second;

//        std::shared_ptr<detail::AccessHandleBase> implicit_source_and_cont;
//
//        // Sanity checks:
//        assert(details.source_and_continuing != nullptr);
//        assert(details.captured != nullptr);
//
//        details.captured->call_make_captured_use_holder(
//          details.captured->var_handle_base_,
//          (HandleUse::permissions_t)details.req_sched_perms,
//          (HandleUse::permissions_t)details.req_immed_perms,
//          *details.source_and_continuing,
//          true // register all continuation uses for now...
//        );
//
//        details.captured->call_add_dependency(task.get());

        details->invoke_capture(task.get());

      }
    }

  public:

    IfThenElseCaptureManager(IfThenElseCaptureManager const&) = delete;
    IfThenElseCaptureManager(IfThenElseCaptureManager&&) = delete;

    template <typename HelperT>
    explicit
    IfThenElseCaptureManager(
      variadic_constructor_tag_t,
      HelperT&& helper,
      std::enable_if_t<
        std::decay_t<HelperT>::is_else_helper and ElseGiven,
        _not_a_type
      > = { }
    ) : if_task_(std::make_unique<if_task_t>(
          std::forward<HelperT>(helper), this
        )),
        then_task_(std::make_unique<then_task_t>(
          std::move(helper.then_helper), this
        )),
        else_task_(std::make_unique<else_task_t>(
          std::forward<HelperT>(helper), this
        ))
    { }

    template <typename HelperT>
    IfThenElseCaptureManager(
      variadic_constructor_tag_t,
      HelperT&& helper,
      std::enable_if_t<
        not std::decay_t<HelperT>::is_else_helper and not ElseGiven,
        _not_a_type
      > = { }
    ) : if_task_(std::make_unique<if_task_t>(
          std::forward<HelperT>(helper), this
        )),
        then_task_(std::make_unique<then_task_t>(
          std::move(helper.then_helper), this
        )),
        else_task_(nullptr)
    { }

    void finish_construction_and_register_if_task(
      std::shared_ptr<IfThenElseCaptureManager> const& shared_ptr_to_this
    ) {
      if_task_->capture_manager_ = shared_ptr_to_this;
      _execute_captures(if_captures_, if_task_);
      abstract::backend::get_backend_runtime()->register_task(std::move(if_task_));
    }


    IfThenElseCaptureManager* in_if_mode() {
      current_capturing_mode_ = CaptureMode::If;
      return this;
    }

    IfThenElseCaptureManager* in_then_mode() {
      current_capturing_mode_ = CaptureMode::Then;
      return this;
    }

    IfThenElseCaptureManager* in_else_mode() {
      current_capturing_mode_ = CaptureMode::Else;
      return this;
    }

    auto&
    get_if_task() {
      return if_task_;
    }

    void register_then_task() {
      _execute_captures(then_captures_, then_task_);
      abstract::backend::get_backend_runtime()->register_task(
        std::move(then_task_)
      );
    }

    template <typename _SFINAE_only=void>
    void
    register_else_task(
      std::enable_if_t<
        std::is_void<_SFINAE_only>::value // always true
          and ElseGiven,
        _not_a_type
      > = { }
    ) {
      _execute_captures(else_captures_, else_task_);
      abstract::backend::get_backend_runtime()->register_task(
        std::move(else_task_)
      );
    }

    template <typename _SFINAE_only=void>
    void
    register_else_task(
      std::enable_if_t<
      std::is_void<_SFINAE_only>::value // always true
        and not ElseGiven,
        _not_a_type
      > = { }
    ) {
      // do nothing, since no else block was given
    }


    void do_capture(
      AccessHandleBase& captured,
      AccessHandleBase const& source_and_continuing
    ) override {

      /* TODO we should probably make copies (or something) of handles explicitly
       * captured in the while block so that they're still capturable in the
       * do block even if the user calls release() on the handle
       */
      // (or we could just prohibit calls to release in the while block...)

      // Since we're doing a deferred capture (rather than replacing the use
      // right away) we always need to make sure that the captured handle
      // isn't holding a use copied from the outer scope.  It's safe to do here,
      // even with the aliasing check flag, since that flag goes on the source
      // rather than the captured handle.
      captured.release_current_use();

      auto const& key = captured.var_handle_base_->get_key();
      auto& if_details = if_captures_[key];

      if(current_capturing_mode_ == CaptureMode::If) {

//        auto initial_permissions =
//          AccessHandleBaseAttorney::get_permissions_before_downgrades(
//            source_and_continuing,
//            // hard-coded for now; could eventually get them from the task, though
//            AccessHandleBase::read_only_capture,
//            AccessHandleBase::read_only_capture
//          );
//
//        auto permissions_pair =
//          CapturedObjectAttorney::get_and_clear_requested_capture_permissions(
//            source_and_continuing,
//            initial_permissions.scheduling,
//            initial_permissions.immediate
//          );
//        if_details.req_sched_perms = permissions_pair.scheduling;
//        if_details.req_immed_perms = permissions_pair.immediate;
//
//        if_details.captured = &captured;
//        if_details.source_and_continuing = &source_and_continuing;

        if_details = CapturedObjectAttorney::get_capture_description(
          source_and_continuing,
          captured,
          // hard-coded for now; could eventually get them from the task, though
          AccessHandleBase::read_only_capture,
          AccessHandleBase::read_only_capture
        );
      }
      else {
        std::unique_ptr<CaptureDescriptionBase>* details = nullptr;
        if(current_capturing_mode_ == CaptureMode::Then) {
          details = &then_captures_[key];
        }
        else {
          assert(current_capturing_mode_ == CaptureMode::Else);
          details = &else_captures_[key];
        }

//        auto initial_permissions =
//          AccessHandleBaseAttorney::get_permissions_before_downgrades(
//            source_and_continuing,
//            // hard-coded for now; could eventually get them from the task, though
//            AccessHandleBase::modify_capture,
//            AccessHandleBase::modify_capture
//          );
//
//        auto permissions_pair =
//          CapturedObjectAttorney::get_and_clear_requested_capture_permissions(
//            source_and_continuing,
//            initial_permissions.scheduling,
//            initial_permissions.immediate
//          );
//
//        details->req_sched_perms = permissions_pair.scheduling;
//        details->req_immed_perms = permissions_pair.immediate;

        (*details) = CapturedObjectAttorney::get_capture_description(
          source_and_continuing,
          captured,
          // hard-coded for now; could eventually get them from the task, though
          AccessHandleBase::modify_capture,
          AccessHandleBase::modify_capture
        );

        if(if_details.get() == nullptr) {
          auto& if_implicit_capture = if_implicit_captures_[key];
          if_implicit_capture = source_and_continuing.copy();

          if_details = CapturedObjectAttorney::get_capture_description(
            source_and_continuing,
            *if_implicit_capture,
            // TODO @bug proper downgrades on the modify_capture here (THIS IS A BUG)
            AccessHandleBase::modify_capture,
            AccessHandleBase::no_capture
          );

          (*details)->replace_source_pointer(if_implicit_capture.get());
        }
        else {
          (*details)->replace_source_pointer(
            if_details->get_captured_pointer()
          );

          if_details->require_ability_to_schedule(*details->get());
        }

        // The if has to be able to both schedule the then/else *and* schedule
        // anything that the then/else schedules (because scheduling permissions
        // must be propatated outwards
//        if_details.req_sched_perms |= details->req_immed_perms;
//        if_details.req_sched_perms |= details->req_sched_perms;
//
//        // handle implicit capture in the if:
//        if(if_details.source_and_continuing == nullptr) {
//          if_details.source_and_continuing = &source_and_continuing;
//
//          auto& if_implicit_capture = if_implicit_captures_[key];
//          if_implicit_capture = source_and_continuing.copy();
//          if_details.captured = if_implicit_capture.get();
//        }
//
//        details->source_and_continuing = if_details.captured;
//        details->captured = &captured;
      }

    }

};

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_CREATE_IF_THEN_H
