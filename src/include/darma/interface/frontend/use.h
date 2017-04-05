/*
//@HEADER
// ************************************************************************
//
//                      use.h
//                         DARMA
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

#ifndef DARMA_IMPLEMENTATION_FRONTEND_USE_H
#define DARMA_IMPLEMENTATION_FRONTEND_USE_H

#include <darma_types.h> // flow_t

#include <darma/interface/frontend/frontend_fwd.h>

#include <darma/interface/backend/flow.h>
#include <darma/interface/frontend/flow_relationship.h>

#include "handle.h"
#include "use_collection.h"
#include "flow_relationship.h"

namespace darma_runtime {
namespace abstract {
namespace frontend {

/** @brief Encapsulates the state, permissions, and data reference for a Handle
 *  at a given point in logical time as required by an operation.
 *
 *  @todo update this.
 *
 *  Use objects have a life cycle with 3 strictly ordered phases.  For some Use
 *  instance u,
 *    + Creation/registration -- `&u` is passed as the argument to
 *      Runtime::register_use().  At this time, u.get_in_flow() and
 *      u.get_out_flow() must return unique, valid Flow objects.
 *    + Task or Publish use (up to once in lifetime):
 *      - Task use: For tasks, `&u` can be accessed through the iterable
 *        returned by t.get_dependencies() for some Task object `t` passed to
 *        Runtime::register_task() after `u` is created and before `u` is
 *        released. At task execution time, u.immediate_permissions(),
 *        u.scheduling_permissions(), and u.get_data_pointer_reference() must
 *        return valid values, and these values must remain valid until
 *        Runtime::release_use(u) is called (note that migration may change this
 *        time frame in future versions of the spec).
 *      - Publish use: A single call to Runtime::publish_use() may be made for
 *        any Use. The frontend may immediately call Runtime::release_use()
 *        after publish_use(). If the publish is deferred and has not completed
 *        by the time release_use() is called, the backend runtime must extract
 *        the necessary Flow and key fields from the Use.
 *    + Release -- Following a task use or a publish use, the translation layer
 *      will make a single call to Runtime::release_use(). The Use instance may
 *      no longer be valid on return. The destructor of Use will NOT delete its
 *      input and output flow. The backend runtime is responsible for deleting
 *      Flow allocations, which may occur during release.
 *
 */
class Use {
  public:

    /**
     * @brief An enumeration of the allowed values that immediate_permissions()
     * and scheduling_permissions() can return
     */
    typedef enum Permissions {
      None=0,   /*!< A Use may not perform any operations (read or write).
                     Usually only immediate_permissions will be None
                 */
      Read=1,   /*!< An immediate (scheduling) Use may only perform read
                     operations (create read-only tasks)
                 */
      Write=2,  /*!< An immediate (scheduling) Use may perform write operations
                     (create write tasks)
                 */
      Modify=3,  /*!< Read|Write. An immediate (scheduling) Use may perform any
                      operations (create any tasks)
                  */
      Commutative=4, /*!< An immediate (scheduling) Use may perform commutative
                      *   operations (create commutative tasks). This is not a
                      *   strict subset of Read/Write privileges
                      */
      Relaxed=8 /*!< An immediate (scheduling) Use may perform relaxed
                 *   operations (create relaxed tasks). This is not a
                 *   strict subset of Read/Write privileges
                 */
    } permissions_t;

    /** @brief Return a pointer to the handle that this object encapsulates a
     *  use of.
     *
     *  @remark If `immediate_permissions()` on this `Use` returns `None`,
     *  a `shared_ptr` to `nullptr` *may* be returned.
     */
    virtual std::shared_ptr<Handle const>
    get_handle() const =0;

    /** @brief Get the immediate permissions needed for the Flow returned by
     *  get_in_flow() to be ready as a precondition for this Use
     *
     *  @return
     */
    virtual permissions_t
    immediate_permissions() const =0;

    /** @brief Get the scheduling permissions needed for the Flow returned by
     *  get_in_flow() to be ready as a precondition for this Use
     *
     *  @return
     */
    virtual permissions_t
    scheduling_permissions() const =0;

