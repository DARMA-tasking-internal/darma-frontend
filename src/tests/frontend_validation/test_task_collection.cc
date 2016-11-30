/*
//@HEADER
// ************************************************************************
//
//                      test_task_collection.cc
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
#include <darma/impl/task_collection/task_collection.h>
#include <darma/impl/task_collection/handle_collection.h>
#include <darma/impl/index_range/mapping.h>
#include <darma/impl/array/index_range.h>

////////////////////////////////////////////////////////////////////////////////

class TestCreateConcurrentWork
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


TEST_F(TestCreateConcurrentWork, simple) {

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access_collection<int>("hello", index_range=Range1D<int>(0, 4));

  }
  //============================================================================




}
