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

#include <darma/impl/feature_testing_macros.h>

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
#include <darma/impl/task_collection/create_concurrent_work.h>

// TODO test conversion/assignment to default constructed AccessHandleCollection

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


TEST_F_WITH_PARAMS(TestCreateConcurrentWork, simple, ::testing::Bool(), bool) {

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow finit, fnull, fout_coll, f_in_idx[4], f_out_idx[4];
  use_t* use_idx[4];
  use_t* use_init = nullptr;
  use_t* use_coll = nullptr, *use_coll_cont = nullptr;
  int values[4];

  bool with_helper = GetParam();

  EXPECT_INITIAL_ACCESS_COLLECTION(finit, fnull, use_init, make_key("hello"), 4);


  EXPECT_CALL(*mock_runtime, make_next_flow_collection(finit))
    .WillOnce(Return(fout_coll));

  if(with_helper) {
    EXPECT_REGISTER_USE_COLLECTION(use_coll, finit, fout_coll, Modify, Modify, 4);
    EXPECT_REGISTER_USE_COLLECTION(use_coll_cont, fout_coll, fnull, Modify, None, 4);
  }
  else {
    EXPECT_CALL(*mock_runtime, legacy_register_use(::testing::AllOf(
      IsUseWithFlows(finit, fout_coll, use_t::Modify, use_t::Modify),
      Truly([](auto* use){
        return (
          use->manages_collection()
          and ::darma_runtime::abstract::frontend::use_cast<
              ::darma_runtime::abstract::frontend::CollectionManagingUse*
            >(use)->get_managed_collection()->size() == 4
        );
      })
    ))).WillOnce(Invoke([&](auto* use) { use_coll = use; }));

    EXPECT_CALL(*mock_runtime, legacy_register_use(AllOf(
      IsUseWithFlows(fout_coll, fnull, use_t::Modify, use_t::None),
      Truly([](auto* use){
        return (
          use->manages_collection()
            and ::darma_runtime::abstract::frontend::use_cast<
                ::darma_runtime::abstract::frontend::CollectionManagingUse*
              >(use)->get_managed_collection()->size() == 4
        );
      })
    ))).WillOnce(Invoke([&](auto* use) { use_coll_cont = use; }));
  }



  EXPECT_RELEASE_USE(use_init);
  EXPECT_RELEASE_USE(use_coll_cont);
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
        coll[index].local_access().set_value(42);
        sequence_marker->mark_sequence("inside task " + std::to_string(index.value));
      }
    };

    create_concurrent_work<Foo>(tmp_c,
      index_range=Range1D<int>(4)
    );

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  for(int i = 0; i < 4; ++i) {
    values[i] = 0;

    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(finit, i))
      .WillOnce(Return(f_in_idx[i]));
    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(fout_coll, i))
      .WillOnce(Return(f_out_idx[i]));
    //EXPECT_REGISTER_USE_AND_SET_BUFFER(use_idx[i], f_in_idx[i], f_out_idx[i], Modify, Modify, (values[i]));
    EXPECT_CALL(*mock_runtime, legacy_register_use(
      IsUseWithFlows(f_in_idx[i], f_out_idx[i], use_t::Modify, use_t::Modify)
    )).WillOnce(Invoke([&](auto* use){
      use_idx[i] = use;
      use->get_data_pointer_reference() = &values[i];
    }));

    auto created_task = mock_runtime->task_collections.front()->create_task_for_index(i);

    EXPECT_THAT(created_task.get(), UseInGetDependencies(use_idx[i]));

    {
      InSequence use_before_release;

      EXPECT_CALL(*sequence_marker, mark_sequence("inside task " + std::to_string(i)));
      // Moved below for now
      //EXPECT_RELEASE_USE(use_idx[i]);
    }

    created_task->run();

    Mock::VerifyAndClearExpectations(mock_runtime.get());

    // TODO we should be able to expect this to be released inside the task itself
    EXPECT_RELEASE_USE(use_idx[i]);

    created_task = nullptr;


    Mock::VerifyAndClearExpectations(mock_runtime.get());

    EXPECT_THAT(values[i], Eq(42));

  }

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_RELEASE_USE(use_coll);

  mock_runtime->task_collections.front().reset(nullptr);

}

////////////////////////////////////////////////////////////////////////////////

