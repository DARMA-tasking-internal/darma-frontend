/*
//@HEADER
// ************************************************************************
//
//                      test_create_concurrent_region.cc
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>


#include "mock_backend.h"
#include "test_frontend.h"

#include <darma/interface/app/initial_access.h>
#include <darma/interface/app/read_access.h>
#include <darma/interface/app/create_work.h>
#include <darma/impl/data_store.h>
#include <darma/impl/array/range_2d.h>

////////////////////////////////////////////////////////////////////////////////

class TestCreateConcurrentRegion
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

struct MyCR {

  void operator()(
    darma_runtime::ConcurrentRegionContext<darma_runtime::Range2D<int>> context
  ) const {
    std::cout << "(" << context.index().x() << ", " << context.index().y() << ") ";
  }
};

TEST_F(TestCreateConcurrentRegion, simple_2d) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_create_concurrent_region;
  using namespace mock_backend;

  testing::internal::CaptureStdout();

  std::shared_ptr<MockDataStoreHandle> ds = std::make_shared<MockDataStoreHandle>();

  EXPECT_CALL(*mock_runtime, make_data_store()).WillOnce(Return(ds));

  EXPECT_CALL(*mock_runtime, register_concurrent_region_gmock_proxy(_, 6, ds.get()));

  //============================================================================
  // actual code to be tested
  {

    auto my_ds = create_data_store();

    create_concurrent_region<MyCR>(
      Range2D<int>(3, 2), data_store=my_ds
    );

  }
  //============================================================================

  run_all_cr_ranks_for_one_region_in_serial();

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "(0, 0) (0, 1) (1, 0) (1, 1) (2, 0) (2, 1) "
  );

}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestCreateConcurrentRegion, simple_2d_argument) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_create_concurrent_region;
  using namespace mock_backend;

  testing::internal::CaptureStdout();

  EXPECT_CALL(*mock_runtime, register_concurrent_region_gmock_proxy(_, 6, nullptr));

  //============================================================================
  // actual code to be tested
  {
    struct MyCRHello {

      void operator()(
        darma_runtime::ConcurrentRegionContext<darma_runtime::Range2D<int>> context,
        int val
      ) const {
        std::cout << val;
        std::cout << " (" << context.index().x() << ", " << context.index().y() << ")";
        std::cout << std::endl;
      }
    };

    create_concurrent_region<MyCRHello>(
      123,
      index_range=Range2D<int>(3, 2)
    );

  }
  //============================================================================

  run_all_cr_ranks_for_one_region_in_serial();

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "123 (0, 0)\n"
      "123 (0, 1)\n"
      "123 (1, 0)\n"
      "123 (1, 1)\n"
      "123 (2, 0)\n"
      "123 (2, 1)\n"
  );

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateConcurrentRegion, simple_1d) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_create_concurrent_region;
  using namespace mock_backend;

  testing::internal::CaptureStdout();

  EXPECT_CALL(*mock_runtime, register_concurrent_region_gmock_proxy(_, 6, nullptr));

  //============================================================================
  // actual code to be tested
  {
    struct MyCR1D {

      void operator()(
        darma_runtime::ConcurrentRegionContext<darma_runtime::Range1D<int>> context
      ) const {
        std::cout << context.index().value;
      }
    };

    create_concurrent_region<MyCR1D>(
      index_range=Range1D<int>(6)
    );

  }
  //============================================================================

  run_all_cr_ranks_for_one_region_in_serial();

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "012345"
  );

}
