/*
//@HEADER
// ************************************************************************
//
//                          runtime.h
//                         dharma_new
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

#ifndef SRC_ABSTRACT_BACKEND_RUNTIME_H_
#define SRC_ABSTRACT_BACKEND_RUNTIME_H_

#include "types.h"
#include "../frontend/types.h"

#include "../frontend/dependency_handle.h"
#include "../frontend/task.h"
#include "../frontend/containment_manager.h"
#include "../frontend/aliasing_manager.h"

namespace dharma_runtime {

namespace abstract {

namespace backend {

/**
 *  @ingroup abstract
 *  @class Runtime
 *  @brief Abstract class implemented by the backend containing much of the runtime
 *
 *  @note Thread safety of all methods in this class should be handled by the backend implementaton;
 *  two threads must  be allowed to call any method in this class simultaneously, though undefined behavior
 *  is allowed if nonsensical simultaneous calls are made (for instance, two calls registering the exact same
 *  task at the same time).
 */
template <
  typename Key,
  typename Version,
  template <typename...> class shared_ptr_template,
  template <typename...> class unique_ptr_template
>
class Runtime {

  public:

    typedef Key key_t;
    typedef Version version_t;
    typedef abstract::frontend::DependencyHandle<Key, Version> handle_t;
    typedef abstract::frontend::ContainmentManager<Key, Version> containment_manager_t;
    typedef abstract::frontend::AliasingManager<Key, Version> aliasing_manager_t;
    typedef abstract::frontend::Task<
      Key, Version, types::handle_container_template, shared_ptr_template
    > task_t;
    typedef shared_ptr_template<task_t> task_ptr;
    typedef shared_ptr_template<const task_t> task_const_ptr;
    typedef unique_ptr_template<task_t> task_unique_ptr;
    typedef unique_ptr_template<const task_t> task_const_unique_ptr;
    typedef shared_ptr_template<handle_t> handle_ptr;
    typedef shared_ptr_template<const handle_t> handle_const_ptr;

    /** @brief Enqueue a task that should run when its dependencies are satisfied
     *
     *  @pre Before this call, it must be valid to call \ref abstract::frontend::Task::get_inputs() and
     *  \ref abstract::frontend::Task::get_outputs() on the parameter \a task (i.e., all inputs and
     *  outputs must be added to \a task before the task is registered).  Each \ref
     *  abstract::frontend::DependencyHandle returned by \ref Task::get_outputs() and \ref Task::get_inputs()
     *  must be registered as either a handle or a fetcher (the latter for inputs only) using the
     *  \ref Runtime::register_handle() or Runtime::register_fetcher() methods, respectively, and must not
     *  have been released yet using the corresponding release methods.
     *
     *  @post \a task may be invoked at any subsequent time pending the satisfaction of the handles returned
     *  by Task::get_inputs() and Task::get_outputs()
     *
     *  @param task A unique pointer to the task to register and enqueue, meeting the preconditions
     *  described above.  The runtime backend now owns the task object and is responsible for deleting it
     *  after the task has finished running.  Any references or pointers to \ref task are considered
     *  invalid after the task has run (i.e., \ref Task::run() was invoked and returned), and thus should only
     *  be held by objects deleted inside Task::run() or objects deleted when \ref task is deleted.
     *
     */
    virtual void
    register_task(
      task_unique_ptr&& task
    ) =0;

    /** @brief Get a pointer to the \ref abstract::frontend::Task object currently running on the thread
     *  from which get_running_task() was invoked.
     *
     *  @return A non-owning pointer to the \ref abstract::frontend::Task object running on the invoking
     *  thread.  The returned pointer must be castable to the same concrete type as was passed to
     *  \ref Runtime::register_task() when the task was registered.
     *
     */
    virtual task_t* const
    get_running_task() const =0;

    // Methods for creating handles and registering fetches of those handles