#if 0
TEST_F(TestCreateConcurrentWork, simple_all_reduce) {

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow finit, fnull, fout_coll, f_in_idx[4], f_out_idx[4];
  MockFlow f_fwd_allred[4] = { "f_fwd_allred[0]", "f_fwd_allred[1]", "f_fwd_allred[2]", "f_fwd_allred[3]"};
  MockFlow f_allred_out[4] = { "f_allred_out[0]", "f_allred_out[1]", "f_allred_out[2]", "f_allred_out[3]"};
  use_t* use_idx[4] = {nullptr, nullptr, nullptr, nullptr};
  use_t* use_allred[4] = {nullptr, nullptr, nullptr, nullptr};
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
      void operator()(
        ConcurrentContext<Index1D<int>> context,
        AccessHandleCollection<int, Range1D<int>> coll
      ) const {
        ASSERT_THAT(context.index().value, Lt(4));
        ASSERT_THAT(context.index().value, Ge(0));
        auto mine = coll[context.index()].local_access();
        mine.set_value(42);
        context.allreduce(mine);
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

    EXPECT_CALL(*mock_runtime, make_forwarding_flow(f_in_idx[i]))
      .WillOnce(Return(f_fwd_allred[i]));
    EXPECT_CALL(*mock_runtime, make_next_flow(f_fwd_allred[i]))
      .WillOnce(Return(f_allred_out[i]));
    EXPECT_REGISTER_USE(use_allred[i], f_fwd_allred[i], f_allred_out[i], None, Modify);

    EXPECT_RELEASE_USE(use_idx[i]);

    EXPECT_CALL(*mock_runtime, allreduce_use_gmock_proxy(
      Eq(ByRef(use_allred[i])), _, _)
    );

    EXPECT_FLOW_ALIAS(f_allred_out[i], f_out_idx[i]);

    created_task->run();

    EXPECT_THAT(values[i], Eq(42));

  }

  EXPECT_RELEASE_USE(use_coll);


  mock_runtime->task_collections.front().reset(nullptr);
  mock_runtime->backend_owned_uses.clear();

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
  MockFlow f_fetch_null[4] = { "f_fetch_null[0]", "f_fetch_null[1]", "f_fetch_null[2]", "f_fetch_null[3]"};
  use_t* use_idx[4] = { nullptr, nullptr, nullptr, nullptr };
  use_t* use_pub[4] = { nullptr, nullptr, nullptr, nullptr };
  use_t* use_pub_contin[4] = { nullptr, nullptr, nullptr, nullptr };
  use_t* use_fetch[4] = { nullptr, nullptr, nullptr, nullptr };
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
          coll[index].local_access().set_value(42);
          coll[index].local_access().publish(version = "hello_world");
        }
        if(index.value < 3) {
          auto neighbor = coll[index+1].read_access(version = "hello_world");
          create_work([=]{
            EXPECT_THAT(neighbor.get_value(), Eq(42));
          });
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
    int fetched_value = 42;

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

      EXPECT_CALL(*mock_runtime, make_null_flow(
        is_handle_with_key(make_key("hello"))
      )).WillOnce(Return(f_fetch_null[i+1]));

      {
        InSequence s;

        EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR_AND_SET_BUFFER(f_fetch[i+1], use_fetch[i+1], fetched_value);

        EXPECT_REGISTER_TASK(use_fetch[i+1]);

        EXPECT_FLOW_ALIAS(f_fetch[i+1], f_fetch_null[i+1]);

        EXPECT_RELEASE_USE(use_fetch[i+1]);

      }

    }

    // TODO assert that the correct uses are in the dependencies of created_task


    auto created_task = mock_runtime->task_collections.front()->create_task_for_index(i);

    EXPECT_THAT(created_task.get(), UseInGetDependencies(use_idx[i]));
    created_task->run();

    // Run the task created inside that does the fetching, if any
    run_all_tasks();

  }

  EXPECT_RELEASE_USE(use_coll);


  mock_runtime->task_collections.front().reset(nullptr);

}

////////////////////////////////////////////////////////////////////////////////

#endif


TEST_F(TestCreateConcurrentWork, migrate_simple) {

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    finit, fnull, fout_coll, finit_unpacked, fout_unpacked
  );
  MockFlow f_in_idx[4], f_out_idx[4];
  use_t* use_idx[4];
  use_t* use_init = nullptr;
  use_t* use_coll_cont = nullptr;
  use_t* use_coll = nullptr;
  use_t* use_migrated = nullptr;
  int values[4];

  EXPECT_INITIAL_ACCESS_COLLECTION(finit, fnull, use_init, make_key("hello"), 4);

  EXPECT_CALL(*mock_runtime, make_next_flow_collection(finit))
    .WillOnce(Return(fout_coll));

  EXPECT_REGISTER_USE_COLLECTION(use_coll, finit, fout_coll, Modify, Modify, 4);
  EXPECT_REGISTER_USE_COLLECTION(use_coll_cont, fout_coll, fnull, Modify, None, 4);
  EXPECT_RELEASE_USE(use_init);

  // TODO properly constrain this
  EXPECT_CALL(*mock_runtime, register_task_collection_gmock_proxy(_));

  EXPECT_RELEASE_USE(use_coll_cont);
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
        coll[index].local_access().set_value(42);
      }
    };

    create_concurrent_work<Foo>(tmp_c,
      index_range=Range1D<int>(4)
    );

  }
  //
  //============================================================================

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

  EXPECT_THAT(copied_collection->get_dependencies().size(), Eq(1));



  // Make sure the thing still works...

  for(int i = 0; i < 4; ++i) {
    values[i] = 0;

    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(finit_unpacked, i))
      .WillOnce(Return(f_in_idx[i]));
    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(fout_unpacked, i))
      .WillOnce(Return(f_out_idx[i]));
    //EXPECT_REGISTER_USE_AND_SET_BUFFER(use_idx[i], f_in_idx[i], f_out_idx[i], Modify, Modify, (values[i]));
    EXPECT_CALL(*mock_runtime, legacy_register_use(
      IsUseWithFlows(f_in_idx[i], f_out_idx[i], use_t::Modify, use_t::Modify)
    )).WillOnce(Invoke([&](auto* use){
      use_idx[i] = use;
      use->get_data_pointer_reference() = &values[i];
    }));

    auto created_task = copied_collection->create_task_for_index(i);

    EXPECT_THAT(created_task.get(), UseInGetDependencies(use_idx[i]));

    EXPECT_RELEASE_USE(use_idx[i]);

    created_task->run();

    EXPECT_THAT(values[i], Eq(42));

  }

  EXPECT_RELEASE_USE(use_migrated);

  copied_collection.reset(nullptr);

  EXPECT_RELEASE_USE(use_coll);

  mock_runtime->task_collections.front().reset(nullptr);

}

