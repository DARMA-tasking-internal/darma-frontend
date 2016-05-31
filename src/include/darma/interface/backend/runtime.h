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
#include <darma/interface/frontend/types.h>

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

    typedef frontend::Task task_t;

    /** @brief Register a task to be run at some future time by the runtime system.
     *
     *  See abstract::frontend::Task for details
     *
     *  @sa abstract::frontend::Task
     */
    virtual void
    register_task(
      types::unique_ptr_template<frontend::Task>&& task
    ) = 0;

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
    *
    *  @sa abstract::frontend::Task
    */
    virtual abstract::frontend::Task*
    get_running_task() const = 0;

    /** @brief Register a Use object
     *
     *  This method registe a Use object to be used later as a possible dereference of
     *  the iterator to the iterable returned by t.get_dependencies() for some task t registered
     *  after this invocation of register_use() but before the corresponding
     *  release_use().  This also indicates the beginning of the window in which make_next()
     *  may be called on either the get_input_flow() or the get_output_flow() returns from the argument
     *  (which also closes with the corresponding call to release_use()).
     */
    virtual void
    register_use(
      frontend::Use* u
    ) =0;

    /** @brief Make an initial Flow to be associated with the handle given as an argument.
     *
     *  The initial Flow will be used as the return value of ha->get_in_flow() for the first
     *  Use* ha registered with write privileges that returns handle for ha->get_handle()
     *  (or any other handle with an equivalent return for get_key() to the one passed in here)
     */
    virtual Flow*
    make_initial_flow(
      frontend::Handle* handle
    ) =0;

    /** @brief Make an fetching Flow to be associated with the handle given as an argument.
     *
     *  The fetching usage will be used as a return value of ha->get_in_flow() for a Use*
     *  ha intended to fetch the data published from a Flow associated with handle and published with
     *  the version_key given in the second argument.
     *
     */
    virtual Flow*
    make_fetching_flow(
      frontend::Handle* handle,
      types::key_t const& version_key
    ) =0;

    /** @brief Make an null Flow to be associated with the handle given as an argument.
     *
     *  A null usage as a return value of ha->get_out_flow() for some Use* ha is
     *  intended to indicate that the data associated with that Use has no consumers
     *  and can safely be deleted.  See release_use().
     *
     */
    virtual Flow*
    make_null_flow(
      frontend::Handle* handle
    ) =0;

    /**
     *  @TODO document this
     */
    typedef enum FlowPropagationPurpose {
      Input,
      Output,
      ForwardingChanges,
      OutputFlowOfReadOperation
    } flow_propagation_purpose_t;

    /**
     *  @TODO document this
     */
    virtual Flow*
    make_same_flow(
      Flow* from,
      flow_propagation_purpose_t purpose
    ) =0;

    /**
     *  @TODO document this
     */
    virtual Flow*
    make_forwarding_flow(
      Flow* from,
      flow_propagation_purpose_t purpose
    ) =0;

    /**
     *  @TODO document this
     */
    virtual Flow*
    make_next_flow(
      Flow* from,
      flow_propagation_purpose_t purpose
    ) =0;


    /** @brief Release a Use object previously registered with register_use.
     *
     *  Upon release, if the Use has at least immediate_permissions() of Write, the data
     *  in the location indicated by ha->get_data_pointer_reference() with size and layout characteristics
     *  described by ha->get_handle()->get_serialization_manager() is the data that should be used for
     *  any Use objects reporting immediate_permissions of Read (or greater) and reporting
     *  a get_in_flow() that is the same as (by Flow sameness rules, see Flow documentation) or aliases (see below)
     *  the Flow reported by ha->get_out_flow().  If the return value of ha->get_out_flow() is the same as
     *  or aliases a Flow created with make_null_flow() at the time release_use() is invoked,
     *  the data at this location may be safely deleted.
     *
     *  A Flow alias between ha->get_in_flow() and ha->get_out_flow() is established when a Use ha
     *  for which an immediate Write use descendent was never created and for which no Use ha2 has been released
     *  such that ha2->get_out_flow() is the same or aliases ha->get_in_flow().  The first of these is equivalent to
     *  the condition that  make_next() was never called on ha->get_in_flow() in the lifetime of ha.   (TODO check the
     *  previous statement).  Alias resolution should be implemented in constant time.  That is, if Flow a aliases b
     *  and Flow b aliases c, the fact that a aliases c should be discernible without linear cost in the size of the
     *  set {a, b, c}.
     *
     *  The evaluation of release_use() on a Use ha1 must be atomic with respect to the evaluation
     *  of release_use() on another Use ha2 if either could establish an alias on the other's
     *  get_out_flow() return value (this may also include get_in_flow() in future versions of the spec).  That is,
     *  if the release of ha1 could establish an alias on the Flow returned by ha2.get_out_flow(), then the evaluation
     *  of release_use(ha1) and release_use(ha2) must be atomic with respect to each other (whichever
     *  evaluation starts first must finish before the other starts).
     *
     */
    virtual void
    release_use(
      frontend::Use* u
    ) =0;


    /** @brief Indicate that the state of a Handle corresponding to a given Flow should
     *  be accessible via a corresponding fetching usage with the same version_key.
     *
     *  See PublicationDetails for more information
     *
     *  @sa PublicationDetails
     */
    virtual void
    publish_flow(
      Flow* f,
      frontend::PublicationDetails* details
    ) =0;

    /** @brief Called on a usage that was used with publish_flow() (i.e., instead of being returned
     *  by a Use instance's get_in_flow() or get_out_flow() methods)
     *
     *  See life cycle of Flow for more.
     *
     *  @sa Flow
     */
    virtual void
    release_published_flow(
      Flow* f
    ) =0;

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
