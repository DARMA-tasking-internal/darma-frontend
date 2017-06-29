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

#include <darma/interface/frontend/frontend_fwd.h>

#include <darma/interface/frontend/types.h>

#include <darma/interface/backend/flow.h>
#include <darma/interface/frontend/handle.h>
#include <darma/interface/frontend/task.h>

#include <darma/interface/frontend/publication_details.h>
#include <darma/interface/frontend/memory_requirement_details.h>
#include <darma/interface/frontend/collective_details.h>
#include <darma/interface/frontend/top_level_task.h>
#include <darma/interface/frontend/task_collection.h>

#include <darma/impl/feature_testing_macros.h>

#include "backend_fwd.h"

namespace darma_runtime {

namespace abstract {

namespace backend {

class Runtime {
  public:

    using task_t = frontend::Task;
    using task_unique_ptr = types::unique_ptr_template<task_t>;
    using handle_t = frontend::Handle;
    using top_level_task_t = abstract::frontend::TopLevelTask<types::concrete_task_t>;
    using top_level_task_unique_ptr = std::unique_ptr<top_level_task_t>;
    using use_t = frontend::Use;
    using dependency_use_t = frontend::DependencyUse;
    using registered_use_t = frontend::RegisteredUse;
    using collection_managing_use_t = frontend::CollectionManagingUse;
    using destructible_use_t = frontend::DestructibleUse;
    using use_pending_release_t = frontend::UsePendingRelease;
    using use_collection_t = abstract::frontend::UseCollection;
    using pub_details_t = darma_runtime::abstract::frontend::PublicationDetails;
    using collective_details_t = darma_runtime::abstract::frontend::CollectiveDetails;
    using memory_details_t = abstract::frontend::MemoryRequirementDetails;
    using task_collection_t = abstract::frontend::TaskCollection;
    using task_collection_unique_ptr = std::unique_ptr<task_collection_t>;
    using task_collection_task_unique_ptr = std::unique_ptr<types::task_collection_task_t>;

    //==========================================================================

    virtual size_t get_execution_resource_count(size_t depth) const =0;

    //==========================================================================
    // <editor-fold desc="Task and TaskCollection handling">

    /** @brief Register a task to be run at some future time by the runtime
     * system.
     *
     *  See frontend::Task for details
     *
     *  @param task A unique_ptr to a task object. Task is moved as rvalue
     *  reference, indicating transfer of ownership to the backend.
     *
     *  @sa frontend::Task
     */
    virtual void
    register_task(task_unique_ptr&& task) = 0;

#if _darma_has_feature(create_concurrent_work)
    virtual void
    register_task_collection(
      task_collection_unique_ptr&& collection
    ) =0;
#endif // _darma_has_feature(create_concurrent_work)

    // </editor-fold> end Task handling
    //==========================================================================

    //==========================================================================
    // <editor-fold desc="Use handling">

    /** @brief Register a frontend::Use object
     *
     *  This method registers a Use object that can be accesses through the
     *  iterator returned by t.get_dependencies() for some task t.
     *  register_use() will always be invoked before register_task() for any
     *  task holding a Use `u`. Accessing a frontend::Use `u` through a
     *  frontend::Task `t` is only valid between the time `register_use(&u)` is
     *  called and `release_use(&u)` returns. No `make_*` functions may be
     *  invoked on either the input or output flows of a Use `u` returned by
     *  Use::get_input_flow() and Use::get_output_flow() before calling
     *  register_use(). Additionally, no `make_*` functions may be invoked on
     *  the input or output flows of a Use `u` after calling release_use().
     */
    virtual void
    register_use(
      frontend::UsePendingRegistration* u
    ) =0;

#if _darma_has_feature(task_migration)
    /** @TODO document this
     *  @remark on entry, the in_flow and out_flow of `u` already be set up
     *  via calls to make_unpacked_flow() on the buffers created by pack_flow()
     *  on the sender side
     */
    // TODO this should work the same way as register_use(), execpt that the
    // flows are described as Packed in the FlowRelationship
    virtual void
    reregister_migrated_use(
      frontend::RegisteredUse* u
    ) =0;
#endif

