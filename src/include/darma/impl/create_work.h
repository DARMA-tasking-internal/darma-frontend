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

#include <darma/impl/handle_attorneys.h>
#include <darma/interface/app/access_handle.h>
#include <darma/impl/runtime.h>
#include <darma/impl/task.h>
#include <darma/impl/util.h>
#include <darma/impl/keyword_arguments/macros.h>

#include <darma/interface/app/keyword_arguments/unless.h>
#include <darma/interface/app/keyword_arguments/only_if.h>
#include <darma/interface/app/keyword_arguments/name.h>

// TODO move this once it becomes part of the specified interface
DeclareDarmaTypeTransparentKeyword(task_creation, allow_aliasing);

namespace darma_runtime {


namespace detail {

namespace mv = tinympl::variadic;

template <typename... Args>
struct reads_decorator_parser {
  inline decltype(auto)
  operator()(Args&&... args) const {
    using detail::create_work_attorneys::for_AccessHandle;

    // NOTE: This is a post-0.2 feature
    bool ignored = not get_typeless_kwarg_with_default_as<
        darma_runtime::keyword_tags_for_create_work_decorators::unless,
        bool
    >(false, std::forward<Args>(args)...);
    ignored = ignored && not get_typeless_kwarg_with_default_as<
      darma_runtime::keyword_tags_for_create_work_decorators::only_if,
      bool
    >(true, std::forward<Args>(args)...);

    // Mark this usage as a read-only capture
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

    // Return the argument as a passthrough
    // TODO the outer std::forward should never need to be there (right?)
    return std::forward<mv::at_t<0, Args...>>(
      std::get<0>(std::forward_as_tuple(std::forward<Args>(args)...))
    );
  }
};



//==============================================================================
// <editor-fold desc="_start_create_work">

inline types::unique_ptr_template<TaskBase>
_start_create_work() {
  auto rv = std::make_unique<TaskBase>();
  detail::TaskBase* parent_task = static_cast<detail::TaskBase* const>(
    abstract::backend::get_backend_context()->get_running_task()
  );
  parent_task->current_create_work_context = rv.get();
  return std::move(rv);
}

// </editor-fold> end _start_create_work
//==============================================================================


//==============================================================================
// <editor-fold desc="_do_create_work_impl, functor version">

template <typename Functor>
struct _do_create_work_impl {

  template <typename... Args>
  inline auto operator()(
    types::unique_ptr_template<TaskBase>&& task_base,
    Args&& ... args
  ) {

    using runnable_t = FunctorRunnable<Functor, Args...>;

    //--------------------------------------------------------------------------
    // Check that the arguments are serializable

    // Don't wrap this line; it's on one line for compiler spew readability
    using _______________see_calling_context_below________________ = typename runnable_t::template static_assert_all_args_serializable<>;

    //--------------------------------------------------------------------------

    task_base->set_runnable(
      std::make_unique<runnable_t>(
        variadic_constructor_arg,
        std::forward<Args>(args)...
      )
    );
    detail::TaskBase* parent_task = static_cast<detail::TaskBase* const>(
      abstract::backend::get_backend_context()->get_running_task()
    );
    parent_task->current_create_work_context = nullptr;

    for (auto&& reg : task_base->registrations_to_run
      ) {
      reg();
    }
    task_base->registrations_to_run.clear();

    for (auto&& post_reg_op : task_base->post_registration_ops
      ) {
      post_reg_op();
    }
    task_base->post_registration_ops.clear();

    return abstract::backend::get_backend_runtime()->register_task(
      std::move(task_base)
    );

  }
};

// </editor-fold> end _do_create_work_impl, functor version
//==============================================================================

//==============================================================================
// <editor-fold desc="_do_create_work_impl, lambda version">

template <>
struct _do_create_work_impl<void> {

  template <typename Lambda, typename... Args>
  struct _reorder_template_args_helper {
    inline auto operator()(
      std::unique_ptr<TaskBase>&& task_base,
      Args&&... args, Lambda&& lambda
    ) const {
      task_base->set_runnable(
        std::make_unique<Runnable<Lambda>>(std::forward<Lambda>(lambda))
      );
      return abstract::backend::get_backend_runtime()->register_task(
        std::move(task_base)
      );
    }
  };