////////////////////////////////////////////////////////////////////////////////

#if 0

TEST_F(TestCreateConcurrentWork, many_to_one) {

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
  use_t* use_idx[4] = { nullptr, nullptr, nullptr, nullptr };
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
        coll[index].local_access().set_value(42);
        coll[index+2].local_access().set_value(73);
      }
    };

    using darma_runtime::Index1D;

    struct ModMapping {
      using from_index_type = Index1D<int>;
      using from_multi_index_type = std::vector<Index1D<int>>;
      using to_index_type = Index1D<int>;
      using is_index_mapping = std::true_type;

      to_index_type map_forward(from_index_type const& from) const {
        return Index1D<int>{from.value % 2, 0, 1};
      }
      from_multi_index_type map_backward(to_index_type const& to) const {
        return std::vector<Index1D<int>>{
          Index1D<int>{0+to.value, 0, 3},
          Index1D<int>{2+to.value, 0, 3}
        };
      }

    };

    create_concurrent_work<Foo>(
      tmp_c.mapped_with( ModMapping() ),
      index_range=Range1D<int>(2)
    );

  }
  //============================================================================

  EXPECT_THAT((*mock_runtime->task_collections.front()->get_dependencies().begin()
    )->get_managed_collection()->task_index_for(0), Eq(0));
  EXPECT_THAT((*mock_runtime->task_collections.front()->get_dependencies().begin()
    )->get_managed_collection()->task_index_for(1), Eq(1));
  EXPECT_THAT((*mock_runtime->task_collections.front()->get_dependencies().begin()
    )->get_managed_collection()->task_index_for(2), Eq(0));
  EXPECT_THAT((*mock_runtime->task_collections.front()->get_dependencies().begin()
    )->get_managed_collection()->task_index_for(3), Eq(1));

  for(int i = 0; i < 2; ++i) {
    values[i] = 0;
    values[i+2] = 0;

    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(finit, i))
      .WillOnce(Return(f_in_idx[i]));
    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(finit, i+2))
      .WillOnce(Return(f_in_idx[i+2]));
    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(fout_coll, i))
      .WillOnce(Return(f_out_idx[i]));
    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(fout_coll, i+2))
      .WillOnce(Return(f_out_idx[i+2]));
    EXPECT_CALL(*mock_runtime, register_use(
      IsUseWithFlows(f_in_idx[i], f_out_idx[i], use_t::Modify, use_t::Modify)
    )).WillOnce(Invoke([&](auto* use){
      use_idx[i] = use;
      use->get_data_pointer_reference() = &values[i];
    }));
    EXPECT_CALL(*mock_runtime, register_use(
      IsUseWithFlows(f_in_idx[i+2], f_out_idx[i+2], use_t::Modify, use_t::Modify)
    )).WillOnce(Invoke([&](auto* use){
      use_idx[i+2] = use;
      use->get_data_pointer_reference() = &values[i+2];
    }));

    auto created_task = mock_runtime->task_collections.front()->create_task_for_index(i);

    EXPECT_THAT(created_task.get(), UseInGetDependencies(use_idx[i]));
    EXPECT_THAT(created_task.get(), UseInGetDependencies(use_idx[i+2]));

    EXPECT_RELEASE_USE(use_idx[i]);
    EXPECT_RELEASE_USE(use_idx[i+2]);

    created_task->run();

    EXPECT_THAT(values[i], Eq(42));
    EXPECT_THAT(values[i+2], Eq(73));

  }

  EXPECT_RELEASE_USE(use_coll);


  mock_runtime->task_collections.front().reset(nullptr);

}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestCreateConcurrentWork, simple_sq_brkt_same) {

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow finit("finit"), fnull("fnull"), fout_coll("fout_coll");
  MockFlow f_in_idx = "f_in_idx";
  MockFlow f_out_idx = "f_out_idx";
  MockFlow f_pub("f_pub"), f_inner_mod("f_inner_mod");
  use_t* use_idx = nullptr;
  use_t* use_coll = nullptr;
  use_t* use_pub = nullptr;
  use_t* use_pub_cont = nullptr;
  use_t* use_inner_mod = nullptr;
  int value = 0;

  EXPECT_INITIAL_ACCESS_COLLECTION(finit, fnull, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow_collection(finit))
    .WillOnce(Return(fout_coll));

  EXPECT_CALL(*mock_runtime, register_use(AllOf(
    IsUseWithFlows(finit, fout_coll, use_t::Modify, use_t::Modify),
    Truly([](auto* use){
      return (
        use->manages_collection()
          and use->get_managed_collection()->size() == 1
      );
    })
  ))).WillOnce(Invoke([&](auto* use) { use_coll = use; }));

  EXPECT_FLOW_ALIAS(fout_coll, fnull);

  //============================================================================
  // actual code being tested
  {

    //auto tmp = initial_access_collection<int>("hello", index_range=Range1D<int>(0, 4));
    //auto tmp = initial_access<int>("hello");
    auto tmp_c = initial_access_collection<int>("hello", index_range=Range1D<int>(1));


    struct Foo {
      void operator()(Index1D<int> index,
        AccessHandleCollection<int, Range1D<int>> coll
      ) const {

        auto ah = coll[index].local_access();

        ah.set_value(42);

        coll[index].local_access().publish();

        create_work([=]{
          ah.set_value(73);
        });

      }
    };

    create_concurrent_work<Foo>(tmp_c,
      index_range=Range1D<int>(1)
    );

  }
  //============================================================================


  EXPECT_CALL(*mock_runtime, make_indexed_local_flow(finit, 0))
    .WillOnce(Return(f_in_idx));
  EXPECT_CALL(*mock_runtime, make_indexed_local_flow(fout_coll, 0))
    .WillOnce(Return(f_out_idx));

  EXPECT_CALL(*mock_runtime, register_use(
    IsUseWithFlows(f_in_idx, f_out_idx, use_t::Modify, use_t::Modify)
  )).WillOnce(Invoke([&](auto* use){
    use_idx = use;
    use->get_data_pointer_reference() = &value;
  }));

  auto created_task = mock_runtime->task_collections.front()->create_task_for_index(0);

  EXPECT_THAT(created_task.get(), UseInGetDependencies(use_idx));

  EXPECT_CALL(*mock_runtime, make_forwarding_flow(f_in_idx))
    .WillOnce(Return(f_pub));

  EXPECT_REGISTER_USE(use_pub, f_pub, f_pub, None, Read);

  EXPECT_REGISTER_USE(use_pub_cont, f_pub, f_out_idx, Modify, Read);

  EXPECT_RELEASE_USE(use_idx);

  EXPECT_CALL(*mock_runtime, publish_use(Eq(ByRef(use_pub)), _));

  EXPECT_RELEASE_USE(use_pub);

  EXPECT_RELEASE_USE(use_pub_cont);

  EXPECT_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(f_pub, f_inner_mod, use_inner_mod, value);

  EXPECT_REGISTER_TASK(use_inner_mod);

  EXPECT_FLOW_ALIAS(f_inner_mod, f_out_idx);

  created_task->run();

  EXPECT_THAT(value, Eq(42));

  EXPECT_RELEASE_USE(use_inner_mod);

  run_all_tasks();

  EXPECT_THAT(value, Eq(73));

  EXPECT_RELEASE_USE(use_coll);

  mock_runtime->task_collections.front().reset(nullptr);

}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestCreateConcurrentWork, simple_unique_owner) {

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;


  MockFlow finit("finit"), fnull("fnull"), ftask_out("ftask_out");
  use_t* use_task = nullptr;
  int value;

  EXPECT_INITIAL_ACCESS(finit, fnull, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(finit, ftask_out, use_task, value);

  EXPECT_FLOW_ALIAS(ftask_out, fnull);

  //============================================================================
  // actual code being tested
  {

    auto tmp_c = initial_access<int>("hello");


    struct Foo {
      void operator()(Index1D<int> index,
        // TODO: this instead: UniquelyOwned<AccessHandle<int>, Index1D<int>> val
        AccessHandle<int> val
      ) const {
        if(index.value == 0) { val.set_value(42); }
      }
    };

    create_concurrent_work<Foo>(
      tmp_c.owned_by(Index1D<int>{0, 0, 3}),
      index_range=Range1D<int>(4)
    );

  }
  //============================================================================

  for(int i = 0; i < 4; ++i) {

    if(i == 0) {
      EXPECT_RELEASE_USE(use_task);
    }
    else {

    }

    auto created_task = mock_runtime->task_collections.front()->create_task_for_index(i);

    if(i == 0) {
      EXPECT_THAT(created_task.get(), UseInGetDependencies(use_task));
    }
    else {
      EXPECT_EQ(created_task->get_dependencies().size(), 0);
    }

    created_task->run();

    if(i == 0) {
      EXPECT_THAT(value, Eq(42));
    }

  }


  mock_runtime->task_collections.front().reset(nullptr);

}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestCreateConcurrentWork, fetch_unique_owner) {

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;


  MockFlow finit("finit"), fnull("fnull"), ftask_out("ftask_out");
  MockFlow fpub("fpub");
  MockFlow f_fetch[4] = { "f_fetch[0]", "f_fetch[1]", "f_fetch[2]", "f_fetch[3]"};
  MockFlow f_fetch_null[4] = { "f_fetch_null[0]", "f_fetch_null[1]", "f_fetch_null[2]", "f_fetch_null[3]"};
  use_t* use_task = nullptr, *use_pub = nullptr, *use_pub_cont = nullptr;
  use_t* use_fetch[4] = {nullptr, nullptr, nullptr, nullptr};
  int value;

  EXPECT_INITIAL_ACCESS(finit, fnull, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(finit, ftask_out, use_task, value);

  EXPECT_FLOW_ALIAS(ftask_out, fnull);

  //============================================================================
  // actual code being tested
  {

    auto tmp_c = initial_access<int>("hello");


    struct Foo {
      void operator()(Index1D<int> index,
        // TODO: this instead: UniquelyOwned<AccessHandle<int>, Index1D<int>> val
        AccessHandle<int> val
      ) const {
        if(index.value == 0) {
          val.set_value(42);
          val.publish(n_readers=3);
        }
        else {
          val.read_access();
          create_work([=]{
            EXPECT_EQ(val.get_value(), 42);
          });

        }
      }
    };

    create_concurrent_work<Foo>(
      tmp_c.owned_by(Index1D<int>{0, 0, 3}),
      index_range=Range1D<int>(4)
    );

  }
  //============================================================================

  for(int i = 0; i < 4; ++i) {

    if(i == 0) {

      {
        InSequence s;

        EXPECT_CALL(*mock_runtime, make_forwarding_flow(finit))
          .WillOnce(Return(fpub));

        EXPECT_REGISTER_USE(use_pub, fpub, fpub, None, Read);

        EXPECT_REGISTER_USE(use_pub_cont, fpub, ftask_out, Modify, Read);

        EXPECT_RELEASE_USE(use_task);

        EXPECT_CALL(*mock_runtime, publish_use(Eq(ByRef(use_pub)), _));

        EXPECT_RELEASE_USE(use_pub);

      }

      EXPECT_RELEASE_USE(use_pub_cont);

      EXPECT_FLOW_ALIAS(fpub, ftask_out);

    }
    else {

      EXPECT_CALL(*mock_runtime, make_fetching_flow(
        is_handle_with_key(make_key("hello")), make_key(), false
      )).WillOnce(Return(f_fetch[i]));

      EXPECT_CALL(*mock_runtime, make_null_flow(
        is_handle_with_key(make_key("hello"))
      )).WillOnce(Return(f_fetch_null[i]));

      EXPECT_REGISTER_USE_AND_SET_BUFFER(
        use_fetch[i], f_fetch[i], f_fetch[i], Read, Read, value
      );

      EXPECT_REGISTER_TASK(use_fetch[i]);

      EXPECT_FLOW_ALIAS(f_fetch[i], f_fetch_null[i]);

    }

    auto created_task = mock_runtime->task_collections.front()->create_task_for_index(i);

    if(i == 0) {
      EXPECT_THAT(created_task.get(), UseInGetDependencies(use_task));
    }
    else {
      EXPECT_EQ(created_task->get_dependencies().size(), 0);
    }

    created_task->run();

    // Run any of the fetching tasks created...
    if(i != 0) {
      EXPECT_RELEASE_USE(use_fetch[i]);

      run_all_tasks();
    }

    if(i == 0) {
      EXPECT_THAT(value, Eq(42));
    }

  }


  mock_runtime->task_collections.front().reset(nullptr);

}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestCreateConcurrentWork, simple_collection_read) {

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  MockFlow finit("finit"), fnull("fnull"), fout_task("fout_task");
  use_t* use_all = nullptr;
  use_t* use_coll[4] = { nullptr, nullptr, nullptr, nullptr };
  use_t* use_task = nullptr;
  int value = 0;

  EXPECT_INITIAL_ACCESS(finit, fnull, make_key("hello"));

  EXPECT_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(finit, fout_task, use_task, value);

  EXPECT_REGISTER_TASK(use_task);

  EXPECT_RELEASE_USE(use_task);

  EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR(fout_task, use_all);

  EXPECT_CALL(*mock_runtime, register_task_collection_gmock_proxy(
    UseInGetDependencies(ByRef(use_all))
  ));

  EXPECT_FLOW_ALIAS(fout_task, fnull);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");


    struct Foo {
      void operator()(Index1D<int> index,
        AccessHandle<int> h
      ) const {
        EXPECT_THAT(h.get_value(), Eq(42));
      }
    };

    create_work([=]{ tmp.set_value(42); });

    create_concurrent_work<Foo>(tmp.shared_read(),
      index_range=Range1D<int>(4)
    );

  }
  //============================================================================

  run_all_tasks();

  for(int i = 0; i < 4; ++i) {
    EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR_AND_SET_BUFFER(fout_task, use_coll[i], value);

    auto created_task = mock_runtime->task_collections.front()->create_task_for_index(i);

    EXPECT_THAT(created_task.get(), UseInGetDependencies(use_coll[i]));

    EXPECT_RELEASE_USE(use_coll[i]);

    created_task->run();

  }

  EXPECT_RELEASE_USE(use_all);

  mock_runtime->task_collections.front().reset(nullptr);

}

