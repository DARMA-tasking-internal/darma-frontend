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
#include <tinympl/identity.hpp>
#include <tinympl/at.hpp>
#include <tinympl/erase.hpp>
#include <tinympl/bind.hpp>
#include <tinympl/logical_and.hpp>
#include <tinympl/vector.hpp>
#include <tinympl/transform2.hpp>
#include <tinympl/as_sequence.hpp>
#include <tinympl/tuple_as_sequence.hpp>
#include <tinympl/stl_integer_sequence.hpp>

#include <darma_types.h>

#include <darma/interface/backend/types.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/frontend/task.h>
#include <darma/interface/frontend/unpack_task.h>

#include <darma/impl/util.h>
#include <darma/impl/runnable/runnable.h>
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

class TaskBase : public abstract::frontend::Task
{
  protected:

    using key_t = types::key_t;
    using abstract_use_t = abstract::frontend::Use;

    using get_deps_container_t = types::handle_container_template<abstract_use_t*>;

    get_deps_container_t dependencies_;

    key_t name_ = darma_runtime::make_key();

  public:

#if _darma_has_feature(create_parallel_for)
    std::size_t width_;

#if _darma_has_feature(create_parallel_for_custom_cpu_set)
    types::cpu_set_t assigned_cpu_set_;

#endif // _darma_has_feature(create_parallel_for_custom_cpu_set)

#endif // _darma_has_feature(create_parallel_for)


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

    virtual get_deps_container_t const&
    get_dependencies() const override {
      return dependencies_;
    }

    const key_t&
    get_name() const override {
      return name_;
    }

    void
    set_name(const key_t& name) override {
      name_ = name;
    }

    virtual void run() override {
      assert(runnable_);
      pre_run_setup();
      runnable_->run();
      post_run_cleanup();
    }

#if _darma_has_feature(task_migration)
    bool
    is_migratable() const override {
      // Ignored for now:
      return false;
    }

    size_t get_packed_size() const override {
      using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
      serialization::SimplePackUnpackArchive ar;

      ArchiveAccess::start_sizing(ar);

      assert(runnable_.get() != nullptr);

      ar % runnable_->get_index();

      if(runnable_->is_lambda_like_runnable) {
        // TODO pack up the flows/uses/etc and their access handle indices
        DARMA_ASSERT_NOT_IMPLEMENTED("packing up lambda-like runnables");
        size_t added_size = runnable_->lambda_size();
        // TODO finish this!
        return ArchiveAccess::get_size(ar);
      }
      else {
        return runnable_->get_packed_size() + ArchiveAccess::get_size(ar);
      }
    }

    void pack(void* allocated) const override {
      using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
      serialization::SimplePackUnpackArchive ar;

      ArchiveAccess::start_packing(ar);
      ArchiveAccess::set_buffer(ar, allocated);

      assert(runnable_.get() != nullptr);

      ar << runnable_->get_index();

      if(runnable_->is_lambda_like_runnable) {
        // TODO pack up the flows/uses/etc and their access handle indices
        DARMA_ASSERT_NOT_IMPLEMENTED("packing up lambda-like runnables");
        assigned_lambda_unpack_index = 1; // trigger recognition in AccessHandle copy ctor
        // TODO finish this
      }
      else {
        runnable_->pack(ArchiveAccess::get_spot(ar));
      }
    }
#endif // _darma_has_feature(task_migration)

    // end implementation of abstract::frontend::Task
    ////////////////////////////////////////////////////////////////////////////////

  public:

    void pre_run_setup() { }

    void post_run_cleanup() { }

    void set_runnable(std::unique_ptr<RunnableBase>&& r) {
      runnable_ = std::move(r);
    }

    //==========================================================================
#if _darma_has_feature(create_parallel_for)
    bool is_parallel_for_task() const override {
      return is_parallel_for_task_;
    }

    std::size_t width() const override {
      return width_;
    }

#if _darma_has_feature(create_parallel_for_custom_cpu_set)
    void set_cpu_set(
      darma_runtime::types::cpu_set_t const& cpuset
    ) override {
      assigned_cpu_set_ = cpuset;
    }

    types::cpu_set_t const&
    get_cpu_set() const override {
      return assigned_cpu_set_;
    }
#endif // _darma_has_feature(create_parallel_for_custom_cpu_set)

#endif // _darma_has_feature(create_parallel_for)
    //==========================================================================

    virtual ~TaskBase() noexcept { }

    //==========================================================================
    // allowed_aliasing_description

    struct allowed_aliasing_description {
      static constexpr struct allowed_aliasing_description_ctor_tag_t { }
        allowed_aliasing_description_ctor_tag { };

      allowed_aliasing_description(
        allowed_aliasing_description_ctor_tag_t,
        bool all_or_nothing
      ) : is_all_or_nothing(true), all_allowed(all_or_nothing)
      { }

      template <typename AccessHandleT>
      bool aliasing_is_allowed_for(AccessHandleT&& ah, TaskBase*) {
        if(is_all_or_nothing) {
          return all_allowed;
        }
        else {
          DARMA_ASSERT_NOT_IMPLEMENTED(
            "more specific aliasing allowed specifications than just true or false"
          );
          return false; // unreachable
        }
      }

      bool is_all_or_nothing = true;
      bool all_allowed = false;
    };

    std::unique_ptr<allowed_aliasing_description> allowed_aliasing = nullptr;

    //==========================================================================

    TaskBase* current_create_work_context = nullptr;

    std::vector<std::function<void()>> registrations_to_run;
    // TODO remove the post_registration_ops, since it's not used
    std::vector<std::function<void()>> post_registration_ops;
    bool is_double_copy_capture = false;
    unsigned default_capture_as_info = AccessHandleBase::CapturedAsInfo::Normal;
    bool is_parallel_for_task_ = false;
    mutable std::size_t assigned_lambda_unpack_index = 0;

  protected:

    std::unique_ptr<RunnableBase> runnable_;

    friend types::unique_ptr_template<abstract::frontend::Task>
    unpack_task(void* packed_data);


};


// </editor-fold>

////////////////////////////////////////////////////////////////////////////////

#if _darma_has_feature(task_migration)
template <typename ConcreteTaskT>
inline std::unique_ptr<ConcreteTaskT>
_unpack_task(void* packed_data) {
  using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
  serialization::SimplePackUnpackArchive ar;

  ArchiveAccess::start_unpacking(ar);
  ArchiveAccess::set_buffer(ar, packed_data);

  std::size_t runnable_index;
  ar >> runnable_index;

  auto rv = std::make_unique<ConcreteTaskT>();

  auto& reg = darma_runtime::detail::get_runnable_registry();
  rv->set_runnable(
    reg.at(runnable_index)(
      (void*)&ar
    )
  );

  return std::move(rv);
}
#endif // _darma_has_feature(task_migration)

} // end namespace detail

// implementation of abstract::frontend::unpack_task

namespace frontend {

#if _darma_has_feature(task_migration)
inline
abstract::backend::runtime_t::task_unique_ptr
unpack_task(void* packed_data) {
  return darma_runtime::detail::_unpack_task<darma_runtime::detail::TaskBase>(
    packed_data
  );
}
#endif // _darma_has_feature(task_migration)

} // end namespace frontend

} // end namespace darma_runtime


#include <darma/impl/task_do_capture.impl.h>


#endif /* DARMA_RUNTIME_TASK_H_ */