  template <typename... Args>
  inline auto
  operator()(
    types::unique_ptr_template<TaskBase>&& task,
    Args&&... args
  ) const {

    // At this point, the copy constructors of the captured handles have been
    // invoked (by the [=] itself), so we can reset the current_create_work_contenxt
    // so as to not confuse any future copy constructor invocations
    detail::TaskBase* parent_task = static_cast<detail::TaskBase* const>(
      abstract::backend::get_backend_context()->get_running_task()
    );
    parent_task->current_create_work_context = nullptr;

    // Now we need to run all of the registrations that were created during capture
    for(auto&& reg : task->registrations_to_run) {
      reg();
    }
    task->registrations_to_run.clear();

    // ... and the post registration ops that were created during capture
    // (might not be needed any more?!?
    for(auto&& post_reg_op : task->post_registration_ops) {
      post_reg_op();
    }
    task->post_registration_ops.clear();

    // call the helper with the template parameters reordered
    // this does the actual task->set_callable and registration
    namespace m = tinympl;
    // Pop off the last type and move it to the front
    typedef typename m::vector<Args...>::back::type lambda_t;
    typedef typename m::vector<Args...>::pop_back::type rest_vector_t;
    typedef typename m::splat_to<
      typename rest_vector_t::template push_front<lambda_t>::type,
      _reorder_template_args_helper
    >::type helper_t;
    return helper_t()(std::forward<types::unique_ptr_template<TaskBase>>(task),
      std::forward<Args>(args)...
    );
  }

};

// </editor-fold> end _do_create_work_impl, lambda version
//==============================================================================

struct _do_create_work {



  explicit
  _do_create_work(types::unique_ptr_template<TaskBase>&& tsk_base)
    : task_(std::move(tsk_base))
  { }

  template <typename Functor=void, typename... Args>
  inline void
  operator()(Args&&... args) {

    //--------------------------------------------------------------------------
    // Check for allowed keywords
    using _check_kwargs_asserting_t = typename detail::only_allowed_kwargs_given<
      darma_runtime::keyword_tags_for_task_creation::name,
      darma_runtime::keyword_tags_for_task_creation::allow_aliasing
    >::template static_assert_correct<Args...>::type;

    //--------------------------------------------------------------------------
    // Handle the name kwarg

    auto name_key = get_typeless_kwarg_with_converter_and_default<
      darma_runtime::keyword_tags_for_task_creation::name
    >([](auto&&... key_parts){
      return make_key(std::forward<decltype(key_parts)>(key_parts)...);
    }, types::key_t(), std::forward<Args>(args)...);

    if(not detail::key_traits<types::key_t>::key_equal()(name_key, types::key_t())) {
      task_->set_name(name_key);
    }

    bool found_aliasing_description = get_typeless_kwarg_with_converter_and_default<
      darma_runtime::keyword_tags_for_task_creation::allow_aliasing
    >([&](auto&&... args) {
        // forward to the allowed_aliasing_description constructor
        task_->allowed_aliasing = std::make_unique<TaskBase::allowed_aliasing_description>(
          TaskBase::allowed_aliasing_description::allowed_aliasing_description_ctor_tag_t(),
          std::forward<decltype(args)>(args)...
        );
        return true;
      }, false /* return value is ignored anyway */, std::forward<Args>(args)...
    );

    //--------------------------------------------------------------------------
    // Handle the allow_aliasing keyword argument

    //--------------------------------------------------------------------------
    // forward to the appropriate specialization (Lambda or functor)

    meta::splat_tuple(
      get_positional_arg_tuple(std::forward<Args>(args)...),
      [&](auto&&... pos_args) {
        _do_create_work_impl<Functor>()(
          std::move(task_),
          std::forward<decltype(pos_args)>(pos_args)...
        );
      }
    );
  }

  types::unique_ptr_template<TaskBase> task_;
};

} // end namespace detail


template <typename... Args>
decltype(auto)
reads(Args&&... args) {
  static_assert(detail::only_allowed_kwargs_given<
      keyword_tags_for_create_work_decorators::unless,
      keyword_tags_for_create_work_decorators::only_if
    >::template apply<Args...>::type::value,
    "Unknown keyword argument given to reads() decorator for create_work()"
  );
  return detail::reads_decorator_parser<Args...>()(std::forward<Args>(args)...);
}

} // end namespace darma_runtime


#endif /* SRC_CREATE_WORK_H_ */