////////////////////////////////////////////////////////////////////////////////

// Actually a compile-time error...
//TEST_F(TestCreateConcurrentWork, collection_capture_death) {
//  using namespace ::testing;
//  using namespace darma_runtime;
//  using namespace darma_runtime::keyword_arguments_for_publication;
//  using namespace darma_runtime::keyword_arguments_for_task_creation;
//  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
//  using namespace mock_backend;
//
//  //============================================================================
//  // actual code being tested
//  struct FooTask {
//    void operator()(AccessHandleCollection<int, Range1D<int>>) { }
//  };
//
//  auto tmp = initial_access_collection<int>(index_range=Range1D<int>(42));
//
//  EXPECT_DEATH({
//    create_work<FooTask>(tmp);
//  },
//    "Capturing AccessHandleCollection objects in regular tasks is not yet supported"
//  );
//
//  //============================================================================
//
//}

////////////////////////////////////////////////////////////////////////////////

//#if 0 // shouldn't compile any more
//TEST_F(TestCreateConcurrentWork, collection_capture_death) {
//  using namespace ::testing;
//  using namespace darma_runtime;
//  using namespace darma_runtime::keyword_arguments_for_publication;
//  using namespace darma_runtime::keyword_arguments_for_task_creation;
//  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
//  using namespace mock_backend;
//
//  //============================================================================
//  // actual code being tested
//  auto tmp = initial_access_collection<int>(index_range=Range1D<int>(42));
//
//  EXPECT_DEATH({
//    create_work([=]{ tmp[5].local_access(); });
//  },
//    "Capturing AccessHandleCollection objects in regular tasks is not yet supported"
//  );
//
//  //============================================================================
//
//}
//#endif


