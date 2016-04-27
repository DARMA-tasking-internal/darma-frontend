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

namespace darma_runtime {
namespace abstract {
namespace backend {

/**
 *  @ingroup abstract
 *  @class Runtime
 *  @brief Abstract class implemented by the backend containing much of the
 *  runtime.
 *
 *  @note Thread safety of all methods in this class should be handled by the
 *  backend implementaton; two threads must be allowed to call any method in
 *  this class simultaneously.
 *
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
    //typedef abstract::frontend::ReduceOp reduce_op_t;

    typedef abstract::frontend::Task<
      Key, Version, types::handle_container_template
    > task_t;
    typedef unique_ptr_template<task_t> task_unique_ptr;
    typedef unique_ptr_template<const task_t> task_const_unique_ptr;

    /** @brief Enqueue a task that should run when its dependencies are satisfied.
     *
     *  @pre At the time of this call, it must be valid to call
     *  task->get_dependencies().  All DependencyHandles returned by
     *  task->get_dependencies() must be registered with this instance of
     *  Runtime using either register_handle() or register_fetching_handle() and
     *  must not have been released yet using release_handle().  It must be
     *  valid to call needs_read_data() and needs_write_data() on \c task with
     *  any of the handles returned by get_dependencies().  At least by the time
     *  that DependencyHandle::is_satisfied() returns true (and h->is_writable()
     *  returns true for DependencyHandle objects h for which
     *  task->needs_write_data(h) returns true) for all DependencyHandle objects
     *  returned by task->get_dependencies(), it must be valid to call
     *  task->run().
     *
     *  @post \a task may be invoked at any subsequent time pending the
     *  satisfaction of the handles returned by Task::get_dependencies().  If
     *  the frontend holds a pointer or reference to the task after this method
     *  is invoked, use of that object will result in undefined behavior.
     *
     *  @remark The handles returned by task->get_dependencies() \b may be
     *  released before task->run() returns.  The registration restriction
     *  applies to the invocation of register_task().  However, as of the 0.2
     *  spec, the frontend may not release any of the handles returned by
     *  task->get_dependencies() at least until the backend calls task->run().
     *  [The previous sentence will likely be changed in a later version of the
     *  spec.]
     *
     *  @param task A unique pointer to the task to register and enqueue,
     *  meeting the preconditions described above.  The runtime backend now
     *  owns the task object and is responsible for deleting it after the task
     *  has finished running.  Any other references or pointers to \ref task are
     *  considered invalid after this method is invoked.
     *
     */
    virtual void
    register_task(
      task_unique_ptr&& task
    ) =0;

    /** @brief Get a pointer to the \ref abstract::frontend::Task object
     *  currently running on the thread from which get_running_task() was
     *  invoked.
     *
     *  @return A non-owning pointer to the \ref abstract::frontend::Task object
     *  running on the invoking thread.  The returned pointer must be castable
     *  to the same concrete type as was passed to \ref Runtime::register_task()
     *  when the task was registered.
     *
     *  @remark If the runtime implements context switching, it must ensure that
     *  the behavior of Runtime::get_running_task() is consistent and correct
     *  for a given running thread as though the switching never occurred.
     *
     *  @remark The pointer returned here is guaranteed to be valid until
     *  Task::run() returns for the returned task.  However, to allow context
     *  switching, it is not guaranteed to be valid in the context of any other
     *  task's run() invocation, including child tasks, and thus it should not
     *  be dereferenced in any other context.
     */
    virtual task_t* const
    get_running_task() const =0;

    // Methods for creating handles and registering fetches of those handles

    /** @brief Register a local handle to a data block.
     *
     *  Register a handle that can be used as a part of the return value of
     *  Task::get_dependencies() for tasks registered on this runtime instance.
     *  Note that the *only* allowed return values of get_dependencies() for a
     *  task passed to register_task() on this instance are values for which
     *  register_handle() (or register_fetching_handle()) has been called and
     *  release_handle() has not yet been called.
     *
     *  @param handle A (non-owning) pointer to a
     *  \ref abstract::frontend::DependencyHandle for which it is valid to call
     *  handle->get_key() and handle->get_version().  register_handle() must not
     *  be called more than once for a given key and version.  The pointer
     *  passed as this parameter must be valid from the time register_handle()
     *  is called until the time release_handle() is called (on this instance!)
     *  with a handle to the same key and version.
     *
     *  @remark In terms of handle life cycle, the handle passed method is
     *  always in a versioned state by the time register_handle() returns
     *  (unlike register_fetching_handle()).  Specifically, the version to be
     *  used is the version returned by handle->get_version(), and
     *  handle->version_is_pending() must return false if handle is a valid
     *  parameter to this method.
     *
     *  @sa Task::needs_read_data()
     *  @sa Runtime::register_task()
     */
    virtual void
    register_handle(
      handle_t* const handle
    ) =0;

