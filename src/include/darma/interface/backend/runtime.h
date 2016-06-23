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

#include <darma/interface/frontend/types.h>
#include <darma/interface/backend/types.h>

#include <darma/interface/backend/flow.h>
#include <darma/interface/frontend/handle.h>
#include <darma/interface/frontend/task.h>
#include <darma/interface/frontend/use.h>
#include <darma/interface/frontend/publication_details.h>

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
class Runtime {
  public:

    typedef frontend::Task<types::concrete_task_t> task_t;
    typedef types::unique_ptr_template<task_t> task_unique_ptr;

    /** @brief Register a task to be run at some future time by the runtime system.
     *
     *  See frontend::Task for details
     *
     *  @param task A unique_ptr to a task object. Task is moved as rvalue reference,
                    indicating transfer of ownership to the backend.
     *  @sa frontend::Task
     */
    virtual void
    register_task( task_unique_ptr&& task ) = 0;

    /** @TODO document this
     */
    virtual bool
    register_condition_task( task_unique_ptr&& task ) = 0;

    /** @brief Get a pointer to the \ref frontend::Task object
    *  currently running on the thread from which get_running_task() was
    *  invoked.
    *
    *  @return A non-owning pointer to the \ref frontend::Task object
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
    *
    *  @sa frontend::Task
    */
    virtual task_t*
    get_running_task() const = 0;

    /** @brief Register a frontend::Use object
     *
     *  This method registers a Use object that can be accesses through the
     *  the iterator returned by t.get_dependencies() for some task t.
     *  register_use() will always be invoked before register_task() for any task holding a Use `u`.
     *  Accessing a frontend::Use `u` through a frontend::Task `t` is only valid between the time
     *  `register_use(&u)` is called and `release_use(&u)` returns.
     *  No `make_*` functions may be invoked on either the input or output flows of a Use `u`
     *  returned by Use::get_input_flow() and Use::get_output_flow() before calling register_use().
     *  Additionally, no `make_*` functions may be invoked on the input or output flows of a Use `u`
     *  after calling release_use().
     */
    virtual void
    register_use(
      frontend::Use* u
    ) =0;

    /** @TODO document this
     */
    virtual void
    reregister_migrated_use(
      frontend::Use* u
    ) =0;

    /** @brief Make an initial Flow to be associated with the handle given as an argument.
     *
     *  The initial Flow will be used as the return value of u->get_in_flow() for the first
     *  Use* u registered with write privileges that returns handle for u->get_handle()
     *  (or any other handle with an equivalent return for get_key() to the one passed in here).
     *  In most cases, this will derive from calls to initial_access in the application code.
     *
     *  @param handle A handle encapsulating a type and unique name (variable) for which the Flow
     *                represents the initial state
     *
     */
    virtual Flow*
    make_initial_flow(
      frontend::Handle* handle
    ) =0;

    /** @brief Make a fetching Flow to be associated with the handle given as an argument.
     *
     *  The fetching usage will be used as a return value of u->get_in_flow() for a Use* u
     *  intended to fetch the data published with a particular handle key and version_key.
     *  @param handle A handle object carrying the key identifer returned by get_key()
     *  @param version_key A unique version for the key returned by handle->get_key()
     */
    virtual Flow*
    make_fetching_flow(
      frontend::Handle* handle,
      types::key_t const& version_key
    ) =0;

    /** @brief Make a null Flow to be associated with the handle given as an argument.
     *
     *  A null usage as a return value of u->get_out_flow() for some Use* u is
     *  intended to indicate that the data associated with that Use has no subsequent consumers
     *  and can safely be deleted.  See release_use().
     *  @param handle   The handle variable associate with the flow
     *
     */
    virtual Flow*
    make_null_flow(
      frontend::Handle* handle
    ) =0;

