/*
//@HEADER
// ************************************************************************
//
//                          create_work.h
//                         darma_new
//              Copyright (C) 2016 Sandia Corporation
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

#ifndef SRC_CREATE_WORK_H_
#define SRC_CREATE_WORK_H_

#include <memory>


#include <tinympl/variadic/at.hpp>
#include <tinympl/variadic/back.hpp>
#include <tinympl/vector.hpp>
#include <tinympl/splat.hpp>
#include <tinympl/lambda.hpp>

#include <darma/impl/meta/tuple_for_each.h>
#include <darma/impl/meta/splat_tuple.h>

#include <darma/interface/app/create_work.h>

#include <darma/impl/handle_attorneys.h>
#include <darma/interface/app/access_handle.h>
#include <darma/impl/runtime.h>
#include <darma/impl/task.h>
#include <darma/impl/util.h>
#include <darma/impl/runnable/runnable.h>
#include <darma/impl/keyword_arguments/macros.h>

#include <darma/interface/app/keyword_arguments/name.h>

// TODO move this once it becomes part of the specified interface
DeclareDarmaTypeTransparentKeyword(task_creation, allow_aliasing);

namespace darma_runtime {

template <typename... Args>
decltype(auto)
schedule_only(Args&&... args) {
  auto make_schedule_only = [](auto const& arg) {
    // No need for perfect forwarding; the argument must be an lvalue
    using detail::create_work_attorneys::for_AccessHandle;
    for_AccessHandle::captured_as_info(arg) |=
      detail::AccessHandleBase::ScheduleOnly;
    // return a value that the compiler will ignore.  This allows us to mimic
    // C++17 fold expressions
    return meta::sentinal_type { };
  };
  std::forward_as_tuple( // return values ignored, but mimics a fold expression
    make_schedule_only(std::forward<Args>(args))...
  );

  // Return the (first) argument as a passthrough
  // TODO return a type that looks like a sensible compile-time error
  // (if more than one positional argument is given and the user tries to
  // use the return value for something like an argument to a functor create_work)
  return std::get<0>(std::forward_as_tuple(std::forward<Args>(args)...));
}


namespace detail {

namespace mv = tinympl::variadic;

template <typename... Args>
struct reads_decorator_parser {
  inline decltype(auto)
  operator()(Args&&... args) const {
    using detail::create_work_attorneys::for_AccessHandle;

    // NOTE: This is a post-0.2 feature
    // TODO we probably should remove ignore/only_if because they are unsafe
    bool ignored = false;
    //not get_typeless_kwarg_with_default_as<
    //    darma_runtime::keyword_tags_for_create_work_decorators::unless,
    //    bool
    //>(false, std::forward<Args>(args)...);
    //ignored = ignored && not get_typeless_kwarg_with_default_as<
    //  darma_runtime::keyword_tags_for_create_work_decorators::only_if,
    //  bool
    //>(true, std::forward<Args>(args)...);

    // Mark this usage as a read-only capture
    // TODO this should be just a splatted tuple.  Order doesn't matter
    meta::tuple_for_each(
      get_positional_arg_tuple(std::forward<Args>(args)...),
      [&](auto const& ah) {
        static_assert(is_access_handle<std::decay_t<decltype(ah)>>::value,
          "Non-AccessHandle<> argument passed to reads() decorator"
        );
        if(ignored) {
          for_AccessHandle::captured_as_info(ah) |= AccessHandleBase::Ignored;
        }
        else {
          for_AccessHandle::captured_as_info(ah) |= AccessHandleBase::ReadOnly;
        }
      }
    );

    // Return the (first) argument as a passthrough
    // TODO return a type that looks like a sensible compile-time error
    // (if more than one positional argument is given and the user tries to
    // use the return value for something like an argument to a functor create_work)
    return std::get<0>(std::forward_as_tuple(std::forward<Args>(args)...));
  }
};

template <typename...>
struct _create_work_impl;

using create_work_argument_parser = detail::kwarg_parser<
  variadic_positional_overload_description<
    _optional_keyword<
      converted_parameter, keyword_tags_for_task_creation::name
    >,
    _optional_keyword<
      converted_parameter, keyword_tags_for_task_creation::allow_aliasing
    >
  >
>;

inline auto setup_create_work_argument_parser() {
  return create_work_argument_parser()
    .with_converters(
      [](auto&& ... parts) {
        return darma_runtime::make_key(std::forward<decltype(parts)>(parts)...);
      },
      [](auto&&... aliasing_desc) {
        return std::make_unique<TaskBase::allowed_aliasing_description>(
          TaskBase::allowed_aliasing_description::allowed_aliasing_description_ctor_tag_t(),
          std::forward<decltype(aliasing_desc)>(aliasing_desc)...
        );
      }
    )
    .with_default_generators(
      keyword_arguments_for_task_creation::name = [] {
        // TODO this should actually return a "pending backend assignment" key
        return make_key();
      },
      keyword_arguments_for_task_creation::allow_aliasing = [] {
        return std::make_unique<TaskBase::allowed_aliasing_description>(
          TaskBase::allowed_aliasing_description::allowed_aliasing_description_ctor_tag_t(),
          false
        );
      }
    );
}

//==============================================================================
// <editor-fold desc="_create_work_impl, functor version">


template <typename Functor, typename LastArg, typename... Args>
struct _create_work_impl<Functor, tinympl::vector<Args...>, LastArg> {

  template <typename... DeducedArgs>
  auto _impl(DeducedArgs&&... in_args) const {

    using _______________see_calling_context_on_next_line________________ = typename create_work_argument_parser::template static_assert_valid_invocation<DeducedArgs...>;

    return setup_create_work_argument_parser()
      .parse_args(std::forward<DeducedArgs>(in_args)...)
      .invoke([](
        types::key_t name_key,
        auto&& allow_aliasing_desc,
        variadic_arguments_begin_tag,
        auto&&... args
      ) {

        using runnable_t = FunctorRunnable<Functor, decltype(args)...>;

        //--------------------------------------------------------------------------
        // Check that the arguments are serializable

        // Don't wrap this line; it's on one line for compiler spew readability
        using _______________see_calling_context_below________________ = typename runnable_t::template static_assert_all_args_serializable<>;

        //--------------------------------------------------------------------------

        auto task = std::make_unique<TaskBase>();

        task->set_name(name_key);
        task->allowed_aliasing = std::move(allow_aliasing_desc);

        auto* parent_task = static_cast<TaskBase* const>(
          abstract::backend::get_backend_context()->get_running_task()
        );
        parent_task->current_create_work_context = task.get();

        // Make sure it's clear that this is not a double-copy capture
        task->is_double_copy_capture = false;

        // The copy happens in here, which triggers the AccessHandle copy ctor hook
        task->set_runnable(
          std::make_unique<runnable_t>(
            variadic_constructor_arg,
            std::forward<decltype(args)>(args)...
          )
        );

        task->post_registration_cleanup();

        // Done with capture; unset the current_create_work_context for safety later
        parent_task->current_create_work_context = nullptr;

        return abstract::backend::get_backend_runtime()->register_task(
          std::move(task)
        );
      });
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

//==============================================================================
// <editor-fold desc="_create_work_impl, lambda version">

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
        types::key_t name_key,
        auto&& allow_aliasing_desc,
        variadic_arguments_begin_tag,
        auto&&... _unused /* unused */ // GCC hates empty, unnamed variadic args for some reason
      ) {

        auto task = std::make_unique<TaskBase>();
        task->set_name(name_key);
        task->allowed_aliasing = std::move(allow_aliasing_desc);

        auto* parent_task = static_cast<TaskBase* const>(
          abstract::backend::get_backend_context()->get_running_task()
        );
        parent_task->current_create_work_context = task.get();

        task->is_double_copy_capture = true;

        // Intentionally don't do perfect forwarding here, to trigger the copy ctor hook
        // This should call the copy ctors of all of the captured variables, triggering
        // the logging of the AccessHandle copies as captures for the task
        task->set_runnable(std::make_unique<Runnable<Lambda>>(lambda_to_be_copied));

        task->post_registration_cleanup();

        // Done with capture; unset the current_create_work_context for safety later
        parent_task->current_create_work_context = nullptr;
        task->is_double_copy_capture = false;

        return abstract::backend::get_backend_runtime()->register_task(
          std::move(task)
        );

      });
  }
};

// </editor-fold> end _create_work_impl, functor version
//==============================================================================





} // end namespace detail


template <
  typename Functor, /*=detail::_create_work_uses_lambda_tag */
  typename... Args
>
void create_work(Args&&... args) {

  return detail::_create_work_impl<
    Functor,
    typename tinympl::vector<Args...>::pop_back::type,
    typename tinympl::vector<Args...>::back::type
  >()(std::forward<Args>(args)...);

}

template <typename Functor>
void create_work() {
  return detail::_create_work_impl<
    Functor,
    typename tinympl::vector<>,
    meta::nonesuch
  >()._impl();
}

template <typename... Args>
decltype(auto)
reads(Args&&... args) {
  return detail::reads_decorator_parser<Args...>()(std::forward<Args>(args)...);
}

} // end namespace darma_runtime


#endif /* SRC_CREATE_WORK_H_ */
