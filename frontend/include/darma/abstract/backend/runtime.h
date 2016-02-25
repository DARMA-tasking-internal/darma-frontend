/*
//@HEADER
// ************************************************************************
//
//                          runtime.h
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

#ifndef SRC_ABSTRACT_BACKEND_RUNTIME_H_
#define SRC_ABSTRACT_BACKEND_RUNTIME_H_

#include "types.h"
#include "../frontend/types.h"

#include "../frontend/dependency_handle.h"
#include "../frontend/task.h"
#include "../frontend/containment_manager.h"
#include "../frontend/aliasing_manager.h"

namespace darma_runtime {

namespace abstract {

namespace backend {

/**
 *  @ingroup abstract
 *  @class Runtime
 *  @brief Abstract class implemented by the backend containing much of the runtime
 *
 *  @note Thread safety of all methods in this class should be handled by the backend implementaton;
 *  two threads must be allowed to call any method in this class simultaneously.
 *
 *  @todo consider the possibility that these methods should be specified as C functions instead to avoid
 *  virtualization overhead.  If so, there would still need to be some sort of runtime context token as the
 *  first argument of each function call to allow multiple SPMD ranks to operate within a single process
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
     *  returned by get_dependencies().  At least by the time that DependencyHandle::is_satisfied() returns true (and
     *  h->is_writable() returns true for DependencyHandle objects h for which task->needs_write_data(h) returns true)
     *  or all DependencyHandle objects returned by task->get_dependencies(), it must be valid to call task->run()
     *
     *  @post \a task may be invoked at any subsequent time pending the satisfaction of the handles returned
     *  by Task::get_dependencies().  If the frontend holds a pointer or reference to the task after this method is
     *  invoked, use of that object will result in undefined behavior
     *
     *  @remark The handles returned by task->get_dependencies() \b may be released before task->run() returns.  The
     *  registration restriction applies to the invocation of register_task().  However, as of the 0.2 spec, the frontend
     *  may not release any of the handles returned by task->get_dependencies() at least until the backend calls task->run()
     *  [the previous sentence will likely be changed in a later version of the spec]
     *
     *  @param task A unique pointer to the task to register and enqueue, meeting the preconditions
     *  described above.  The runtime backend now owns the task object and is responsible for deleting it
     *  after the task has finished running.  Any other references or pointers to \ref task are considered
     *  invalid after this method is invoked.
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
     *
     *  @todo return value lifetime?  Maybe this should be a weak_ptr since there's no logical way to limit
     *  the lifetime of the returned value, especially with work stealing and context switching
     */
    virtual task_t* const
    get_running_task() const =0;

    // Methods for creating handles and registering fetches of those handles

    /** @brief Register a local handle to a data block
     *
     *  Register a handle that can be used as a part of the return value of Task::get_dependencies() for tasks
     *  registered on this runtime instance.  Note that the *only* allowed return values of get_dependencies()
     *  for a task passed to register_task() on this instance are values for which register_handle() (or
     *  register_fetching_handle()) has been called and release_handle() has not yet been called.
     *
     *  @param handle A (non-owning) pointer to a \ref abstract::frontend::DependencyHandle for which it is
     *  valid to call handle->get_key() and handle->get_version().  register_handle() must not be called more
     *  than once for a given key and version.  The pointer passed as this parameter must be valid from the
     *  time register_handle() is called until the time release_handle() is called (on this instance!) with
     *  a handle to the same key and version.
     *
     *  @remark In terms of handle life cycle, the handle passed method is always in a versioned state by the
     *  time register_handle() returns (unlike register_fetching_handle()).  Specifically, the version to be used
     *  is the version returned by handle->get_version(), and handle->version_is_pending() must return false
     *  if handle is a valid parameter to this method.
     *
     *  @sa Task::needs_read_data()
     *  @sa Runtime::register_task()
     */
    virtual void
    register_handle(
      handle_t* const handle
    ) =0;


    /** @brief Register a dependency handle that is satisfied by retrieving data from the data store
     *
     *  The handle may then be used preliminarily as a dependency to tasks, etc.  Upon return, the handle's state
     *  in its lifecycle may be *either* registered (i.e., unversioned) or a versioned.  The return value of
     *  handle->get_version() is ignored upon invocation and handle->version_is_pending() must return true
     *  upon invocation.  At some point after this invocation but before the first task depending on handle
     *  is run (which means potentially even before this method returns), handle->set_version() must be called
     *  exactly once to update the handle's version to the version with which it was published (potentially
     *  remotely).
     *
     *  @param handle A (non-owning) pointer to a DependencyHandle for which it must be valid to call get_key()
     *  but for which the return value of get_version() is ignored.
     *
     *  @param user_version_tag A Key to be used to match a publication version tag of a publish (potentially elsewhere)
     *  with a handle that reports the same key as handle->get_key().
     *
     */
    virtual void
    register_fetching_handle(
      handle_t* const handle,
      const Key& user_version_tag
    ) =0;

    /** @brief Indicate that the last task holding read-only usage or the ability to schedule other
     *  tasks with read-only priviledges has completed or explicitly released the handle.
     *
     *  After this call is made, the runtime is free to schedule the up to one modify usage (also known as "final"
     *  usage) of the handle, which may be registered before this method is called or at any time before
     *  release_handle() is called.
     *
     *  @remark It is a debug-mode error to call release_read_only_usage() on a handle after release_handle() has been
     *  called or before the handle has been registered.
     *
     *  @remark The runtime is allowed to assume that the handle here is the same as the handle passed in during the
     *  registration process if it has the same key and same version.  This means that, e.g., calling
     *  handle->satisfy_with_data_block() on this handle must be equivalent to calling it on the one stored from
     *  an earlier registration if the key and version are the same.
     *
     *  @remark Since a publication is a read-only usage, all publishes must be invoked on a handle before this
     *  method is called (note that they need not be finished).
     *
     *  @param handle a non-owning pointer to a DependencyHandle for which register_handle() or
     *  register_fetching_handle() has been called on this instance but for which release_handle() has not
     *  yet been called.  The handle must already be properly versioned (i.e., handle->version_is_pending()
     *  returns false).
     *
     *
     */
    virtual void
    release_read_only_usage(
      const handle_t* const handle
    ) =0;


    /** @brief Release a previously registered handle, indicating that the handle will no longer be used
     *  for any method calls on this instance.
     *
     *  Releasing a handle indicates the handle has completed the modifiable phase of its life cycle, and thus
     *  the return value of handle->get_data_block() represents completion of all modifications on that DataBlock
     *  through that handle.  Thus, the DataBlock associated with a handle upon its release should be used to
     *  satisfy up to one registered \a subsequent of the handle, which must be both registered and versioned
     *  at the time of release_handle() invocation (and not yet satisfied, writable, or released).  The subsequent
     *  of a handle should be satisfied by calling subsequent->satisfy_with_data_block() with the return value of
     *  handle->get_data_block().  Subsequents of a handle may be indicated in several ways.  Given k = handle->get_key()
     *  and v = handle->get_version() for the handle parameter, the following rules may be used to determine the
     *  correct subsequent to satisfy with handle's data upon release:
     *    1. first, if a handle is registered with key k and version ++(v.push(0)), that handle is the subsequent
     *       and should be satisfied.  In this case, there must also be a handle registered with key k and version
     *       ++v which is \b not the subsequent (but debug mode should check for that handle's existence).
     *    2. If no handle with {k, ++(v.push(0))} is registered, the runtime should check for {k, ++v}.  If that
     *       handle exists, it is the subsequent.  (Further, none of the following rules should yield a subsequent,
     *       and debug mode should check for this and raise an error).
     *    3. If handle_done_with_version_depth() was called with handle as an argument before this release call, or
     *       if {k, ++v} is not found to exist (e.g., potentially it has already been released) but a handle h' with
     *       {k, ++v} was an argument ot handle_done_with_version_depth() before the release of h', the runtime should
     *       check for the existence of {k', v'} = {k, ++(v.pop())}.
     *       Then,
     *          * if {k', v'} exists, it is the subsequent
     *          * if {k', v'} does not exist but handle_done_with_version_depth() was called on a handle with it
     *            during that handle's life cycle, repeat with {k', v'} = {k', ++(v'.pop())}
     *          * otherwise, {k, v} has no subsequent.  This is also true if a {k', v'} is reached for which
     *            handle_done_with_version_depth() was called with v'.depth() == 1 (or if v'.depth() == 1 at
     *            any time in this process taht pop() would need to be called)
     *
     *  If a handle has no subsequent, the runtime should garbage collect the DataBlock associated with handle
     *  (as soon as any pending publish/fetch interactions complete, see remark below).
     *
     *  @remark this may be invoked before all publishes are *completed* (though they still have to be invoked
     *  before release_read_only_usage() is called), but only if there is to be no modify usage of the handle
     *
     *  @remark the runtime may safely assume that any subsequent candidates will be in a versioned state at
     *  the time of release_handle() invocation.  In fact, as of the 0.2 spec, they must have been handles
     *  registered with register_handle() and not register_fetching_handle()
     *
     *  @remark The runtime is allowed to assume that the handle here is the same as the handle passed in during the
     *  registration process if it has the same key and same version.  This means that, e.g., calling
     *  handle->satisfy_with_data_block() on this handle must be equivalent to calling it on the one stored from
     *  an earlier registration if the key and version are the same. [This requirement may change in the future
     *  to facilitate work stealing and other migrations)
     *
     *  @remark Even though the handle must be the same, the version \b increment behavior may be different
     *  from that of the version when the handle was registered.  The version must still return true for the
     *  equals comparison to the registered version and they must hash to the same value.  However, because
     *  the frontend is allowed to append zeros to the version to indicate its desired increment behavior,
     *  the backend should determine subsequents from the version returned by calling handle->get_version()
     *  on the handle given as a parameter here.  Since this handle must be the same object as the one registered,
     *  this is only an issue if the raw version captured at the time of registration is somehow used here,
     *  which should not be done anyway since it is cheaper to store the handle pointer and the frontend must
     *  guarantee that the handle pointer is valid until after release_handle() returns.
     *
     *  @param handle A (non-owning) pointer to the same object with which Runtime::register_handle() (or
     *  register_fetching_handle()) was previously invoked.  Any frontend uses of \c handle
     *  after release_handle() returns are invalid and result in debug-mode errors (undefined behavior is
     *  allowed in optimized mode).  Furthermore, the pointer to the handle itself is not valid after release_handle()
     *  returns, so if there are pending publishes (arising from a lack of modify usage, as remarked above),
     *  the runtime must retrieve the data block before returning (furthermore, if handle is not satisfied because
     *  it had no read uses other than the publish(), the runtime will need to track any satisfactions that would
     *  satisfy handle and use the data block from that satisfaction to fulfill corresponding fetches on the
     *  pending publishes of handle).
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

    /** @brief Indicate that no further subversions with key handle->get_key() at the current version
     *  depth will be created.
     *
     *  Essentially, this establishes a satisfy-upon-release relationship between \c handle and a handle
     *  with the subsequent of the penultimate subversion of handle.  See \ref release_handle() for
     *  more details.
     *
     */
    virtual void
    handle_done_with_version_depth(
      const handle_t* const handle
    ) =0;

    /** @brief Indicate to the backend that the key and version reported by \c handle should be fetchable
     *  with the user version tag \c version_tag exactly \c n_additional_fetchers times.
     *
     *  In other words, \ref Runtime::register_fetching_handle() must be called exactly \c n_fetchers times
     *  \b globally with the key reported by \c handle and the \c version_tag given here before the runtime
     *  can overwrite or delete the data associated with the key and version reported by \c handle.
     *
     *  @remark All publish_handle() calls must be made for a given handle before release_read_only_usage()
     *  is called with that handle.  However, not all fetches need to be completed before release_read_only_usage()
     *  or even release_handle() is called.  See details in release_handle()
     *
     *  @param handle A (non-owning) pointer to a DependencyHandle registered with register_handle() but
     *  not yet released with release_handle().
     *  @param version_tag A user-space tag to be associated with the internal version reported by handle
     *  and to be fetched as such
     *  @param n_fetchers The number of times register_fetching_handle() must be called globally (and the corresponding
     *  fetching handles released) before antidependencies on \c handle are cleared and its data can be
     *  overwritten or deleted.
     *  @param is_final Whether or not the publish is intented to indicate the key and data associated with
     *  handle are to be considered globally read-only for the rest of its lifetime.  If true, it is a (debug-mode)
     *  error to register a handle (anywhere) with the same key and a version v > handle->get_version().  For
     *  version 0.2 of the spec, is_final should always be false
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

    ///**
    // * @todo Document this for 0.3(?) spec
    // */
    //virtual void
    //satisfy_handle(
    //  handle_t* const to_fill,
    //  bool needs_write_access = false
    //) =0;

    // Methods for establishing containment and/or aliasing relationships

    ///**
    // * @todo Document this for 0.3 spec
    // */
    //virtual void
    //establish_containment_relationship(
    //  const handle_t* const inner_handle,
    //  const handle_t* const outer_handle,
    //  containment_manager_t const& manager
    //) =0;

    ///**
    // * @todo Document this for 0.3 spec
    // */
    //virtual void
    //establish_aliasing_relationship(
    //  const handle_t* const handle_a,
    //  const handle_t* const handle_b,
    //  aliasing_manager_t const& manager
    //) =0;

    /** @brief signifies the end of the outer SPMD task from which darma_backend_initialize() was called
     *
     *  @remark Note that after finalize() returns, the only valid methods that may be invoked on this instance are
     *  release_read_only_usage(), release_handle(), and handle_done_with_version_depth().  No handle released
     *  after finalize() returns may have a subsequent.  However, when finalize is \b invoked, there may still
     *  be pending tasks that schedule other tasks (the frontend has no way to know this), and thus any method on
     *  this instance must be valid to call \b between the invocation and return of finalize()
     */
    virtual void
    finalize() =0;


    virtual ~Runtime() noexcept = default;

};


