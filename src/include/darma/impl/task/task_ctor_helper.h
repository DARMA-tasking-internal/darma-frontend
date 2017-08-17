/*
//@HEADER
// ************************************************************************
//
//                      task_ctor_helper.h
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

#ifndef DARMAFRONTEND_TASK_CTOR_HELPER_H
#define DARMAFRONTEND_TASK_CTOR_HELPER_H

#include "task_base.h"

namespace darma_runtime {
namespace detail {

// Empty class used for actions that need to take place before the constructor
// of LambdaTask executes but after the constructor of TaskBase executes
struct TaskCtorHelper : TaskBase {


  // Use a tag as the first argument to prevent messing with copy ctors and such
  template <typename PreConstructAction>
  TaskCtorHelper(
    variadic_constructor_arg_t,
    PreConstructAction&& action
  ) : TaskBase()
  {
    std::forward<PreConstructAction>(action)(this);
  }

  TaskCtorHelper() = default;
  TaskCtorHelper(TaskCtorHelper&&) = default;
  TaskCtorHelper(TaskCtorHelper const&) = delete;

};


} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_TASK_CTOR_HELPER_H
