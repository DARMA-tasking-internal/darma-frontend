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

#include <deque>
#include <iomanip>
#include <iostream>

#include <gtest/gtest.h>

#include <darma.h>

#include <darma/impl/task.h>
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

} // end namespace _impl

// </editor-fold> end wrapper to make EXPECT_*().InSequence() work as expected
//==============================================================================


//==============================================================================
// <editor-fold desc="Special EXPECT_* macros for DARMA">

// Note that these can't just be functions or methods (like they used to be
// because the line numbers get screwed up and debugging is much harder

#define EXPECT_MOD_CAPTURE_MN_OR_MR(f_in, f_out, use) { \
  EXPECT_CALL(*mock_runtime, make_next_flow(&f_in)) \
    .WillOnce(::testing::Return(&f_out)); \
  EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows( \
    &f_in, &f_out, use_t::Modify, use_t::Modify \
  ))).WillOnce(::testing::SaveArg<0>(&use)); \
}

#define EXPECT_INITIAL_ACCESS(fin, fout, key) { \
  EXPECT_CALL(*mock_runtime, make_initial_flow(is_handle_with_key(key))) \
    .WillOnce(::testing::Return(fin)); \
  EXPECT_CALL(*mock_runtime, make_null_flow(is_handle_with_key(key))) \
    .WillOnce(::testing::Return(fout)); \
}
#define EXPECT_RELEASE_USE(use_ptr) \
  ::_impl::in_sequence_wrapper( \
    EXPECT_CALL(*mock_runtime, release_use( \
      ::testing::Eq(::testing::ByRef(use_ptr))\
    )), [&](auto&& exp) -> decltype(auto) { \
      return exp.WillOnce(::testing::Assign(&use_ptr, nullptr)); \
  })


#define EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR(fread, use_ptr) \
  ::_impl::in_sequence_wrapper( \
    EXPECT_CALL(*mock_runtime, register_use( \
        IsUseWithFlows( \
          &fread, &fread, use_t::Read, use_t::Read \
        ) \
    )), [&](auto&& exp) -> decltype(auto) { return exp.WillOnce(SaveArg<0>(&use_ptr)); } \
  )

#define EXPECT_REGISTER_USE(use_ptr, fin, fout, sched, immed, ...) \
  ::_impl::in_sequence_wrapper( \
    EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows( \
      &fin, &fout, use_t::sched, use_t::immed \
    ))), [&](auto&& exp) -> decltype(auto) { return exp.WillOnce(SaveArg<0>(&use_ptr)); } \
  )

#define EXPECT_REGISTER_TASK(...) \
  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy( \
    UsesInGetDependencies(VectorOfPtrsToArgs(__VA_ARGS__)) \
  ))

