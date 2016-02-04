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
     *  @pre At the time of this call, it must be valid to call task->get_dependencies().  All DependencyHandles
     *  returned by task->get_dependencies() must be registered with this instance of Runtime using either
     *  register_handle() or register_fetching_handle() and must not have been released yet using release_handle().
     *  It must be valid to call needs_read_data() and needs_write_data() on \c task with any of the handles
     *  returned by get_dependencies().  At least by the time that DependencyHandle::is_satisfied() returns
     *  true for all DependencyHandles returned by task->get_dependencies(), it must be valid to call task->run()
     *
     *  @post \a task may be invoked at any subsequent time pending the satisfaction of the handles returned
     *  by Task::get_dependencies()
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
     *  @remark If the runtime implements context switching, it must ensure that
     *  the behavior of Runtime::get_running_task() is consistent and correct for a given
     *  running thread as though the switching never occurred.
     *
     *  @remark The pointer returned here is guaranteed to be valid until Task::run() returns for
     *  the returned task.  However, to allow context switching, it is not guaranteed to be valid
     *  in the context of any other task's run() invocation, including child tasks, and thus it should
     *  not be dereferenced in any other context.
     */
    virtual task_t* const
    get_running_task() const =0;

    // Methods for creating handles and registering fetches of those handles


    /** @brief Register a local handle to a data block
     *
     *  Register a handle that can be used as a part of the return value of Task::get_dependencies() for tasks
     *  registered on this runtime instance.  Note that the *only* allowed return values of get_dependencies()
     *  for a task passed to register_task() on this instance are values for which register_handle() (or
     *  register_fetching_handle()) has been called and release_handle() has not yet been called.  If the
     *  dependency requires readable data to be marked as satisfied (i.e., if \c t.needs_read_data(handle)
     *  returns true when handle is returned as part of Task::get_dependencies() for a registered task \c t),
     *  the readable data requirement must be satisfied by the release of all previous versions of handle
     *  \a and a call to handle_done_with_version_depth() on a handle with a penultimate subversion that is
     *  exactly 1 increment prior to the final subversion of \c handle and is at the same depth.  See
     *  Runtime::handle_done_with_version_depth() for more details.
     *
     *
     *  @sa Task::needs_read_data()
     *  @sa Runtime::register_task()
     *  @sa Runtime::register_fetcher()
     *
     *  @param handle A (non-owning) pointer to a \ref abstract::frontend::DependencyHandle for which it is
     *  valid to call \ref DependencyHandle::get_key() and \ref DependencyHandle::get_version() and for
     *  which \ref DependencyHandle::satisfy_with_data_block() has not yet been called by the backend.  (The only
     *  way to ensure the latter of these conditions is to ensure it is not in the return value of
     *  Task::get_dependencies() for any task that Runtime::register_task() has been invoked with). The pointer
     *  passed to this parameter must be valid from the time register_handle() is called until the time
     *  release_handle() is called (on this instance!) with a handle to the same key and version.  This method
     *  may be called more than once globally with the same key and the same version, but it is a debug-mode
     *  error to register more than one task (\b globally) that returns true for Task::needs_write_data() on
     *  that handle.  (Undefined behavior is still allowed in optimized mode)
     *
     *  @TODO !!! solver the reader-release problem
     *
     */
    virtual void
    register_handle(
      const handle_t* const handle,
      const handle_t* const as_reader_of = nullptr
    ) =0;


    /** @brief Register a dependency handle that is satisfied by retrieving data from the data store
     *
     *  The handle may then be used preliminarily as a dependency to tasks, etc.  Subsequents to the handle
     *  may be created with handle versions in similarly pending states.  Upon resolution of the user_version_tag
     *  to an internal Version (i.e., triggered by a publish of a handle with a matching key and matching user
     *  version tag), the handle and all registered subsequents should have their versions updated (i.e., using
     *  Version::operator+() and DependencyHandle::set_version()) to incorporate the fetched base version.
     *
     *  @param handle A (non-owning) pointer to a DependencyHandle for which it must be valid to call get_key()
     *  but for which the return value of get_version() is ignored.
     *
     *  @param user_version_tag A Key to be used to match a publication version of the same key reported
     *  by handle->get_key() published elsewhere.
     *
     *  @param write_access_allowed A boolean indicating that the handle is allowed to occupy a write role in
     *  a task; i.e., t.needs_write_data(handle) can return true for up to one Task t registered with this instance.
     *  It is the responsibility of the frontend to ensure (either through semantics or user responsibility) that
     *  register_handle() is not called with a handle to the same key and the same version tag and
     *  write_access_allowed=true more than once \b globally.  To do so is a debug mode error/optimized mode
     *  undefined behavior.  Also, if a frontend::Task instance returns true for Task::needs_write_data()
     *  on a handle that was registered with write_access_allowed=false, a debug mode error should be raised
     *  (undefined behavior is allowed in optimized mode).
     *
     *  @todo for 0.2.1 spec, revise the write_access_allowed description to incorporate the fact that the remote handle must
     *  also not create any post-publication subsequents (probably need a special publish)
     *
     *  @todo 0.3.1 spec: separate the concept of read-only retrieval from that of ownership transfer
     */
    virtual void
    register_fetching_handle(
      handle_t* const handle,
      const Key& user_version_tag,
      bool write_access_allowed
    ) =0;


    /** @brief Release a previously registered handle, indicating that no more tasks will be registered
     *  with \c handle as an output (at least using the instance registered with register_handle()).
     *
     *  @param handle A (non-owning) pointer to the same object with which Runtime::register_handle() (or
     *  register_fetching_handle()) was previously invoked.  Any (frontend or backend) uses of \c handle
     *  after release_handle() is returns are invalid and result in undefined behavior.  Upon release,
     *  if the handle has been published with Runtime::publish_handle() or if the handle was registered
     *  as the last subversion at a given depth using Runtime::handle_done_with_version_depth(), the
     *  appropriate action(s) should be taken to satisfy dependencies requiring data from the data block
     *  in \c handle (i.e., the return value of handle->get_data_block()).  If neither of these uses have
     *  been performed at the time of release, it is safe for the runtime to garbage collect the data block
     *  returned by handle->get_data_block().
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

    /** @brief Indicate that no further subversions at the current version depth will be created.
     *
     *  Essentially, this establishes a satisfy-upon-release relationship between \c handle and a handle
     *  with the subsequent of the penultimate subversion of handle.  In other words, if \c handle has
     *  version a.b.c.d (for some values a, b, c, and d of incrementable type, e.g., integers), then this
     *  indicates to the runtime that a call to \c Runtime::release_handle() on \c handle should lead to a
     *  call of (for some handle \c h2 with the same key and version a.b.(++c)):
     *      h2->satisfy_with_data_block(handle->get_data_block());
     *  where \c h2 must be registered at the time of that \c Runtime::handle_done_with_version_depth() is invoked
     *  on \c handle.  If no such handle is registered and no publications of handle have been made at the time
     *  of this invocation, the runtime is free to garbage collect the data block upon release.  Registering a
     *  handle with a version subsequent to the penultimate version of \c handle between this invocation and
     *  the release of handle will lead to undefined behavior.
     *
     *  @todo perhaps a better name is needed here?
     *
     */
    virtual void
    handle_done_with_version_depth(
      const handle_t* const handle
    ) =0;

    /** @brief Indicate to the backend that the key and version reported by \c handle should be fetchable
     *  with the user version tag \c vertion_tag exactly \c n_additional_fetchers times.
     *
     *  In other words, \ref Runtime::register_fetching_handle() must be called exactly \c n_fetchers times
     *  \b globally with the key reported by \c handle and the \c version_tag given here before the runtime
     *  can overwrite or delete the data associated with the key and version reported by \c handle.
     *
     *  The behavior also depends on whether or not handle is satisfied (i.e., handle->is_satisfied() returns true)
     *  at the time of the publish_handle() invocation.  If it is satisfied, the runtime publishes the
     *  actual data associated with the data block returned by handle->get_data_block().  This must happen
     *  immediately, before returning from this method, since the user is free to modify the data afterwards,
     *  which would lead to undefined beahvior.  If the handle is not satisfied, the runtime should enqueue
     *  a publication of the handle upon satisfaction.  The latter should return immediately.
     *
     *  @param handle A (non-owning) pointer to a DependencyHandle registered with register_handle() but
     *  not yet released with release_handle().  The handle must have been registered with write_access_allowed=true.
     *  @param version_tag A user-space tag to be associated with the internal version reported by handle
     *  and to be fetched as such
     *  @param n_fetchers The number of times register_fetcher() must be called globally (and the corresponding
     *  fetching handles released) before antidependencies on \c handle are cleared.
     *  @param is_final Whether or not the publish is intented to indicate the key and data associated with
     *  handle are to be considered globally read-only for the rest of its lifetime.  If true, it is a (debug-mode)
     *  error to register a handle (anywhere) with the same key and a version v > handle->get_version()
     *
     */
    virtual void
    publish_handle(
      const handle_t* const handle,
      const Key& version_tag,
      const size_t n_fetchers = 1,
      bool is_final = false
    ) =0;

    // Methods for "bare" dependency satisfaction and use.  Not used
    // for task dependencies

    /**
     * @todo Document this for 0.2.1 spec
     */
    virtual void
    satisfy_handle(
      handle_t* const to_fill,
      bool needs_write_access = false
    ) =0;

    // Methods for establishing containment and/or aliasing relationships

    /**
     * @todo Document this for 0.2.1 spec
     */
    virtual void
    establish_containment_relationship(
      const handle_t* const inner_handle,
      const handle_t* const outer_handle,
      containment_manager_t const& manager
    ) =0;

    /**
     * @todo Document this for 0.2.1 spec
     */
    virtual void
    establish_aliasing_relationship(
      const handle_t* const handle_a,
      const handle_t* const handle_b,
      aliasing_manager_t const& manager
    ) =0;

    /** @brief signifies the end of the outer SPMD task from which dharma_backend_initialize() was called
     */
    virtual void
    finalize() =0;


    virtual ~Runtime() noexcept = default;

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
     *
     *  the readable data requirement may be satisfied in one of two ways.  If Runtime::register_fetcher() is called with the handle after calling register_handle(),
     *  then the data to be read must be fetched from the data store under the key and user version tag reported
     *  by the fetcher.  Otherwise, the runtime will wait until a writable handle with the preceding version is
     *  released (actually, until the last subversion, indicated by Runtime::handle_done_with_version_depth(),
     *  of the preceding version is released) and satisfy input uses of handle with the released previous version's
     *  DependencyHandle::get_data_block() return value.  It is an error to specify both data retrieval methods,
     *  and specifying neither will lead to deadlock upon usage of the handle (thus, it is prudent for the frontend
     *  developier to ensure that exactly one of these two read data specifications is used through, for instance,
     *  some sort of RIIA-like mechanism).
**/
