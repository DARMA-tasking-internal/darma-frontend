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
#define DARMA_SAFE_TEST_FRONTEND_PRINTERS 1


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

#define EXPECT_MOD_CAPTURE_MN_OR_MR(f_in, f_out, use) \
  EXPECT_CALL(*mock_runtime, make_next_flow(f_in)) \
    .WillOnce(::testing::Return(f_out)); \
  EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows( \
    f_in, f_out, use_t::Modify, use_t::Modify \
  ))).WillOnce(::testing::SaveArg<0>(&use)); \

#define EXPECT_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(f_in, f_out, use, value) \
  EXPECT_CALL(*mock_runtime, make_next_flow(f_in)) \
    .WillOnce(::testing::Return(f_out)); \
  EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows( \
    f_in, f_out, use_t::Modify, use_t::Modify \
  ))).WillOnce(Invoke([&](auto&& use_arg){ \
    use = use_arg; use->get_data_pointer_reference() = &value; \
  }));

#define EXPECT_LEAF_MOD_CAPTURE_MN_OR_MR(f_in, f_out, use) \
  EXPECT_CALL(*mock_runtime, make_next_flow(f_in)) \
    .WillOnce(::testing::Return(f_out)); \
  EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows( \
    f_in, f_out, use_t::None, use_t::Modify \
  ))).WillOnce(::testing::SaveArg<0>(&use)); \

#define EXPECT_LEAF_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(f_in, f_out, use, value) \
  EXPECT_CALL(*mock_runtime, make_next_flow(f_in)) \
    .WillOnce(::testing::Return(f_out)); \
  EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows( \
    f_in, f_out, use_t::None, use_t::Modify \
  ))).WillOnce(Invoke([&](auto&& use_arg){ \
    use = use_arg; use->get_data_pointer_reference() = &value; \
  }));

/* eventually expect release of flow */ \
/* EXPECT_CALL(*mock_runtime, release_flow(::testing::Eq(f_out))); */ \

#define EXPECT_INITIAL_ACCESS(fin, fout, key) \
  EXPECT_CALL(*mock_runtime, make_initial_flow(is_handle_with_key(key))) \
    .WillOnce(::testing::Return(fin)); \
  EXPECT_CALL(*mock_runtime, make_null_flow(is_handle_with_key(key))) \
    .WillOnce(::testing::Return(fout)); \

#define EXPECT_INITIAL_ACCESS_COLLECTION(fin, fout, key) \
  EXPECT_CALL(*mock_runtime, make_initial_flow_collection(is_handle_with_key(key))) \
    .WillOnce(::testing::Return(fin)); \
  EXPECT_CALL(*mock_runtime, make_null_flow_collection(is_handle_with_key(key))) \
    .WillOnce(::testing::Return(fout)); \

/* eventually expect release of flow */ \
/* EXPECT_CALL(*mock_runtime, release_flow(::testing::Eq(f_in))); */ \
/* EXPECT_CALL(*mock_runtime, release_flow(::testing::Eq(f_out))); */ \

#define EXPECT_READ_ACCESS(f_init, f_null, key, version_key) \
  EXPECT_CALL(*mock_runtime, make_fetching_flow(is_handle_with_key(key), ::testing::Eq(version_key), ::testing::Eq(false))) \
    .WillOnce(Return(f_init)); \
  EXPECT_CALL(*mock_runtime, make_null_flow(is_handle_with_key(key))) \
    .WillOnce(Return(f_null));

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
          &fread, &fread, -1, use_t::Read \
        ) \
    )), [&](auto&& exp) -> decltype(auto) { return exp.WillOnce(SaveArg<0>(&use_ptr)); } \
  )

#define EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR_AND_SET_BUFFER(fread, use_ptr, value) \
  ::_impl::in_sequence_wrapper( \
    EXPECT_CALL(*mock_runtime, register_use( \
        IsUseWithFlows( \
          &fread, &fread, -1, use_t::Read \
        ) \
    )), [&](auto&& exp) -> decltype(auto) { \
          return exp.WillOnce(::testing::Invoke([&](auto&& use_arg) { \
            use_ptr = use_arg; use_ptr->get_data_pointer_reference() = &value; \
          })); \
        } \
  )

#define EXPECT_REGISTER_USE(use_ptr, fin, fout, sched, immed) \
  ::_impl::in_sequence_wrapper( \
    EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows( \
      &fin, &fout, use_t::sched, use_t::immed \
    ))), [&](auto&& exp) -> decltype(auto) { return exp.WillOnce(SaveArg<0>(&use_ptr)); } \
  )

