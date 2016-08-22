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

#ifndef DARMA_RUNTIME_TASK_H_
#define DARMA_RUNTIME_TASK_H_

#include <typeindex>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <set>

#include <tinympl/greater.hpp>
#include <tinympl/int.hpp>
#include <tinympl/delay.hpp>
#include <tinympl/identity.hpp>
#include <tinympl/delay.hpp>
#include <tinympl/at.hpp>
#include <tinympl/erase.hpp>
#include <tinympl/bind.hpp>
#include <tinympl/logical_and.hpp>
#include <tinympl/vector.hpp>
#include <tinympl/transform2.hpp>
#include <tinympl/as_sequence.hpp>
#include <tinympl/tuple_as_sequence.hpp>
#include <tinympl/stl_integer_sequence.hpp>

#include <darma/interface/backend/types.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/frontend/task.h>

#include <darma/impl/util.h>
#include <darma/impl/runnable.h>
#include <darma/impl/runtime.h>
#include <darma/impl/handle_fwd.h>
#include <darma/impl/meta/callable_traits.h>
#include <darma/impl/handle.h>
#include <darma/impl/functor_traits.h>
#include <darma/impl/serialization/nonintrusive.h>
#include <darma/impl/use.h>
#include <darma/impl/util/smart_pointers.h>


namespace darma_runtime {

namespace detail {


////////////////////////////////////////////////////////////////////////////////

// <editor-fold desc="TaskBase and its descendants">

class TaskBase : public abstract::frontend::Task<TaskBase>
{
  protected:

    using key_t = types::key_t;
    using abstract_use_t = abstract::frontend::Use;

    using get_deps_container_t = types::handle_container_template<abstract_use_t*>;

    get_deps_container_t dependencies_;

    key_t name_;

  public:

    TaskBase() = default;

    // Directly construct from a conditional callable
    template <typename LambdaCallable,
      typename = std::enable_if_t<
        not std::is_base_of<std::decay_t<LambdaCallable>, TaskBase>::value
      >
    >
    TaskBase(LambdaCallable&& bool_callable) {
      TaskBase* parent_task = static_cast<detail::TaskBase* const>(
        abstract::backend::get_backend_context()->get_running_task()
      );
      parent_task->current_create_work_context = this;
      default_capture_as_info |= AccessHandleBase::CapturedAsInfo::ReadOnly;
      default_capture_as_info |= AccessHandleBase::CapturedAsInfo::Leaf;
      runnable_ =
        // *Intentionally* avoid perfect forwarding here, causing a copy to happen,
        // which then triggers all of the captures.  We do this by adding an lvalue reference
        // to the type and not forwarding the value
        detail::make_unique<RunnableCondition<std::remove_reference_t<LambdaCallable>&>>(
          bool_callable
        );
      default_capture_as_info = AccessHandleBase::CapturedAsInfo::Normal;
      parent_task->current_create_work_context = nullptr;
    }

    void add_dependency(HandleUse& use) {
      dependencies_.insert(&use);
    }

    template <typename AccessHandleT1, typename AccessHandleT2>
    void do_capture(
      AccessHandleT1& captured,
      AccessHandleT2 const& source_and_continuing
    );

    ////////////////////////////////////////////////////////////////////////////////
    // Implementation of abstract::frontend::Task

    get_deps_container_t const&
    get_dependencies() const {
      return dependencies_;
    }

    const key_t&
    get_name() const {
      return name_;
    }

    void
    set_name(const key_t& name) {
      name_ = name;
    }

    bool
    is_migratable() const {
      // Ignored for now:
      return false;
    }

    template <typename ReturnType = void>
    ReturnType run()  {
      static_assert(
        std::is_same<ReturnType, bool>::value or std::is_void<ReturnType>::value,
        "Only bool and void for ReturnType in Task::run<>() are currently supported"
      );
      assert(runnable_);
      pre_run_setup();
      return _do_run<ReturnType>(typename std::is_void<ReturnType>::type{});
    }


    size_t get_packed_size() const  {
      using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
      serialization::SimplePackUnpackArchive ar;

      ArchiveAccess::start_sizing(ar);

      assert(runnable_.get() != nullptr);

      ar % runnable_->get_index();
      return runnable_->get_packed_size() + ArchiveAccess::get_size(ar);
    }

    void pack(void* allocated) const  {
      using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
      serialization::SimplePackUnpackArchive ar;

      ArchiveAccess::start_packing(ar);
      ArchiveAccess::set_buffer(ar, allocated);

      assert(runnable_.get() != nullptr);

      ar << runnable_->get_index();

      return runnable_->pack(ArchiveAccess::get_spot(ar));
    }

    // end implementation of abstract::frontend::Task
    ////////////////////////////////////////////////////////////////////////////////

  private:
    template <typename ReturnType>
    inline void
    _do_run(std::true_type&&) {
      runnable_->run();
      post_run_cleanup();
    }

    template <typename ReturnType>
    inline std::enable_if_t<
      not std::is_void<ReturnType>::value,
      ReturnType
    >
    _do_run(std::false_type&&) {
      ReturnType rv = runnable_->run();
      post_run_cleanup();
      return rv;
    }

  public:

    void pre_run_setup() { }

    void post_run_cleanup() { }

    void set_runnable(std::unique_ptr<RunnableBase>&& r) {
      runnable_ = std::move(r);
    }

    virtual ~TaskBase() noexcept { }

    TaskBase* current_create_work_context = nullptr;

    std::vector<std::function<void()>> registrations_to_run;
    std::vector<std::function<void()>> post_registration_ops;
    unsigned default_capture_as_info = AccessHandleBase::CapturedAsInfo::Normal;

  private:

    std::unique_ptr<RunnableBase> runnable_;

    friend types::unique_ptr_template<abstract::frontend::Task<TaskBase>>
    unpack_task(void* packed_data);


};

class TopLevelTask
  : public TaskBase
{
  public:

    bool run()  {
      // Abort, as specified.  This should never be called.
      assert(false);
      return false;
    }

};


// </editor-fold>

////////////////////////////////////////////////////////////////////////////////

} // end namespace detail

// implementation of abstract::frontend::unpack_task

namespace abstract {

namespace frontend {

inline backend::runtime_t::task_unique_ptr
unpack_task(void* packed_data) {
  using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
  serialization::SimplePackUnpackArchive ar;

  ArchiveAccess::start_unpacking(ar);
  ArchiveAccess::set_buffer(ar, packed_data);

  std::size_t runnable_index;
  ar >> runnable_index;

  auto rv = detail::make_unique<detail::TaskBase>();

  rv->set_runnable(
    darma_runtime::detail::get_runnable_registry().at(runnable_index)(
      (void*)&ar
    )
  );

  return std::move(rv);
}

} // end namespace frontend

} // end namespace abstract

} // end namespace darma_runtime


#include <darma/impl/task_do_capture.impl.h>


#endif /* DARMA_RUNTIME_TASK_H_ */