// </editor-fold> end Special EXPECT_* macros for DARMA
//==============================================================================

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
      top_level_task = std::make_unique<darma_runtime::detail::TopLevelTask>();
    }

    template <template <class...> class Strictness = ::testing::StrictMock>
    void setup_mock_runtime() {
      using namespace ::testing;

      if(!top_level_task) {
        setup_top_level_task();
      }

      assert(mock_runtime == nullptr);
      mock_runtime = std::make_unique<Strictness<mock_backend::MockRuntime>>();
      ON_CALL(*mock_runtime, get_running_task())
        .WillByDefault(Return(top_level_task.get()));

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

      delete sequence_marker;

    }

    auto is_handle_with_key(
      darma_runtime::types::key_t const& key
    ) {
      return ::testing::Truly([=](handle_t const* h) {
        return h->get_key() == key;
      });
    }

    ////////////////////////////////////////

    auto expect_initial_access(
      mock_backend::MockFlow& fin,
      mock_backend::MockFlow& fout,
      darma_runtime::types::key_t const& key,
      ::testing::Sequence const& s_register = Sequence(),
      ::testing::Sequence const& s_release = Sequence()
    ) {
      using namespace darma_runtime;
      using namespace mock_backend;
      using namespace ::testing;

      Sequence s1, s2;

      std::map<std::string, Expectation> expectations;

      expectations["make in flow"] =
        EXPECT_CALL(*mock_runtime, make_initial_flow(is_handle_with_key(key)))
          .InSequence(s1)
          .WillOnce(Return(&fin));
      expectations["make out flow"] =
        EXPECT_CALL(*mock_runtime, make_null_flow(is_handle_with_key(key)))
          .InSequence(s2)
          .WillOnce(Return(&fout));


      return expectations;
    }

    ////////////////////////////////////////

    auto expect_read_access(
      mock_backend::MockFlow& fin,
      mock_backend::MockFlow& fout,
      darma_runtime::types::key_t const& key,
      darma_runtime::types::key_t const& version_key,
      ::testing::Sequence const& s_register = ::testing::Sequence(),
      ::testing::Sequence const& s_release = ::testing::Sequence()
    ) {
      using namespace darma_runtime;
      using namespace mock_backend;
      using namespace ::testing;

      Sequence s1, s2;

      std::map<std::string, Expectation> expectations;

      expectations["make in flow"] =
        EXPECT_CALL(*mock_runtime, make_fetching_flow(is_handle_with_key(key), Eq(version_key)))
          .Times(1).InSequence(s1)
          .WillOnce(Return(&fin));
      expectations["make out flow"] =
        EXPECT_CALL(*mock_runtime, make_null_flow(is_handle_with_key(key)))
          .Times(1).InSequence(s2)
          .WillOnce(Return(&fout));

      return expectations;
    }


    ////////////////////////////////////////

    auto expect_mod_capture_MN_or_MR(
      mock_backend::MockFlow& f_in, mock_backend::MockFlow& f_out, use_t*& use
    ) {

      using namespace mock_backend;

      EXPECT_CALL(*mock_runtime, make_next_flow(&f_in))
        .WillOnce(Return(&f_out));

      EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows(
        &f_in, &f_out, use_t::Modify, use_t::Modify
      ))).WillOnce(SaveArg<0>(&use));


    }

    ////////////////////////////////////////

    auto expect_ro_capture_RN_RR_MN_or_MR(
      mock_backend::MockFlow& fread, use_t*& use_ptr_capt,
      ::testing::Sequence const& seq = ::testing::Sequence()
    ) {
      using namespace mock_backend;

      EXPECT_CALL(*mock_runtime, register_use(
        IsUseWithFlows(
          &fread, &fread, use_t::Read, use_t::Read
        )
      )).InSequence(seq).WillOnce(SaveArg<0>(&use_ptr_capt));

    }

    ////////////////////////////////////////

    void
    run_all_tasks() {
      while(not mock_runtime->registered_tasks.empty()) {
        mock_runtime->registered_tasks.front()->run<void>();
        mock_runtime->registered_tasks.pop_front();
      }
    }

    ////////////////////////////////////////

    ////////////////////////////////////////

    mock_backend::MockRuntime::task_unique_ptr top_level_task;
    bool mock_runtime_setup_done = false;
};

//namespace frontend_test_helpers {
//
//using use_t = darma_runtime::abstract::frontend::Use;
//using handle_t = darma_runtime::abstract::frontend::Handle;
//using flow_t = mock_backend::MockFlow;
//
//struct captured;
//struct continuing;
//
//struct handle_expectation {
//  protected:
//    use_t* this_use;
//
//    flow_t* in_flow;
//    flow_t* out_flow;
//
//    struct state_t {
//      use_t::permissions_t scheduleing;
//      use_t::permissions_t immediate;
//    };
//
//    state_t state;
//
//    virtual void do_expects() = 0;
//
//    virtual ~handle_expectations() { do_expects(); }
//
//  public:
//
//    handle_expectation()
//      : in_flow(new mock_backend::MockFlow),
//        out_flow(new mock_backend::MockFlow)
//    {
//      ::testing::Mock::AllowLeak(in_flow);
//      ::testing::Mock::AllowLeak(out_flow);
//    }
//
//    handle_expectation* ro_capture(
//      captured&&, continuing&&
//    );
//
//    handle_expectation* mod_capture(
//      captured&&, continuing&&
//    );
//
//};
//
//struct captured: handle_expectation {
//
//};
//
//struct continuing: handle_expectation {
//
//};
//
//struct initial: handle_expectation {
//  protected:
//
//    darma_runtime::types::key_t name_key_;
//
//  public:
//
//    initial(darma_runtime::types::key_t& name_key)
//      : name_key_(name_key),
//        state(state_t{use_t::Modify, use_t::None})
//    {
//
//    }
//
//};
//
//handle_expectation*
//handle_expectation::ro_capture(
//  captured&& capt,
//  continuing&& cont
//) {
//  using namespace ::testing;
//
//  Sequence s1;
//
//  switch(state) {
//    // ro-capture of RN, RR, MN, or MR
//    case state_t{use_t::Read, use_t::None}:
//    case state_t{use_t::Read, use_t::Read}:
//    case state_t{use_t::Modify, use_t::None}:
//    case state_t{use_t::Modify, use_t::Read}: {
//      EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_in_0), MockRuntime::Input))
//        .Times(1).InSequence(s1)
//        .WillOnce(Return(&fl_in_1));
//      EXPECT_CALL(*mock_runtime, make_same_flow(Eq(&fl_in_1), MockRuntime::OutputFlowOfReadOperation))
//        .Times(1).InSequence(s1)
//        .WillOnce(Return(&fl_out_1));
//
//      EXPECT_CALL(*mock_runtime, register_use(
//        IsUseWithFlows(&fl_in_1, &fl_out_1, use_t::Read, use_t::Read)
//      )).Times(1).InSequence(s1)
//        .WillOnce(SaveArg<0>(&use_1));
//
//      break;
//    }
//}
//
//EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(UseInGetDependencies(ByRef(use_1))))
//.Times(1).InSequence(s1);
//
//{
//auto tmp = read_access<int>("hello", version="world");
//create_work([=]{
//  std::cout << tmp.get_value();
//  FAIL() << "This code block shouldn't be running in this example";
//});
//
//ASSERT_THAT(use_0, NotNull());
//
//}
//
//} // end namespace test_frontend_helpers

