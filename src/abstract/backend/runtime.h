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
      Key, Version, types::handle_container_template
    > task_t;
    typedef unique_ptr_template<task_t> task_unique_ptr;
    typedef unique_ptr_template<const task_t> task_const_unique_ptr;

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
     */
    virtual void
    register_handle(
      const handle_t* const handle
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
     *  @remark for the 0.2.0 spec implementation, write_access_allowed should always be false
     *
     *  @todo 0.2.1 spec remove write_access_allowed, create a special publish and a special fetch for ownership transfer
     *
     *  @todo 0.3.1 spec: separate the concept of read-only retrieval from that of ownership transfer
     */
    virtual void
    register_fetching_handle(
      handle_t* const handle,
      const Key& user_version_tag,
      bool write_access_allowed
    ) =0;

    /** @brief Release the ability to create tasks that depend on handle as read-only.  The handle can
     *  then only be used up to once more in a (read/)write context before it is released.
     *
     *  All handles can be used in a write context (i.e., be part of the return for Task::get_dependencies()
     *  for a task that returns true for needs_write_data() on that handle) at most once in their lifetime
     *  (from register_handle()/register_fetching_handle() to release_handle()).  This (up to) one "final"
     *  usage must run after all read-only uses of the handle; however not all read-only uses of
     *  a handle need to be registered by time the task making the "final" usage is registered.  Thus,
     *  the frontend needs to indicate when no more read-only usages will be created, thus allowing
     *  the "final" usage to clear its antidependencies.
     *
     *  This method indicates that no more tasks will be registered that use handle in a read context
     *  but not in a write context (a maximum of one task may use handle in a write context anyway).
     *  It must be called for all handles before release_handle() is called.  It is an error to call
     *  release_handle() on a handle before calling release_read_only_usage().
     *
     *  @param handle a non-owning pointer to a DependencyHandle for which register_handle() or
     *  register_fetching_handle() has been called on this instance but for which release_handle() has not
     *  yet been called.
     *
     *  @todo !! check that this actually says the right thing and UPDATE
     *
     */
    virtual void
    release_read_only_usage(
      const handle_t* const handle
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
     *  @todo 0.2.0.1 satisfied needs to be changed to writable
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
     *  @todo 0.2.2?? Perhaps split into publish_handle/publish_data?
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
     * @todo Document this for 0.2.2 spec
     */
    virtual void
    establish_containment_relationship(
      const handle_t* const inner_handle,
      const handle_t* const outer_handle,
      containment_manager_t const& manager
    ) =0;

    /**
     * @todo Document this for 0.2.2 spec
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


/** @brief initialize the backend::Runtime instance
 *
 *  @remark This should be called once per top-level task.  The backend may chose whether
 *  the frontend is allowed to have multiple top-level tasks in one process.  If the backend
 *  supports multiple top-level tasks, it should define the preprocessor constant
 *  DHARMA_BACKEND_TOP_LEVEL_TASK_MULTIPLE.  If not, it should define the constant
 *  DHARMA_BACKEND_TOP_LEVEL_TASK_SINGLE.  The frontend is free to wrap these macro constants
 *  in more user-friendly names before exposing them to the user.
 *
 *  @pre The frontend should do nothing that interacts with the backend before this
 *  function is called.
 *
 *  @post Upon return, the (output) parameter backend_runtime should point to  a valid
 *  instance of backend::Runtime.  All methods of backend_runtime should be invocable
 *  from any thread with access to the pointer backend_runtime.  The runtime should assume
 *  control of the (owning) pointer passed in through top_level_task and should not delete
 *  it before Runtime::finalize() is called on the backend_runtime object.  The top-level
 *  task object should be setup as described below
 *
 *  @param argc An lvalue reference to the argc passed into main().
 *  @param argv An lvalue reference to the argv passed into main().
 *
 *  @param backend_runtime The input value of this parameter is ignored.  On return, it should
 *  point to a properly initialized instance of backend::Runtime.  The backend owns this
 *  instance, and should delete it at some point after the frontend calls Runtime::finalize() on
 *  this instance.
 *
 *  @param top_level_task The task object to be returned by Runtime::get_running_task() if that
 *  method is invoked (on the instance pointed to by backend_runtime upon return) from the outermost
 *  task context within which dharma_backend_initialize() was invoked.  (Inside of any task context
 *  created by an invocation of Task::run(), of course, the runtime should still behave as documented
 *  in the Runtime::get_running_task() method).  It is \a not valid to call Task::run() on this
 *  top-level task object (i.e., it is an error), and doing so will cause the frontend to abort.
 *  Indeed (as of 0.2 spec), the only valid methods for the backend to call on this object are
 *  Task::set_name() and Task::get_name().  At least before returning top_level_task from any calls to
 *  Runtime::get_running_task(), the backend runtime should assign a name to the top-level task
 *  with at least three parts, the first three of which must be: a string constant defined by the
 *  preprocessor macro DHARMA_BACKEND_SPMD_KEY_PREFIX, a std::size_t for the rank of the SPMD-like
 *  top-level task from which initialize was invoked, and a std::size_t givin the total number of
 *  ranks in the SPMD launch (which must be known at launch time; SPMD ranks cannot be dynamically
 *  allocated).  In other words, the backend should make a call of the form:
 *    top_level_task->set_name(DHARMA_BACKEND_SPMD_NAME_PREFIX, rank, size);
 *
 *  @remark The backend is free to extract backend-specific command-line arguments provided it
 *  updates argc and argv.  When dharma_backend_initialize() returns, backend-specific parameters
 *  must no longer be in argc and argv.
 *
 */
extern void
dharma_backend_initialize(
  int& argc, char**& argv,
  runtime_t*& backend_runtime,
  types::unique_ptr_template<typename runtime_t::task_t> top_level_task
);


} // end namespace backend

} // end namespace abstract

} // end namespace dharma_runtime




#endif /* SRC_ABSTRACT_BACKEND_RUNTIME_H_ */
