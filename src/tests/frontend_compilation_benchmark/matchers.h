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

inline std::string
relationship_to_string(darma_runtime::abstract::frontend::FlowRelationship::flow_relationship_description_t rel) {
#define _flow_rel_str_case(rel) case darma_runtime::abstract::frontend::FlowRelationship::rel : return #rel
  switch(rel) {
    _flow_rel_str_case(Same);
    _flow_rel_str_case(Initial);
    _flow_rel_str_case(Null);
    _flow_rel_str_case(Next);
    _flow_rel_str_case(Forwarding);
    _flow_rel_str_case(Indexed);
    _flow_rel_str_case(IndexedLocal);
    _flow_rel_str_case(IndexedFetching);
    _flow_rel_str_case(InitialCollection);
    _flow_rel_str_case(NullCollection);
    _flow_rel_str_case(NextCollection);
    _flow_rel_str_case(SameCollection);
#if _darma_has_feature(anti_flows)
    _flow_rel_str_case(Insignificant);
    _flow_rel_str_case(InsignificantCollection);
#endif // _darma_has_feature(anti_flows)
    default: return std::to_string(rel);
  };
#undef _flow_rel_str_case
}

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

// Ignoring out flow
MATCHER_P3(IsUseWithInFlow, f_in, scheduling_permissions, immediate_permissions,
  "arg->get_in_flow(): " + PrintToString(f_in) + ", arg->scheduling_permissions(): "
    + permissions_to_string(scheduling_permissions)
    + ", arg->immediate_permissions(): " + permissions_to_string(immediate_permissions)
) {
  if(arg == nullptr) {
    *result_listener << "arg is null";
    return false;
  }

#if DARMA_SAFE_TEST_FRONTEND_PRINTERS
  *result_listener << "arg (unprinted for safety)";
#else
  *result_listener << "arg->get_in_flow(): " << PrintToString(arg->get_in_flow())
                   << ", arg->scheduling_permissions(): " << permissions_to_string(arg->scheduling_permissions())
                     + ", arg->immediate_permissions(): " + permissions_to_string(arg->immediate_permissions());
#endif

  using namespace mock_backend;
  if(f_in != arg->get_in_flow()) return false;
  if(immediate_permissions != ::darma_runtime::frontend::Permissions::_notGiven) {
    if(immediate_permissions != (arg->immediate_permissions())) return false;
  }
  if(scheduling_permissions != ::darma_runtime::frontend::Permissions::_notGiven) {
    if(scheduling_permissions != (arg->scheduling_permissions())) return false;
  }
  return true;
}

MATCHER_P4(IsUseWithFlows, f_in, f_out, scheduling_permissions, immediate_permissions,
  "arg->get_in_flow(): " + PrintToString(f_in) + ", arg->get_out_flow(): "
  + PrintToString(f_out) + ", arg->scheduling_permissions(): " + permissions_to_string(scheduling_permissions)
  + ", arg->immediate_permissions(): " + permissions_to_string(immediate_permissions)
) {
  if(arg == nullptr) {
    *result_listener << "arg is null";
    return false;
  }

  auto* arg_cast = darma_runtime::abstract::frontend::use_cast<darma_runtime::abstract::frontend::RegisteredUse*>(arg);

#if DARMA_SAFE_TEST_FRONTEND_PRINTERS
  *result_listener << "arg (unprinted for safety)";
#else
  *result_listener << "arg->get_in_flow(): " << PrintToString(arg_cast->get_in_flow()) + ", arg->get_out_flow(): "
    + PrintToString(arg_cast->get_out_flow())
    << ", arg->scheduling_permissions(): " << permissions_to_string(arg->scheduling_permissions())
      + ", arg->immediate_permissions(): " + permissions_to_string(arg->immediate_permissions());
#endif

  using namespace mock_backend;
  if(f_in != arg_cast->get_in_flow()) return false;
  if(f_out != arg_cast->get_out_flow()) return false;
  if(immediate_permissions != ::darma_runtime::frontend::Permissions::_notGiven) {
    if(immediate_permissions != (arg->immediate_permissions())) return false;
  }
  if(scheduling_permissions != ::darma_runtime::frontend::Permissions::_notGiven) {
    if(scheduling_permissions != (arg->scheduling_permissions())) return false;
  }
  return true;
}

MATCHER_P(InFlowRelationshipIndex, value, "use->get_in_flow_relationship().index() = "
  + PrintToString(value)
) {
  if(arg == nullptr) {
    *result_listener << "arg is null";
    return false;
  }
  *result_listener << "use->get_in_flow_relationship().index() = "
                   << arg->get_in_flow_relationship().index();
  return arg->get_in_flow_relationship().index() == value;
}

MATCHER_P(OutFlowRelationshipIndex, value, "use->get_out_flow_relationship().index() = "
  + PrintToString(value)
) {
  if(arg == nullptr) {
    *result_listener << "arg is null";
    return false;
  }
  *result_listener << "use->get_out_flow_relationship().index() = "
                   << arg->get_out_flow_relationship().index();
  return arg->get_out_flow_relationship().index() == value;
}