#define EXPECT_REGISTER_USE_AND_SET_BUFFER(use_ptr, fin, fout, sched, immed, value) \
  ::_impl::in_sequence_wrapper( \
    EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows( \
      &fin, &fout, use_t::sched, use_t::immed \
    ))), [&](auto&& exp) -> decltype(auto) { return exp.WillOnce( \
      ::testing::Invoke( \
         [&](auto&& use_arg) { use_ptr = use_arg; use_ptr->get_data_pointer_reference() = &value; } \
    )); } \
  )

#define EXPECT_REGISTER_TASK(...) \
  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy( \
    UsesInGetDependencies(VectorOfPtrsToArgs(__VA_ARGS__)) \
  ))

#define EXPECT_RELEASE_FLOW(...) \
  EXPECT_CALL(*mock_runtime, release_flow(::testing::Eq(__VA_ARGS__)))

#define EXPECT_FLOW_ALIAS(f1, f2) \
  EXPECT_CALL(*mock_runtime, establish_flow_alias(::testing::Eq(f1), ::testing::Eq(f2)))

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
      ON_CALL(*mock_runtime, get_running_task())
        .WillByDefault(Return(top_level_task.get()));
      ON_CALL(*mock_runtime, allocate(_, _))
        .WillByDefault(Invoke([](auto size, auto const& details) {
          return ::operator new(size);
        }));
      ON_CALL(*mock_runtime, deallocate(_, _))
        .WillByDefault(Invoke([](auto* ptr, auto size) {
#if defined(__cpp_sized_deallocation)
          ::operator delete(ptr, size);
#else
          ::operator delete(ptr);
#endif
        }));

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
      return ::testing::Truly([=](auto&& h) {
        return h->get_key() == key;
      });
    }

    ////////////////////////////////////////

    void
    run_all_tasks() {
      while(not mock_runtime->registered_tasks.empty()) {
        mock_runtime->registered_tasks.front()->run();
        mock_runtime->registered_tasks.pop_front();
      }
    }

    ////////////////////////////////////////

//    void
//    run_one_cr_rank(bool do_move = true) {
//      using namespace mock_backend;
//      auto& cr = mock_runtime->concurrent_regions.front();
//      std::shared_ptr<MockConcurrentRegionContextHandle> ctxt =
//        std::make_shared<MockConcurrentRegionContextHandle>();
//      EXPECT_CALL(*ctxt, get_backend_index())
//        .Times(AtLeast(0))
//        .WillRepeatedly(Return(cr.n_run_so_far));
//
//      // do SerDes on the task object so that the same instance doesn't get
//      // run multiple times, in keeping with task life cycle
//      size_t task_size = cr.task->get_packed_size();
//      char buffer[task_size];
//      cr.task->pack(buffer);
//
//      auto task_copy =
//        darma_runtime::frontend::unpack_concurrent_region_task(buffer);
//
//      task_copy->set_region_context(ctxt);
//      task_copy->run();
//    }
//
//    void
//    run_all_cr_ranks_for_one_region_in_serial() {
//      auto& cr = mock_runtime->concurrent_regions.front();
//      while(cr.n_run_so_far < cr.n_indices) {
//        run_one_cr_rank();
//        ++cr.n_run_so_far;
//      }
//      mock_runtime->concurrent_regions.pop_front();
//    }

    ////////////////////////////////////////

    mock_backend::MockRuntime::top_level_task_unique_ptr top_level_task;
    bool mock_runtime_setup_done = false;
};


namespace std {

using use_t = darma_runtime::abstract::frontend::Use;

inline std::ostream&
operator<<(std::ostream& o, use_t const* const& u) {
  if(u == nullptr) {
    o << "<null Use ptr>";
  }
  else {
#if DARMA_SAFE_TEST_FRONTEND_PRINTERS
    o << "<non-null use (unprinted)>";
#else
    auto handle = u->get_handle();
    if(handle) {
      o << "<Use ptr for handle with key {" << handle->get_key()
        << "}, in_flow " << u->get_in_flow() << ", out_flow" << u->get_out_flow() << ", sched="
        << permissions_to_string(u->scheduling_permissions()) << ", immed="
        << permissions_to_string(u->immediate_permissions()) << ">";
    }
    else {
      o << "<Use ptr with null handle>";
    }
#endif
  }
  return o;
}

} // end namespace std

#endif /* SRC_TESTS_FRONTEND_VALIDATION_TEST_FRONTEND_H_ */
