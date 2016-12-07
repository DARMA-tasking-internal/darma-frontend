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
  use_t* use_coll = nullptr;
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
  ))).WillOnce(Invoke([&](auto* use) { use_coll = use; }));

  EXPECT_FLOW_ALIAS(fout_coll, fnull);

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

    EXPECT_THAT(created_task.get(), UseInGetDependencies(use_idx[i]));

    created_task->run();
    EXPECT_THAT(values[i], Eq(42));

    EXPECT_RELEASE_USE(use_idx[i]);

  }

  EXPECT_RELEASE_USE(use_coll);


  mock_runtime->task_collections.front().reset(nullptr);

}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestCreateConcurrentWork, fetch) {

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow finit("finit"), fnull("fnull"), fout_coll("fout_coll");
  MockFlow f_in_idx[4] = { "f_in_idx[0]", "f_in_idx[1]", "f_in_idx[2]", "f_in_idx[3]"};
  MockFlow f_out_idx[4] = { "f_out_idx[0]", "f_out_idx[1]", "f_out_idx[2]", "f_out_idx[3]"};
  MockFlow f_pub[4] = { "f_pub[0]", "f_pub[1]", "f_pub[2]", "f_pub[3]"};
  MockFlow f_fetch[4] = { "f_fetch[0]", "f_fetch[1]", "f_fetch[2]", "f_fetch[3]"};
  use_t* use_idx[4], *use_pub[4], *use_pub_contin[4], *use_fetch[4];
  use_t* use_coll = nullptr;
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
  ))).WillOnce(Invoke([&](auto* use) { use_coll = use; }));

  EXPECT_FLOW_ALIAS(fout_coll, fnull);

  //============================================================================
  // actual code being tested
  {

    auto tmp_c = initial_access_collection<int>("hello", index_range=Range1D<int>(4));


    struct Foo {
      void operator()(Index1D<int> index,
        AccessHandleCollection<int, Range1D<int>> coll
      ) const {
        ASSERT_THAT(index.value, Lt(4));
        ASSERT_THAT(index.value, Ge(0));
        if(index.value > 0) {
          coll[index].publish(version = "hello_world");
        }
        if(index.value < 3) {
          auto neighbor = coll[index+1].fetch(version = "hello_world");
        }
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

    // Publish...
    if(i > 0) {
      EXPECT_CALL(*mock_runtime, make_forwarding_flow(f_in_idx[i]))
        .WillOnce(Return(f_pub[i]));
      EXPECT_REGISTER_USE(use_pub[i], f_pub[i], f_pub[i], None, Read);
      EXPECT_RELEASE_USE(use_idx[i]);
      EXPECT_CALL(*mock_runtime, publish_use(Eq(ByRef(use_pub[i])), _));
      EXPECT_REGISTER_USE(use_pub_contin[i], f_pub[i], f_out_idx[i], Modify, Read);
      // may be removed...
      EXPECT_RELEASE_USE(use_pub[i]);
      EXPECT_RELEASE_USE(use_pub_contin[i]);
      EXPECT_FLOW_ALIAS(f_pub[i], f_out_idx[i]);
    }
    else {
      EXPECT_RELEASE_USE(use_idx[i]);
    }

    // Fetch...
    if(i < 3) {
      EXPECT_CALL(*mock_runtime, make_indexed_fetching_flow(
        finit, Eq(make_key("hello_world")), i+1)
      ).WillOnce(Return(f_fetch[i+1]));
      EXPECT_REGISTER_USE(use_fetch[i+1], f_fetch[i+1], f_fetch[i+1], Read, None);

      EXPECT_RELEASE_USE(use_fetch[i+1]);
    }

    // TODO assert that the correct uses are in the dependencies of created_task


    auto created_task = mock_runtime->task_collections.front()->create_task_for_index(i);

    EXPECT_THAT(created_task.get(), UseInGetDependencies(use_idx[i]));
    created_task->run();

  }

  EXPECT_RELEASE_USE(use_coll);


  mock_runtime->task_collections.front().reset(nullptr);

}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestCreateConcurrentWork, migrate_simple) {

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow finit, fnull, fout_coll, f_in_idx[4], f_out_idx[4];
  MockFlow finit_unpacked, fout_unpacked;
  use_t* use_idx[4];
  use_t* use_coll = nullptr;
  use_t* use_migrated = nullptr;
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
  ))).WillOnce(Invoke([&](auto* use) { use_coll = use; }));

  EXPECT_FLOW_ALIAS(fout_coll, fnull);

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

  // "migrate" the task collection
  EXPECT_CALL(*mock_runtime, get_packed_flow_size(finit))
    .WillOnce(Return(sizeof(int)));
  EXPECT_CALL(*mock_runtime, get_packed_flow_size(fout_coll))
    .WillOnce(Return(sizeof(int)));

  auto& created_collection = mock_runtime->task_collections.front();
  size_t tcsize = created_collection->get_packed_size();

  EXPECT_CALL(*mock_runtime, pack_flow(finit, _))
    .WillOnce(Invoke([](auto&&, void*& buffer){
      int value = 42;
      ::memcpy(buffer, &value, sizeof(int));
      reinterpret_cast<char*&>(buffer) += sizeof(int);
    }));
  EXPECT_CALL(*mock_runtime, pack_flow(fout_coll, _))
    .WillOnce(Invoke([](auto&&, void*& buffer){
      int value = 73;
      ::memcpy(buffer, &value, sizeof(int));
      reinterpret_cast<char*&>(buffer) += sizeof(int);
    }));

  char buffer[tcsize];
  created_collection->pack(buffer);

  EXPECT_CALL(*mock_runtime, make_unpacked_flow(_))
    .Times(2)
    .WillRepeatedly(Invoke([&](void const*& buffer) -> MockFlow {
      if(*reinterpret_cast<int const*>(buffer) == 42) {
        reinterpret_cast<char const*&>(buffer) += sizeof(int);
        return finit_unpacked;
      }
      else if(*reinterpret_cast<int const*>(buffer) == 73) {
        reinterpret_cast<char const*&>(buffer) += sizeof(int);
        return fout_unpacked;
      }
      // Otherwise, fail
      EXPECT_TRUE(false);
      return finit; // ignored
    }));

  EXPECT_CALL(*mock_runtime, reregister_migrated_use(
    IsUseWithFlows(finit_unpacked, fout_unpacked, use_t::Modify, use_t::Modify)
  )).WillOnce(SaveArg<0>(&use_migrated));


  auto copied_collection = abstract::frontend::PolymorphicSerializableObject<
    abstract::frontend::TaskCollection
  >::unpack(buffer, tcsize);

  //============================================================================


  // Make sure the thing still works...

  for(int i = 0; i < 4; ++i) {
    values[i] = 0;

    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(finit_unpacked, i))
      .WillOnce(Return(f_in_idx[i]));
    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(fout_unpacked, i))
      .WillOnce(Return(f_out_idx[i]));
    EXPECT_CALL(*mock_runtime, register_use(
      IsUseWithFlows(f_in_idx[i], f_out_idx[i], use_t::Modify, use_t::Modify)
    )).WillOnce(Invoke([&](auto* use){
      use_idx[i] = use;
      use->get_data_pointer_reference() = &values[i];
    }));

    auto created_task = copied_collection->create_task_for_index(i);

    EXPECT_THAT(created_task.get(), UseInGetDependencies(use_idx[i]));

    created_task->run();

    EXPECT_THAT(values[i], Eq(42));

    EXPECT_RELEASE_USE(use_idx[i]);

  }

  EXPECT_RELEASE_USE(use_migrated);

  copied_collection.reset(nullptr);

  EXPECT_RELEASE_USE(use_coll);

  mock_runtime->task_collections.front().reset(nullptr);

}
