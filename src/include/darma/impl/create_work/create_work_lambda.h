/*
//@HEADER
// ************************************************************************
//
//                      create_work_lambda.h
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

#ifndef DARMAFRONTEND_CREATE_WORK_LAMBDA_H
#define DARMAFRONTEND_CREATE_WORK_LAMBDA_H

#include <darma/interface/app/create_work.h>

#include <darma/impl/create_work/create_work_argument_parser.h>

#include <darma/impl/task/lambda_task.h>

namespace darma_runtime {
namespace detail {


template <typename Lambda, typename... Args>
struct _create_work_impl<detail::_create_work_uses_lambda_tag, tinympl::vector<Args...>, Lambda> {
  auto operator()(
    Args&&... in_args,
    Lambda&& lambda_to_be_copied
  ) const {
    using _______________see_calling_context_on_next_line________________ = typename create_work_argument_parser::template static_assert_valid_invocation<Args...>;

    return setup_create_work_argument_parser()
      .parse_args(std::forward<Args>(in_args)...)
      .invoke([&](
        darma_runtime::types::key_t name_key,
        auto&& allow_aliasing_desc,
        bool data_parallel,
        darma_runtime::detail::variadic_arguments_begin_tag,
        auto&&... _unused // GCC hates empty, unnamed variadic args for some reason
      ) {

        auto* parent_task = static_cast<darma_runtime::detail::TaskBase* const>(
          darma_runtime::abstract::backend::get_backend_context()->get_running_task()
        );

        auto task = std::make_unique<
          darma_runtime::detail::LambdaTask<std::decay_t<Lambda>>
        >(
          // before the copy of the lambda that triggers the captures:
          [&](darma_runtime::detail::TaskBase* task_base) {
            task_base->set_name(name_key);
            task_base->allowed_aliasing =
              std::forward<decltype(allow_aliasing_desc)>(allow_aliasing_desc);

            parent_task->current_create_work_context = task_base;
            task_base->propagate_parent_context(parent_task);

            task_base->is_data_parallel_task_ = data_parallel;

            task_base->is_double_copy_capture = true;
          },
          // Perfect forward the lambda here; the copy ctors will get invoked
          // in the constructor of LambdaTask (after the above stuff gets run)
          std::forward<Lambda>(lambda_to_be_copied)
        );

        // Intentionally don't do perfect forwarding here, to trigger the copy ctor hook
        // This should call the copy ctors of all of the captured variables, triggering
        // the logging of the AccessHandle copies as captures for the task
        //task->set_runnable(std::make_unique<darma_runtime::detail::Runnable<Lambda>>(lambda_to_be_copied));

        task->post_registration_cleanup();

        // Done with capture; unset the current_create_work_context for safety later
        // (do this after post_registration_cleanup() because it might need to know
        // what the current create_work context is)
        parent_task->current_create_work_context = nullptr;

        task->is_double_copy_capture = false;

        return darma_runtime::abstract::backend::get_backend_runtime()->register_task(
          std::move(task)
        );

      });
  }
};

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_CREATE_WORK_LAMBDA_H
