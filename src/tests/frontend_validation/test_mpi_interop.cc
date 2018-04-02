/*
//@HEADER
// ************************************************************************
//
//                       test_mpi_interop.cc
//                         darma_new
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

class TestCreateWork_mod_capture_MM_Test;
#define DARMA_TEST_FRONTEND_VALIDATION_MPI_INTEROP 1

#include "mock_free_functions.h"
#include "test_frontend.h"

#include <darma/impl/mpi/mpi_context.h>
#include <darma/impl/index_range/mapping.h>
#include <darma/impl/array/index_range.h>

////////////////////////////////////////////////////////////////////////////////

class TestMPIInterop
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

TEST_F(TestMPIInterop, baseline_test) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_mpi_context;
  using namespace mock_backend;

/*
  DECLARE_MOCK_FLOWS(finit1, finit2, fnull1, fnull2, fout1, fout2);
  use_t *use_1, *use_2, *init_use_1, *init_use_2, *cont_use_1, *cont_use_2;

  EXPECT_INITIAL_ACCESS(finit1, fnull1, init_use_1, make_key("hello"));
  EXPECT_INITIAL_ACCESS(finit2, fnull2, init_use_2, make_key("world"));

  EXPECT_MOD_CAPTURE_MN_OR_MR(finit1, fout1, use_1, fnull1, cont_use_1);

  EXPECT_RELEASE_USE(init_use_1);

  EXPECT_MOD_CAPTURE_MN_OR_MR(finit2, fout2, use_2, fnull2, cont_use_2);

  EXPECT_RELEASE_USE(init_use_2);

  EXPECT_REGISTER_TASK(use_1, use_2);

  EXPECT_FLOW_ALIAS(fout1, fnull1);

  EXPECT_RELEASE_USE(cont_use_1);

  EXPECT_FLOW_ALIAS(fout2, fnull2);

  EXPECT_RELEASE_USE(cont_use_2);
*/

  //============================================================================
  // Actual code being tested
  {

/*
    std::vector<AccessHandle<int>> handles;

    handles.push_back(initial_access<int>("hello"));
    handles.emplace_back(initial_access<int>("world"));

    create_work([=]{
      handles[0].set_value(5);
      FAIL() << "This code block shouldn't be running in this example";
    });
*/

    auto context = mpi_context(types::MPI_Comm{});

    double gb = 4;
    double gb1 = 1;
    double gb2 = 3;  
    double gb3 = 6;
    auto p1 = context.piecewise_acquired_collection<double>("Giulio", index_range = Range1D<int>(4));
    auto p2 = context.piecewise_acquired_collection<double>("Giulio2", size = 10);
    auto token3 = context.piecewise_acquired_collection<double>("Giulio3", index_range = Range1D<int>(5), darma_runtime::keyword_arguments_for_mpi_context::index = 2, data = gb);
    auto token4 = context.piecewise_acquired_collection<double>("Giulio4", size = 10, darma_runtime::keyword_arguments_for_mpi_context::index = 2, data = gb);
    auto token5 = context.piecewise_acquired_collection<double>("Giulio5", index_range = Range1D<int>(6), indices(1,2,3),data(gb1,gb2,gb3));
    auto token6 = context.piecewise_acquired_collection<double>("Giulio6", size = 10, indices(1,2,3),data(gb1,gb2,gb3));

    p1.acquire_access(gb,darma_runtime::keyword_arguments_for_piecewise_handle::index = 2);
    //conversion_to_ahc(gb,p1);

  } // handles deleted
  //============================================================================

/*
  EXPECT_RELEASE_USE(use_1);

  EXPECT_RELEASE_USE(use_2);

  mock_runtime->registered_tasks.clear();
*/

}

