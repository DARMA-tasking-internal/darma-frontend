/*
//@HEADER
// ************************************************************************
//
//                      task_base.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_TASK_BASE_H
#define DARMAFRONTEND_TASK_BASE_H

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

#include <darma/impl/util/safe_static_cast.h>

#include <darma/interface/backend/types.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/frontend/task.h>
#include <darma/interface/frontend/unpack_task.h>

#include <darma/impl/handle_fwd.h>

#include <darma/impl/access_handle_base.h>

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
#include <darma/impl/capture.h>
#include <darma/impl/polymorphic_serialization.h>

namespace darma_runtime {
namespace detail {

class CaptureManager {
  public:

    using abstract_use_t = abstract::frontend::DependencyUse;
    using get_deps_container_t = types::handle_container_template<abstract_use_t*>;

  public:


    virtual void
    do_capture(
      AccessHandleBase& captured,
      AccessHandleBase const& source_and_continuing,
      bool register_continuation_use
    ) =0;

    void add_dependency(HandleUseBase& use) {
      dependencies_.insert(&use);
    }

    virtual void
    pre_capture_setup() {
      assert(uses_to_unmark_already_captured.empty());
    }

    virtual void
    post_capture_cleanup() {
      for(auto* use : uses_to_unmark_already_captured) {
        use->already_captured = false;
      }
      uses_to_unmark_already_captured.clear();
    }

    std::set<HandleUseBase*> uses_to_unmark_already_captured;
    bool is_double_copy_capture = false;
    unsigned default_capture_as_info = AccessHandleBase::CapturedAsInfo::Normal;
    mutable std::size_t lambda_serdes_computed_size = 0;
    mutable serialization::detail::SerializerMode lambda_serdes_mode = serialization::detail::SerializerMode::None;
    mutable char* lambda_serdes_buffer = nullptr;

    get_deps_container_t dependencies_;

  protected:

    /**
     * Protected destructor
     * An object should never be deleted via a pointer to a CaptureManager
     */
    virtual ~CaptureManager() = default;

};

constexpr struct unpacking_task_constructor_tag_t {} unpacking_task_constructor_tag = { };
constexpr struct allowed_aliasing_description_ctor_tag_t { } allowed_aliasing_description_ctor_tag { };

