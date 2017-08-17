/*
//@HEADER
// ************************************************************************
//
//                          test_frontend.h
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

#ifndef SRC_TESTS_FRONTEND_VALIDATION_TEST_FRONTEND_H_
#define SRC_TESTS_FRONTEND_VALIDATION_TEST_FRONTEND_H_

#define DEBUG_CREATE_WORK_HANDLES 0
#define DARMA_SAFE_TEST_FRONTEND_PRINTERS 0


#include <deque>
#include <iomanip>
#include <iostream>
#include <cstring> // std::strtok()

#include <gtest/gtest.h>

#include <tinympl/fill_n.hpp>

#include <darma.h> // TODO remove this in favor of finer-grained dependency inclusion
#include <darma/impl/task/task.h>
#include <darma/impl/runtime.h>
#include <darma/interface/app/access_handle.h>
#include <darma/impl/handle_attorneys.h>

#include "mock_backend.h"
#include "darma_types.h"

#include "helpers.h"
#include "matchers.h"


// A mock and corresponding abstract base used for arbitrarily establishing
// ordering of specific lines of code
class AbstractSequenceMarker {
  public:
    virtual void mark_sequence(std::string const& marker) const =0;
};

class MockSequenceMarker {
  public:
    MOCK_CONST_METHOD1(mark_sequence, void(std::string const&));
};

extern ::testing::StrictMock<MockSequenceMarker>* sequence_marker;

extern std::unique_ptr<mock_backend::MockRuntime> mock_runtime;

namespace darma_runtime {
namespace abstract {
namespace backend {

inline Runtime* get_backend_runtime() {
  return mock_runtime.get();
}

inline Context* get_backend_context() {
  return mock_runtime.get();
}

inline MemoryManager* get_backend_memory_manager() {
  return mock_runtime.get();
}

} // end namespace backend
} // end namespace abstract
} // end namespace darma_runtime

//==============================================================================
// <editor-fold desc="wrapper to make EXPECT_*().InSequence() work as expected">

namespace _impl {

template <typename Expectation, typename Lambda>
struct InSequenceWrapper {
  InSequenceWrapper(Expectation&& exp, Lambda&& lamb)
    : exp_(exp), lambda(lamb) {}
  InSequenceWrapper(InSequenceWrapper&& other)
    : exp_(other.exp_), lambda(other.lambda) { other.invoked_ = true; }
  template <typename... Args>
  decltype(auto)
  InSequence(Args&& ... args)&& {
    assert(not invoked_);
    invoked_ = true;
    return lambda(
      exp_.InSequence(::testing::Sequence(), std::forward<Args>(args)...)
    );
  }
  operator ::testing::Expectation()&& {
    invoked_ = true;
    return lambda(std::forward<Expectation>(exp_));
  }
  ~InSequenceWrapper() { if (not invoked_) lambda(std::forward<Expectation>(exp_)); }
  Expectation&& exp_;
  Lambda lambda;
  bool invoked_ = false;
};

template <typename Expectation, typename Lambda>
InSequenceWrapper<Expectation, Lambda>
in_sequence_wrapper(Expectation&& exp, Lambda&& lambda) {
  return InSequenceWrapper<Expectation, Lambda>(
    std::forward<Expectation>(exp), std::forward<Lambda>(lambda)
  );
};

struct UseDescription {
  mock_backend::MockFlow expected_in_flow;
  mock_backend::MockFlow expected_out_flow;
  std::string var_name;
  darma_runtime::abstract::frontend::Use::permissions_t expected_sched_permissions;
  darma_runtime::abstract::frontend::Use::permissions_t expected_immed_permissions;
  UseDescription() = default;
  UseDescription(UseDescription const&) = default;
  UseDescription(
    std::string const& name,
    mock_backend::MockFlow in,
    mock_backend::MockFlow out,
    darma_runtime::abstract::frontend::Use::permissions_t sched,
    darma_runtime::abstract::frontend::Use::permissions_t immed
  ) : var_name(name), expected_in_flow(in),
      expected_out_flow(out), expected_sched_permissions(sched),
      expected_immed_permissions(immed)
  { }
};

} // end namespace _impl

// </editor-fold> end wrapper to make EXPECT_*().InSequence() work as expected
//==============================================================================


//==============================================================================
// <editor-fold desc="Special EXPECT_* macros for DARMA">

// Note that these can't just be functions or methods (like they used to be
// because the line numbers get screwed up and debugging is much harder

#define EXPECT_REGISTER_USE(use_ptr, fin, fout, sched, immed) \
  ::_impl::in_sequence_wrapper( \
    EXPECT_CALL(*mock_runtime, legacy_register_use(IsUseWithFlows( \
      fin, fout, use_t::sched, use_t::immed \
    ))), [&](auto&& exp) -> decltype(auto) { \
       return exp.WillOnce(::testing::Invoke([&](auto&& use_arg) { \
         use_ptr = use_arg; \
         described_uses_[use_ptr] = ::_impl::UseDescription(#use_ptr, fin, fout, use_t::sched, use_t::immed); \
       })); \
    } \
  )

#define EXPECT_REGISTER_USE_AND_SET_BUFFER(use_ptr, fin, fout, sched, immed, value) \
  ::_impl::in_sequence_wrapper( \
    EXPECT_CALL(*mock_runtime, legacy_register_use(IsUseWithFlows( \
      fin, fout, use_t::sched, use_t::immed \
    ))), [&](auto&& exp) -> decltype(auto) { return exp.WillOnce( \
      ::testing::Invoke( \
         [&](auto&& use_arg) { use_ptr = use_arg; \
           described_uses_[use_ptr] = _impl::UseDescription(#use_ptr, fin, fout, use_t::sched, use_t::immed); \
           use_arg->get_data_pointer_reference() = &value; \
         } \
    )); } \
  )

#define EXPECT_MOD_CAPTURE_MN_OR_MR(f_in, f_out, use, f_cont_out, cont_use) \
  EXPECT_CALL(*mock_runtime, make_next_flow(f_in)) \
    .WillOnce(::testing::Return(f_out)); \
  EXPECT_REGISTER_USE(use, f_in, f_out, Modify, Modify); \
  EXPECT_REGISTER_USE(cont_use, f_out, f_cont_out, Modify, None)

#define EXPECT_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(f_in, f_out, use, f_cont_out, cont_use, value) \
  EXPECT_CALL(*mock_runtime, make_next_flow(f_in)) \
    .WillOnce(::testing::Return(f_out)); \
  EXPECT_REGISTER_USE_AND_SET_BUFFER(use, f_in, f_out, Modify, Modify, value); \
  EXPECT_REGISTER_USE(cont_use, f_out, f_cont_out, Modify, None)

#define EXPECT_LEAF_MOD_CAPTURE_MN_OR_MR(f_in, f_out, use, f_cont_out, cont_use) \
  EXPECT_CALL(*mock_runtime, make_next_flow(f_in)) \
    .WillOnce(::testing::Return(f_out)); \
  EXPECT_REGISTER_USE(use, f_in, f_out, None, Modify); \
  EXPECT_REGISTER_USE(cont_use, f_out, f_cont_out, Modify, None)

#define EXPECT_LEAF_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(f_in, f_out, use, f_cont_out, cont_use, value) \
  EXPECT_CALL(*mock_runtime, make_next_flow(f_in)) \
    .WillOnce(::testing::Return(f_out)); \
  EXPECT_REGISTER_USE_AND_SET_BUFFER(use, f_in, f_out, None, Modify, value); \
  EXPECT_REGISTER_USE(cont_use, f_out, f_cont_out, Modify, None)

/* eventually expect release of flow */ \
/* EXPECT_CALL(*mock_runtime, release_flow(::testing::Eq(f_out))); */ \


