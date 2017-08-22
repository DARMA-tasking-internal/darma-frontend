/*
//@HEADER
// ************************************************************************
//
//                      lambda_task.h
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

template <typename Lambda>
struct LambdaTask
#if _darma_has_feature(task_migration)
  : PolymorphicSerializationAdapter<
      LambdaTask<Lambda>,
      abstract::frontend::Task,
      TaskCtorHelper
    >
#else
  : TaskCtorHelper
#endif
{

#if _darma_has_feature(task_migration)
  using base_t = PolymorphicSerializationAdapter<
    LambdaTask<Lambda>,
    abstract::frontend::Task,
    TaskCtorHelper
  >;
#else
  using base_t = TaskCtorHelper;
#endif

  Lambda lambda_;

  template <
    typename PreConstructAction,
    typename LambdaDeduced
  >
  LambdaTask(
    PreConstructAction&& action,
    LambdaDeduced&& lambda_in
  ) : base_t(
        variadic_constructor_arg,
        std::forward<PreConstructAction>(action)
      ),
      lambda_(
        // Intentionally *don't* forward to trigger copy ctors of captured vars
        lambda_in
      )
  { }


  ~LambdaTask() override = default;

  template <typename ArchiveT>
  static LambdaTask& reconstruct(void* allocated_void, ArchiveT& ar) {
    auto* allocated = static_cast<char*>(allocated_void);

    auto* running_task = static_cast<darma_runtime::detail::TaskBase*>(
      abstract::backend::get_backend_context()->get_running_task()
    );

    auto*& archive_spot = darma_runtime::serialization::detail::DependencyHandle_attorneys
      ::ArchiveAccess::get_spot(ar);

    auto& data_as_lambda = *reinterpret_cast<Lambda*>(archive_spot);
    reinterpret_cast<char*&>(archive_spot) += sizeof(Lambda);

    auto* rv_ptr = new (allocated_void) LambdaTask(
      [&](detail::TaskBase* task_base){
        running_task->current_create_work_context = task_base;
        task_base->lambda_serdes_mode = serialization::detail::SerializerMode::Unpacking;
        task_base->lambda_serdes_buffer = static_cast<char*>(archive_spot);
      },
      data_as_lambda
    );

    rv_ptr->lambda_serdes_mode = serialization::detail::SerializerMode::None;
    running_task->current_create_work_context = nullptr;

    reinterpret_cast<char*&>(
      darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess::get_spot(ar)
    ) = rv_ptr->lambda_serdes_buffer;

    return *rv_ptr;
  }

  template <typename ArchiveT>
  size_t compute_size(ArchiveT& ar) const {
    auto* running_task = static_cast<darma_runtime::detail::TaskBase*>(
      abstract::backend::get_backend_context()->get_running_task()
    );
    running_task->current_create_work_context = const_cast<LambdaTask*>(this);
    this->lambda_serdes_mode = serialization::detail::SerializerMode::Sizing;
    this->lambda_serdes_computed_size = 0;

    // Trigger a copy, but be sure not to use _garbage for anything!!!
    // in fact, make sure it doesn't get destroyed
    char* _garbage_as_raw = new char[sizeof(Lambda)];
    new (_garbage_as_raw) Lambda(lambda_);
    delete[] _garbage_as_raw;

    this->lambda_serdes_mode = serialization::detail::SerializerMode::None;
    running_task->current_create_work_context = nullptr;

    auto size_before = darma_runtime::serialization::detail::DependencyHandle_attorneys
      ::ArchiveAccess::get_size(ar);
    const_cast<LambdaTask*>(this)->TaskBase::template do_serialize(ar);
    auto size_after = darma_runtime::serialization::detail::DependencyHandle_attorneys
      ::ArchiveAccess::get_size(ar);

    return sizeof(Lambda) + this->lambda_serdes_computed_size + (size_after - size_before);
  }

  template <typename ArchiveT>
  void pack(ArchiveT& ar) const {
    auto*& archive_spot = darma_runtime::serialization::detail::DependencyHandle_attorneys
      ::ArchiveAccess::get_spot(ar);

    ::memcpy(archive_spot, (void const*)&lambda_, sizeof(Lambda));
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
    new (_garbage_as_raw) Lambda(lambda_);
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

  void run() override {
    lambda_();
  }

};

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_LAMBDA_TASK_H