    /**
     *  @brief A set of enums identifying the relationship between two flows
     */
    typedef enum FlowPropagationPurpose {
      Input, /*!< The new flow will be used as the input to another logical Use of the data*/
      Output, /*!< The new flow will be used as the output for another logical Use of the data */
      ForwardingChanges, /*!< The new flow will be used as an input to another logical Use of the
                            data that incorporates changes made to data associated with an input Flow
                            for which Modify immediate permissions were requested, thus "forwarding"
                            the modifications to a new logical Use.
                            Only ever used with make_forwarding_flow() */
      OutputFlowOfReadOperation /*!< The new flow will be used as the corresponding return of get_out_flow()
                                    for a read-only Use that returns a given flow for get_in_flow().
                                */
    } flow_propagation_purpose_t;

    /** @brief Make a flow that is logically identical to the input parameter
     *
     * Calls to make_same_flow() indicate a logical identity between Flows in different Use instances.
     * make_same_flow() may not return the original pointer passed in. Flow objects must be unique to a Use.
     * Flows are registered and released indirectly through calls to register_use()/release_use().
     * The input Flow to make_same_flow() must have been registered through a register_use() call,
     * but not yet released through a release_use() call.
     * There is no restriction on the number of times make_same_flow() can be called with a given input.
     *
     * @param from    An already initialized and registered flow returned from `make_*_flow`
     * @param purpose An enum indicating the relationship between logically identical flows (purpose of the function).
     *                For example, this indicates whether the two flows are both inputs to different tasks or whether
     *                the new flow is the sequential continuation of a previous write (forwarding changes). This
     *                information is provided for optimization purposes
     * @return A new Flow object that is equivalent to the input flow
     */
    virtual Flow*
    make_same_flow(
      Flow* from,
      flow_propagation_purpose_t purpose
    ) =0;

    /**
     *  @brief Make a new input Flow that receives forwarded changes from another input Flow, the latter
     *  of which is associated with a Use on which Modify immediate permissions were requested.
     *
     *  Flows are registered and released indirectly through calls to register_use()/release_use().
     *  The translation layer will never share a given `Flow*` returned by the backend
     *  across multiple Use instances.
     *  The input Flow to make_forwarding_flow() must have been registered through a register_use() call,
     *  but not yet released through a release_use() call.
     *  make_forwarding_flow() can be called at most once with a given input.
     *
     *  @param from    An already initialized and registered flow returned from `make_*_flow`
     *  @param purpose An enum indicating the relationship between the flows (purpose of the function).
     *                 This information is provided for optimization purposes.
     *                 In the current specification, this enum will always be ForwardingChanges
     *  @return A new Flow object indicating that new data is the produced by immediate modifications
     *          to the data from the Flow given as a parameter
     */
    virtual Flow*
    make_forwarding_flow(
      Flow* from,
      flow_propagation_purpose_t purpose
    ) =0;

    /**
     *  @brief Make a flow that will be logically (not necessarily immediately) subsequent to another Flow
     *
     *  Calls to make_next_flow() indicate a producer-consumer relationship between Flows.
     *  make_next_flow() indicates that an operation consumes Flow* from and produces the returned Flow*.
     *  A direct subsequent relationship should not be inferred here; the direct subsequent of input flow
     * `from` will only be the output flow within the same Use if no other subsequents of the input use are
     *  created and registered in its lifetime.
     *  Flows are registered and released indirectly through calls to register_use()/release_use().
     *  Flow instances cannot be shared across Use instances.
     *  The input to make_next_flow() must have been registered with register_use(), but not yet released
     *  through release_use().
     *  make_next_flow() can be called at most once with a given input.
     *
     *  @param from    The flow consumed by an operation to produce the Flow returned by make_next_flow()
     *  @param purpose An enum indicating the purpose of the next flow.  This information is provided for
     *                 optimization purposes
     *  @return A new Flow object indicating that new data will be produced by the data incoming
     *          from the Flow given as a parameter
     */
    virtual Flow*
    make_next_flow(
      Flow* from,
      flow_propagation_purpose_t purpose
    ) =0;