#define EXPECT_RELEASE_USE(use_ptr) \
  ::_impl::in_sequence_wrapper( \
    EXPECT_CALL(*mock_runtime, legacy_release_use( \
      ::testing::Eq(::testing::ByRef(use_ptr))\
    )), [&](auto&& exp) -> decltype(auto) { \
      return exp.WillOnce(::testing::Assign(&use_ptr, nullptr)); \
  })

#define EXPECT_INITIAL_ACCESS(fin, fout, use, key) \
  EXPECT_CALL(*mock_runtime, make_initial_flow(is_handle_with_key(key))) \
    .WillOnce(::testing::Return(fin)); \
  EXPECT_CALL(*mock_runtime, make_null_flow(is_handle_with_key(key))) \
    .WillOnce(::testing::Return(fout)); \
  EXPECT_REGISTER_USE(use, fin, fout, Modify, None);

#define EXPECT_INITIAL_ACCESS_COLLECTION(fin, fout, use, key, size) \
  EXPECT_CALL(*mock_runtime, make_initial_flow_collection(is_handle_with_key(key))) \
    .WillOnce(::testing::Return(fin)); \
  EXPECT_CALL(*mock_runtime, make_null_flow_collection(is_handle_with_key(key))) \
    .WillOnce(::testing::Return(fout)); \
  EXPECT_REGISTER_USE_COLLECTION(use, fin, fout, Modify, None, size);