//namespace _impl {
//
//
//using namespace darma_runtime;
//using namespace darma_runtime::detail;
//typedef typename abstract::backend::runtime_t::handle_t handle_t;
//
//template <typename Handle, typename Enable=void>
//struct assert_version_is_helper;
//
//template <>
//struct assert_version_is_helper<handle_t*> {
//  ::testing::AssertionResult
//  operator()(
//    const char* v1_expr,
//    const char* v2_expr,
//    handle_t* h,
//    darma_runtime::types::version_t v2,
//    bool exact=false
//  ) const {
//    auto& v1 = h->get_version();
//
//    bool success = v1 == v2;
//    if(success and exact) success = v1.depth() == v2.depth();
//
//    if(success) return ::testing::AssertionSuccess();
//
//    return ::testing::AssertionFailure()
//      << "Version associated with " << v1_expr << ", which is "
//      << v1 << ", is not"
//      << (exact ? std::string(" exactly ") : std::string(" "))
//      << "equal to expected value of " << v2 << " (from expression "
//      << v2_expr << ")";
//  }
//};
//
//template <typename... Args>
//struct assert_version_is_helper<AccessHandle<Args...>> {
//  ::testing::AssertionResult
//  operator()(
//    const char* v1_expr,
//    const char* v2_expr,
//    AccessHandle<Args...> const& ah,
//    darma_runtime::types::version_t v2,
//    bool exact=false
//  ) const {
//    using darma_runtime::detail::create_work_attorneys::for_AccessHandle;
//    handle_t* handle = for_AccessHandle::get_dep_handle(ah);
//    return assert_version_is_helper<handle_t*>()(v1_expr, v2_expr, handle, v2);
//  }
//
//
//};
//
//} // end namespace _impl
//
//template <bool exact, typename VersionFrom>
//::testing::AssertionResult
//AssertHandleVersionIs(
//  const char* v1_expr,
//  const char* v2_expr,
//  VersionFrom&& vf,
//  darma_runtime::types::version_t v2
//) {
//  return _impl::assert_version_is_helper<std::decay_t<VersionFrom>>()(
//    v1_expr, v2_expr, std::forward<VersionFrom>(vf), v2, exact
//  );
//}
//
//#define EXPECT_VERSION_EQ(h, ...) EXPECT_PRED_FORMAT2(AssertHandleVersionIs<false>, h, TestFrontend::version_t(__VA_ARGS__))
//#define EXPECT_VERSION_EQ_EXACT(h, ...) EXPECT_PRED_FORMAT2(AssertHandleVersionIs<true>, h, TestFrontend::version_t(__VA_ARGS__))
//#define ASSERT_VERSION_EQ(h, ...) ASSERT_PRED_FORMAT2(AssertHandleVersionIs<false>, h, TestFrontend::version_t(__VA_ARGS__))
//#define ASSERT_VERSION_EQ_EXACT(h, ...) ASSERT_PRED_FORMAT2(AssertHandleVersionIs<true>, h, TestFrontend::version_t(__VA_ARGS__))


namespace std {

using use_t = darma_runtime::abstract::frontend::Use;

inline std::ostream&
operator<<(std::ostream& o, use_t const* const& u) {
  if(u == nullptr) {
    o << "<null Use ptr>";
  }
  else {
    auto handle = u->get_handle();
    if(handle) {
      o << "<Use ptr for handle with key " << handle->get_key() << ">";
    }
    else {
      o << "<Use ptr with null handle>";
    }
  }
  return o;
}

} // end namespace std

#endif /* SRC_TESTS_FRONTEND_VALIDATION_TEST_FRONTEND_H_ */