class TaskBase
  : public abstract::frontend::Task,
    public CaptureManager
{
  protected:

    using key_t = types::key_t;

    key_t name_ = darma_runtime::make_key();

  public:

    //------------------------------------------------------------------------------
    // <editor-fold desc="_darma_has_feature(create_parallel_for_custom_cpu_set)"> {{{2
    #if _darma_has_feature(create_parallel_for)
    std::size_t width_;

    //------------------------------------------------------------------------------
    // <editor-fold desc="_darma_has_feature(create_parallel_for)"> {{{2
    #if _darma_has_feature(create_parallel_for_custom_cpu_set)

    types::resource_pack_t assigned_resource_pack_;

    #endif // _darma_has_feature(create_parallel_for_custom_cpu_set)
    // </editor-fold> end _darma_has_feature(create_parallel_for_custom_cpu_set) }}}2
    //------------------------------------------------------------------------------

    #endif // _darma_has_feature(create_parallel_for)
    // </editor-fold> end _darma_has_feature(create_parallel_for) }}}2
    //------------------------------------------------------------------------------

    //==========================================================================
    // <editor-fold desc="allowed_aliasing_description"> {{{1

    struct allowed_aliasing_description {

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

    // </editor-fold> end allowed_aliasing_description }}}1
    //==========================================================================

    //==============================================================================
    // <editor-fold desc="Constructors and destructor"> {{{1

    TaskBase() = default;

    TaskBase(TaskBase&&) = default;

    TaskBase(TaskBase const&) = delete;

    explicit
    TaskBase(
      unpacking_task_constructor_tag_t,
      TaskBase* parent_task // may *not* always be the running task!!!
    ) {
      propagate_parent_context(parent_task);
    }

    // note: do not store the parent_task argument; the pointer might not be
    // valid for the lifetime of this object
    template <
      typename AllowAliasingDescription
    >
    TaskBase(
      TaskBase* parent_task, // may *not* always be the running task!!!
      types::key_t const& name_key,
      AllowAliasingDescription&& aliasing_desc,
      bool is_data_parallel=false
    ) : name_(name_key),
        allowed_aliasing(std::forward<AllowAliasingDescription>(aliasing_desc)),
        is_data_parallel_task_(is_data_parallel)
    {
      propagate_parent_context(parent_task);
    }

    TaskBase(
      TaskBase* parent_task, // may *not* always be the running task!!!
      types::key_t const& name_key,
      bool is_data_parallel=false
    ) : TaskBase(
          parent_task,
          name_key,
          std::make_unique<allowed_aliasing_description>(
            allowed_aliasing_description_ctor_tag, false
          ),
          is_data_parallel
        )
    { /* forwarding ctor, must be empty */ }

    explicit
    TaskBase(
      TaskBase* parent_task, // may *not* always be the running task!!!
      bool is_data_parallel=false
    ) : TaskBase(parent_task, make_key(), is_data_parallel)
    { /* forwarding ctor, must be empty */ }

    virtual ~TaskBase() noexcept = default;

    // </editor-fold> end Constructors and destructor }}}1
    //==============================================================================


    //==============================================================================
    // <editor-fold desc="Task migration"> {{{1
    #if _darma_has_feature(task_migration)

    // Called from serialize implementations of derived classes
    template <typename ArchiveT>
    void do_serialize(ArchiveT& ar) {
      ar | name_;
      #if _darma_has_feature(create_parallel_for)
      ar | width_;
      #if _darma_has_feature(create_parallel_for_custom_cpu_set)
      ar | assigned_resource_pack_;
      #endif
      #endif

      #if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
      ar | calling_file | calling_function | calling_line;
      #endif

      #if _darma_has_feature(task_collection_token)
      if(not ar.is_unpacking() and parent_token_available) {
        DARMA_ASSERT_NOT_IMPLEMENTED("Task collection token migration");
      }
      // ar | parent_token_available;
      // if(parent_token_available) {
      //   ar | token_;
      // }
      #endif
    }

    #endif // darma_has_feature(task_migration)
    // </editor-fold> end Task migration }}}1
    //==============================================================================

    void add_dependency(HandleUseBase& use) {
      dependencies_.insert(&use);
    }

    bool
    do_capture_checks(
      AccessHandleBase const& source_and_continuing
    );

    void
    do_capture(
      AccessHandleBase& captured,
      AccessHandleBase const& source_and_continuing,
      bool register_continuation_use = true
    ) override;

    //==========================================================================
    // <editor-fold desc="Implementation of abstract::frontend::Task"> {{{1

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
      runnable_->run();
    }

    //--------------------------------------------------------------------------
    // <editor-fold desc="DARMA feature: task_migration"> {{{2
    #if _darma_has_feature(task_migration)

    virtual bool
    is_migratable() const override {
      // if it's not overridden by the time it gets here, it's not migratable
      return false;
    }

    friend types::unique_ptr_template<abstract::frontend::Task>
    unpack_task(void* packed_data);

    #endif // _darma_has_feature(task_migration)
    // </editor-fold> end task_migration }}}2
    //--------------------------------------------------------------------------

    // </editor-fold> end Implementation of abstract::frontend::Task }}}1
    //==========================================================================

  public:


    void set_runnable(std::unique_ptr<RunnableBase>&& r) {
      runnable_ = std::move(r);
    }

    //==========================================================================
    // <editor-fold desc="line number and file name recording"> {{{1
    #if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS

    // These probably don't need to be strings to be portable, but just in case:
    std::string calling_file;
    std::string calling_function;
    size_t calling_line;


    void set_context_information(
      const char* file, size_t line, const char* func_name
    ) {
      calling_file = file;
      calling_function = func_name;
      calling_line = line;
    }

    void copy_context_information_from(
      TaskBase const& other
    ) {
      calling_file = other.calling_file;
      calling_function = other.calling_function;
      calling_line = other.calling_line;
    }

    std::string const& get_calling_filename() const override {
      return calling_file;
    }

    size_t get_calling_line_number() const override {
      return calling_line;
    }

    std::string const& get_calling_function_name() const override {
      return calling_function;
    }

    #endif
    // </editor-fold> end line number and file name recording }}}1
    //==========================================================================


    //==========================================================================
    // <editor-fold desc="parallel for tasks and other multi-threaded task details"> {{{1

    //--------------------------------------------------------------------------
    // <editor-fold desc="_darma_has_feature(create_parallel_for)"> {{{2
    #if _darma_has_feature(create_parallel_for)

    bool is_parallel_for_task() const override {
      return is_parallel_for_task_;
    }

    std::size_t width() const override {
      return width_;
    }

    //--------------------------------------------------------------------------
    // <editor-fold desc="_darma_has_feature(create_parallel_for_custom_cpu_set)"> {{{2

    #if _darma_has_feature(create_parallel_for_custom_cpu_set)
    void set_resource_pack(
      darma_runtime::types::resource_pack_t const& cpuset
    ) override
    {
      assert(runnable_);
      runnable_->set_resource_pack(cpuset);
    }
    #endif // _darma_has_feature(create_parallel_for_custom_cpu_set)

    // </editor-fold> end _darma_has_feature(create_parallel_for_custom_cpu_set) }}}2
    //--------------------------------------------------------------------------

    #endif // _darma_has_feature(create_parallel_for)

    // </editor-fold> end _darma_has_feature(create_parallel_for) }}}2
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    // <editor-fold desc="_darma_has_feature(mark_parallel_tasks)"> {{{2
    #if _darma_has_feature(mark_parallel_tasks)

    bool is_data_parallel_task() const override {
      return is_data_parallel_task_;
    }

    #endif
    // </editor-fold> end _darma_has_feature(mark_parallel_tasks) }}}2
    //------------------------------------------------------------------------------

    // </editor-fold> end parallel for tasks and other multi-threaded task details }}}1
    //==========================================================================




    //==========================================================================

    // TODO refactor this to group some of these "capture context" properties into a seperate class
    CaptureManager* current_create_work_context = nullptr;


#if _darma_has_feature(task_collection_token)
    // TODO @cleanup @dependent remove this when free-function-style collectives are fully deprecated
    types::task_collection_token_t token_;
    bool parent_token_available = false;
#endif

    // Note: may be called pre-registration of dependencies or not.  If we need
    // a similar hook that *must* be called specifically pre- or
    // post-registration of dependencies, we'll need a different hook
    void propagate_parent_context(TaskBase* parent_task) {
#if _darma_has_feature(task_collection_token)
      if(parent_task->parent_token_available) {
        token_ = parent_task->token_;
        parent_token_available = true;
      }
#endif // _darma_has_feature(task_collection_token)
    }

    bool is_parallel_for_task_ = false;
    bool is_data_parallel_task_ = false;

  protected:

    std::unique_ptr<RunnableBase> runnable_;


};