TEST_F(TestCreateConcurrentWork, nested_in_create_work) {

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(finit, fouter_out, fnull, fout_coll);
  MockFlow f_in_idx[4], f_out_idx[4];
  use_t* use_idx[4];
  use_t* use_coll = nullptr;
  use_t* use_task_cont = nullptr;
  use_t* use_outer_capture = nullptr;
  int values[4];

  EXPECT_INITIAL_ACCESS_COLLECTION(finit, fnull, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow_collection(
    finit
  )).WillOnce(Return(fouter_out));

  EXPECT_REGISTER_USE(use_outer_capture, finit, fouter_out, Modify, None);

  EXPECT_REGISTER_TASK(use_outer_capture);

  EXPECT_FLOW_ALIAS(fouter_out, fnull);

  //============================================================================
  // actual code being tested
  {

    auto tmp_c = initial_access_collection<int>("hello", index_range=Range1D<int>(4));


    struct Foo {
      void operator()(Index1D<int> index,
        AccessHandleCollection<int, Range1D<int>> coll
      ) const {
        coll[index].local_access().set_value(42);
      }
    };

    create_work([=]{
      create_concurrent_work<Foo>(tmp_c,
        index_range=Range1D<int>(4)
      );
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_next_flow_collection(
    finit
  )).WillOnce(Return(fout_coll));

  EXPECT_CALL(*mock_runtime, register_use(AllOf(
    IsUseWithFlows(finit, fout_coll, use_t::Modify, use_t::Modify),
    Truly([](auto* use){
      return (
        use->manages_collection()
          and use->get_managed_collection()->size() == 4
      );
    })
  ))).WillOnce(Invoke([&](auto* use) { use_coll = use; }));

  {
    InSequence reg_before_release;

    EXPECT_REGISTER_USE(use_task_cont, fout_coll, fouter_out, Modify, None);

    EXPECT_RELEASE_USE(use_outer_capture);

    EXPECT_CALL(*mock_runtime, register_task_collection_gmock_proxy(
      UseInGetDependencies(ByRef(use_coll))
    ));

    EXPECT_FLOW_ALIAS(fout_coll, fouter_out);
  }

  EXPECT_RELEASE_USE(use_task_cont);

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

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

    EXPECT_RELEASE_USE(use_idx[i]);

    created_task->run();
    EXPECT_THAT(values[i], Eq(42));

  }

  EXPECT_RELEASE_USE(use_coll);


  mock_runtime->task_collections.front().reset(nullptr);

}
#endif

////////////////////////////////////////////////////////////////////////////////

#if _darma_has_feature(handle_collection_based_collectives)

//==============================================================================
// <editor-fold desc="handle_reduce"> {{{1

TEST_F(TestCreateConcurrentWork, handle_reduce)
{

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_collectives;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(finit, fnull, fout_coll, finit_tmp, fnull_tmp, fout_tmp);
  MockFlow f_in_idx[4], f_out_idx[4];
  MockFlow f_fwd_allred[4] =
    {"f_fwd_allred[0]", "f_fwd_allred[1]", "f_fwd_allred[2]", "f_fwd_allred[3]"
    };
  MockFlow f_allred_out[4] =
    {"f_allred_out[0]", "f_allred_out[1]", "f_allred_out[2]", "f_allred_out[3]"
    };
  use_t* use_idx[4] = {nullptr, nullptr, nullptr, nullptr};
  use_t* use_coll = nullptr;
  use_t* use_coll_collective = nullptr;
  use_t* use_tmp_collective = nullptr;
  use_t* use_initial, * use_initial_coll;
  use_t* use_coll_cont, * use_tmp_cont;

  int values[4];

  EXPECT_INITIAL_ACCESS_COLLECTION(finit,
    fnull,
    use_initial_coll,
    make_key("hello"),
    4);
  EXPECT_INITIAL_ACCESS(finit_tmp, fnull_tmp, use_initial, make_key("world"));

  EXPECT_CALL(*mock_runtime, make_next_flow_collection(finit))
    .WillOnce(Return(fout_coll));
  EXPECT_CALL(*mock_runtime, make_next_flow(finit_tmp))
    .WillOnce(Return(fout_tmp));

  // We need to sequence registers before releases so that gmock doesn't freak out
  // when checking expectations on null pointers
  {
    InSequence s;
    EXPECT_REGISTER_USE(use_coll_cont, fout_coll, fnull, Modify, None);
    EXPECT_FLOW_ALIAS(fout_coll, fnull);
    EXPECT_RELEASE_USE(use_coll_cont);
  }

  {
    InSequence s;
    EXPECT_REGISTER_USE(use_tmp_cont, fout_tmp, fnull_tmp, Modify, None);
    EXPECT_FLOW_ALIAS(fout_tmp, fnull_tmp);
    EXPECT_RELEASE_USE(use_tmp_cont);
  }

  EXPECT_RELEASE_USE(use_initial_coll);
  EXPECT_RELEASE_USE(use_initial);

  {
    InSequence s;
    EXPECT_REGISTER_USE_COLLECTION(use_coll,
      finit,
      fout_coll,
      Modify,
      Modify,
      4);

    EXPECT_CALL(*mock_runtime, register_task_collection_gmock_proxy(
      CollectionUseInGetDependencies(ByRef(use_coll))
    ));
  }

  EXPECT_REGISTER_USE_COLLECTION(use_coll_collective,
    fout_coll,
    fout_coll,
    None,
    Read,
    4);
  EXPECT_REGISTER_USE(use_tmp_collective, finit_tmp, fout_tmp, None, Modify);

  EXPECT_CALL(*mock_runtime, reduce_collection_use_gmock_proxy(
    Eq(ByRef(use_coll_collective)),
    Eq(ByRef(use_tmp_collective)),
    _,
    make_key("myred")
  ));


  //============================================================================
  // actual code being tested
  {

    auto tmp_c =
      initial_access_collection<int>("hello", index_range = Range1D<int>(4));
    auto tmp = initial_access<int>("world");


    struct Foo
    {
      void operator()(
        ConcurrentContext<Index1D<int>> context,
        AccessHandleCollection<int, Range1D<int>> coll
      ) const
      {
        ASSERT_THAT(context.index().value, Lt(4));
        ASSERT_THAT(context.index().value, Ge(0));
        auto mine = coll[context.index()].local_access();
        mine.set_value(42);
      }
    };

    create_concurrent_work<Foo>(tmp_c,
      index_range = Range1D<int>(4)
    );

    tmp_c.reduce(tag = "myred", output = tmp);

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  for (int i = 0; i < 4; ++i) {
    values[i] = 0;

    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(finit, i))
      .WillOnce(Return(f_in_idx[i]));
    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(fout_coll, i))
      .WillOnce(Return(f_out_idx[i]));
    EXPECT_CALL(*mock_runtime, legacy_register_use(
      IsUseWithFlows(f_in_idx[i], f_out_idx[i], use_t::Modify, use_t::Modify)
    )).WillOnce(Invoke([&](auto* use) {
        use_idx[i] = use;
        use->get_data_pointer_reference() = &values[i];
      }
      )
    );

    auto created_task =
      mock_runtime->task_collections.front()->create_task_for_index(i);

    EXPECT_THAT(created_task.get(), UseInGetDependencies(use_idx[i]));

    EXPECT_RELEASE_USE(use_idx[i]);

    created_task->run();

    EXPECT_THAT(values[i], Eq(42));

  }

  EXPECT_RELEASE_USE(use_coll);


  mock_runtime->task_collections.front().reset(nullptr);

  // Leak the memory for now, just to get things working
  //new (&mock_runtime->backend_owned_uses) std::deque<std::unique_ptr<use_t>>();
  mock_runtime->backend_owned_uses.clear();

}