/* eventually expect release of flow */ \
/* EXPECT_CALL(*mock_runtime, release_flow(::testing::Eq(f_in))); */ \
/* EXPECT_CALL(*mock_runtime, release_flow(::testing::Eq(f_out))); */ \

#define EXPECT_READ_ACCESS(f_init, f_null, key, version_key) \
  EXPECT_CALL(*mock_runtime, make_fetching_flow(is_handle_with_key(key), ::testing::Eq(version_key), ::testing::Eq(false))) \
    .WillOnce(Return(f_init)); \
  EXPECT_CALL(*mock_runtime, make_null_flow(is_handle_with_key(key))) \
    .WillOnce(Return(f_null));


// Works either way with anti-flows enabled or disabled
#define EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR(fread, use_ptr) \
  EXPECT_CALL(*mock_runtime, legacy_register_use( \
    IsUseWithInFlow( \
      fread, -1, use_t::Read \
    ) \
  )).WillOnce(SaveArg<0>(&use_ptr))

// Works either way with anti-flows enabled or disabled
#define EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR_AND_SET_BUFFER(fread, use_ptr, value) \
    EXPECT_CALL(*mock_runtime, legacy_register_use( \
        IsUseWithInFlow( \
          fread, -1, use_t::Read \
        ) \
    )).WillOnce(::testing::Invoke([&](auto&& use_arg) { \
      use_ptr = use_arg; use_arg->get_data_pointer_reference() = &value; \
    }))


#define EXPECT_REGISTER_TASK(...) \
  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy( \
    UsesInGetDependencies(VectorOfPtrsToArgs(__VA_ARGS__)) \
  ))

#define EXPECT_RELEASE_FLOW(...) \
  EXPECT_CALL(*mock_runtime, release_flow(::testing::Eq(__VA_ARGS__)))

#define EXPECT_FLOW_ALIAS(f1, f2) \
  EXPECT_CALL(*mock_runtime, establish_flow_alias(::testing::Eq(f1), ::testing::Eq(f2)))

#define EXPECT_NEW_REGISTER_USE(use_ptr, fin, fin_rel, fin_src, fout, fout_rel, fout_src, out_rel_is_in, sched, immed, will_be_dep) \
  EXPECT_CALL(*mock_runtime, register_use(::testing::AllOf( \
    IsUseWithFlowRelationships( \
      ::darma_runtime::abstract::frontend::FlowRelationship::fin_rel, fin_src, \
      ::darma_runtime::abstract::frontend::FlowRelationship::fout_rel, fout_src, out_rel_is_in, \
      use_t::sched, use_t::immed \
    ), \
    UseWillBeDependency(will_be_dep) \
  ))).WillOnce(::testing::Invoke([&](auto&& use_arg) { \
    use_ptr = use_arg; \
    use_arg->set_in_flow(fin); \
    use_arg->set_out_flow(fout); \
  }))

#define EXPECT_REGISTER_USE_WITH_ANTI_FLOWS(use_ptr, \
  fin, fin_desc, fin_src, \
  fout, fout_desc, fout_src, out_rel_is_in, \
  fanti_in, fantiin_desc, fantiin_src, \
  fanti_out, fantiout_desc, fantiout_src, antiout_rel_is_anti_in, \
  sched, immed, will_be_dep \
) \
  EXPECT_CALL(*mock_runtime, register_use(::testing::AllOf( \
    IsUseWithFlowRelationships( \
      ::darma_runtime::abstract::frontend::FlowRelationship::fin_desc, fin_src, \
      ::darma_runtime::abstract::frontend::FlowRelationship::fout_desc, fout_src, out_rel_is_in, \
      use_t::sched, use_t::immed \
    ), \
    IsUseWithAntiFlowRelationships( \
      ::darma_runtime::abstract::frontend::FlowRelationship::fantiin_desc, fantiin_src, \
      ::darma_runtime::abstract::frontend::FlowRelationship::fantiout_desc, fantiout_src, antiout_rel_is_anti_in \
    ), \
    UseWillBeDependency(will_be_dep) \
  ))).WillOnce(::testing::Invoke([&](auto&& use_arg) { \
    use_ptr = use_arg; \
    use_arg->set_in_flow(fin); \
    use_arg->set_out_flow(fout); \
    use_arg->set_anti_in_flow(fanti_in); \
    use_arg->set_anti_out_flow(fanti_out); \
  }))