    /**
      Performance an all-reduce operation over a given input data and store
      into a new output dependency handle
      @param input_key
    */
    //virtual void
    //allreduce(
    //  key_t* input_key,
    //  handle_t* const input,
    //  handle_t* const outpt,
    //  int chunkNumber,
    //  int begin,
    //  int end,
    //  allreduce_op_t* op
    //) = 0;


    /** @brief Register a dependency handle that is satisfied by retrieving data
     *  from the data store.
     *
     *  The handle may then be used preliminarily as a dependency to tasks, etc.
     *  Upon return, the handle's state in its lifecycle may be *either*
     *  registered (i.e., unversioned) or a versioned.  The return value of
     *  handle->get_version() is ignored upon invocation and
     *  handle->version_is_pending() must return true upon invocation.  At some
     *  point after this invocation but before the first task depending on
     *  handle is run (which means potentially even before this method
     *  returns), handle->set_version() must be called exactly once to update
     *  the handle's version to the version with which it was published
     *  (potentially remotely).
     *
     *  @param handle A (non-owning) pointer to a DependencyHandle for which it
     *  must be valid to call get_key() but for which the return value of
     *  get_version() is ignored.
     *
     *  @param user_version_tag A Key to be used to match a publication version
     *  tag of a publish (potentially elsewhere) with a handle that reports the
     *  same key as handle->get_key().
     *
     */
    virtual void
    register_fetching_handle(
      handle_t* const handle,
      const Key& user_version_tag
    ) =0;

    /** @brief Indicate that the last task holding read-only usage or the
     *  ability to schedule other tasks with read-only priviledges has
     *  completed or explicitly released the handle.
     *
     *  After this call is made, the runtime is free to schedule the up to one
     *  modify usage (also known as "final" usage) of the handle, which may be
     *  registered before this method is called or at any time before
     *  release_handle() is called.
     *
     *  @remark It is a debug-mode error to call release_read_only_usage() on a
     *  handle after release_handle() has been called or before the handle has
     *  been registered.
     *
     *  @remark The runtime is allowed to assume that the handle here is the
     *  same as the handle passed in during the registration process if it has
     *  the same key and same version.  This means that, e.g., calling
     *  handle->satisfy_with_data_block() on this handle must be equivalent to
     *  calling it on the one stored from an earlier registration if the key and
     *  version are the same.
     *
     *  @remark Since a publication is a read-only usage, all publishes must be
     *  invoked on a handle before this method is called (note that they need
     *  not be finished).
     *
     *  @param handle a non-owning pointer to a DependencyHandle for which
     *  register_handle() or register_fetching_handle() has been called on this
     *  instance but for which release_handle() has not yet been called.  The
     *  handle must already be properly versioned (i.e.,
     *  handle->version_is_pending() returns false).
     *
     */
    virtual void
    release_read_only_usage(
      handle_t* const handle
    ) =0;