MATCHER_P(UseEstablishesAlias, value, "use->establishes_alias() = " + PrintToString(value)) {
  if(arg == nullptr) {
    *result_listener << "arg is null";
    return false;
  }
  *result_listener << "arg->establishes_alias() " << std::boolalpha << arg->establishes_alias();
  return arg->establishes_alias() == value;
}

MATCHER_P(UseWillBeDependency, value, "use->is_dependency() = " + PrintToString(value)) {
  if(arg == nullptr) {
    *result_listener << "arg is null";
    return false;
  }
  *result_listener << "arg->is_dependency() = " << std::boolalpha << arg->is_dependency();
  return arg->is_dependency() == value;
}

MATCHER_P(IsUseWithHandleKey, key, "handle->get_key() = " + PrintToString(key)) {
  if(arg == nullptr) {
    *result_listener << "arg is null";
    return false;
  }
  *result_listener << "arg has key " << arg->get_handle()->get_key();
  return arg->get_handle()->get_key() == key;
}

MATCHER_P(HasTaskCollectionTokenNamed, token_name, "object with get_task_collection_token().name = " + PrintToString(token_name)) {
  *result_listener << "object with get_task_collection_token().name = "
                   << arg->get_task_collection_token().name;
  return token_name == arg->get_task_collection_token().name;
}

template <typename T> struct _deref_non_null { decltype(auto) operator()(T val) const { return *val; } };
template <> struct _deref_non_null<std::nullptr_t> { decltype(auto) operator()(std::nullptr_t) const { return nullptr; } };
template <> struct _deref_non_null<std::nullptr_t&> { decltype(auto) operator()(std::nullptr_t) const { return nullptr; } };
template <> struct _deref_non_null<std::nullptr_t const&> { decltype(auto) operator()(std::nullptr_t) const { return nullptr; } };
template <> struct _deref_non_null<std::nullptr_t&&> { decltype(auto) operator()(std::nullptr_t) const { return nullptr; } };
template <typename T> decltype(auto) deref_non_null(T&& val) { return _deref_non_null<T&&>{}(std::forward<T>(val)); }

MATCHER_P7(IsUseWithFlowRelationships, f_in_rel, f_in, f_out_rel, f_out, out_rel_is_in, scheduling_permissions, immediate_permissions,
  "\n  ------------ Use with ------------\n  in_flow relationship: " + relationship_to_string(f_in_rel)
    + "\n  in_flow related: " + PrintToString(deref_non_null(f_in))
    + "\n  out_flow relationship: " + relationship_to_string(f_out_rel)
    + "\n  out_flow_related: " + PrintToString(deref_non_null(f_out))
    + "\n  out_flow_related_is_in: " + PrintToString(out_rel_is_in)
    + "\n  scheduling_permissions: " + permissions_to_string(scheduling_permissions)
    + "\n  immediate_permissions: " + permissions_to_string(immediate_permissions)
) {
  if(arg == nullptr) {
    *result_listener << "arg is null";
    return false;
  }

#if DARMA_SAFE_TEST_FRONTEND_PRINTERS
  *result_listener << "arg (unprinted for safety)";
#else
  *result_listener << "\n  ------------ Use with ------------";
  *result_listener << "\n  in_flow relationship: " << relationship_to_string(arg->get_in_flow_relationship().description());
  if(arg->get_in_flow_relationship().related_flow() == nullptr) {
    *result_listener << "\n  in_flow_related: NULL";
  }
  else {
    *result_listener << "\n  in_flow related: "
                     << PrintToString(*arg->get_in_flow_relationship().related_flow());
  }
  *result_listener << "\n  out_flow relationship: " << relationship_to_string(arg->get_out_flow_relationship().description());
  if(arg->get_out_flow_relationship().related_flow() == nullptr) {
    *result_listener << "\n  out_flow_related: NULL";
  }
  else {
    *result_listener << "\n  out_flow related: "
                     << PrintToString(*arg->get_out_flow_relationship().related_flow());
  }
  *result_listener << "\n  out_flow_related_is_in: " << PrintToString(arg->get_out_flow_relationship().use_corresponding_in_flow_as_related())
    << "\n  scheduling_permissions: " << permissions_to_string(arg->scheduling_permissions())
    << "\n  immediate_permissions: " << permissions_to_string(arg->immediate_permissions());
#endif

  using namespace mock_backend;
  if(f_in == nullptr) {
    if(arg->get_in_flow_relationship().related_flow() != nullptr) return false;
  }
  else {
    if(arg->get_in_flow_relationship().related_flow() == nullptr) return false;
    if(*arg->get_in_flow_relationship().related_flow() != deref_non_null(f_in)) return false;
    if(arg->get_in_flow_relationship().description() != f_in_rel) return false;
  }

  if(f_out == nullptr) {
    if(arg->get_out_flow_relationship().related_flow() != nullptr) return false;
  }
  else {
    if(arg->get_out_flow_relationship().related_flow() == nullptr) return false;
    if(*arg->get_out_flow_relationship().related_flow() != deref_non_null(f_out)) return false;
    if(arg->get_out_flow_relationship().description() != f_out_rel) return false;
  }
  if(arg->get_out_flow_relationship().use_corresponding_in_flow_as_related() xor out_rel_is_in) return false;

  if(immediate_permissions != darma_runtime::frontend::Permissions::_notGiven) {
    if(immediate_permissions != arg->immediate_permissions()) return false;
  }
  if(scheduling_permissions != darma_runtime::frontend::Permissions::_notGiven) {
    if(scheduling_permissions != arg->scheduling_permissions()) return false;
  }
  return true;
}

