/*
//@HEADER
// ************************************************************************
//
//                      create_work_while.h
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

#ifndef DARMA_IMPL_CREATE_WORK_WHILE_H
#define DARMA_IMPL_CREATE_WORK_WHILE_H

#include <tuple>
#include <type_traits>
#include <utility> // std::forward

#include <tinympl/vector.hpp>

#include <darma/impl/meta/detection.h>

#include <darma/impl/access_handle_base.h>

#include <darma/utility/not_a_type.h>

#include <darma/utility/config.h>

#include <darma/impl/create_work/create_work_while_fwd.h>
#include <darma/impl/task/lambda_task.h>
#include <darma/impl/task/functor_task.h>

#include <darma/impl/create_work/create_work_while_lambda.h>
#include <darma/impl/create_work/create_work_while_functor.h>

#include <darma/impl/create_work/record_line_numbers.h>

// TODO Propagate task options and permissions downgrades

namespace darma_runtime {

namespace detail {

struct WhileDoCaptureManagerSetupHelper {

  /* TODO we might be able to stick some extra fields on AccessHandleBase rather
   * than using maps here
   */
  // We put these in a base class since they need to be constructed before other
  // members of WhileDoCaptureManager
  std::map<types::key_t, std::unique_ptr<CaptureDescriptionBase>> while_captures_;
  std::map<types::key_t, std::unique_ptr<CaptureDescriptionBase>> do_captures_;

  std::map<types::key_t, std::shared_ptr<detail::AccessHandleBase>> while_implicit_captures_;

  enum struct WhileDoCaptureMode { While, Do, None };
  WhileDoCaptureMode current_capturing_task_mode_ = WhileDoCaptureMode::None;
  bool current_capture_is_nested = false;

};


template <
  typename WhileCallable, typename... WhileArgs, bool WhileIsLambda,
  typename DoCallable, typename... DoArgs, bool DoIsLambda
>
struct WhileDoCaptureManager<
  WhileCallable, std::tuple<WhileArgs...>, WhileIsLambda,
  DoCallable, std::tuple<DoArgs...>, DoIsLambda
