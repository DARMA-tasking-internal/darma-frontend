/*
//@HEADER
// ************************************************************************
//
//                      test_resource_count.cc
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

#include <darma/interface/app/resource_count.h>

////////////////////////////////////////////////////////////////////////////////

class TestResourceCount
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

TEST_F(TestResourceCount, execution_resources_simple) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::constants_for_resource_count;
  using namespace darma_runtime::keyword_arguments;

  EXPECT_CALL(*mock_runtime, get_execution_resource_count(0)).Times(4);
  darma_runtime::resource_count(Execution, Processes);
  darma_runtime::resource_count(Execution, depth=0);
  darma_runtime::resource_count(Execution, depth=Process);
  darma_runtime::resource_count(Processes);

  EXPECT_CALL(*mock_runtime, get_execution_resource_count(1)).Times(4);
  darma_runtime::resource_count(Execution, Sockets);
  darma_runtime::resource_count(Execution, depth=1);
  darma_runtime::resource_count(Execution, depth=Socket);
  darma_runtime::resource_count(Sockets);

  EXPECT_CALL(*mock_runtime, get_execution_resource_count(2)).Times(4);
  darma_runtime::resource_count(Execution, Cores);
  darma_runtime::resource_count(Execution, depth=2);
  darma_runtime::resource_count(Execution, depth=Core);
  darma_runtime::resource_count(Cores);

  EXPECT_CALL(*mock_runtime, get_execution_resource_count(3)).Times(4);
  darma_runtime::resource_count(Execution, HardwareThreads);
  darma_runtime::resource_count(Execution, depth=3);
  darma_runtime::resource_count(Execution, depth=HardwareThread);
  darma_runtime::resource_count(HardwareThreads);

}

TEST_F(TestResourceCount, execution_resources_per) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::constants_for_resource_count;
  using namespace darma_runtime::keyword_arguments;

  EXPECT_CALL(*mock_runtime, get_execution_resource_count(0)).Times(3).WillRepeatedly(Return(10));
  EXPECT_CALL(*mock_runtime, get_execution_resource_count(2)).Times(3).WillRepeatedly(Return(50));
  EXPECT_EQ(darma_runtime::resource_count(Execution, Cores, per=Process), 5);
  EXPECT_EQ(darma_runtime::resource_count(Cores, per=Process), 5);
  // needs generalized keyword argument parsing, which isn't done yet
  //EXPECT_EQ(darma_runtime::resource_count(per=Process, depth=2), 5);
  //EXPECT_EQ(darma_runtime::resource_count(depth=2, per=Process), 5);
  EXPECT_EQ(darma_runtime::resource_count(Execution, depth=2, per=Process), 5);


}
