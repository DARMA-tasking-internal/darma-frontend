/*
//@HEADER
// ************************************************************************
//
//                          task.h
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

#ifndef SRC_ABSTRACT_FRONTEND_TASK_H_
#define SRC_ABSTRACT_FRONTEND_TASK_H_


#include "dependency_handle.h"

namespace dharma_runtime {

namespace abstract {

namespace frontend {

/**
 *  @ingroup abstract
 *
 *  @class Task
 *
 *  @brief The fundamental abstraction for the frontend to communicate units of
 *  dependency-driven work to the backend.
 *
 *  @tparam Key must meet the Key concept
 *  @tparam Version must meet the Version concept
 *  @tparam Container must meet the standard library Container concept
 *  @tparam smart_ptr_template A template for a shared pointer to a type and for which
 *  dharma_runtime::detail::smart_ptr_traits is specialized
 *
 *  @todo 0.3 spec: task migration hooks
 *
 */
template <
  typename Key, typename Version,
  template <typename...> class Container
>
class Task {
  public:

    typedef abstract::frontend::DependencyHandle<Key, Version> handle_t;

    /** @brief returns the dependencies and antidependencies of the task
     *
     *  All dependencies will return true when given as the argument of Task::needs_read_data()
     *  on this instance and all antidependencies will return true when given as the argument of
     *  Task::needs_write_data() (though write dependencies that do not overwrite data --- that is,
     *  initial versions --- will have version 0 and will not be true antidependencies).
     *
     *  @return An iterable container of (non-owning) pointers to DependencyHandle objects.  The
     *  frontend must ensure that only pointers to handles registered with Runtime::register_hanlde()
     *  but not yet released with Runtime::release_handle() are returned by this method, and the
     *  pointers in the returned container must be remain valid until Runtime::release_handle()
     *  is called.  Furthermore, it is a debug-mode error for the frontend to return a handle that
     *  returns the same values for get_key() and get_version() but points to a different location
     *  in memory than the corresponding pointer registered with Runtime::register_handle().
     *
     */
    virtual
    const Container<handle_t*>&
    get_dependencies() const =0;

    /** @brief returns true iff the task needs to read data from handle
     *
     *  @param handle A (non-owning) pointer to the handle to be queried
     *
     *  @remark The frontend must ensure that a Task \b must return true for at least one of
     *  needs_read_data() or needs_write_data() for each handle in the container returned by
     *  get_dependencies().  Failure to do so is a debug-mode error.
     *
     *  @remark The return value of this method is unrelated to whether a dependency is
     *  satisfied or not.  Even for a handle that returns true for handle->is_satisfied(),
     *  this method should return true iff the task needs to read the data from the handle.
     *
     */
    virtual bool
    needs_read_data(
      const handle_t* handle
    ) const =0;

    /** @brief returns true iff the task needs to write data to handle
     *
     *  @param handle A (non-owning) pointer to the handle to be queried
     *
     *  @remark The frontend must ensure that a Task \b must return true for at least one of
     *  needs_read_data() or needs_write_data() for each handle in the container returned by
     *  get_dependencies().  Failure to do so is a debug-mode error.
     *
     *  @remark The return value of this method is unrelated to whether a dependency is
     *  satisfied or not.  Even for a handle that returns true for handle->is_satisfied(),
     *  this method should return true iff the task needs to write data to the handle.
     *
     *  @remark Not all handles for which this method returns true will be real antidependencies.
     *  If a handle reports a version equal to 0 (i.e., equal to the value given by the default
     *  constructor of Version), there is no previous version to overwrite, and thus no antidependency
     *  is created.  However, the runtime should still allocate the data for the object in this case.
     *
     *  @todo 0.3 spec: Ability to get handles that are not preallocated but instead can acquire
     *  data from some existing object (not another dependency, just another object in general).
     *
     */
    virtual bool
    needs_write_data(
      const handle_t* handle
    ) const =0;

    /** @brief returns the name of the task if one has been assigned with set_name(), or
     *  a reference to a default-constructed Key if not.
     *
     *  In the 0.2 spec this is only used with the outermost task, which is named with
     *  a key of two size_t values: the SPMD rank and the SPMD size.  See dharma_backend_initialize()
     *  for more information
     *
     *  @todo 0.3 spec: user task naming interface
     *
     */
    virtual const Key&
    get_name() const =0;

    /** @brief returns the name of the task if one has been assigned with set_name(), or
     *  a reference to a default-constructed Key if not
     *
     *  In the 0.2 spec this is only used with the outermost task, which is named with
     *  a key of two size_t values: the SPMD rank and the SPMD size.  See dharma_backend_initialize()
     *  for more information
     *
     *  @todo 0.3 spec: user task naming interface
     */
    virtual void
    set_name(const Key& name_key) =0;

    /** @brief returns true iff the task can be migrated
     *
     *  @remark always return false in the 0.2 spec implementation.  Later specs will need
     *  additional hooks for migration
     *
     */
    virtual bool
    is_migratable() const =0;


    /** @brief Run the task.
     *
     *  This should be invoked only once all dependencies returned by get_dependencies()
     *  are in a satisfied state (i.e., they return true for DependencyHandle::is_satisfied()).
     *
     *  @post Upon return, it is no longer valid to call get_dependencies(), needs_read_data(),
     *  or needs_write_data().  In fact, the task is free to release these pointers (using
     *  Runtime::release_handle()) before Task::run() returns.
     *
     *  @remark Task::run() need not be invoked on the same thread as it was created, nor on the
     *  same thread as the runtime querying its above methods.  However, while run() is executing
     *  on a given thread, any calls to Runtime::get_running_task() must return a pointer to
     *  this task object.  (If the runtime implements context switching, it must ensure that
     *  the behavior of Runtime::get_running_task() is consistent and correct for a given
     *  running thread as though the switching never occurred)
     *
     */
    virtual void
    run() const =0;


    virtual ~Task() noexcept = default;
};


} // end namespace frontend

} // end namespace abstract

} // end namespace dharma_runtime



#endif /* SRC_ABSTRACT_FRONTEND_TASK_H_ */