typedef Runtime<
  darma_runtime::types::key_t,
  darma_runtime::types::version_t,
  darma_runtime::types::shared_ptr_template,
  darma_runtime::types::unique_ptr_template
> runtime_t;


/** @brief initialize the backend::Runtime instance
 *
 *  @remark This should be called once per top-level task.  The backend may chose whether
 *  the frontend is allowed to have multiple top-level tasks in one process.  If the backend
 *  supports multiple top-level tasks, it should define the preprocessor constant
 *  DARMA_BACKEND_TOP_LEVEL_TASK_MULTIPLE.  If not, it should define the constant
 *  DARMA_BACKEND_TOP_LEVEL_TASK_SINGLE.  The frontend is free to wrap these macro constants
 *  in more user-friendly names before exposing them to the user.
 *
 *  @pre The frontend should do nothing that interacts with the backend before this
 *  function is called.
 *
 *  @post Upon return, the (output) parameter backend_runtime should point to a valid
 *  instance of backend::Runtime.  All methods of backend_runtime should be invocable
 *  from any thread with access to the pointer backend_runtime until that thread returns
 *  from a call to Runtime::finalize() on that instance.  The runtime should assume
 *  control of the (owning) pointer passed in through top_level_task and should not delete
 *  it before Runtime::finalize() is invoked on the backend_runtime object.  The top-level
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
 *  task context within which darma_backend_initialize() was invoked.  (Inside of any task context
 *  created by an invocation of Task::run(), of course, the runtime should still behave as documented
 *  in the Runtime::get_running_task() method).  It is \a not valid to call Task::run() on this
 *  top-level task object [test: DARMABackendInitialize.top_level_run_not_called] (i.e., it is an error),
 *  and doing so will cause the frontend to abort.  Indeed (as of 0.2 spec), the only valid methods for
 *  the backend to call on this object are
 *  Task::set_name() and Task::get_name().  At least before returning top_level_task from any calls to
 *  Runtime::get_running_task(), the backend runtime should assign a name to the top-level task
 *  with at least three parts, the first three of which must be: a string constant defined by the
 *  preprocessor macro DARMA_BACKEND_SPMD_KEY_PREFIX, a std::size_t for the rank of the SPMD-like
 *  top-level task from which initialize was invoked, and a std::size_t givin the total number of
 *  ranks in the SPMD launch (which must be known at launch time; SPMD ranks cannot be dynamically
 *  allocated) [test: DARMABackendInitialize.rank_size].  In other words, the backend should make a call of the form:
 *    top_level_task->set_name(DARMA_BACKEND_SPMD_NAME_PREFIX, rank, size);
 *
 *  @remark The backend is free to extract backend-specific command-line arguments provided it
 *  updates argc and argv.  When darma_backend_initialize() returns, backend-specific parameters
 *  must no longer be in argc and argv.
 *
 */
