/*
//@HEADER
// ************************************************************************
//
//                          task.h
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

#ifndef SRC_ABSTRACT_FRONTEND_TASK_H_
#define SRC_ABSTRACT_FRONTEND_TASK_H_

#include <darma/utility/config.h>
#include <darma/impl/feature_testing_macros.h>

#include <darma_types.h>
#include <darma/impl/feature_testing_macros.h>
#include <darma/interface/frontend/types.h>

#include <darma/impl/feature_testing_macros.h>

#include "use.h"

namespace darma_runtime {

namespace abstract {

namespace frontend {

class Closure {
  public:

#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    virtual std::string const&
    get_calling_function_name() const =0;
    virtual std::string const&
    get_calling_filename() const =0;
    virtual size_t
    get_calling_line_number() const =0;
#endif

    /** @brief returns the name of the task if one has been assigned with set_name(), or
     *  a reference to a default-constructed Key if not.
     *
     *  In the current spec this is only used with the outermost task, which is named with
     *  a key of one string and two size_t values: DARMA_BACKEND_SPMD_NAME_PREFIX,
     *  the SPMD rank and the SPMD size.  See darma_backend_initialize()
     *  for more information
     *
     *  @return A key object giving a unique name to the task
     *
     *  @todo >0.3.1 spec: user task naming interface
     *
     */
    virtual const types::key_t& get_name() const =0;

    /** @brief sets the unique name of the task
     *
     *  In the current spec this is only used with the outermost task, which is named with
     *  a key of one string and two size_t values: DARMA_BACKEND_SPMD_NAME_PREFIX,
     *  the SPMD rank and the SPMD size.  See darma_backend_initialize()
     *  for more information
     *
     *  @param name_key A key object containing a unique name for the task
     *  @todo >0.3.1 spec: user task naming interface
     */
    virtual void set_name(const types::key_t& name_key) =0;
};

/** @brief A piece of work that acts on (accesses) zero or more Handle objects
 *  at a particular point in the apparently sequential uses of these Handle objects.
 *
 *
 *  Life-cycle of a Task, for some Task instance t:
 *    + registration -- register_task() is called by moving a unique_ptr to t into the first argument.
 *      At registration time, all of the Use objects returned by the dereference of the iterator
 *      to the iterable returned by t.get_dependencies() must be registered and must not be released at least
 *      until the backend invokes t.run() method.
 *    + execution -- the backend calls t.run() once all of the dependent Uses have their required
 *      permissions to their data.  By this point (and not necessarily sooner), the backend must have assigned
 *      the return of get_data_pointer_reference() to the beginning of the actual data for any Use
 *      dependencies requiring immediate permissions.
 *    + release -- when Task.run() returns, the task is ready to be released.  The backend may do this by deleting
 *      or resetting the unique_ptr passed to it during registration, which will in turn trigger the ~Task() virtual
 *      method invocation.  At this point (in the task destructor), the frontend is responsible for calling
 *      release_use() on any Use instances requested by the task and not explicitly released in the
 *      task body by the user.
 *
 */
class Task
#if _darma_has_feature(task_migration)
  : public PolymorphicSerializableObject<Task>,
#endif
    public Closure
{
  public:

    /** @brief Return an Iterable of Use objects whose permission requests must be satisfied before the task
     *  can run.
     *
     *  See description in Task and Use life cycle discussions.
     *  @return An iterable container of Use objects whose availability are preconditions for task execution
     */
    virtual types::handle_container_template<DependencyUse*> const&
    get_dependencies() const =0;

    /** @brief Invoked by the backend to start the execution phase of the task's life cycle.
     */
    virtual void run() =0;

    //==========================================================================

#if _darma_has_feature(task_migration)
    /** @brief returns true iff the task can be migrated
     *
     *  @remark always return false in the current spec implementation.  Later specs will need
     *  additional hooks for migration
     *  @return Whether the task is migratable.
     */
    virtual bool is_migratable() const =0;

#endif // _darma_has_feature(task_migration)

    //==========================================================================

#if _darma_has_feature(create_parallel_for)
    virtual size_t width() const { return 1; }

#if _darma_has_feature(create_parallel_for_custom_cpu_set)

    //virtual darma_runtime::types::resource_pack_t const& get_resource_pack() const =0;

    virtual void set_resource_pack(darma_runtime::types::resource_pack_t const&) =0;

#endif

    /** @brief Indicates whether the task represents a parallel_for loop
     *
     * @return True if and only if this task directly wraps a parallel_for created via
     * darma_runtime::backend::execute_parallel_for()
     */
    virtual bool is_parallel_for_task() const =0;


#endif // _darma_has_feature(create_parallel_for)

#if _darma_has_feature(mark_parallel_tasks)
    virtual bool is_data_parallel_task() const =0;
#endif

    //==========================================================================

#if _darma_has_feature(resilient_tasks)
    virtual bool is_replayable() const =0;
#endif

    virtual ~Task() = default;

};

} // end namespace frontend

} // end namespace abstract

namespace frontend {

std::unique_ptr<abstract::frontend::Task>
make_running_task_to_return_when_unpacking();

} // end namespace frontend

} // end namespace darma_runtime



#endif /* SRC_ABSTRACT_FRONTEND_TASK_H_ */