    /** @brief Release a Use object previously registered with register_use().
     *
     *  Upon release, if the Use* u has immediate_permissions() of at least
     *  Write and was not propagated into a Modify context (defined below),
     *  the release allows
     *  the runtime to match the producer flow to pending Use instances where
     *  u->get_out_flow() is equivalent to the consumer pending->get_in_flow()
     *  (with equivalence for Flow defined in flow.h).  The location provided to
     *  u->get_data_pointer_reference() holds the data that satisfies the
     *  pending->get_in_flow() if the Use requested immediate permissions of Write
     *  or greater.
     *
     *  The Use* u has been propagated into a Modify context if another Use* u2
     *  has been registered where u2->get_out_flow() was constructed from
     *  either make_next(make_same(u1->get_in_flow())) or
     *  make_next(make_forwarding(u1->get_in_flow())).
     *
     *  If the Use* u has scheduling_permissions() of at least Write but no
     *  immediate permissions and was not propagated into a Modify context by
     *  the time it was released, the
     *  Use* is an "alias" use.  As such, u->get_out_flow() only provides an
     *  alias for u->get_in_flow().  u->get_in_flow() is the actual producer
     *  flow that satisfies all tasks/uses depending on u->get_out_flow().
     *  There will (almost always, unless the corresponding Handle is completely
     *  unused in its lifetime) be some other task t2 with Use* u2 such that
     *  u2->get_out_flow() and u->get_in_flow() are equivalent.  release_use(u2)
     *  may have already been called, may be in process, or may not have been
     *  called when release_use(u) is invoked. The backend runtime is
     *  responsible for ensuring correct satisfaction of pending flows and
     *  thread safety (atomicity) of release_use(...) with aliases.  An alias
     *  use can correspond to another alias use, creating a chain of aliases
     *  that the backend runtime must resolve.
     *
     *  If the return value of u->get_out_flow() is the same as or aliases a
     *  Flow created with make_null_flow() at the time release_use() is invoked,
     *  the data at this location may be safely deleted (unless the backend has
     *  some other pending operations on that data, such as unfulfilled publishes).
     *
     *  If the Use* u has immediate_permissions() of Read, the release allows
     *  the runtime to clear anti-dependencies. For a task t2 with Write
     *  privileges on Use* u2 such that u2->get_in_flow() is equivalent to
     *  u->get_in_flow() (or u->get_out_flow(), depending on backend
     *  implementation), if u is the last use (there are no other Use* objects
     *  registered with u->get_in_flow() equivalent to u2->get_in_flow()) then
     *  task t2 has its preconditions on u2 satisfied.
     *
     *  Alias resolution should be implemented in constant time.  That is, if
     *  Flow a aliases b and Flow b aliases c, the fact that a aliases c should
     *  be discernible without linear cost in the size of the set {a, b, c}.
     *
     *  @param u The Use being released, which consequently releases an in and
     *  out flow with particular permissions.
     */
    virtual void
    release_use(
      frontend::Use* u
    ) =0;


    /** @brief Indicate that the state of a Handle corresponding to a given Use should
     *  be accessible via a corresponding fetching usage with the same version_key.
     *
     *  See PublicationDetails for more information
     *  @param u       The particular use being published
     *  @param details This encapsulates at least a version_key and an n_readers
     *
     *  @sa PublicationDetails
     */
    virtual void
    publish_use(
      frontend::Use* u,
      frontend::PublicationDetails* details
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

typedef Runtime runtime_t;

/** @brief initialize the backend::Runtime instance
 *
 *  @todo update this for the new backend
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
 *  an error), and doing so will cause the frontend to abort.  Indeed,
 *  the only valid methods for the backend to call on this object are
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
  runtime_t::task_unique_ptr&& top_level_task
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