> : public CaptureManager, public WhileDoCaptureManagerSetupHelper
{

  //----------------------------------------------------------------------------
  // <editor-fold desc="type aliases"> {{{2

  using while_task_t = typename std::conditional<
    WhileIsLambda,
    tinympl::lazy<WhileLambdaTask>::template instantiated_with<WhileCallable, WhileDoCaptureManager>,
    tinympl::lazy<WhileFunctorTask>::template instantiated_with<WhileDoCaptureManager, WhileCallable, WhileArgs...>
  >::type::type;

  using do_task_t = typename std::conditional<
    DoIsLambda,
    tinympl::lazy<DoLambdaTask>::template instantiated_with<DoCallable, WhileDoCaptureManager>,
    tinympl::lazy<DoFunctorTask>::template instantiated_with<WhileDoCaptureManager, DoCallable, DoArgs...>
  >::type::type;

  // </editor-fold> end type aliases }}}2
  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------
  // <editor-fold desc="data members"> {{{2

  std::unique_ptr<while_task_t> while_task_;
  std::unique_ptr<do_task_t> do_task_;

  // </editor-fold> end data members }}}2
  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------
  // <editor-fold desc="Constructor and helpers"> {{{2

  WhileDoCaptureManager(WhileDoCaptureManager const&) = delete;
  WhileDoCaptureManager(WhileDoCaptureManager&&) = delete;

  template <typename HelperT>
  WhileDoCaptureManager(
    variadic_constructor_tag_t /*unused*/,
    HelperT&& helper
  ) : while_task_(std::make_unique<while_task_t>(
        this, std::forward<HelperT>(helper),
        std::make_index_sequence<std::tuple_size<
          typename std::decay_t<HelperT>::while_helper_t::task_option_args_tuple_t
        >::value>{}
      )),
      do_task_(std::make_unique<do_task_t>(
        this, std::forward<HelperT>(helper),
        std::make_index_sequence<std::tuple_size<
          typename std::decay_t<HelperT>::task_option_args_tuple_t
        >::value>{}
      ))
  {
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    while_task_->set_context_information(
      helper.while_helper.context_->file,
      helper.while_helper.context_->line,
      helper.while_helper.context_->func
    );
    do_task_->set_context_information(
      helper.while_helper.context_->file,
      helper.while_helper.context_->line,
      helper.while_helper.context_->func
    );
#endif
  }

  // essentially done during construction, but because shared_ptr construction
  // needs to complete before we start passing around the shared pointer,
  // we need to invoke it separately from the constructor
  void set_capture_managers(
    std::shared_ptr<WhileDoCaptureManager> const& this_as_shared
  ) {
    while_task_->capture_manager_ = this_as_shared;
    do_task_->capture_manager_ = this_as_shared;
  }

  // also essentially done during construction; see note for
  // set_capture_managers()
  void register_while_task() {
    execute_while_captures(true);
    abstract::backend::get_backend_runtime()->register_task(
      std::move(while_task_)
    );
  }

  // </editor-fold> end Constructor and helpers }}}2
  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------
  // <editor-fold desc="do_capture() implementation and helpers"> {{{2

  void recapture(
    while_task_t& while_task,
    do_task_t& do_task
  ) {
    // whenever recapture is being invoked, we must be in a nested context:
    current_capture_is_nested = true;

    // Note: recapture should *not* move any of the access handles captured
    // in while_task or do_task

    while_task_ = while_task_t::recapture(while_task);
    do_task_ = do_task_t::recapture(do_task);
  }

  void execute_while_captures(bool is_outermost) {
    for(auto& pair : while_captures_) {
      auto const& key = pair.first;
      auto& while_details = pair.second;

      std::shared_ptr<detail::AccessHandleBase> implicit_source_and_cont;

      auto found = while_implicit_captures_.find(key);
      if(found != while_implicit_captures_.end()) {
        // Make a new implicitly captured object for the inner while to use

        // Keep the old one around until after the capture, though
        implicit_source_and_cont = found->second;
        while_details->replace_source_pointer(implicit_source_and_cont.get());


        found->second = utility::safe_static_cast<AccessHandleBase*>(
          while_details->get_captured_pointer()
        )->copy();
        while_details->replace_captured_pointer(found->second.get());

        // at this point, we know that since the do is nested in the while (and
        // since this is an implicit capture, we know the while block won't
        // be making any changes), the source for the do capture with the same
        // key will be the handle implicilty captured here in the while
        auto& do_details = do_captures_[key];
        do_details->replace_source_pointer(found->second.get());
      }

//      if(while_details.needs_implicit_capture) {
//        auto found = while_implicit_captures_.find(key);
//        assert(found != while_implicit_captures_.end());
//
//        // copy the shared pointer from the outer implicit capture to the local
//        // temporary that we will use as the source and continuing for the capture
//        implicit_source_and_cont = found->second;
//        while_details.source_and_continuing = implicit_source_and_cont.get();
//
//        // set the current implicitly captured handle to a copy of the source
//        // and continuing handle (essentially the same thing that happens when
//        // a copy of a callable is made for explicit capture)
//        found->second = implicit_source_and_cont->copy(false);
//        while_details.captured = while_implicit_captures_[key].get();
//
//        // at this point, we know that since the do is nested in the while (and
//        // since this is an implicit capture, we know the while block won't
//        // be making any changes), the source for the do capture with the same
//        // key will be the handle implicilty captured here in the while
//        auto& do_details = do_captures_[key];
//        do_details.source_and_continuing = while_details.captured;
//      }

      // Sanity checks:
//      assert(while_details.source_and_continuing != nullptr);
//      assert(while_details.captured != nullptr);

      // Note that this function must be executed:
      //   - before the return of create_work_while(...).do_(...) if is_outermost
      //     is true (since otherwise source_and_continuing could be a stale pointer)
      //   - before the parent while task's run() function returns if not
      //     is_outermost, since source_and_continuing should be owned by the
      //     parent while block (usually in the form of a continuation use from
      //     the do block, unless the do block doesn't capture the source handle)
      //

      while_details->invoke_capture(while_task_.get());
//      while_details.captured->call_make_captured_use_holder(
//        while_details.captured->var_handle_base_,
//        (HandleUse::permissions_t)while_details.req_sched_perms,
//        (HandleUse::permissions_t)while_details.req_immed_perms,
//        *while_details.source_and_continuing,
//        true // register all continuation uses for now...
//        //is_outermost // only register continuation uses if we're in the outermost context
//      );
//
//      while_details.captured->call_add_dependency(while_task_.get());

      // We can now release implicit_source_and_cont (automatically release at the end
      // of this scope) which holds the source and continuing for the while
      // capture.  At this point, it represents the continuation of the
      // tail recursion for the while, which by definition must be empty and
      // do nothing.
      // Eventually, we could make it so that neither the captured use
      // nor the continuation use gets registered in call_make_captured_use_holder,
      // but instead the alias operation implied by releasing the continuation
      // could be done in-place here (i.e., the out flow of the captured use
      // could be set to the out flow of the continuation use and only the
      // captured use would need to be registered)
    }

    // Note that we do *not* clear the captures here; instead, we'll just update
    // the source_and_continuing and captured pointers in the do_capture()
    // method the next time we recapture.  The permissions remain the same,
    // so we only need to figure those out the first time around and leave them
    // in the map
  }

  void execute_do_captures() {
    for(auto& pair : do_captures_) {
      auto const& key = pair.first;
      auto& do_details = pair.second;

      do_details->invoke_capture(do_task_.get());

      // Sanity checks:
//      assert(do_details.source_and_continuing != nullptr);
//      assert(do_details.captured != nullptr);

      // Note that this function must be executed before the parent while task's
      // run() function returns, since source_and_continuing should be owned by
      // the parent while block.

//      do_details.captured->call_make_captured_use_holder(
//        do_details.captured->var_handle_base_,
//        (HandleUse::permissions_t)do_details.req_sched_perms,
//        (HandleUse::permissions_t)do_details.req_immed_perms,
//        *do_details.source_and_continuing,
//        true // yes, we do want to register continuation uses here, since the
//             // while always uses them
//      );


//      do_details.captured->call_add_dependency(do_task_.get());
    }

    // Note that we do *not* clear the captures here; see note at the end of
    // execute_while_captures()
  }

  void post_capture_cleanup() override {
    // TODO technically unnecessary, so probably should only do it in debug mode
    current_capturing_task_mode_ = WhileDoCaptureMode::None;

    CaptureManager::post_capture_cleanup();
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

    auto const& key = captured.var_handle_base_->get_key();
    auto& while_details = while_captures_[key];

    // Since we're doing a deferred capture (rather than replacing the use
    // right away) we always need to make sure that the captured handle
    // isn't holding a use copied from the outer scope.  It's safe to do here,
    // even with the aliasing check flag, since that flag goes on the source
    // rather than the captured handle.
    captured.release_current_use();

    if(not current_capture_is_nested) {
      // First time through; need to establish the pattern that will
      // be repeated every iteration

      if(current_capturing_task_mode_ == WhileDoCaptureMode::While) {

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
//
//        while_details.req_sched_perms = permissions_pair.scheduling;
//        while_details.req_immed_perms = permissions_pair.immediate;

//        // this time through, it's safe to set these directly
//        while_details.captured = &captured;
//        while_details.source_and_continuing = &source_and_continuing;

        while_details = CapturedObjectAttorney::get_capture_description(
          source_and_continuing,
          captured,
          // hard-coded for now; could eventually get them from the task, though
          AccessHandleBase::read_only_capture,
          AccessHandleBase::read_only_capture
        );

      }
      else {
        assert(current_capturing_task_mode_ == WhileDoCaptureMode::Do);

        auto& do_details = do_captures_[key];

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
//        do_details.req_sched_perms = permissions_pair.scheduling;
//        do_details.req_immed_perms = permissions_pair.immediate;
//
//        // need to be able to schedule the do block and anything it schedules
//        while_details.req_sched_perms |= do_details.req_immed_perms;
//        while_details.req_sched_perms |= do_details.req_sched_perms;

        do_details = CapturedObjectAttorney::get_capture_description(
          source_and_continuing,
          captured,
          // hard-coded for now; could eventually get them from the task, though
          AccessHandleBase::modify_capture,
          AccessHandleBase::modify_capture
        );

        if(while_details.get() == nullptr) {

          auto& while_implicit_capture = while_implicit_captures_[key];
          while_implicit_capture = source_and_continuing.copy();

          while_details = CapturedObjectAttorney::get_capture_description(
            source_and_continuing,
            *while_implicit_capture.get(),
            AccessHandleBase::modify_capture,
            AccessHandleBase::no_capture
          );

          do_details->replace_source_pointer(while_implicit_capture.get());
        }
        else {

          do_details->replace_source_pointer(
            while_details->get_captured_pointer()
          );

          while_details->require_ability_to_schedule(*do_details.get());
        }

//        // setup details for implicit capture...
//        if(while_details.source_and_continuing == nullptr) {
//          while_details.source_and_continuing = &source_and_continuing;
//
//          // do an implicit capture
//          while_details.needs_implicit_capture = true;
//
//          auto& while_implicit_capture = while_implicit_captures_[key];
//          while_implicit_capture = source_and_continuing.copy(false);
//
//          do_details.source_and_continuing = while_implicit_capture.get();
//          while_details.captured = while_implicit_capture.get();
//        }

//        do_details.source_and_continuing = while_details.captured;
//        do_details.captured = &captured;
      }

    }
    else {

      // All of the permissions of the captures should be populated, we just
      // need to set up the source_and_continuing and the captured variables

      if(current_capturing_task_mode_ == WhileDoCaptureMode::While) {

        // The source should be the continuation of the do task (if it was
        // captured in the do task; if not, the source is just the handle
        // captured in the parent while task)
        auto found = do_captures_.find(key);
        if(found != do_captures_.end()) {
          auto& do_details = found->second;
          while_details->replace_source_pointer(do_details->get_source_pointer());
        }
        else {
          while_details->replace_source_pointer(while_details->get_captured_pointer());
        }

        // now replace the captured variable with the one from the arguments
        while_details->replace_captured_pointer(&captured);
      }
      else {
        // sanity check
        assert(current_capturing_task_mode_ == WhileDoCaptureMode::Do);

        auto& do_details = do_captures_[key];


        // The source of an inner do will always be captured in its parent while
        do_details->replace_source_pointer(while_details->get_captured_pointer());

        // replace the captured variable with the one from the arguments
        do_details->replace_captured_pointer(&captured);
      }

    }

  }

  // </editor-fold> end do_capture() implementation and helpers }}}2
  //----------------------------------------------------------------------------


  //----------------------------------------------------------------------------
  // <editor-fold desc="passthrough helpers for setting mode during task construction"> {{{2

  WhileDoCaptureManager* in_while_mode() {
    assert(current_capturing_task_mode_ == WhileDoCaptureMode::None);
    current_capturing_task_mode_ = WhileDoCaptureMode::While;
    return this;
  }

  WhileDoCaptureManager* in_do_mode() {
    assert(current_capturing_task_mode_ == WhileDoCaptureMode::None);
    current_capturing_task_mode_ = WhileDoCaptureMode::Do;
    return this;
  }

  // </editor-fold> end passthrough helpers for setting mode during task construction }}}2
  //----------------------------------------------------------------------------

};

// </editor-fold> end WhileDoTask }}}1
//==============================================================================

} // end namespace detail

#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
#endif

} // end namespace darma_runtime

#endif //DARMA_IMPL_CREATE_WORK_WHILE_H