#define EXPECT_REGISTER_USE_WITH_ANTI_FLOWS_AND_SET_BUFFER(use_ptr, \
  fin, fin_desc, fin_src, \
  fout, fout_desc, fout_src, out_rel_is_in, \
  fanti_in, fantiin_desc, fantiin_src, \
  fanti_out, fantiout_desc, fantiout_src, antiout_rel_is_anti_in, \
  sched, immed, will_be_dep, value \
) \
  EXPECT_CALL(*mock_runtime, register_use(::testing::AllOf( \
    IsUseWithFlowRelationships( \
      ::darma_runtime::abstract::frontend::FlowRelationship::fin_desc, fin_src, \
      ::darma_runtime::abstract::frontend::FlowRelationship::fout_desc, fout_src, out_rel_is_in, \
      use_t::sched, use_t::immed \
    ), \
    IsUseWithAntiFlowRelationships( \
      ::darma_runtime::abstract::frontend::FlowRelationship::fantiin_desc, fantiin_src, \
      ::darma_runtime::abstract::frontend::FlowRelationship::fantiout_desc, fantiout_src, antiout_rel_is_anti_in \
    ), \
    UseWillBeDependency(will_be_dep) \
  ))).WillOnce(::testing::Invoke([&](auto&& use_arg) { \
    use_ptr = use_arg; \
    use_arg->set_in_flow(fin); \
    use_arg->set_out_flow(fout); \
    use_arg->set_anti_in_flow(fanti_in); \
    use_arg->set_anti_out_flow(fanti_out); \
    ::darma_runtime::abstract::frontend::use_cast<::darma_runtime::abstract::frontend::DependencyUse*>( \
      use_ptr \
    )->get_data_pointer_reference() = &(value); \
  }))

#define EXPECT_NEW_REGISTER_USE_AND_SET_BUFFER(use_ptr, fin, fin_rel, fin_src, fout, fout_rel, fout_src, out_rel_is_in, sched, immed, will_be_dep, value) \
  EXPECT_CALL(*mock_runtime, register_use(::testing::AllOf( \
    IsUseWithFlowRelationships( \
      ::darma_runtime::abstract::frontend::FlowRelationship::fin_rel, fin_src, \
      ::darma_runtime::abstract::frontend::FlowRelationship::fout_rel, fout_src, out_rel_is_in, \
      use_t::sched, use_t::immed \
    ), \
    UseWillBeDependency(will_be_dep) \
  ))).WillOnce(::testing::Invoke([&](auto&& use_arg) { \
    use_ptr = use_arg; \
    use_arg->set_in_flow(fin); \
    use_arg->set_out_flow(fout); \
    ::darma_runtime::abstract::frontend::use_cast<::darma_runtime::abstract::frontend::DependencyUse*>( \
      use_ptr \
    )->get_data_pointer_reference() = &(value); \
  }))

#define EXPECT_NEW_REGISTER_USE_WITH_EXTRA_CONSTRAINTS(use_ptr, fin, fin_rel, fin_src, fout, fout_rel, fout_src, out_rel_is_in, sched, immed, will_be_dep, extra_constraints...) \
  EXPECT_CALL(*mock_runtime, register_use(::testing::AllOf( \
    IsUseWithFlowRelationships( \
      ::darma_runtime::abstract::frontend::FlowRelationship::fin_rel, fin_src, \
      ::darma_runtime::abstract::frontend::FlowRelationship::fout_rel, fout_src, out_rel_is_in, \
      use_t::sched, use_t::immed \
    ), \
    UseWillBeDependency(will_be_dep), \
    extra_constraints \
  ))).WillOnce(::testing::Invoke([&](auto&& use_arg) { \
    use_ptr = use_arg; \
    use_arg->set_in_flow(fin); \
    use_arg->set_out_flow(fout); \
  }))

