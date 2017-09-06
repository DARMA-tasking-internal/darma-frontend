/*
//@HEADER
// ************************************************************************
//
//                      create_work_functor.h
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

#ifndef DARMAFRONTEND_CREATE_WORK_FUNCTOR_H
#define DARMAFRONTEND_CREATE_WORK_FUNCTOR_H

#include <darma/interface/app/create_work.h>

#include <darma/impl/create_work/create_work_argument_parser.h>

#include <darma/impl/task/functor_task.h>

#include "record_line_numbers.h"

namespace darma_runtime {
namespace detail {

//==============================================================================
// <editor-fold desc="_create_work_impl, functor version">

template <typename Functor, typename LastArg, typename... Args>
struct _create_work_impl<Functor, tinympl::vector<Args...>, LastArg> {

#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
  _create_work_creation_context* ctxt;
  explicit
  _create_work_impl(_create_work_creation_context* ctxt) : ctxt(ctxt) { }
#endif

  template <typename... DeducedArgs>
  auto _impl(DeducedArgs&&... in_args) const {

    using _______________see_calling_context_on_next_line________________ = typename create_work_argument_parser::template static_assert_valid_invocation<DeducedArgs...>;

    auto* parent_task = darma_runtime::detail::get_running_task_impl();

    auto task = darma_runtime::detail::make_functor_task_ptr<Functor>(
      parent_task, parent_task,
      variadic_arguments_begin_tag{},
      std::forward<DeducedArgs>(in_args)...
    );

#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    task->set_context_information(
      ctxt->file, ctxt->line, ctxt->func
    );
#endif

    return abstract::backend::get_backend_runtime()->register_task(
      std::move(task)
    );

//    return setup_create_work_argument_parser()
//      .parse_args(std::forward<DeducedArgs>(in_args)...)
//      .invoke([this](
//        types::key_t name_key,
//        auto&& allow_aliasing_desc,
//        bool data_parallel,
//        variadic_arguments_begin_tag,
//        auto&&... args
//      ) {
//
//        //--------------------------------------------------------------------------
//        // Check that the arguments are serializable
//
//        // Don't wrap this line; it's on one line for compiler spew readability
//
//        //using runnable_t = FunctorRunnable<Functor, decltype(args)...>;
//        //using _______________see_calling_context_below________________ = typename runnable_t::template static_assert_all_args_serializable<>;
//
//        //--------------------------------------------------------------------------
//
//        auto* parent_task = darma_runtime::detail::get_running_task_impl();
//
//        auto task = std::make_unique<
//          darma_runtime::detail::FunctorTask<Functor, decltype(args)...>
//        >(
//          variadic_constructor_arg,
//          parent_task,
//          name_key,
//          std::forward<decltype(allow_aliasing_desc)>(allow_aliasing_desc),
//          data_parallel,
//          std::forward<decltype(args)>(args)...
//        );
//        // before the copy of the args that triggers the captures:
////        [&](darma_runtime::detail::TaskBase* task_base) {
////          task_base->set_name(name_key);
////          task_base->allowed_aliasing = std::move(allow_aliasing_desc);
////
////          parent_task->current_create_work_context = task_base;
////          task_base->propagate_parent_context(parent_task);
////
////          task_base->is_data_parallel_task_ = data_parallel;
////
////          // Make sure it's clear that this is not a double-copy capture
////          task_base->is_double_copy_capture = false;
////
////        },
////          std::forward<decltype(args)>(args)...
//
//#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
//        task->set_context_information(
//          ctxt->file, ctxt->line, ctxt->func
//        );
//#endif
//
//        //task->post_registration_cleanup();
//
//        // Done with capture; unset the current_create_work_context for safety later
//        //parent_task->current_create_work_context = nullptr;
//
//        return abstract::backend::get_backend_runtime()->register_task(
//          std::move(task)
//        );
//      });
  }

  decltype(auto) operator()(
    Args&&... args,
    LastArg&& last_arg
  ) const {
    return this->_impl(std::forward<Args>(args)..., std::forward<LastArg>(last_arg));
  }
};

// </editor-fold> end _create_work_impl, functor version
//==============================================================================


} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_CREATE_WORK_FUNCTOR_H
