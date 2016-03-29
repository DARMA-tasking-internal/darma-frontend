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

// TODO move these to their own files in interface/app when they become part of the spec
DeclareDarmaTypeTransparentKeyword(create_work_decorators, unless);
DeclareDarmaTypeTransparentKeyword(create_work_decorators, only_if);


namespace darma_runtime {

namespace detail {

namespace mv = tinympl::variadic;

template <typename... Args>
struct create_work_parser {
  typedef /* TODO ??? */ void return_type;
  typedef typename mv::back<Args...>::type lambda_type;
};

struct reads_decorator_return {
  typedef abstract::backend::runtime_t runtime_t;
  typedef runtime_t::key_t key_t;
  typedef runtime_t::version_t version_t;
  typedef runtime_t::handle_t handle_t;
  reads_decorator_return(std::initializer_list<types::shared_ptr_template<handle_t>> args)
    : handles(args)
  { }
  std::vector<types::shared_ptr_template<handle_t>> handles;
  bool use_it = true;
};

template <typename ReturnType>
struct forward_to_get_dep_handle {
  template <typename... AccessHandles>
  constexpr inline ReturnType
  operator()(AccessHandles&&... ah) const {
    using namespace detail::create_work_attorneys;
    return { for_AccessHandle::get_dep_handle_ptr(std::forward<AccessHandles>(ah))... };
  }
};

struct forward_to_uncaptured_deps {
  explicit
  forward_to_uncaptured_deps(TaskBase* t) : task(t) { }
  template <typename... AccessHandles>
  void
  operator()(AccessHandles&&... ah) const {
    meta::tuple_for_each(std::forward_as_tuple(std::forward<AccessHandles>(ah)...),
      [&](auto&& h){
        //std::unique_ptr<AccessHandleBase> ahbptr(
        //);
        // TODO FIX THIS!!!
        task->uncaptured_deps.emplace_back(
          std::make_unique<std::decay_t<decltype(h)>>(h)
        );
      }
    );
  }
  TaskBase* task;
};

template <typename... Args>
struct reads_decorator_parser {
  typedef reads_decorator_return return_type;
  inline return_type
  operator()(Args&&... args) const {
    using namespace detail::create_work_attorneys;
    detail::TaskBase* parent_task = safe_static_cast<detail::TaskBase* const>(
      detail::backend_runtime->get_running_task()
    );
    detail::TaskBase* task = parent_task->current_create_work_context;
    forward_to_uncaptured_deps f(task);

    auto rv = meta::splat_tuple(
      get_positional_arg_tuple(std::forward<Args>(args)...),
      forward_to_get_dep_handle<return_type>()
    );
    rv.use_it = not get_typeless_kwarg_with_default_as<
        darma_runtime::keyword_tags_for_create_work_decorators::unless,
        bool
    >(false, std::forward<Args>(args)...);
    meta::splat_tuple(
      get_positional_arg_tuple(std::forward<Args>(args)...), f
    );
    return rv;
  }
};

// Removed from 0.2 spec
//struct waits_decorator_return {
//  typedef abstract::backend::runtime_t runtime_t;
//  typedef runtime_t::key_t key_t;
//  typedef runtime_t::version_t version_t;
//  typedef runtime_t::handle_t handle_t;
//  typedef types::shared_ptr_template<handle_t> handle_ptr;
//  handle_ptr const& handle;
//};
//
//template <typename... Args>
//struct waits_decorator_parser {
//  typedef waits_decorator_return return_type;
//  // For now:
//  static_assert(sizeof...(Args) == 1, "multi-args not yet implemented");
//  return_type
//  operator()(Args&&... args) {
//    using namespace detail::create_work_attorneys;
//    assert(false); // not implemented
//    // TODO implement this
//    return {
//      for_AccessHandle::get_dep_handle_ptr(
//        std::get<0>(std::forward_as_tuple(args...))
//      )
//    };
//
//  }
//};
//
//template <typename... Args>
//struct writes_decorator_parser {
//  typedef /* TODO */ int return_type;
//};
//
//template <typename... Args>
//struct reads_writes_decorator_parser {
//  typedef /* TODO */ int return_type;
//};


template <typename Lambda, typename... Args>
struct create_work_impl {
  typedef create_work_parser<Args..., Lambda> parser;
  typedef detail::Task<Lambda> task_t;
  typedef detail::TaskBase task_base_t;

