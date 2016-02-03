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


    /** @brief Register a local handle to a data block
     *  @TODO update this to talk about get_dependencies rather than get_inputs and get_outputs
     *
     *  Register a handle that can be used locally as a return value of Task::get_dependencies() for tasks
     *  registered on this runtime instance.  Note that the *only* allowed return values of get_dependencies()
     *  for a task passed to register_task() on this instance are values for which register_handle() has been
     *  called and release_handle() has not yet been called.  If the dependency requires readable data to be
     *  marked as satisfied (i.e., if \c t.needs_read_data(handle) returns true when handle is returned as part of
     *  Task::get_dependencies() for a registered task \c t), the readable data requirement may be satisfied in one
     *  of two ways.  If Runtime::register_fetcher() is called with the handle after calling register_handle(),
     *  then the data to be read must be fetched from the data store under the key and user version tag reported
     *  by the fetcher.  Otherwise, the runtime will wait until a writable handle with the preceding version is
     *  released (actually, until the last subversion, indicated by Runtime::handle_done_with_version_depth(),
     *  of the preceding version is released) and satisfy input uses of handle with the released previous version's
     *  DependencyHandle::get_data_block() return value.  It is an error to specify both data retrieval methods,
     *  and specifying neither will lead to deadlock upon usage of the handle (thus, it is prudent for the frontend
     *  developier to ensure that exactly one of these two read data specifications is used through, for instance,
     *  some sort of RIIA-like mechanism).
     *
     *  @sa Task::needs_read_data()
     *  @sa Runtime::register_task()
     *  @sa Runtime::register_fetcher()
     *
     *  @param handle A (non-owning) pointer to a \ref abstract::frontend::DependencyHandle for which it is
     *  valid to call \ref DependencyHandle::get_key() and \ref DependencyHandle::get_version() and for
     *  which \ref DependencyHandle::satisfy_with_data_block() has not yet been called by the backend.  (The only
     *  way to ensure the latter of these conditions is to ensure it is not in the return value of
     *  Task::get_dependencies() for any task that Runtime::register_task() has been invoked with).  Furthermore,
     *  the frontend must ensure (either explicitly or through user requirements)  that register_handle() is never
     *  called more than once (\b globally) with a handle referring to the same key and version pair.  The pointer
     *  passed to this parameter must be valid from the time register_handle() is called until the time
     *  release_handle() is called with a handle to the same key and version.
     *
     */
    virtual void
    register_handle(
      const handle_t* const handle
    ) =0;


    /** @brief Release a previously registered handle, indicating that no more tasks will be registered
     *  with \c handle as an output (at least using the instance registered with register_handle()).
     *
     *  @param handle A (non-owning) pointer to the same object with which Runtime::register_handle() was previously
     *  invoked.  Any (frontend or backend) uses of \c handle after release_handle() is returns are invalid and
     *  result in undefined behavior.  Upon release, if the handle has been published with Runtime::publish_handle()
     *  or if the handle was registered as the last subversion at a given depth using
     *  Runtime::handle_done_with_version_depth(), the appropriate action(s) should be taken to satisfy dependencies
     *  requiring data from the data block in \c handle (i.e., the return value of handle->get_data_block()).  If
     *  neither of these uses have been performed at the time of release, it is safe for the runtime to garbage
     *  collect the data block returned by handle->get_data_block().
     *
     *  @sa DependencyHandle::get_data_block()
     *  @sa Runtime::handle_done_with_version_depth()
     *  @sa Runtime::publish_handle()
     *
     */
    virtual void
    release_handle(
      const handle_t* const handle
    ) =0;

    // TODO need a better name for this
    /** @brief Indicate that no further subversions at the current version depth will be created.
     *
     *  Essentially, this establishes a satisfy-upon-release relationship between \c handle and a handle
     *  with the subsequent of the penultimate subversion of handle.  In other words, if \c handle has
     *  version a.b.c.d (for some values a, b, c, and d of incrementable type, e.g., integers), then this
     *  indicates to the runtime that a call to \c Runtime::release_handle() on \c handle should lead to a
     *  call of (for some handle \c h2 with the same key and version a.b.(++c)):
     *    h2->satisfy_with_data_block(handle->get_data_block());
     *  where \c h2 must be registered at the time of that \c Runtime::handle_done_with_version_depth() is invoked
     *  on \c handle.  If no such handle is registered and no publications of handle have been made at the time
     *  of this invocation, the runtime is free to garbage collect the data block upon release.  Registering a
     *  handle with a version subsequent to the penultimate version of \c handle between this invocation and
     *  the release of handle will lead to undefined behavior.
     *
     */
    virtual void
    handle_done_with_version_depth(
      const handle_t* const handle
    ) =0;

    /** @brief Indicate to the backend that the key and version reported by \c handle should be fetchable
     *  with the user version tag \c vertion_tag exactly \c n_additional_fetchers times.
     *
     *  @TODO update this
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
      const size_t n_additional_fetchers = 1,
      bool is_final = false
    ) =0;

    //virtual handle_t*
    //resolve_version_tag(
    //  const Key& handle_key,
    //  const Key& version_tag
    //) =0;

    virtual void
    register_fetcher(
      const handle_t* const fetcher_handle,
      const Key& version_tag
    ) =0;

    //virtual void
    //release_fetcher(
    //  const handle_t* const fetcher_handle
    //) =0;

    //// Methods for "bare" dependency satisfaction and use.  Not used
    //// for task dependencies
    //// TODO decide between this and the wait_* methods in the DataBlock class

    virtual void
    satisfy_handle(
      handle_t* const to_fill,
      bool needs_read_access = true,
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
extern void
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

// ATTIC

/**
     *  @param needs_data_from_fetcher (Optional) Either a (non-owning) pointer to the handle reportingVjjj
     *
     *  will be retrieved before marking the handle as satisfied or \c nullptr.  If this parameter is non-null,
     *  the handle may be used as an input, and the data to be read will correspond
     *
     *  ********** OLD *****************
     *  The handle registered here indicates that it will be used subsequently in a write or read/write
     *  context of a task.  That is, it may be eventually used either in as a task output or in a task
     *  that has an output with the same key and the version ++handle.version (or an increment of some
     *  equivalent subversion of handle.version).  In either case, this indicates that \ref handle is allowed
     *  to be used in a context were it is the final use of a dependency with that specific key and version
     *  before it is (over-)written, and all other read-only uses must complete before the write can occur.
     *  For simplicity, we'll refer to this usage allowed for handles registered here as their "final" usage.
     *  Read-only uses of the handle registered here are also allowed, but at least an outermost task read-only usage
     *  must be registered before  the task making the "final" usage is registered (though eventual support for
     *  extra read-only handles may eventually be allowed).  Note that read-only uses of the handle that must
     *  preceed the "final" usage  may create read-only tasks of their own, which must in turn be scheduled
     *  before the final usage (even though they are allowed to be scheduled after the "final" usage).  Note also
     *  that the "final" usage may schedule subtasks that require a (non-strict) subset of the "final" usage's
     *  privledges.  For a TODO finish this
     *
     *  @todo mechanism to allow extra read-only handles to be registered without using the publish/fetch interface
     *
     *
     *
     *  @param needs_data_from (Optional) Either a (non-owning) pointer to the handle from which readable data
     *  will be retrieved before marking the handle as satisfied or \c nullptr.  If this parameter is non-null,
     *  the handle may be used as an input, and the data to be read will correspond
     *
     *  @param is_initial TODO
**/
