/*
//@HEADER
// ************************************************************************
//
//                      test_create_parallel_for.cc
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>

#include "mock_backend.h"
#include "test_frontend.h"

#include <darma/interface/app/initial_access.h>
#include <darma/interface/app/read_access.h>
#include <darma/interface/app/create_work.h>
#include <darma/impl/parallel_for.h>


#if _darma_has_feature(create_parallel_for)

////////////////////////////////////////////////////////////////////////////////

class TestCreateParallelFor
  : public TestFrontend
{
  protected:

    virtual void SetUp() {
      using namespace ::testing;

      setup_mock_runtime<::testing::NiceMock>();
      TestFrontend::SetUp();
      ON_CALL(*mock_runtime, get_running_task())
        .WillByDefault(Return(top_level_task.get()));
    }

    virtual void TearDown() {
      TestFrontend::TearDown();
    }

};

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateParallelFor, simple_lambda) {
  using namespace darma;
  using namespace ::testing;
  using namespace darma::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  //============================================================================
  // actual code being tested
  {
    create_parallel_for(n_iterations=1, [=](int i) {
      ASSERT_EQ(i, 0);
    });
  }
  //============================================================================

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateParallelFor, simple_lambda_capture) {
  using namespace darma;
  using namespace ::testing;
  using namespace darma::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(finit, fnull, fcapt);
  use_t* use_capt, *use_init, *use_cont;

  std::vector<int> five_items{0, 0, 0, 0, 0};

  EXPECT_INITIAL_ACCESS(finit, fnull, use_init, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(finit, fcapt, use_capt, fnull, use_cont, five_items);
  EXPECT_RELEASE_USE(use_init);

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(
    AllOf(
      UseInGetDependencies(ByRef(use_capt)),
      Truly([&](auto* task) {
        return task->is_parallel_for_task();
      })
    )
  ));

  EXPECT_FLOW_ALIAS(fcapt, fnull);
  EXPECT_RELEASE_USE(use_cont);

  //============================================================================
  // actual code being tested
  {
    auto tmp = initial_access<std::vector<int>>("hello");

    create_parallel_for(n_iterations=5, [=](int i) {
      (*tmp)[i] = i;
    });
  }
  //============================================================================

  EXPECT_RELEASE_USE(use_capt);

  run_all_tasks();

  EXPECT_THAT(five_items, ElementsAre(0, 1, 2, 3, 4));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateParallelFor, simple_functor) {
  using namespace darma;
  using namespace ::testing;
  using namespace darma::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  //============================================================================
  // actual code being tested
  {
    struct Simple {
      void operator()(int i) const {
        ASSERT_EQ(i, 0);
      }
    };

    create_parallel_for<Simple>(n_iterations=1);
  }
  //============================================================================

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateParallelFor, simple_functor_args) {
  using namespace darma;
  using namespace ::testing;
  using namespace darma::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  //============================================================================
  // actual code being tested
  {
    struct Simple {
      void operator()(int i, int j) const {
        ASSERT_EQ(i, 0);
        ASSERT_EQ(j, 42);
      }
    };

    create_parallel_for<Simple>(42, n_iterations=1);
  }
  //============================================================================

  run_all_tasks();

}


////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateParallelFor, functor_args) {
  using namespace darma;
  using namespace ::testing;
  using namespace darma::keyword_arguments_for_parallel_for;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow finit, fnull, fcapt;
  use_t* use_capt, *use_init, *use_cont;

  std::vector<int> five_items{0, 0, 0, 0, 0};

  EXPECT_INITIAL_ACCESS(finit, fnull, use_init, make_key("hello"));

  EXPECT_LEAF_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(finit, fcapt, use_capt, fnull, use_cont, five_items);
  EXPECT_RELEASE_USE(use_init);

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(
    AllOf(
      UseInGetDependencies(ByRef(use_capt)),
      Truly([&](auto* task) {
        return task->is_parallel_for_task();
      })
    )
  ));

  EXPECT_FLOW_ALIAS(fcapt, fnull);
  EXPECT_RELEASE_USE(use_cont);

  //============================================================================
  // actual code being tested
  {
    struct Simple {
      void operator()(int i, std::vector<int>& vect) {
        vect[i] = i;
      }
    };

    auto tmp = initial_access<std::vector<int>>("hello");

    create_parallel_for<Simple>(tmp, n_iterations=5);
  }
  //============================================================================

  EXPECT_RELEASE_USE(use_capt);

  run_all_tasks();

  EXPECT_THAT(five_items, ElementsAre(0, 1, 2, 3, 4));
  mock_runtime->save_tasks = true;

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateParallelFor, resource_pack_passthrough) {
  using namespace darma;
  using namespace ::testing;
  using namespace darma::keyword_arguments_for_task_creation;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(
    Truly([](auto&& task){ return task->is_data_parallel_task(); })
  )).WillOnce(
    Invoke([](auto&& task) {
      task->set_resource_pack(MockResourcePack("hello"));
    }
  ));

  //============================================================================
  // actual code being tested
  {
    create_work(data_parallel=true, [=](auto const& resource_pack){
      ASSERT_THAT(resource_pack.name, Eq("hello"));
    });
  }
  //============================================================================

  run_all_tasks();
}


#endif