// </editor-fold> end handle_reduce }}}1
//==============================================================================

#endif // _darma_has_feature(handle_collection_based_collectives)

////////////////////////////////////////////////////////////////////////////////


//==============================================================================
// <editor-fold desc="simple_read_only"> {{{1

TEST_F(TestCreateConcurrentWork, simple_read_only)
{

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(finit, fnull);
  MockFlow f_in_idx[4], f_out_idx[4];
  use_t* use_idx[4];
  use_t* use_init = nullptr;
  use_t* use_coll = nullptr;
  int values[4];

  EXPECT_INITIAL_ACCESS_COLLECTION(finit,
    fnull,
    use_init,
    make_key("hello"),
    4);

  EXPECT_REGISTER_USE_COLLECTION(use_coll, finit, finit, Read, Read, 4);

  EXPECT_FLOW_ALIAS(finit, fnull);

  EXPECT_RELEASE_USE(use_init);

  //============================================================================
  // actual code being tested
  {

    auto tmp_c =
      initial_access_collection<int>("hello", index_range = Range1D<int>(4));


    struct Foo
    {
      void operator()(
        Index1D<int> index,
        ReadAccessHandleCollection<int, Range1D<int>> coll
      ) const
      {
        sequence_marker->mark_sequence("inside task "
          + std::to_string(coll[index].local_access().get_value())
        );
      }
    };

    create_concurrent_work<Foo>(tmp_c,
      index_range = Range1D<int>(4)
    );

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  for (int i = 0; i < 4; ++i) {
    values[i] = 0;

    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(finit, i))
      .WillOnce(Return(f_in_idx[i]));
    EXPECT_CALL(*mock_runtime, legacy_register_use(
      IsUseWithFlows(f_in_idx[i], f_in_idx[i], use_t::Read, use_t::Read)
    )).WillOnce(Invoke([&](auto* use) {
        use_idx[i] = use;
        use->get_data_pointer_reference() = &values[i];
      }
      )
    );

    auto created_task =
      mock_runtime->task_collections.front()->create_task_for_index(i);

    EXPECT_THAT(created_task.get(), UseInGetDependencies(use_idx[i]));

    values[i] = 42 + i;

    {
      InSequence use_before_release;

      EXPECT_CALL(*sequence_marker,
        mark_sequence("inside task " + std::to_string(42 + i)));
      // Moved below for now
      //EXPECT_RELEASE_USE(use_idx[i]);
    }

    created_task->run();

    Mock::VerifyAndClearExpectations(mock_runtime.get());

    // TODO we should be able to expect this to be released inside the task itself
    EXPECT_RELEASE_USE(use_idx[i]);

    created_task = nullptr;


    Mock::VerifyAndClearExpectations(mock_runtime.get());

  }

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_RELEASE_USE(use_coll);

  mock_runtime->task_collections.front().reset(nullptr);

}