    /** @brief Whether or not the Use manages the outer scope control flow for
     *  a UseCollection.
     *
     *  @remark If true, the return values of get_in_flow() and get_out_flow()
     *  must be references to objects returned by one of the
     *  Runtime::make_*_flow_collection() methods.
     *
     *  @return True if and only if the Use manages a UseCollection
     */
    virtual bool
    manages_collection() const { return false; }

#if _darma_has_feature(create_concurrent_work_owned_by)
    virtual bool
    is_uniquely_owned() const { return false; }
#endif // _darma_has_feature(create_concurrent_work_owned_by)

    // Deletions should only ever occur on the most derived class (i.e., done
    // by the translation layer itself) or on pointer to the most base class,
    // `Use`.
    virtual ~Use() = default;

};

/** @todo document this
 *
 *  @remark The return values of get_in_flow() and get_out_flow() are undefined
 *  when the `Use` is accessed via a pointer to a `UsePendingRegistration
 */
class UsePendingRegistration : virtual public Use {
  public:

    virtual void set_in_flow(types::flow_t const& new_flow) =0;

    virtual void set_out_flow(types::flow_t const& new_flow) =0;

    virtual FlowRelationship const&
    get_in_flow_relationship() const =0;

    virtual FlowRelationship const&
    get_out_flow_relationship() const =0;

    virtual bool will_be_dependency() const =0;

  protected:
    // Deletions should only ever occur on the most derived class (i.e., done
    // by the translation layer itself) or on pointer to the most base class,
    // `Use`.
    ~UsePendingRegistration() = default;

};

class RegisteredUse : virtual public Use {

  public:

    /** @brief Get the Flow that must be ready for use as a precondition for the
     *  Task t that depends on this Use
     */
    virtual types::flow_t&
    get_in_flow() =0;

    /** @brief Get the Flow that is produced or made available when this Use is
     *  released
     */
    virtual types::flow_t&
    get_out_flow() =0;

};

class UsePendingRelease : virtual public RegisteredUse {
  public:

    virtual bool establishes_alias() const =0;

    virtual bool was_dependency() const =0;

  protected:
    // Deletions should only ever occur on the most derived class (i.e., done
    // by the translation layer itself) or on pointer to the most base class,
    // `Use`.
    ~UsePendingRelease() = default;
};

class DependencyUse : virtual public RegisteredUse {
  public:

    /** @brief Get a reference to the data pointer on which the requested
     *  immediate permissions have been granted.
     *
     *  For a Use requesting immediate permissions, the runtime will set the
     *  value of the reference returned by this function to the beginning of the
     *  data requested at least by the time the backend calls Task::run() on the
     *  task requesting this Use
     *
     */
    virtual void*&
    get_data_pointer_reference() =0;

  protected:
    // Deletions should only ever occur on the most derived class (i.e., done
    // by the translation layer itself) or on pointer to the most base class,
    // `Use`.
    ~DependencyUse() = default;
};

class CollectionManagingUse : virtual public RegisteredUse {

  public:
    /** @todo document this
     *
     *  @remark the lifetime of the returned pointer is tied to the lifetime of
     *  the Use that manages it
     *
     *  @return
     */
    virtual UseCollection*
    get_managed_collection() { return nullptr; }

  protected:
    // Deletions should only ever occur on the most derived class (i.e., done
    // by the translation layer itself) or on pointer to the most base class,
    // `Use`.
    ~CollectionManagingUse() = default;
};

#if _darma_has_feature(create_concurrent_work_owned_by)
class UniquelyOwnedUse : virtual public RegisteredUse {

  public:
    /** @todo document this
     *
     */
    virtual std::size_t
    task_collection_owning_index() const =0;

  protected:
    // Deletions should only ever occur on the most derived class (i.e., done
    // by the translation layer itself) or on pointer to the most base class,
    // `Use`.
    ~UniquelyOwnedUse() = default;

};
#endif // _darma_has_feature(create_concurrent_work_owned_by)


template <typename ToUse>
ToUse
use_cast(
  Use* from_use
) {
  return darma_runtime::detail::safe_dynamic_cast<ToUse>(
    from_use
  );
}

template <typename ToUse>
ToUse
use_cast(
  Use const* from_use
) {
  return darma_runtime::detail::safe_dynamic_cast<ToUse>(
    from_use
  );
}

} // end namespace frontend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_IMPLEMENTATION_FRONTEND_USE_H
