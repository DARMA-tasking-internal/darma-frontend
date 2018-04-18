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

// #include "mock_free_functions.h"
#include "test_frontend.h"
#include "mock_free_functions.h"

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

TEST_F(TestMPIInterop, piecewise_collection_test) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_mpi_context;
  using namespace mock_backend;

  types::MPI_Comm mpi_comm{};

  EXPECT_CALL(*mock_mpi_backend, create_runtime_context(_)).WillOnce(::testing::Return(types::runtime_context_token_t{}));

  EXPECT_CALL(*mock_mpi_backend, register_piecewise_collection(_, _, _)).WillOnce(::testing::Return(types::piecewise_collection_token_t{}));

  //============================================================================
  // Actual code being tested
  {

    auto context = mpi_context(types::MPI_Comm{});

    auto piecewise_collection = context.piecewise_acquired_collection<double>("Hello", index_range = Range1D<int>(4));

  } 
  //============================================================================

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestMPIInterop, acquire_access_test) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_mpi_context;
  using namespace mock_backend;

  double buffer_data = 1.0;
  types::MPI_Comm mpi_comm{};

  EXPECT_CALL(*mock_mpi_backend, create_runtime_context(_)).WillOnce(::testing::Return(types::runtime_context_token_t{}));

  EXPECT_CALL(*mock_mpi_backend, register_piecewise_collection(_, _, _)).WillOnce(::testing::Return(types::piecewise_collection_token_t{}));

  EXPECT_CALL(*mock_mpi_backend, register_piecewise_collection_piece(_, _, 0, &buffer_data, _, _));

  //============================================================================
  // Actual code being tested
  {

    auto context = mpi_context(types::MPI_Comm{});

    auto piecewise_collection = context.piecewise_acquired_collection<double>("Hello", index_range = Range1D<int>(4));

    piecewise_collection.acquire_access(buffer_data, darma_runtime::keyword_arguments_for_piecewise_handle::index = 0);

  }
  //============================================================================

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestMPIInterop, piecewise_collection_with_data_test) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_mpi_context;
  using namespace mock_backend;

  double buffer_data_1 = 1.0;
  double buffer_data_2 = 2.0;
  types::MPI_Comm mpi_comm{};

  EXPECT_CALL(*mock_mpi_backend, create_runtime_context(_)).WillOnce(::testing::Return(types::runtime_context_token_t{}));

  EXPECT_CALL(*mock_mpi_backend, register_piecewise_collection(_, _, _)).WillOnce(::testing::Return(types::piecewise_collection_token_t{}));

  EXPECT_CALL(*mock_mpi_backend, register_piecewise_collection_piece(_, _, 0, &buffer_data_1, _, _));

  EXPECT_CALL(*mock_mpi_backend, register_piecewise_collection_piece(_, _, 1, &buffer_data_2, _, _));

  //============================================================================
  // Actual code being tested
  {

    auto context = mpi_context(types::MPI_Comm{});

    auto piecewise_collection = context.piecewise_acquired_collection<double>(
      "Hello", 
      index_range = Range1D<int>(4),
      data(buffer_data_1, buffer_data_2),
      indices(0, 1)
    );

  }
  //============================================================================

}