    /** @brief Release a previously registered handle, indicating that the
     *  handle will no longer be used for any method calls on this instance.
     *
     *  Releasing a handle indicates the handle has completed the modifiable
     *  phase of its life cycle, and thus the return value of
     *  handle->get_data_block() represents completion of all modifications on
     *  that DataBlock through that handle.  Thus, the DataBlock associated with
     *  a handle upon its release should be used to satisfy up to one registered
     *  \a subsequent of the handle, which must be both registered and versioned
     *  at the time of release_handle() invocation (and not yet satisfied,
     *  writable, or released).  If the handle is satisfied upon its release,
     *  the subsequent should be satisfied by calling
     *  subsequent->satisfy_with_data_block() with the return value of
     *  handle->get_data_block().
     *
     *  Given k = handle->get_key() and v = handle->get_version() for the handle
     *  parameter, the following rules may be used to determine the correct
     *  subsequent to satisfy with the handle's data upon release:
     *    1. First, if a handle with key k and version ++(v.push_subversion())}
     *       is registered, that handle is the subsequent.
     *    2. If no handle with {k, ++(v.push_subversion())} is registered, but a
     *       handle with {k, ++v} is registered, then {k, ++v} is the subsequent.
     *    3. If neither {k, ++(v.push_subversion())} nor {k, ++v} is registered,
     *       then the following procedure should be followed to determine the
     *       subsequent:
     *         a. Set v'=++(v.pop_subversion()).
     *         b. If {k, v'} exists, it is the subsequent.
     *         c. If {k, v'} does not exist and v'.depth() == 1, no subsequent
     *            exists.
     *         d. Set v'=++(v'.pop_subversion()) and return to step b.
     *
     *  If a handle has no subsequent, the runtime should garbage collect the
     *  DataBlock associated with handle (as soon as any pending publish/fetch
     *  interactions complete; see remark below).
     *
     *  @remark While this (and release_read_only_usage()) must be invoked
     *  after all associated published are invoked, it is possible for this to
     *  be invoked before all publishes have \b completed if there is to be no
     *  modify usage of the handle.
     *
     *  @remark The runtime may safely assume that any subsequent candidates
     *  will be in a versioned state at the time of release_handle() invocation.
     *  In fact, as of the 0.2 spec, they must have been handles registered with
     *  register_handle() and not register_fetching_handle().
     *
     *  @remark The runtime is allowed to assume that the handle here is the
     *  same as the handle passed in during the registration process if it has
     *  the same key and same version.  This means that, e.g., calling
     *  handle->satisfy_with_data_block() on this handle must be equivalent to
     *  calling it on the one stored from an earlier registration if the key and
     *  version are the same. [This requirement may change in the future to
     *  facilitate work stealing and other migrations.]
     *
     *  @remark Even though the handle must be the same, the version
     *  \b increment behavior may be different from that of the version when the
     *  handle was registered.  The version must still return true for the
     *  equals comparison to the registered version and they must hash to the
     *  same value.  However, because the frontend is allowed to append zeros to
     *  the version to indicate its desired increment behavior, the backend
     *  should determine subsequents from the version returned by calling
     *  handle->get_version() on the handle given as a parameter here.  Since
     *  this handle must be the same object as the one registered, this is only
     *  an issue if the raw version captured at the time of registration is
     *  somehow used here, which should not be done anyway since it is cheaper
     *  to store the handle pointer and the frontend must guarantee that the
     *  handle pointer is valid until after release_handle() returns.
     *
     *  @remark Note that this specification implies, e.g., that
     *  release_handle() may be called with, e.g., a handle with {k, 1.1.1.2}
     *  before a handle with {k, 1.1.1.1}.  This is absolutely the case,
     *  although currently the only example of that pattern involves
     *  {k, 1.1.1.2} having no read or modify usage, aside from perhaps a
     *  publish (read) usage.  As the specification evolves, more situations
     *  like this may arise, though if they don't this pattern may be
     *  consigned to its own special method.
     *
     *  @param handle A (non-owning) pointer to the same object with which
     *  Runtime::register_handle() (or register_fetching_handle()) was
     *  previously invoked.  Any frontend uses of \c handle after
     *  release_handle() returns are invalid and result in debug-mode errors
     *  (undefined behavior is allowed in optimized mode).  Furthermore, the
     *  pointer to the handle itself is not valid after release_handle()
     *  returns, so if there are pending publishes (arising from a lack of
     *  modify usage, as remarked above), the runtime must retrieve the data
     *  block before returning (furthermore, if handle is not satisfied because
     *  it had no read uses other than the publish(), the runtime will need to
     *  track any satisfactions that would satisfy handle and use the data block
     *  from that satisfaction to fulfill corresponding fetches on the pending
     *  publishes of handle).
     *
     *  @sa DependencyHandle::get_data_block()
     *  @sa Runtime::publish_handle()
     *
     */
    virtual void
    release_handle(
      const handle_t* const handle
    ) =0;

    /** @brief Indicate to the backend that the key and version reported by
     *  \c handle should be fetchable with the user version tag \c version_tag
     *  exactly \c n_fetchers times.
     *
     *  In other words, \ref Runtime::register_fetching_handle() must be called
     *  exactly \c n_fetchers times \b globally with the key reported by
     *  \c handle and the \c version_tag given here before the runtime can
     *  overwrite or delete the data associated with the key and version
     *  reported by \c handle.
     *
     *  @remark All publish_handle() calls must be made for a given handle
     *  before release_read_only_usage() is called with that handle.  However,
     *  not all fetches need to be completed before release_read_only_usage()
     *  or even release_handle() is called.  See details in release_handle().
     *
     *  @param handle A (non-owning) pointer to a DependencyHandle registered
     *  with register_handle() but not yet released with release_handle().
     *  @param version_tag A user-space tag to be associated with the internal
     *  version reported by handle and to be fetched as such.
     *  @param n_fetchers The number of times register_fetching_handle() must
     *  be called globally (and the corresponding fetching handles released)
     *  before antidependencies on \c handle are cleared and its data can be
     *  overwritten or deleted.
     *  @param is_final Whether or not the publish is intended to indicate the
     *  key and data associated with handle are to be considered globally
     *  read-only for the rest of its lifetime.  If true, it is a (debug-mode)
     *  error to register a handle (anywhere) with the same key and a version
     *  v > handle->get_version().  For version 0.2 of the spec, is_final
     *  should always be false.
     *
     */
    virtual void
    publish_handle(
      handle_t* const handle,
      const Key& version_tag,
      const size_t n_fetchers = 1,
      bool is_final = false
    ) =0;

