/*
//@HEADER
// ************************************************************************
//
//                      callable_task.h
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

#ifndef DARMAFRONTEND_LAMBDA_TASK_H
#define DARMAFRONTEND_LAMBDA_TASK_H

#include <darma/impl/polymorphic_serialization.h>

#include "task_base.h"
#include "task_ctor_helper.h"

namespace darma_runtime {
namespace detail {


struct LambdaCaptureSetupHelper : private CaptureSetupHelperBase {

  template <typename CaptureManagerT>
  LambdaCaptureSetupHelper(
    TaskBase* parent_task,
    CaptureManagerT* current_capture_context,
    bool double_copy_capture = true // not always double copy, like in inner
                                    // captures of create_work_while
  ) {
    // Note that the arguments (especially parent_task) to this constructor
    // should *not* be stored as data members because they may not be valid
    // for the entirety of this object's lifetime
    pre_capture_setup(parent_task, current_capture_context, double_copy_capture);
  }

  template <typename CaptureManagerT>
  void pre_capture_setup(
    TaskBase* parent_task,
    CaptureManagerT* current_capture_context,
    bool double_copy_capture = true
  ) {
    CaptureSetupHelperBase::template pre_capture_setup(parent_task, current_capture_context);
    current_capture_context->is_double_copy_capture = double_copy_capture;
  }

  template <typename CaptureManagerT>
  void post_capture_cleanup(
    TaskBase* parent_task,
    CaptureManagerT* current_capture_context
  ) {
    current_capture_context->is_double_copy_capture = false;
    CaptureSetupHelperBase::template post_capture_cleanup(parent_task, current_capture_context);
  }

  //==============================================================================
  // <editor-fold desc="Task migration"> {{{1
  #if _darma_has_feature(task_migration)

  template <
    typename CaptureManagerT,
    typename ArchiveT
  >
  LambdaCaptureSetupHelper(
    unpacking_task_constructor_tag_t,
    TaskBase* running_task,
    CaptureManagerT* current_capture_context,
    ArchiveT& ar
  ) {
    pre_unpack_setup(running_task, current_capture_context, ar);
  }

  template <
    typename CaptureManagerT,
    typename ArchiveT
  >
  void pre_unpack_setup(
    TaskBase* running_task,
    CaptureManagerT* current_capture_context,
    ArchiveT& ar
  ) {
    CaptureSetupHelperBase::pre_capture_setup(running_task, current_capture_context);

    using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
    current_capture_context->lambda_serdes_mode = serialization::detail::SerializerMode::Unpacking;
    current_capture_context->lambda_serdes_buffer = static_cast<char*>(ArchiveAccess::get_spot(ar));
  }

  template <
    typename CaptureManagerT,
    typename ArchiveT
  >
  void post_unpack_cleanup(
    TaskBase* running_task,
    CaptureManagerT* current_capture_context,
    ArchiveT& ar
  ) {
    using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;

    current_capture_context->lambda_serdes_mode = serialization::detail::SerializerMode::None;
    running_task->current_create_work_context = nullptr;

    reinterpret_cast<char*&>(ArchiveAccess::get_spot(ar)) = current_capture_context->lambda_serdes_buffer;

    CaptureSetupHelperBase::post_capture_cleanup(running_task, current_capture_context);
  }

  #endif // darma_has_feature(task_migration)
  // </editor-fold> end Task migration }}}1
  //==============================================================================

};

// Inheriting from LambdaCaptureSetupHelper ensures the setup happens before
// the capture
template <typename Lambda>
struct LambdaCapturer
  : protected LambdaCaptureSetupHelper
{

  //============================================================================
  // <editor-fold desc="Constructors and destructor"> {{{1

  template <
    typename LambdaDeduced,
    typename CaptureManagerT
  >
  LambdaCapturer(
    LambdaDeduced&& callable_in,
    TaskBase* parent_task,
    CaptureManagerT* capture_manager,
    bool double_copy_capture = true
  ) : LambdaCaptureSetupHelper(parent_task, capture_manager, double_copy_capture),
      callable_(
        // Intentionally *don't* forward to trigger copy ctors of captured vars
        callable_in
      )
  {
    post_capture_cleanup(parent_task, capture_manager);
  }

  LambdaCapturer(LambdaCapturer&&)
    noexcept(std::is_nothrow_move_constructible<Lambda>::value) = default;

  //------------------------------------------------------------------------------
  // <editor-fold desc="darma_has_feature(task_migration)"> {{{2
  #if _darma_has_feature(task_migration)

  template <
    typename LambdaDeduced,
    typename CaptureManagerT,
    typename ArchiveT
  >
  LambdaCapturer(
    unpacking_task_constructor_tag_t,
    LambdaDeduced&& callable_in,
    TaskBase* parent_task,
    CaptureManagerT* capture_manager,
    ArchiveT& ar
  ) : LambdaCaptureSetupHelper(
        unpacking_task_constructor_tag, parent_task, capture_manager, ar
      ),
      callable_(
        // Intentionally *don't* forward to trigger copy ctors of captured vars
        callable_in
      )
  {
    post_unpack_cleanup(parent_task, capture_manager, ar);
  }

  #endif // darma_has_feature(task_migration)
  // </editor-fold> end darma_has_feature(task_migration) }}}2
  //------------------------------------------------------------------------------

  // </editor-fold> end Constructors and destructor }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="data members"> {{{1

  Lambda callable_;

  // </editor-fold> end data members }}}1
  //============================================================================

};

namespace LambdaTask_tags {
constexpr struct no_double_copy_capture_tag_t {} no_double_copy_capture_tag = { };
} // end namespace LambdaTask_tags

template <typename Lambda>
struct LambdaTask
#if _darma_has_feature(task_migration)
  : PolymorphicSerializationAdapter<
      LambdaTask<Lambda>,
      abstract::frontend::Task,
      TaskBase
    >,
#else
  : TaskCtorHelper,
#endif
    // LambdaCapturer should always follow the TaskCtorHelper base, since it's constructor
    // relies on a TaskBase object being fully constructed
    LambdaCapturer<Lambda>
{

  //============================================================================
  // <editor-fold desc="typedefs and type aliases"> {{{1

  using capturer_t = LambdaCapturer<Lambda>;

  #if _darma_has_feature(task_migration)
  using base_t = PolymorphicSerializationAdapter<
    LambdaTask<Lambda>,
    abstract::frontend::Task,
    TaskBase
  >;
  #else
  using base_t = TaskCtorHelper;
  #endif

  // </editor-fold> end typedefs and type aliases }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="Constructors and Destructor"> {{{1

  template <
    typename LambdaDeduced,
    typename CaptureManagerT,
    typename... TaskCtorArgs
  >
  LambdaTask(
    LambdaDeduced&& callable_in,
    detail::TaskBase* parent_task,
    CaptureManagerT* capture_manager,
    TaskCtorArgs&&... other_ctor_args
  ) : base_t(
        parent_task,
        std::forward<TaskCtorArgs>(other_ctor_args)...
      ),
      capturer_t(
        std::forward<LambdaDeduced>(callable_in),
        parent_task, capture_manager
      )
  { }

  template <
    typename LambdaDeduced,
    typename... TaskCtorArgs
  >
  LambdaTask(
    LambdaDeduced&& callable_in,
    detail::TaskBase* parent_task,
    TaskCtorArgs&&... other_ctor_args
  ) : LambdaTask<Lambda>::LambdaTask(
        std::forward<LambdaDeduced>(callable_in),
        parent_task,
        this,
        std::forward<TaskCtorArgs>(other_ctor_args)...
      )
  { /* forwarding ctor, must be empty */ }

  template <
    typename LambdaDeduced,
    typename CaptureManagerT,
    typename... TaskCtorArgs
  >
  LambdaTask(
    LambdaDeduced&& callable_in,
    CaptureManagerT* capture_manager,
    TaskCtorArgs&&... other_ctor_args
  ) : LambdaTask<Lambda>::LambdaTask(
        std::forward<LambdaDeduced>(callable_in),
        static_cast<darma_runtime::detail::TaskBase* const>(
          darma_runtime::abstract::backend::get_backend_context()->get_running_task()
        ),
        capture_manager,
        std::forward<TaskCtorArgs>(other_ctor_args)...
      )
  { /* forwarding ctor, must be empty */ }

  /**
   *  Non-double-copy constructor, for things like the inner recapture of
   *  a while-do loop.  Uses a tag type because adding a bool with a default
   *  would interact awkwardly with the forwarding of variadic arguments to
   *  the TaskBase constructor
   */
  template <
    typename LambdaDeduced,
    typename CaptureManagerT,
    typename... TaskCtorArgs
  >
  LambdaTask(
    LambdaTask_tags::no_double_copy_capture_tag_t,
    LambdaDeduced&& callable_in,
    detail::TaskBase* parent_task,
    detail::TaskBase* running_task,
    CaptureManagerT* capture_manager,
    TaskCtorArgs&&... other_ctor_args
  ) : base_t(
        parent_task,
        std::forward<TaskCtorArgs>(other_ctor_args)...
      ),
      capturer_t(
        std::forward<LambdaDeduced>(callable_in), running_task, capture_manager,
        /* double_copy_capture = */ false
      )
  { }

  template <
    typename LambdaDeduced,
    typename CaptureManagerT,
    typename... TaskCtorArgs
  >
  LambdaTask(
    LambdaTask_tags::no_double_copy_capture_tag_t,
    LambdaDeduced&& callable_in,
    detail::TaskBase* parent_task,
    CaptureManagerT* capture_manager,
    TaskCtorArgs&&... other_ctor_args
  ) : LambdaTask<Lambda>::LambdaTask(
        LambdaTask_tags::no_double_copy_capture_tag,
        std::forward<LambdaDeduced>(callable_in),
        parent_task, parent_task, // if running task isn't given, it's the parent task
        capture_manager,
        std::forward<TaskCtorArgs>(other_ctor_args)...
      )
  { /* forwarding ctor, must be empty */ }

  template <
    typename LambdaDeduced,
    typename ArchiveT,
    typename... TaskCtorArgs
  >
  LambdaTask(
    unpacking_task_constructor_tag_t,
    LambdaDeduced&& callable_in,
    detail::TaskBase* parent_task,
    ArchiveT& ar,
    TaskCtorArgs&&... other_ctor_args
  ) : base_t(
        unpacking_task_constructor_tag,
        parent_task,
        std::forward<TaskCtorArgs>(other_ctor_args)...
      ),
      capturer_t(
        unpacking_task_constructor_tag,
        std::forward<LambdaDeduced>(callable_in),
        parent_task, this, ar
      )
  { }

  LambdaTask(LambdaTask const&) = delete;

  LambdaTask(LambdaTask&&) = delete;

  //LambdaTask(LambdaTask&&)
  //  noexcept(std::is_nothrow_move_constructible<base_t>::value
  //    and std::is_nothrow_move_constructible<capturer_t>::value
  //  ) = default;

  ~LambdaTask() override = default;

  // </editor-fold> end Constructors and Destructor }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="Migration"> {{{1

  //------------------------------------------------------------------------------
  // <editor-fold desc="darma_has_feature(task_migration)"> {{{2
  #if _darma_has_feature(task_migration)

  template <typename ArchiveT>
  static LambdaTask& reconstruct(void* allocated_void, ArchiveT& ar) {
    using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;

    auto* allocated = static_cast<char*>(allocated_void);

    auto* running_task = static_cast<darma_runtime::detail::TaskBase*>(
      abstract::backend::get_backend_context()->get_running_task()
    );

    auto*& archive_spot = ArchiveAccess::get_spot(ar);

    auto& data_as_lambda = *reinterpret_cast<Lambda*>(archive_spot);
    reinterpret_cast<char*&>(archive_spot) += sizeof(Lambda);

    auto* rv_ptr = new (allocated_void) LambdaTask(
      unpacking_task_constructor_tag,
      std::move(data_as_lambda),
      running_task, ar
    );

    return *rv_ptr;
  }

  template <typename ArchiveT>
  size_t compute_size(ArchiveT& ar) const {

    auto size_before = darma_runtime::serialization::detail::DependencyHandle_attorneys
      ::ArchiveAccess::get_size(ar);

    ar.add_to_size_indirect(sizeof(Lambda));

    auto* running_task = static_cast<darma_runtime::detail::TaskBase*>(
      abstract::backend::get_backend_context()->get_running_task()
    );
    running_task->current_create_work_context = const_cast<LambdaTask*>(this);
    this->lambda_serdes_mode = serialization::detail::SerializerMode::Sizing;
    this->lambda_serdes_computed_size = 0;

    // Trigger a copy, but be sure not to use _garbage for anything!!!
    // in fact, make sure it doesn't get destroyed
    char* _garbage_as_raw = new char[sizeof(Lambda)];
    new (_garbage_as_raw) Lambda(this->callable_);
    delete[] _garbage_as_raw;

    this->lambda_serdes_mode = serialization::detail::SerializerMode::None;
    running_task->current_create_work_context = nullptr;

    ar.add_to_size_indirect(this->lambda_serdes_computed_size);
    this->lambda_serdes_computed_size = 0;

    const_cast<LambdaTask*>(this)->TaskBase::template do_serialize(ar);

    auto size_after = darma_runtime::serialization::detail::DependencyHandle_attorneys
      ::ArchiveAccess::get_size(ar);

    return (size_after - size_before);
  }

  template <typename ArchiveT>
  void pack(ArchiveT& ar) const {
    auto*& archive_spot = darma_runtime::serialization::detail::DependencyHandle_attorneys
      ::ArchiveAccess::get_spot(ar);

    ::memcpy(archive_spot, (void const*)&this->callable_, sizeof(Lambda));
    reinterpret_cast<char*&>(archive_spot) += sizeof(Lambda);

    auto* running_task = static_cast<darma_runtime::detail::TaskBase*>(
      abstract::backend::get_backend_context()->get_running_task()
    );
    running_task->current_create_work_context = const_cast<LambdaTask*>(this);
    this->lambda_serdes_mode = serialization::detail::SerializerMode::Packing;
    this->lambda_serdes_buffer = static_cast<char*>(archive_spot);

    // Trigger a copy, but be sure not to use _garbage for anything!!!
    // in fact, make sure it doesn't get destroyed
    char* _garbage_as_raw = new char[sizeof(Lambda)];
    new (_garbage_as_raw) Lambda(this->callable_);
    delete[] _garbage_as_raw;

    this->lambda_serdes_mode = serialization::detail::SerializerMode::None;
    running_task->current_create_work_context = nullptr;

    // And advance the buffer
    reinterpret_cast<char*&>(
      darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess::get_spot(ar)
    ) = this->lambda_serdes_buffer;
    this->lambda_serdes_buffer = nullptr;

    const_cast<LambdaTask*>(this)->TaskBase::template do_serialize(ar);
  }

  template <typename ArchiveT>
  void serialize(ArchiveT& ar) {
    // Only used for unpacking
    assert(ar.is_unpacking());
    this->TaskBase::template do_serialize(ar);
  }

  bool is_migratable() const override {
    return true;
  }

  #endif // darma_has_feature(task_migration)
  // </editor-fold> end darma_has_feature(task_migration) }}}2
  //----------------------------------------------------------------------------

  // </editor-fold> end Migration }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="darma_runtime::abstract::frontend::Task method implementations"> {{{1

  void run() override {
    this->callable_();
  }

  // </editor-fold> end darma_runtime::abstract::frontend::Task method implementations }}}1
  //============================================================================



};

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_LAMBDA_TASK_H