// </editor-fold> end simple_read_only }}}1
//==============================================================================

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestCreateConcurrentWork, simple_commutative) {

  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  using namespace darma_runtime::keyword_arguments_for_commutative_access;
  using namespace darma_runtime::keyword_arguments_for_task_creation;
  using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(finit, f_out_coll, fnull);
  MockFlow f_comm_in_idx[2] = {"f_comm_in_idx[0]", "f_comm_in_idx[1]"};
  MockFlow f_comm_out_idx[2] = {"f_comm_out_idx[0]", "f_comm_out_idx[1]"};
  use_t* use_idx_0[4];
  use_t* use_subtask_idx_0[4];
  use_t* use_idx_1[4];
  use_t* use_subtask_idx_1[4];
  use_t* use_init = nullptr;
  use_t* use_coll = nullptr;
  use_t* use_comm_outer = nullptr;
  use_t* use_last_outer = nullptr;
  int values[4];

  {
    InSequence s1;

    EXPECT_NEW_INITIAL_ACCESS_COLLECTION(finit, fnull, use_init, make_key("hello"), 2);

    EXPECT_NEW_REGISTER_USE_COLLECTION(use_comm_outer,
      finit, SameCollection, &finit,
      f_out_coll, NextCollection, nullptr, true,
      Commutative, None, false, 2
    );

    EXPECT_NEW_RELEASE_USE(use_init, false);

    EXPECT_NEW_REGISTER_USE_COLLECTION(use_coll,
      finit, SameCollection, &finit,
      f_out_coll, SameCollection, &f_out_coll, false,
      Commutative, None, true, 2
    );

    //EXPECT_REGISTER_TASK_COLLECTION(use_coll);

    EXPECT_NEW_REGISTER_USE_COLLECTION(use_last_outer,
      f_out_coll, SameCollection, &f_out_coll,
      fnull, SameCollection, &fnull, false,
      Commutative, None, false, 2
    );

    EXPECT_NEW_RELEASE_USE(use_comm_outer, false);

    EXPECT_NEW_RELEASE_USE(use_last_outer, true);

  }

  //============================================================================
  // actual code being tested
  {

    struct Foo {
      void operator()(Index1D<int> index,
        CommutativeAccessHandleCollection<int, Range1D<int>> coll
      ) const {
        for(int i = 0; i < coll.get_index_range().size(); ++i) {
          auto comm_i = coll[i].commutative_access();
          create_work([=]{
            comm_i.set_value(comm_i.get_value() + i + index.value);
          });
        }
      }
    };

    auto tmp_c = initial_access_collection<int>("hello", index_range=Range1D<int>(2));
    auto tmp_comm = commutative_access(to_collection=std::move(tmp_c));

    create_concurrent_work<Foo>(tmp_comm,
      index_range=Range1D<int>(4)
    );

    tmp_c = noncommutative_access_to_collection(std::move(tmp_comm));

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  for(int i = 0; i < 4; ++i) {
    // Run 4 tasks

    // No expectations in the create task phase

    auto created_task = mock_runtime->task_collections.front()->create_task_for_index(i);

    Mock::VerifyAndClearExpectations(mock_runtime.get());

    // Collection is size 2, so we expect two uses to be created per indexed
    // task (plus two uses for the subtasks)

    {
      InSequence s1;

      EXPECT_NEW_REGISTER_USE_WITH_EXTRA_CONSTRAINTS(use_idx_0[i],
        f_comm_in_idx[0], Indexed, &finit,
        f_comm_out_idx[0], Indexed, &f_out_coll, false,
        Commutative, None, false,
        /* Extra constraints */
        InFlowRelationshipIndex(0),
        OutFlowRelationshipIndex(0)
      );

      EXPECT_NEW_REGISTER_USE_AND_SET_BUFFER(use_subtask_idx_0[i],
        f_comm_in_idx[0], Same, &(f_comm_in_idx[0]),
        f_comm_out_idx[0], Same, &(f_comm_out_idx[0]), false,
        Commutative, Commutative, true, values[0]
      );

      EXPECT_NEW_RELEASE_USE(use_idx_0[i], false);
    }

    {
      InSequence s2;

      EXPECT_NEW_REGISTER_USE_WITH_EXTRA_CONSTRAINTS(use_idx_1[i],
        f_comm_in_idx[1], Indexed, &finit,
        f_comm_out_idx[1], Indexed, &f_out_coll, false,
        Commutative, None, false,
        /* Extra constraints */
        InFlowRelationshipIndex(1),
        OutFlowRelationshipIndex(1)
      );

      EXPECT_NEW_REGISTER_USE_AND_SET_BUFFER(use_subtask_idx_1[i],
        f_comm_in_idx[1], Same, &(f_comm_in_idx[1]),
        f_comm_out_idx[1], Same, &(f_comm_out_idx[1]), false,
        Commutative, Commutative, true, values[1]
      );

      EXPECT_NEW_RELEASE_USE(use_idx_1[i], false);
    }

    created_task->run();

    Mock::VerifyAndClearExpectations(mock_runtime.get());

    ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(2));

    EXPECT_NEW_RELEASE_USE(use_subtask_idx_0[i], false);

    auto old_value = values[0];

    run_one_task();

    ASSERT_THAT(values[0], Eq(old_value + i));

    Mock::VerifyAndClearExpectations(mock_runtime.get());

    ASSERT_THAT(mock_runtime->registered_tasks.size(), Eq(1));

    EXPECT_NEW_RELEASE_USE(use_subtask_idx_1[i], false);

    old_value = values[1];

    run_one_task();

    ASSERT_THAT(values[1], Eq(old_value + i + 1));
  }

  EXPECT_NEW_RELEASE_USE(use_coll, false);

  mock_runtime->task_collections.front().reset(nullptr);

}


// TODO test commutative migration
