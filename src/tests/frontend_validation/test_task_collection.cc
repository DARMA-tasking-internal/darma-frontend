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

  MockFlow finit, fnull, fout_coll, f_in_idx[4], f_out_idx[4];
  use_t* use_idx[4];
  int values[4];

  EXPECT_INITIAL_ACCESS_COLLECTION(finit, fnull, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow_collection(finit))
    .WillOnce(Return(fout_coll));

  EXPECT_CALL(*mock_runtime, register_use(AllOf(
    IsUseWithFlows(finit, fout_coll, use_t::Modify, use_t::Modify),
    Truly([](auto* use){
      return (
        use->manages_collection()
          and use->get_managed_collection()->size() == 4
      );
    })
  )));

  //============================================================================
  // actual code being tested
  {

    //auto tmp = initial_access_collection<int>("hello", index_range=Range1D<int>(0, 4));
    //auto tmp = initial_access<int>("hello");
    auto tmp_c = initial_access_collection<int>("hello", index_range=Range1D<int>(4));


    struct Foo {
      void operator()(Index1D<int> index,
        AccessHandleCollection<int, Range1D<int>> coll
      ) const {
        ASSERT_THAT(index.value, Lt(4));
        ASSERT_THAT(index.value, Ge(0));
        coll[index].set_value(42);
      }
    };

    create_concurrent_work<Foo>(tmp_c,
      index_range=Range1D<int>(4)
    );

  }
  //============================================================================

  for(int i = 0; i < 4; ++i) {
    values[i] = 0;

    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(finit, i))
      .WillOnce(Return(f_in_idx[i]));
    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(fout_coll, i))
      .WillOnce(Return(f_out_idx[i]));
    EXPECT_CALL(*mock_runtime, register_use(
      IsUseWithFlows(f_in_idx[i], f_out_idx[i], use_t::Modify, use_t::Modify)
    )).WillOnce(Invoke([&](auto* use){
      use_idx[i] = use;
      use->get_data_pointer_reference() = &values[i];
    }));

    auto created_task = mock_runtime->task_collections.front()->create_task_for_index(i);
    created_task->run();
    EXPECT_THAT(values[i], Eq(42));
  }

}