// for use with make_running_task_to_return_when_unpacking()
class EmptyTask : public TaskBase {
  public:
    EmptyTask() = default;

    size_t get_packed_size() const override {
      assert(false); // not migratable
      return 0;
    }

    void pack(char*) const override {
      assert(false); // not migratable
    }
};


class NonMigratableTaskBase
  : public PolymorphicSerializationAdapter<
      NonMigratableTaskBase,
      TaskBase
    >
{
  public:

    //------------------------------------------------------------------------------
    // <editor-fold desc="not serializable, but still need stubs"> {{{2
#if _darma_has_feature(task_migration)
    template <typename ArchiveT>
    void serialize(ArchiveT&) {
      // TODO this should throw (or something like that), not assert
      DARMA_ASSERT_NOT_IMPLEMENTED("Migration of task collection tasks");
    }
    template <typename ArchiveT>
    static NonMigratableTaskBase& reconstruct(void* allocated, ArchiveT& ar) {
      // TODO this should throw (or something like that), not assert
      DARMA_ASSERT_NOT_IMPLEMENTED("Migration of task collection tasks");
      // unreachable, but helps avoid compiler warnings
      return *reinterpret_cast<NonMigratableTaskBase*>(allocated);
    }
    bool is_migratable() const override {
      return false;
    }
#endif //_darma_has_feature(task_migration)
    // </editor-fold> end not serializable, but still need stubs }}}2
    //------------------------------------------------------------------------------

};


} // end namespace detail

namespace frontend {

inline
std::unique_ptr<abstract::frontend::Task>
make_running_task_to_return_when_unpacking() {
  return std::make_unique<darma_runtime::detail::EmptyTask>();
}

} // end namespace frontend

} // end namespace darma_runtime

// </editor-fold>

////////////////////////////////////////////////////////////////////////////////

namespace darma_runtime {
namespace detail {

// Convenience methods (must be here because TaskBase must be a complete type)

inline TaskBase*
get_running_task_impl(
  abstract::backend::Context* rt
) {
  return darma_runtime::detail::safe_static_cast<TaskBase*>(
    rt->get_running_task()
  );
}

inline TaskBase*
get_running_task_impl() {
  return darma_runtime::detail::get_running_task_impl(
    abstract::backend::get_backend_context()
  );
}

} // end namespace detail
} // end namespace darma_runtime


#endif //DARMAFRONTEND_TASK_BASE_H