  inline typename parser::return_type
  operator()(
    std::unique_ptr<TaskBase>&& task_base,
    Args&&... args, Lambda&& lambda) const {
    task_base->set_runnable(
      std::make_shared<Runnable<Lambda>>(std::forward<Lambda>(lambda))
    );
    detail::TaskBase* parent_task = safe_static_cast<detail::TaskBase* const>(
      detail::backend_runtime->get_running_task()
    );
    return detail::backend_runtime->register_task(
      std::move(task_base)
      //std::make_unique<task_t>(
      //  std::forward<Lambda>(lambda)
      //)
    );
  }
};


inline types::unique_ptr_template<TaskBase>
_start_create_work() {
  auto rv = std::make_unique<TaskBase>();
  detail::TaskBase* parent_task = safe_static_cast<detail::TaskBase* const>(
    detail::backend_runtime->get_running_task()
  );
  parent_task->current_create_work_context = rv.get();
  return std::move(rv);
}

struct _do_create_work {
  explicit
  _do_create_work(types::unique_ptr_template<TaskBase>&& tsk_base)
    : task_(std::move(tsk_base))
  { }
  template <typename... Args>
  inline void
  operator()(Args&&... args) {
    static_assert(detail::only_allowed_kwargs_given<
      >::template apply<Args...>::type::value,
      "Unknown keyword argument given to create_work()"
    );

    namespace m = tinympl;
    // Pop off the last type and move it to the front
    typedef typename m::vector<Args...>::back::type lambda_t;
    typedef typename m::vector<Args...>::pop_back::type rest_vector_t;
    typedef typename m::splat_to<
      typename rest_vector_t::template push_front<lambda_t>::type, detail::create_work_impl
    >::type helper_t;
    namespace m = tinympl;
    namespace mp = tinympl::placeholders;

    detail::TaskBase* parent_task = safe_static_cast<detail::TaskBase* const>(
      detail::backend_runtime->get_running_task()
    );
    parent_task->current_create_work_context = nullptr;

    meta::tuple_for_each_filtered_type<
      m::lambda<std::is_same<std::decay<mp::_>, reads_decorator_return>>::template apply
    >(std::forward_as_tuple(std::forward<Args>(args)...), [&](auto&& rdec){
      if(rdec.use_it) {
        for(auto&& h : rdec.handles) {
          task_->read_only_handles.emplace(h);
        }
      }
      else {
        for(auto&& h : rdec.handles) {
          task_->ignored_handles.emplace(h.get());
        }
      }
    });

    for(auto&& reg : task_->registrations_to_run) {
      reg();
    }

    // TODO waits() decorator
    //meta::tuple_for_each_filtered_type<
    //  m::lambda<std::is_same<std::decay<mp::_>, detail::waits_decorator_return>>::template apply
    //>(std::forward_as_tuple(std::forward<Args>(args)...), [&](auto&& rdec){
    //  parent_task->waits_handles.emplace(rdec.handle);
    //});


    return helper_t()(std::move(task_), std::forward<Args>(args)...);

  }
  types::unique_ptr_template<TaskBase> task_;
};



} // end namespace detail


template <typename... Args>
typename detail::reads_decorator_parser<Args...>::return_type
reads(Args&&... args) {
  static_assert(detail::only_allowed_kwargs_given<
      keyword_tags_for_create_work_decorators::unless
    >::template apply<Args...>::type::value,
    "Unknown keyword argument given to reads() decorator for create_work()"
  );
  return detail::reads_decorator_parser<Args...>()(std::forward<Args>(args)...);
}

// Removed from 0.2 spec
//template <typename... Args>
//typename detail::waits_decorator_parser<Args...>::return_type
//waits(Args&&... args) {
//  // TODO implement this
//  return typename detail::waits_decorator_parser<Args...>::return_type();
//}

// Removed from 0.2 spec
//template <typename... Args>
//typename detail::writes_decorator_parser<Args...>::return_type
//writes(Args&&... args) {
//  // TODO implement this
//  assert(false); // not implemented
//  return typename detail::writes_decorator_parser<Args...>::return_type();
//}

// Removed from 0.2 spec
//template <typename... Args>
//typename detail::reads_writes_decorator_parser<Args...>::return_type
//reads_writes(Args&&... args) {
//  // TODO implement this
//  assert(false); // not implemented
//  return typename detail::reads_writes_decorator_parser<Args...>::return_type();
//}



} // end namespace darma_runtime



#endif /* SRC_CREATE_WORK_H_ */