    /** @brief Register a handle that can write (and also potentially read) a block of data
     *
     *  @param handle A shared pointer to a \ref abstract::frontend::DependencyHandle for which it is
     *  valid to call \ref DependencyHandle::get_key() and \ref DependencyHandle::get_version() and for
     *  which \ref DependencyHandle::satisfy_with_data() has not yet been called by the backend.  (The only
     *  way to ensure the latter of these conditions is to ensure it is not a potential return value of
     *  Task::get_inputs() or Task::get_outputs() for any task that Runtime::register_task() has not been
     *  invoked with).  Furthermore, the frontend must ensure (either explicitly or through user requirements)
     *  that register_handle() is never called more than once (\b globally) with a handle referring to the same key and
     *  version pair.  The shared reference to the \ref DependencyHandle object pointed to by this argument
     *  should be held until \ref Runtime::release_handle() is called with a handle having the same key and
     *  version, but must be released thereafter.
     *
     *  @param needs_data_from (Optional) Either a (non-owning) pointer to the handle from which readable data
     *  will be retrieved before marking the handle as satisfied or \c nullptr.  If this parameter is non-null,
     *  \ref handle will be satisfied with the data in the handle registered with the key and version reported
     *  by \c needs_data_from upon a call of Runtime::release_handle() with a handle having that key and version.
     *  The key and version reported by needs_data_from must be registered locally as either a handle or a
     *  fetcher (using \ref Runtime::register_handle() or \ref Runtime::register_fetcher()) before this method
     *  is invoked.
     *
     */
    virtual void
    register_handle(
      const handle_ptr& handle,
      // TODO think through whether this is necessary
      const handle_t* const needs_data_from = nullptr
    ) =0;


    /** @brief Release a previously registered handle, indicating that no more tasks will be registered
     *  with \c handle as an output (at least using the instance registered with register_handle()).
     *
     *  @param handle A shared pointer to the same object with which Runtime::register_handle() was previously
     *  invoked.  Any (frontend) uses of \c handle after release_handle() is returns are invalid and result in
     *  undefined behavior.  There may be  up to one remaining shared reference to the handle which is held
     *  by the task for which it was registered as an output.  Upon release, if \c handle has been
     *  used as an output to any registered task, the input uses of handles for which a handle with the same
     *  key and version has been registered as the needs_data_from parameter will be satisfied with the
     *  data in handle (after the up to one task with it registered as an output has finished running, or
     *  immediately if no such registration occurred).
     *
     */
    virtual void
    release_handle(
      const handle_ptr& handle
    ) =0;

    /** @brief
     *  @todo document this
     */
    virtual void
    satisfy_when_released(
      const handle_ptr& handle,
      const handle_ptr& input_to_be_satisfied
    )

    /** @brief Indicate to the backend that the key and version reported by \c handle should be fetchable
     *  with the user version tag \c vertion_tag exactly \c n_additional_fetchers times.
     *
     *  In other words, \ref Runtime::register_fetcher() must be called \c n_additional_fetchers times \b globally with
     *  the key reported by \c handle and the \c version_tag given here before the runtime can overwrite or delete
     *  the data associated with the key and version reported by handle.
     *
     *  @todo describe these
     *  @param handle
     *  @param version_tag
     *  @param n_additional_fetchers
     *
     */
    virtual void
    publish_handle(
      const handle_t* const handle,
      const Key& version_tag,
      const size_t n_additional_fetchers = 1
    ) =0;

    virtual handle_t*
    resolve_version_tag(
      const Key& handle_key,
      const Key& version_tag
    ) =0;

    virtual void
    register_fetcher(
      const handle_t* const fetcher_handle
    ) =0;

    virtual void
    release_fetcher(
      const handle_t* const fetcher_handle
    ) =0;

    // Methods for "bare" dependency satisfaction and use.  Not used
    // for task dependencies
    // TODO decide between this and the wait_* methods in the DataBlock class

    virtual void
    fill_handle(
      handle_t* const to_fill,
      bool needs_write_access = false
    ) =0;

    // Methods for establishing containment and/or aliasing relationships

    virtual void
    establish_containment_relationship(
      const handle_t* const inner_handle,
      const handle_t* const outer_handle,
      containment_manager_t const& manager
    ) =0;

    virtual void
    establish_aliasing_relationship(
      const handle_t* const handle_a,
      const handle_t* const handle_b,
      aliasing_manager_t const& manager
    ) =0;

    // Virtual destructor, since we have virtual methods

    virtual ~Runtime() noexcept { }

};


typedef Runtime<
  dharma_runtime::types::key_t,
  dharma_runtime::types::version_t,
  dharma_runtime::types::shared_ptr_template,
  dharma_runtime::types::unique_ptr_template
> runtime_t;


// An initialize function that the backend should implement which the frontend
// calls to setup the static instance of backend_runtime
void
dharma_backend_initialize(
  int& argc,
  char**& argv,
  runtime_t*& backend_runtime,
  types::unique_ptr_template<typename runtime_t::task_t> top_level_task
);


} // end namespace backend

} // end namespace abstract

} // end namespace dharma_runtime




#endif /* SRC_ABSTRACT_BACKEND_RUNTIME_H_ */