    // TODO re-enable this if/when it's used on the frontend?
    //virtual void
    //register_use_copy(
    //  frontend::RegisteredUse* u
    //) {
    //    // By default, just pass through to register_use()
    //    return register_use(u);
    //}

    /** @todo update this
     *  @brief Release a Use object previously registered with register_use().
     *
     *  Upon release, if the Use* u has immediate_permissions() of at least
     *  Write and was not propagated into a Modify context (defined below), the
     *  release allows the runtime to match the producer flow to pending Use
     *  instances where u->get_out_flow() is equivalent to the consumer
     *  pending->get_in_flow() (with equivalence for Flow defined in flow.h).
     *  The location provided to u->get_data_pointer_reference() holds the data
     *  that satisfies the pending->get_in_flow() if the Use requested immediate
     *  permissions of Write or greater.
     *
     *  The Use* u has been propagated into a Modify context if another Use* u2
     *  has been registered where u2->get_out_flow() was constructed from either
     *  make_next_flow(make_same_flow(u->get_in_flow())) or
     *  make_next_flow(make_forwarding_flow(u->get_in_flow())).
     *
     *  If the Use* u has scheduling_permissions() of at least Write but no
     *  immediate permissions and was not propagated into a Modify context by
     *  the time it was released, the Use* is an "alias" use.  As such,
     *  u->get_out_flow() only provides an alias for u->get_in_flow().
     *  u->get_in_flow() is the actual producer flow that satisfies all
     *  tasks/uses depending on u->get_out_flow(). There will (almost always,
     *  unless the corresponding Handle is completely unused in its lifetime) be
     *  some other task t2 with Use* u2 such that u2->get_out_flow() and
     *  u->get_in_flow() are equivalent.  release_use(u2) may have already been
     *  called, may be in process, or may not have been called when
     *  release_use(u) is invoked. The backend runtime is responsible for
     *  ensuring correct satisfaction of pending flows and thread safety
     *  (atomicity) of release_use(...) with aliases.  An alias use can
     *  correspond to another alias use, creating a chain of aliases that the
     *  backend runtime must resolve.
     *
     *  If the return value of u->get_out_flow() is the same as or aliases a
     *  Flow created with make_null_flow() at the time release_use() is invoked,
     *  the data at this location may be safely deleted (unless the backend has
     *  some other pending operations on that data, such as unfulfilled
     *  publishes).
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
      frontend::UsePendingRelease* u
    ) =0;

    virtual void
    release_use(
      std::unique_ptr<frontend::UsePendingRelease> u
    ) {
        release_use(u.get());
        u = nullptr;
    }

    // </editor-fold> end Use handling
    //==========================================================================

    //==========================================================================
    // <editor-fold desc="publication, collectives, etc">

#if _darma_has_feature(publish_fetch)
    /** @brief Indicate that the state of a Handle corresponding to a given Use
     *  should be accessible via a corresponding fetching usage with the same
     *  version_key.
     *
     *  See PublicationDetails for more information
     *  @param u       The particular use being published
     *  @param details This encapsulates at least a version_key and an n_readers
     *
     *  @sa PublicationDetails
     */
    virtual void
    publish_use(
      std::unique_ptr<frontend::DestructibleUse>&& u,
      frontend::PublicationDetails* details
    ) =0;
#endif

#if _darma_has_feature(simple_collectives)
    virtual void
    allreduce_use(
      std::unique_ptr<frontend::DestructibleUse>&& use_in_out,
      frontend::CollectiveDetails const* details,
      types::key_t const& tag
    ) =0;

    virtual void
    allreduce_use(
      std::unique_ptr<frontend::DestructibleUse>&& use_in,
      std::unique_ptr<frontend::DestructibleUse>&& use_out,
      frontend::CollectiveDetails const* details,
      types::key_t const& tag
    ) =0;
#endif

#if _darma_has_feature(handle_collection_based_collectives)
    virtual void
    reduce_collection_use(
      std::unique_ptr<frontend::DestructibleUse>&& use_collection_in,
      std::unique_ptr<frontend::DestructibleUse>&& use_out,
      frontend::CollectiveDetails const* details,
      types::key_t const& tag
    ) =0;
#endif
    // </editor-fold> end publication, collectives, etc
    //==========================================================================

#if _darma_has_feature(task_migration)
    virtual size_t
    get_packed_flow_size(
      types::flow_t const& f
    ) =0;