#define EXPECT_NEW_REGISTER_USE_WITH_EXTRA_CONSTRAINTS_AND_SET_BUFFER( \
  use_ptr, fin, fin_rel, fin_src, fout, fout_rel, fout_src, out_rel_is_in,\
  sched, immed, will_be_dep, buffer, extra_constraints... \
) \
  EXPECT_CALL(*mock_runtime, register_use(::testing::AllOf( \
    IsUseWithFlowRelationships( \
      ::darma_runtime::abstract::frontend::FlowRelationship::fin_rel, fin_src, \
      ::darma_runtime::abstract::frontend::FlowRelationship::fout_rel, fout_src, out_rel_is_in, \
      use_t::sched, use_t::immed \
    ), \
    UseWillBeDependency(will_be_dep), \
    extra_constraints \
  ))).WillOnce(::testing::Invoke([&](auto&& use_arg) { \
    use_ptr = use_arg; \
    use_arg->set_in_flow(fin); \
    use_arg->set_out_flow(fout); \
    ::darma_runtime::abstract::frontend::use_cast<::darma_runtime::abstract::frontend::DependencyUse*>( \
      use_ptr \
    )->get_data_pointer_reference() = &(value); \
  }))

#define EXPECT_NEW_REGISTER_USE_COLLECTION(use_ptr, fin, fin_rel, fin_src, \
  fout, fout_rel, fout_src, out_rel_is_in, sched, immed, will_be_dep, collsize) \
  EXPECT_CALL(*mock_runtime, register_use(::testing::AllOf(IsUseWithFlowRelationships( \
    ::darma_runtime::abstract::frontend::FlowRelationship::fin_rel|::darma_runtime::abstract::frontend::FlowRelationship::Collection, fin_src, \
    ::darma_runtime::abstract::frontend::FlowRelationship::fout_rel|::darma_runtime::abstract::frontend::FlowRelationship::Collection, fout_src, out_rel_is_in, \
    use_t::sched, use_t::immed \
  ), \
  UseWillBeDependency(will_be_dep), \
  ::testing::Truly([=](auto* use){ \
    return ( \
      use->manages_collection() \
      and \
        ::darma_runtime::abstract::frontend::use_cast< \
          ::darma_runtime::abstract::frontend::CollectionManagingUse* \
        >(use)->get_managed_collection()->size() == collsize \
    ); \
  })))).WillOnce(::testing::Invoke([&](auto&& use_arg) { \
    use_ptr = use_arg; \
    use_arg->set_in_flow(fin); \
    use_arg->set_out_flow(fout); \
  }))

#define EXPECT_NEW_INITIAL_ACCESS(fin, fout, use_ptr, key) \
  EXPECT_CALL(*mock_runtime, register_use(::testing::AllOf( \
    IsUseWithFlowRelationships( \
      ::darma_runtime::abstract::frontend::FlowRelationship::Initial, nullptr, \
      ::darma_runtime::abstract::frontend::FlowRelationship::Null, nullptr, false, \
      use_t::Modify, use_t::None \
    ), \
    UseWillBeDependency(false), \
    IsUseWithHandleKey(key) \
  ))).WillOnce(::testing::Invoke([&](auto&& use_arg) { \
    use_ptr = use_arg; \
    use_arg->set_in_flow(fin); \
    use_arg->set_out_flow(fout); \
  }))

#define EXPECT_NEW_INITIAL_ACCESS_COLLECTION(fin, fout, use_ptr, key, collsize) \
  EXPECT_CALL(*mock_runtime, register_use(::testing::AllOf( \
    IsUseWithFlowRelationships( \
      ::darma_runtime::abstract::frontend::FlowRelationship::InitialCollection, nullptr, \
      ::darma_runtime::abstract::frontend::FlowRelationship::NullCollection, nullptr, false, \
      use_t::Modify, use_t::None \
    ), \
    UseWillBeDependency(false), \
    IsUseWithHandleKey(key), \
    ::testing::Truly([=](auto* use){ \
      return ( \
        use->manages_collection() \
        and \
          ::darma_runtime::abstract::frontend::use_cast< \
            ::darma_runtime::abstract::frontend::CollectionManagingUse* \
          >(use)->get_managed_collection()->size() == collsize \
      ); \
    }) \
  ))).WillOnce(::testing::Invoke([&](auto&& use_arg) { \
    use_ptr = use_arg; \
    use_arg->set_in_flow(fin); \
    use_arg->set_out_flow(fout); \
  }))

