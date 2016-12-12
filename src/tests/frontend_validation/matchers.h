/*
//@HEADER
// ************************************************************************
//
//                      matchers.h
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

#ifndef DARMA_TESTS_FRONTEND_VALIDATION_MATCHERS_H
#define DARMA_TESTS_FRONTEND_VALIDATION_MATCHERS_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "helpers.h"
#include "mock_backend.h"
#include <darma/interface/frontend/use.h>

using namespace ::testing;

//bool operator==(
//  darma_runtime::abstract::frontend::Use const* const a,
//  darma_runtime::abstract::frontend::Use const* const b
//) {
//  return a->get_in_flow() == b->get_in_flow()
//    and  a->get_out_flow() == b->get_out_flow()
//    and  a->immediate_permissions() == b->immediate_permissions()
//    and  a->scheduling_permissions() == b->scheduling_permissions();
//}

MATCHER_P4(IsUseWithFlows, f_in, f_out, scheduling_permissions, immediate_permissions,
  "arg->get_in_flow(): " + PrintToString(f_in) + ", arg->get_out_flow(): "
  + PrintToString(f_out) + ", arg->scheduling_permissions(): " + permissions_to_string(scheduling_permissions)
  + ", arg->immediate_permissions(): " + permissions_to_string(immediate_permissions)
) {
  if(arg == nullptr) {
    *result_listener << "arg is null";
    return false;
  }

#if DARMA_SAFE_TEST_FRONTEND_PRINTERS
  *result_listener << "arg (unprinted for safety)";
#else
  *result_listener << "arg->get_in_flow(): " << PrintToString(arg->get_in_flow()) + ", arg->get_out_flow(): "
    + PrintToString(arg->get_out_flow())
    << ", arg->scheduling_permissions(): " << permissions_to_string(arg->scheduling_permissions())
      + ", arg->immediate_permissions(): " + permissions_to_string(arg->immediate_permissions());
#endif

  using namespace mock_backend;
  if(f_in != arg->get_in_flow()) return false;
  if(f_out != arg->get_out_flow()) return false;
  if(immediate_permissions != -1) {
    if(immediate_permissions != int(arg->immediate_permissions())) return false;
  }
  if(scheduling_permissions != -1) {
    if(scheduling_permissions != int(arg->scheduling_permissions())) return false;
  }
  return true;
}

MATCHER_P(UseRefIsNonNull, use, "Use* reference is non-null at time of invocation") {
  if(use == nullptr) {
    *result_listener << "Use* reference is null at time of invocation";
    return false;
  }
  else {
    return true;
  }
}

MATCHER_P(UseInGetDependencies, use, "task->get_dependencies() contains " + PrintToString(use)) {
  auto deps = arg->get_dependencies();
  if(deps.find(use) != deps.end()) {
    return true;
  }
  else {
    *result_listener << "task->get_dependencies() contains: ";
    bool is_first = true;
    for(auto&& dep : deps) {
      if(not is_first) *result_listener << ", ";
      is_first = false;
      *result_listener << PrintToString(dep);
    }
    return false;
  }
}

template <typename Arg, typename... Args>
auto
VectorOfPtrsToArgs(Arg& arg, Args&... args) {
  return std::vector<std::add_pointer_t<Arg>>{ &arg, &args... };
}

MATCHER_P(UsesInGetDependencies, uses,
  "task->get_dependencies() contains all of [%(uses)s]"
) {
  if(arg == nullptr) {
    *result_listener << "task is null";
    return false;
  }
  auto deps = arg->get_dependencies();
  bool rv = true;
  for(auto&& use : uses) {
    bool found = false;
    //if(*use == nullptr) found = false;
    //else {
    //  for (auto&& dep : deps) {
    //    if (
    //      dep->get_in_flow() == (*use)->get_in_flow()
    //      and dep->get_out_flow() == (*use)->get_out_flow()
    //      and dep->immediate_permissions() == (*use)->immediate_permissions()
    //      and dep->scheduling_permissions() == (*use)->scheduling_permissions()
    //    ) found = true;
    //  }
    //}
    //if (not found) {
    if(deps.find(*use) == deps.end()) {
      *result_listener << PrintToString(*use) << " not found; ";
      rv = false;
    }
  }
  *result_listener << "task->get_dependencies() contains: ";
  bool is_first = true;
  for (auto&& dep : deps) {
    if (not is_first) *result_listener << ", ";
    is_first = false;
    *result_listener << "0x"
      << std::hex << intptr_t(dep) << "--"
      << PrintToString(dep);
  }
  return rv;
}

MATCHER_P(HasName, name_key, "object has name key " + PrintToString(name_key)) {
  auto arg_name = arg->get_name();
  if(darma_runtime::detail::key_traits<darma_runtime::types::key_t>::key_equal()(arg_name, name_key)) {
    return true;
  }
  else {
    *result_listener << "object has name key: " << arg_name;
    return false;
  }
}

#endif //DARMA_TESTS_FRONTEND_VALIDATION_MATCHERS_H