    /** @brief signifies the end of the outer SPMD task from which
     *  darma_backend_initialize() was called.
     *
     *  @remark Note that after finalize() returns, the only valid methods that
     *  may be invoked on this instance are release_read_only_usage() and
     *  release_handle().  No handle released after finalize() returns may have
     *  a subsequent.  However, when finalize is \b invoked, there may still be
     *  pending tasks that schedule other tasks (the frontend has no way to
     *  know this), and thus any method on this instance must be valid to call
     *  \b between the invocation and return of finalize().
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
 *  @remark This should be called once per top-level task.  Only one top-level
 *  task is allowed per process.
 *
 *  @pre The frontend should do nothing that interacts with the backend before
 *  this function is called.
 *
 *  @post Upon return, the (output) parameter backend_runtime should point to a
 *  valid instance of backend::Runtime.  All methods of backend_runtime should
 *  be invocable from any thread with access to the pointer backend_runtime
 *  until that thread returns from a call to Runtime::finalize() on that
 *  instance.  The runtime should assume control of the (owning) pointer passed
 *  in through top_level_task and should not delete it before
 *  Runtime::finalize() is invoked on the backend_runtime object.  The top-level
 *  task object should be setup as described below.
 *
 *  @param argc An lvalue reference to the argc passed into main().
 *  @param argv An lvalue reference to the argv passed into main().
 *
 *  @param backend_runtime The input value of this parameter is ignored.  On
 *  return, it should point to a properly initialized instance of
 *  backend::Runtime.  The backend owns this instance and should delete it
 *  after darma_main() returns, at which point Runtime::finalize() should have
 *  returned.
 *
 *  @param top_level_task The task object to be returned by
 *  Runtime::get_running_task() if that method is invoked (on the instance
 *  pointed to by backend_runtime upon return) from the outermost task context
 *  within which darma_backend_initialize() was invoked.  (Inside of any task
 *  context created by an invocation of Task::run(), of course, the runtime
 *  should still behave as documented in the Runtime::get_running_task()
 *  method).  It is \a not valid to call Task::run() on this top-level task
 *  object [test: DARMABackendInitialize.top_level_run_not_called] (i.e., it is
 *  an error), and doing so will cause the frontend to abort.  Indeed (as of 0.2
 *  spec), the only valid methods for the backend to call on this object are
 *  Task::set_name() and Task::get_name().  At least before returning
 *  top_level_task from any calls to Runtime::get_running_task(), the backend
 *  runtime should assign a name to the top-level task with at least three
 *  parts, the first three of which must be: a string constant defined by the
 *  preprocessor macro DARMA_BACKEND_SPMD_KEY_PREFIX, a std::size_t for the rank
 *  of the SPMD-like top-level task from which initialize was invoked, and a
 *  std::size_t givin the total number of ranks in the SPMD launch (which must
 *  be known at launch time; SPMD ranks cannot be dynamically allocated)
 *  [test: DARMABackendInitialize.rank_size].  In other words, the backend
 *  should make a call of the form:
 *  top_level_task->set_name(DARMA_BACKEND_SPMD_NAME_PREFIX, rank, size);
 *
 *  @remark The backend is free to extract backend-specific command-line
 *  arguments provided it updates argc and argv.  When
 *  darma_backend_initialize() returns, backend-specific parameters must no
 *  longer be in argc and argv.
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


/** @brief Calls the application-provided darma_main() and then deletes the
 *  backend runtime before returning.
 *
 *  Invokes darma_main() via a call to the function pointer returned by
 *  darma_runtime::detail::_darma__generate_main_function_ptr<>().  After
 *  returning from darma_main(), the darma_runtime::detail::backend_runtime
 *  pointer must be deleted.  It is a debug-mode error for darma_main() to
 *  return without having invoked Runtime::finalize() if
 *  darma_backend_initialize() was also called.
 *
 */
int main(int argc, char **argv);

#endif /* SRC_ABSTRACT_BACKEND_RUNTIME_H_ */