extern void
darma_backend_initialize(
  int& argc, char**& argv,
  runtime_t*& backend_runtime,
  types::unique_ptr_template<typename runtime_t::task_t> top_level_task
);


} // end namespace backend

} // end namespace abstract

} // end namespace darma_runtime




#endif /* SRC_ABSTRACT_BACKEND_RUNTIME_H_ */

/**
 * attic
 *      *    If the
     *  dependency requires readable data to be marked as satisfied (i.e., if \c t.needs_read_data(handle)
     *  returns true when handle is returned as part of Task::get_dependencies() for a registered task \c t),
     *  the readable data requirement must be satisfied by the release of all previous versions of handle
     *  \a and a call to handle_done_with_version_depth() on a handle with a penultimate subversion that is
     *  exactly 1 increment prior to the final subversion of \c handle and is at the same depth.  See
     *  Runtime::handle_done_with_version_depth() for more details.
 *
     *   and for
     *  which \ref DependencyHandle::satisfy_with_data_block() has not yet been called by the backend.  (The only
     *  way to ensure the latter of these conditions is to ensure it is not in the return value of
     *  Task::get_dependencies() for any task that Runtime::register_task() has been invoked with).
     *  This method
     *  may be called more than once globally with the same key and the same version, but it is a debug-mode
     *  error to register more than one task (\b globally) that returns true for Task::needs_write_data() on
     *  that handle.  (Undefined behavior is still allowed in optimized mode)
     *  Subsequents to the handle may be created with handle versions in similarly pending states.
     *
     *  All handles can be used in a write context (i.e., be part of the return for Task::get_dependencies()
     *  for a task that returns true for needs_write_data() on that handle) at *most* once in their lifetime
     *  (from register_handle()/register_fetching_handle() to release_handle()).  This (up to) one "final"
     *  usage must run after all read-only uses of the handle; however not all read-only uses of
     *  a handle need to be registered by time the task making the "final" usage is registered.  Thus,
     *  the frontend needs to indicate when no more read-only usages will be created, thus allowing
     *  the "final" usage to clear its antidependencies.
     *
     *  This method indicates that no more tasks will be registered that use handle in a read context
     *  but not in a write context (a maximum of one task may use handle in a write context anyway).
     *  The frontend may choose to call this after all tasks making read uses have *finished*, but this
     *  is not a requirement currently.  It must be called for all handles before release_handle() is
     *  called.  It is an error to call release_handle() on a handle before calling release_read_only_usage().
     *
     *  Upon release, if the handle has been published with Runtime::publish_handle() or if the handle was registered
     *  as the last subversion at a given depth using Runtime::handle_done_with_version_depth(), the
     *  appropriate action(s) should be taken to satisfy dependencies requiring data from the data block
     *  in \c handle (i.e., the return value of handle->get_data_block()).  If neither of these uses have
     *  been performed at the time of release, it is safe for the runtime to garbage collect the data block
     *  returned by handle->get_data_block().
     *
     *  subsequent is a handle registered with key k and version ++v.  If this subsequent exists, the backend
     *  should satisfy it (i.e., using satisfy_with_data_block()) with the value returned by handle->get_data_block(),
     *  and no further action is required (in debug mode, the backend should raise an error if subsequents are
     *  indicated by any of the other means).  If this subsequent does not exist, the backend should check
     *  if the handle has been registered as the last at its version depth (i.e., if the frontend has called
     *  handle_done_with_version_depth() before calling release_handle()).  If
     *
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
     *  @todo 0.2.0.1 satisfied needs to be changed to writable
     *  The behavior also depends on whether or not handle is satisfied (i.e., handle->is_satisfied() returns true)
     *  at the time of the publish_handle() invocation.  If it is satisfied, the runtime publishes the
     *  actual data associated with the data block returned by handle->get_data_block().  This must happen
     *  immediately, before returning from this method, since the user is free to modify the data afterwards,
     *  which would lead to undefined beahvior.  If the handle is not satisfied, the runtime should enqueue
     *  a publication of the handle upon satisfaction.  The latter should return immediately.
 */