MATCHER_P(ObjectNamed, name, "Pointer to object for which get_name() returns " + PrintToString(name)) {
  if(arg == nullptr) {
    *result_listener << "Pointer to null";
    return false;
  }
  else {
    *result_listener << "Pointer to object for which get_name() returns " << PrintToString(arg->get_name());
    return name == arg->get_name();
  }
}

#if _darma_has_feature(anti_flows)
MATCHER_P5(IsUseWithAntiFlowRelationships, f_anti_in_rel, f_anti_in_arg, f_anti_out_rel, f_anti_out_arg, anti_out_rel_is_anti_in,
  "\n  ------------ Use with ------------"
  "\n  anti_in_flow relationship: " + relationship_to_string(f_anti_in_rel)
    + "\n  anti_in_flow related: " + PrintToString(::mock_backend::MockAntiFlow(f_anti_in_arg))
    + "\n  anti_out_flow relationship: " + relationship_to_string(f_anti_out_rel)
    + "\n  anti_out_flow_related: " + PrintToString(::mock_backend::MockAntiFlow(f_anti_out_arg))
    + "\n  anti_out_flow_related_is_anti_in: " + PrintToString(anti_out_rel_is_anti_in)
) {
  if(arg == nullptr) {
    *result_listener << "arg is null";
    return false;
  }
  using namespace mock_backend;

  auto f_anti_in_flow = ::mock_backend::MockAntiFlow(f_anti_in_arg);
  auto f_anti_out_flow = ::mock_backend::MockAntiFlow(f_anti_out_arg);
  auto related_anti_in_flow = MockAntiFlow(arg->get_anti_in_flow_relationship().related_anti_flow());
  auto related_anti_out_flow = MockAntiFlow(arg->get_anti_out_flow_relationship().related_anti_flow());

#if DARMA_SAFE_TEST_FRONTEND_PRINTERS
  *result_listener << "arg (unprinted for safety)";
#else
  *result_listener << "\n  ------------ Use with ------------"
                   "\n  anti_in_flow relationship: " << relationship_to_string(arg->get_anti_in_flow_relationship().description());
  *result_listener << "\n  anti_anti_in_flow related: "
                   << PrintToString(related_anti_in_flow);
  *result_listener << "\n  anti_out_flow relationship: " << relationship_to_string(arg->get_anti_out_flow_relationship().description());
  *result_listener << "\n  anti_out_flow related: "
                   << PrintToString(related_anti_out_flow);
  *result_listener << "\n  anti_out_flow_related_is_in: " << PrintToString(arg->get_anti_out_flow_relationship().use_corresponding_in_flow_as_anti_related())
                   << "\n  scheduling_permissions: " << permissions_to_string(arg->scheduling_permissions())
                   << "\n  immediate_permissions: " << permissions_to_string(arg->immediate_permissions());
#endif

  if(f_anti_in_flow != related_anti_in_flow) return false;
  if(arg->get_anti_in_flow_relationship().description() != f_anti_in_rel) return false;

  if(f_anti_out_flow != related_anti_out_flow) return false;
  if(arg->get_anti_out_flow_relationship().description() != f_anti_out_rel) return false;
  if(arg->get_anti_out_flow_relationship().use_corresponding_in_flow_as_anti_related() xor anti_out_rel_is_anti_in) return false;

  return true;
}
#endif

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
  if(deps.find(
    ::darma_runtime::abstract::frontend::use_cast<
      ::darma_runtime::abstract::frontend::DependencyUse*
    >(use)
  ) != deps.end()) {
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

MATCHER_P(CollectionUseInGetDependencies, use, "task->get_dependencies() contains " + PrintToString(use)) {
  auto deps = arg->get_dependencies();
  if(deps.find(
    ::darma_runtime::abstract::frontend::use_cast<
      ::darma_runtime::abstract::frontend::DependencyUse*
    >(use)
  ) != deps.end()) {
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
  "task->get_dependencies() contains all of " + PrintToString(uses)
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
    if(use == nullptr) {
      rv = false;
      break;
    }
    try {
      if (deps.find(
        ::darma_runtime::detail::try_dynamic_cast<
          ::darma_runtime::abstract::frontend::DependencyUse*
        >(*use)
      ) == deps.end()) {
        *result_listener << "at least one use not found; ";
        rv = false;
        break;
      }
    } catch (std::bad_cast const&) {
      rv = false;
      break;
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
