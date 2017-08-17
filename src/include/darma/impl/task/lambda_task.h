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
  static LambdaTask& reconstruct(void* allocated, ArchiveT& ar) {
    DARMA_ASSERT_NOT_IMPLEMENTED("serialization of lambda tasks");
    // Unreachable, but to avoid compiler warnings
    return *reinterpret_cast<LambdaTask*>(allocated);
  }

  template <typename ArchiveT>
  void serialize(ArchiveT& ar) {
    DARMA_ASSERT_NOT_IMPLEMENTED("serialization of lambda tasks");
  }

  bool is_migratable() const override {
    return false;
  }

  void run() override {
    lambda_();
  }

};

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_LAMBDA_TASK_H
