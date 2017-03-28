/*
//@HEADER
// ************************************************************************
//
//                      create_condition.h
//                         DARMA
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

#ifndef DARMA_IMPL_CREATE_CONDITION_H
#define DARMA_IMPL_CREATE_CONDITION_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(create_condition)

#include <tinympl/vector.hpp>

#include <darma/impl/util/smart_pointers.h>

#include <darma/impl/condition_task.h>

//namespace darma_runtime {
//
//namespace detail {

// DEPRECATED

//template <typename Callable, typename... Args>
//struct _create_condition_impl {
//  //static_assert(
//  //  std::is_convertible<decltype(std::declval<Callable>()()), bool>::value,
//  //  "Callable given to create_condition() must return a value convertible to bool"
//  //);
//
//  // FunctorWrapper version
//  inline bool
//  operator()(Args&&... args) const {
//    detail::TaskBase* parent_task = static_cast<detail::TaskBase* const>(
//      abstract::backend::get_backend_context()->get_running_task()
//    );
//    assert(parent_task != nullptr);
//
//    //auto task = detail::make_unique<TaskBase>(std::forward<Callable>(callable));
//    auto task = detail::make_unique<ConditionTaskImpl>();
//    task->default_capture_as_info |= AccessHandleBase::ReadOnly;
//    parent_task->current_create_work_context = task.get();
//
//    // Now trigger the capture by setting the runnable:
//    task->set_runnable(
//      std::make_unique<FunctorRunnable<Callable, Args...>>(
//        variadic_constructor_arg,
//        std::forward<Args>(args)...
//      )
//    );
//
//    // Reset the default_capture_as_info that we changed
//    task->default_capture_as_info &= ~AccessHandleBase::ReadOnly;
//
//    // And set the current create_work context to null
//    parent_task->current_create_work_context = nullptr;
//
//    // Now we need to run all of the registrations that were created during capture
//    task->post_registration_cleanup();
//
//    return abstract::backend::get_backend_runtime()->register_condition_task(
//      std::move(task)
//    );
//
//  }
//
//  // Lambda version
//  inline bool
//  operator()(Args&&... args, Callable&& callable) const {
//    detail::TaskBase* parent_task = static_cast<detail::TaskBase* const>(
//      abstract::backend::get_backend_context()->get_running_task()
//    );
//    assert(parent_task != nullptr);
//
//    //auto task = detail::make_unique<TaskBase>(std::forward<Callable>(callable));
//    auto task = detail::make_unique<ConditionTaskImpl>();
//    task->default_capture_as_info |= AccessHandleBase::ReadOnly;
//    parent_task->current_create_work_context = task.get();
//
//    // Indicate that this is a "double copy" capture and the AccessHandle copy ctor
//    // needs to follow a back pointer to get the outer context (continuing) handle
//    task->is_double_copy_capture = true;
//
//    // Now trigger the capture by doing a copy and making a lambda runnable that
//    // works on that copy
//    task->set_runnable(std::make_unique<RunnableCondition<Callable>>(
//      // Note: copied, not moved!!!
//      callable
//    ));
//
//    // TODO we actually want to destroy the original callable ASAP (perhaps by ending the function here?)
//
//    // Reset the default_capture_as_info that we changed
//    task->default_capture_as_info &= ~AccessHandleBase::ReadOnly;
//
//    // And set the current create_work context to null
//    parent_task->current_create_work_context = nullptr;
//
//    task->post_registration_cleanup();
//
//    return abstract::backend::get_backend_runtime()->register_condition_task(
//      std::move(task)
//    );
//
//  }
//};
//
//// FunctorWrapper version
//template <typename Functor, typename... Args>
//struct _create_condition_select_functor {
//  bool operator()(Args&&... args) const {
//    return _create_condition_impl<Functor, Args...>()(
//      std::forward<Args>(args)...
//    );
//  }
//};
//
//// Lambda version
//template <typename... Args>
//struct _create_condition_select_functor<void, Args...> {
//  bool operator()(Args&&... args) const {
//    namespace m = tinympl;
//    // Pop off the last type and move it to the front
//    using lambda_t = typename m::vector<Args...>::back::type;
//    using rest_vector_t = typename m::vector<Args...>::pop_back::type;
//    using helper_t = typename m::splat_to<
//      typename rest_vector_t::template push_front<lambda_t>::type,
//      _create_condition_impl
//    >::type;
//
//    return helper_t()(std::forward<Args>(args)...);
//  }
//};
//
//} // end namespace detail
//
//
//template <
//  // Defaulted to void in forward declaration
//  typename Functor /*=void*/,
//  typename... Args
//>
//bool
//create_condition(Args&&... args) {
//  return detail::_create_condition_select_functor<Functor, Args...>()(
//    std::forward<Args>(args)...
//  );
//}
//
//} // end namespace darma_runtime

#endif // _darma_has_feature(create_condition)

#endif //DARMA_IMPL_CREATE_CONDITION_H