    /** @todo document this
     *
     *  @remark this method should advance the buffer to after the end of the
     *  packed storage used for `f`
     */
    virtual void
    pack_flow(
      types::flow_t& f, void*& buffer
    ) =0;

    /** @todo document this
     *
     *  @remark this method should advance the buffer to after the end of the
     *  packed data used to recreate the return value
     *
     *  @param buffer
     *  @return
     */
    virtual types::flow_t
    make_unpacked_flow(
      void const*& buffer
    ) =0;
#endif
};



class Context {
  public:

    /** @brief Get a pointer to the \ref frontend::Task object currently running
     * on the thread from which get_running_task() was invoked.
    *
    *  @return A non-owning pointer to the \ref frontend::Task object running on
    *  the invoking thread.  The returned pointer must be castable to the same
    *  concrete type as was passed to \ref Runtime::register_task() when the
    *  task was registered.
    *
    *  @remark If the runtime implements context switching, it must ensure that
    *  the behavior of Runtime::get_running_task() is consistent and correct for
    *  a given running thread as though the switching never occurred.
    *
    *  @remark The pointer returned here is guaranteed to be valid until
    *  Task::run() returns for the returned task.  However, to allow context
    *  switching, it is not guaranteed to be valid in the context of any other
    *  task's run() invocation, including child tasks, and thus it should not be
    *  dereferenced in any other context.
    *
    *  @TODO decide what this should do if called before or after a migrated
    *  task runs
    *
    *  @sa frontend::Task
    */
    virtual Runtime::task_t*
    get_running_task() const = 0;

    virtual ~Context() noexcept = default;
};

class MemoryManager {
  public:

    /** @brief Request that the backend allocate a contiguous piece of memory of
     *  size `n_bytes` and with the hinted attributes described by `details`.
     *
     *  @remark The backend is free to block, raise an exception, or even abort
     *  if the allocation is not possible.  However, it must not return an
     *  invalid pointer.
     *
     *  @param n_bytes The number of bytes to allocate
     *
     *  @param details Catagorical hints (for performance purposes) about the
     *  way the memory to be allocated will be used in its lifetime
     *
     *  @return A pointer to the beginning of the allocated region.  Any
     *  accesses to memory in the region `[rv, rv+n_bytes)` (where `rv` is the
     *  returned pointer) must be valid until deallocated is called with the
     *  `rv` and the same `n_bytes` argument
     */
    virtual void*
    allocate(
      size_t n_bytes,
      frontend::MemoryRequirementDetails const& details
    ) =0;

    /** @brief Release memory allocated by a previous call to
     *  `Runtime::allocate()`.
     *
     *  @param ptr A pointer returned by a previous call to
     *  `Runtime::allocate()`.  It is an error if the pointer refers to memory
     *  allocated by any other means.
     *  @param n_bytes The same `n_bytes` argument passed to the
     *  `Runtime::allocate()` call that returned `ptr`.  If a different
     *  value than the one passed to the corresponding allocate is given for
     *  this argument, the backend is allowed to either raise an error or
     *  have undefined behavior occur.
     */
    virtual void
    deallocate(
      void* ptr,
      size_t n_bytes
    ) =0;

    virtual ~MemoryManager() noexcept = default;

};


/** @todo
 *
 * @return
 */
Context*
get_backend_context();

/** @todo
 *
 * @return
 */
MemoryManager*
get_backend_memory_manager();

/** @todo
 *
 * @return
 */
Runtime*
get_backend_runtime();


typedef Runtime runtime_t;

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
void backend_init_finalize(int argc, char** argv);
void backend_santioned_exit(int const);

#include <string>

namespace darma_runtime {
void
abort(
  std::string const& abort_str = std::string{}
);
} // end darma_runtime

#include <darma/interface/backend/types.h>

#endif /* SRC_ABSTRACT_BACKEND_RUNTIME_H_ */