#define EXPECT_NEW_RELEASE_USE(use_ptr, est_alias) \
  EXPECT_CALL(*mock_runtime, release_use(::testing::AllOf( \
    ::testing::Eq(::testing::ByRef(use_ptr)), \
    UseEstablishesAlias(est_alias) \
  ))).WillOnce(::testing::Assign(&use_ptr, nullptr));

// (Move this to somewhere more general if/when necessary)
#define _DARMA_PP_NUM_ARGS_HELPER(X100, X99, X98, X97, X96, X95, X94, X93, X92, X91, X90, X89, X88, X87, X86, X85, X84, X83, X82, X81, X80, X79, X78, X77, X76, X75, X74, X73, X72, X71, X70, X69, X68, X67, X66, X65, X64, X63, X62, X61, X60, X59, X58, X57, X56, X55, X54, X53, X52, X51, X50, X49, X48, X47, X46, X45, X44, X43, X42, X41, X40, X39, X38, X37, X36, X35, X34, X33, X32, X31, X30, X29, X28, X27, X26, X25, X24, X23, X22, X21, X20, X19, X18, X17, X16, X15, X14, X13, X12, X11, X10, X9, X8, X7, X6, X5, X4, X3, X2, X1, N, ...)   N

#define DARMA_PP_NUM_ARGS(...) _DARMA_PP_NUM_ARGS_HELPER(__VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define DECLARE_MOCK_FLOW(f1) \
  ::mock_backend::MockFlow f1(#f1);

namespace _make_mock_flows_impl {

template <typename FlowT, size_t I>
FlowT _do_make_flow(char const* names) {
  std::string all_names(names);
  all_names = all_names + ", ";
  std::string name;
  unsigned long pos = 0;
  int count = 0;
  while ((pos = all_names.find(", ")) != std::string::npos) {
    name = all_names.substr(0, pos);
    all_names.erase(0, pos + 2);
    if(count == I) return FlowT(name.c_str());
    ++count;
  }
  assert(false); // something went wrong...
  return FlowT();
}

template <typename FlowT, size_t... Idxs>
auto _make_flows_impl(char const* names, std::integer_sequence<std::size_t, Idxs...>) {
  return std::make_tuple(
    _do_make_flow<FlowT, Idxs>(names)...
  );
};

template <typename FlowT, size_t NFlows>
auto make_flows(char const* names) {
  return _make_flows_impl<FlowT>(names, std::make_index_sequence<NFlows>{});
}

} // end namespace _make_mock_flows_impl

#define DECLARE_MOCK_FLOWS(Args...) \
  ::mock_backend::MockFlow Args; std::tie(Args) = ::_make_mock_flows_impl::make_flows<::mock_backend::MockFlow, DARMA_PP_NUM_ARGS(Args)>(#Args);

#define DECLARE_MOCK_ANTI_FLOWS(Args...) \
  ::mock_backend::MockAntiFlow Args; std::tie(Args) = ::_make_mock_flows_impl::make_flows<::mock_backend::MockAntiFlow, DARMA_PP_NUM_ARGS(Args)>(#Args);

#define EXPECT_REGISTER_USE_COLLECTION(use_coll, fin, fout, sched, immed, collsize) \
    EXPECT_CALL(*mock_runtime, legacy_register_use(::testing::AllOf( \
      IsUseWithFlows(fin, fout, use_t::sched, use_t::immed), \
      ::testing::Truly([=](auto* use){ \
        return ( \
          use->manages_collection() \
          and \
            ::darma_runtime::abstract::frontend::use_cast< \
              ::darma_runtime::abstract::frontend::CollectionManagingUse* \
            >(use)->get_managed_collection()->size() == collsize \
        ); \
      }) \
    ))).WillOnce(::testing::Invoke([&](auto* use) { use_coll = use; }));

// </editor-fold> end Special EXPECT_* macros for DARMA
//==============================================================================


#define TEST_F_WITH_PARAMS(fixture, test_name, paramvals, params...) \
  struct _with_params_##fixture##_##test_name : fixture, ::testing::WithParamInterface<params> { }; \
  class GTEST_TEST_CLASS_NAME_(fixture, test_name); \
  INSTANTIATE_TEST_CASE_P(_cases_for_##fixture##_##test_name, _with_params_##fixture##_##test_name, paramvals); \
  TEST_P(_with_params_##fixture##_##test_name, test_name)


class TestFrontend
  : public ::testing::Test
{
  public:

    typedef darma_runtime::abstract::backend::runtime_t::task_t task_t;
    typedef darma_runtime::types::key_t key_t;
    using use_t = darma_runtime::abstract::frontend::Use;
    using handle_t = darma_runtime::abstract::frontend::Handle;
    using flow_t = mock_backend::MockFlow;

  protected:


    void setup_top_level_task() {
      int argc = 1;
      char const* args[] = { "<launched from gtest>" };
      char** argv = const_cast<char**>(&args[0]);
      top_level_task = darma_runtime::frontend::darma_top_level_setup(
        argc, argv
      );
    }

    template <template <class...> class Strictness = ::testing::StrictMock>
    void setup_mock_runtime() {
      using namespace ::testing;

      if(!top_level_task) {
        setup_top_level_task();
      }

      assert(mock_runtime == nullptr);
      mock_runtime = std::make_unique<Strictness<mock_backend::MockRuntime>>();
      mock_runtime->setup_default_delegators(top_level_task);

      mock_runtime_setup_done = true;
    }

    virtual void SetUp() {
      if(not mock_runtime_setup_done) {
        setup_mock_runtime();
      }
      sequence_marker = new StrictMock<MockSequenceMarker>();
    }

    virtual void TearDown() {
      top_level_task.reset();
      if(mock_runtime_setup_done) {
        mock_runtime = nullptr;
        mock_runtime_setup_done = false;
      }

      described_uses_.clear();

      delete sequence_marker;

    }

    auto is_handle_with_key(
      darma_runtime::types::key_t const& key
    ) {
      return ::testing::Truly([=](auto&& h) {
        return h->get_key() == key;
      });
    }

    ////////////////////////////////////////

    void
    run_all_tasks() {
      while(not mock_runtime->registered_tasks.empty()) {
        run_one_task();
      }
    }

    void
    run_one_task() {
      mock_runtime->registered_tasks.front()->run();
      mock_runtime->registered_tasks.pop_front();
    }

    void
    run_most_recently_added_task() {
      mock_runtime->registered_tasks.back()->run();
      mock_runtime->registered_tasks.pop_back();
    }
    ////////////////////////////////////////

  public:
    static std::map<use_t const *, ::_impl::UseDescription> described_uses_;

    ////////////////////////////////////////
  protected:

    mock_backend::MockRuntime::top_level_task_unique_ptr top_level_task;
    bool mock_runtime_setup_done = false;
};


namespace std {

using use_t = darma_runtime::abstract::frontend::Use;

inline std::ostream&
operator<<(std::ostream& o, use_t* const& u) {
  auto use_desc_iter = TestFrontend::described_uses_.find(u);
  if(use_desc_iter != TestFrontend::described_uses_.end()) {
    auto& use_desc = use_desc_iter->second;
    o << "<Use variable named \"" << use_desc.var_name
      << "\", expected to have in flow: " << use_desc.expected_in_flow
      << "\", out flow: " << use_desc.expected_out_flow
      << "\", sched: " << use_desc.expected_sched_permissions
      << "\", immed: " << use_desc.expected_immed_permissions
      << ">";
  }
  else if(u != nullptr) {
#if DARMA_SAFE_TEST_FRONTEND_PRINTERS
    o << "<non-null use (unprinted)>";
#else
    auto handle = u->get_handle();
    if(handle) {
      o << "<Use ptr for handle with key {" << handle->get_key()
        << "}, in_flow "
        << darma_runtime::abstract::frontend::use_cast<
             darma_runtime::abstract::frontend::RegisteredUse*
           >(u)->get_in_flow()
        << ", out_flow"
        << darma_runtime::abstract::frontend::use_cast<
             darma_runtime::abstract::frontend::RegisteredUse*
           >(u)->get_out_flow()
        << ", sched="
        << permissions_to_string(u->scheduling_permissions()) << ", immed="
        << permissions_to_string(u->immediate_permissions()) << ">";
    }
    else {
      o << "<Use ptr with null handle>";
    }
#endif
  }
  else {
    o << "<Undescribed null Use>";
  }
  /*
  if(u == nullptr) {
    o << "<null Use ptr>";
  }
  }
  */
  return o;
}

} // end namespace std

#endif /* SRC_TESTS_FRONTEND_VALIDATION_TEST_FRONTEND_H_ */
