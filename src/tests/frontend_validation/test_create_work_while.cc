/*
//@HEADER
// ************************************************************************
//
//                      test_create_work_if.cc
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#include "mock_backend.h"
#include "test_frontend.h"

#include <darma/impl/create_work_while.h>
#include <darma/interface/app/initial_access.h>

////////////////////////////////////////////////////////////////////////////////

class TestCreateWorkWhile
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

using namespace darma_runtime;
using namespace darma_runtime::experimental;

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkWhile, basic_same_always_false) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_while_out
  );
  use_t* while_use = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_while_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, f_while_out, Modify, Read, value);

  EXPECT_REGISTER_TASK(while_use);

  EXPECT_FLOW_ALIAS(f_while_out, f_null);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");

    create_work_while([=]{
      return tmp.get_value() != 0; // should always be False
    }).do_([=]{
      tmp.set_value(73);
      // This doesn't work like you think it does
      //FAIL() << "Ran do clause when while part should have been false";
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_RELEASE_USE(while_use);

  run_all_tasks();


}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkWhile, basic_same_one_iteration) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_while_out, f_do_out,
    f_while_fwd, f_while_out_2
  );
  use_t* while_use = nullptr;
  use_t* do_use = nullptr;
  use_t* while_use_2 = nullptr;
  use_t* do_cont_use = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_while_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, f_while_out, Modify, Read, value);

  EXPECT_REGISTER_TASK(while_use);

  EXPECT_FLOW_ALIAS(f_while_out, f_null);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");

    create_work_while([=]{
      return tmp.get_value() != 73; // should only be false the first time
    }).do_([=]{
      tmp.set_value(73);
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_do_out));

  {
    InSequence reg_before_release;

    EXPECT_REGISTER_USE_AND_SET_BUFFER(do_use, f_init, f_do_out, Modify, Modify, value);

    EXPECT_RELEASE_USE(while_use);

    // TODO get rid of this and the corresponding make_next
    EXPECT_FLOW_ALIAS(f_do_out, f_while_out);

    EXPECT_REGISTER_TASK(do_use);

  }

  run_one_task(); // the first while

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_forwarding_flow(f_init))
    .WillOnce(Return(f_while_fwd));
  EXPECT_CALL(*mock_runtime, make_next_flow(f_while_fwd))
    .WillOnce(Return(f_while_out_2));

  {
    InSequence reg_before_release;

    EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use_2, f_while_fwd, f_while_out_2,
      Modify, Read, value
    );

    EXPECT_RELEASE_USE(do_use);

    // TODO get rid of this and the corresponding make_next
    EXPECT_FLOW_ALIAS(f_while_out_2, f_do_out);

    EXPECT_REGISTER_TASK(while_use_2);
  }

  run_one_task(); // the do part

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_RELEASE_USE(while_use_2);

  run_one_task(); // the second while

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_THAT(value, Eq(73));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkWhile, basic_different_always_false) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null,
    f_init_2, f_null_2, f_while_out_2
  );
  use_t* while_use = nullptr;
  use_t* do_sched_use = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, make_key("hello"));
  EXPECT_INITIAL_ACCESS(f_init_2, f_null_2, make_key("world"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init_2))
    .WillOnce(Return(f_while_out_2));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, f_init, Read, Read, value);
  EXPECT_REGISTER_USE(do_sched_use, f_init_2, f_while_out_2, Modify, None);

  EXPECT_REGISTER_TASK(while_use, do_sched_use);

  EXPECT_FLOW_ALIAS(f_init, f_null);
  EXPECT_FLOW_ALIAS(f_while_out_2, f_null_2);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");
    auto tmp2 = initial_access<int>("world");

    create_work_while([=]{
      return tmp.get_value() != 0; // should always be False
    }).do_([=]{
      tmp2.set_value(73);
      // This doesn't work like you think it does
      //FAIL() << "Ran do clause when while part should have been false";
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_RELEASE_USE(while_use);

  EXPECT_RELEASE_USE(do_sched_use);

  run_all_tasks();


}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkWhile, basic_same_four_iterations) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null
  );
  MockFlow f_do_out[] = { "f_do_out[0]", "f_do_out[1]", "f_do_out[2]", "f_do_out[3]",  "f_do_out[4]"};
  MockFlow f_while_fwd[] = { "f_while_fwd[0]", "f_while_fwd[1]", "f_while_fwd[2]", "f_while_fwd[3]",  "f_while_fwd[4]"};
  MockFlow f_while_out[] = { "f_while_out[0]", "f_while_out[1]", "f_while_out[2]", "f_while_out[3]",  "f_while_out[4]"};
  use_t* do_use[] = { nullptr, nullptr, nullptr, nullptr };
  use_t* while_use[] = { nullptr, nullptr, nullptr, nullptr, nullptr };

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_while_out[0]));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use[0], f_init, f_while_out[0], Modify, Read, value);

  EXPECT_REGISTER_TASK(while_use[0]);

  EXPECT_FLOW_ALIAS(f_while_out[0], f_null);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");

    create_work_while([=]{
      return tmp.get_value() < 4; // should do 4 iterations
    }).do_([=]{
      *tmp += 1;
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_do_out[0]));

  {
    InSequence reg_before_release;

    EXPECT_REGISTER_USE_AND_SET_BUFFER(do_use[0], f_init, f_do_out[0], Modify, Modify, value);

    EXPECT_RELEASE_USE(while_use[0]);

    EXPECT_REGISTER_TASK(do_use[0]);
  }

  EXPECT_FLOW_ALIAS(f_do_out[0], f_while_out[0]);

  run_one_task(); // the first while

  for(int i = 0; i < 4; ++i) {

    Mock::VerifyAndClearExpectations(mock_runtime.get());

    // the next while part is registered in the do part

    EXPECT_CALL(*mock_runtime, make_forwarding_flow(
      i == 0 ? f_init : f_while_fwd[i-1]
    )).WillOnce(Return(f_while_fwd[i]));
    EXPECT_CALL(*mock_runtime, make_next_flow(f_while_fwd[i]))
      .WillOnce(Return(f_while_out[i+1]));

    {
      InSequence reg_before_release;

      EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use[i+1], f_while_fwd[i], f_while_out[i+1],
        Modify, Read, value
      );

      EXPECT_RELEASE_USE(do_use[i]);

      // TODO get rid of this and the corresponding make_next and just use existing out
      EXPECT_FLOW_ALIAS(f_while_out[i+1], f_do_out[i]);

      EXPECT_REGISTER_TASK(while_use[i+1]);
    }


    run_one_task(); // run the do part

    // the next do part is registered as part of the while

    Mock::VerifyAndClearExpectations(mock_runtime.get());

    if(i < 3) {
      EXPECT_CALL(*mock_runtime, make_next_flow(f_while_fwd[i]))
        .WillOnce(Return(f_do_out[i + 1]));

      {

        InSequence reg_before_release;

        EXPECT_REGISTER_USE_AND_SET_BUFFER(do_use[i + 1],
          f_while_fwd[i], f_do_out[i + 1], Modify, Modify, value
        );

        EXPECT_RELEASE_USE(while_use[i + 1]);

        // TODO get rid of this and the corresponding make_next and just use existing out
        EXPECT_FLOW_ALIAS(f_do_out[i+1], f_while_out[i+1]);

        EXPECT_REGISTER_TASK(do_use[i + 1]);

      }


    }
    else {

      EXPECT_RELEASE_USE(while_use[i+1]);

    }


    run_one_task(); // run the next while part

  }

  EXPECT_THAT(value, Eq(4));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkWhile, collection_one_iteration) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_while_out, f_do_out,
    f_while_fwd, f_while_out_2,
    f_init_c, f_null_c, f_while_out_c, f_while_out_c_2,
    f_do_out_c, f_out_coll
  );
  MockFlow f_in_idx[4] = { "f_in_idx[0]", "f_in_idx[1]", "f_in_idx[2]", "f_in_idx[3]"};
  MockFlow f_out_idx[4] = { "f_out_idx[0]", "f_out_idx[1]", "f_out_idx[2]", "f_out_idx[3]"};
  use_t* use_idx[4] = { nullptr, nullptr, nullptr, nullptr };
  use_t* while_use = nullptr;
  use_t* do_use = nullptr;
  use_t* while_use_2 = nullptr;
  use_t* do_cont_use = nullptr;
  use_t* while_use_c = nullptr;
  use_t* do_use_c = nullptr;
  use_t* while_use_2_c = nullptr;
  use_t* do_cont_use_c = nullptr;
  use_t* use_coll = nullptr;
  use_t* use_coll_cont = nullptr;
  use_t* coll_cont_use = nullptr;
  use_t* while_cont_use_2_c = nullptr;

  int value = 0;
  int values[4] = {0, 0, 0, 0};

  EXPECT_INITIAL_ACCESS(f_init, f_null, make_key("hello"));
  EXPECT_INITIAL_ACCESS_COLLECTION(f_init_c, f_null_c, make_key("world"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_while_out));
  EXPECT_CALL(*mock_runtime, make_next_flow_collection(f_init_c))
    .WillOnce(Return(f_while_out_c));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, f_while_out, Modify, Read, value);
  EXPECT_REGISTER_USE_COLLECTION(while_use_c, f_init_c, f_while_out_c, Modify, None, 4);

  EXPECT_REGISTER_TASK(while_use, while_use_c);

  EXPECT_FLOW_ALIAS(f_while_out, f_null);
  EXPECT_FLOW_ALIAS(f_while_out_c, f_null_c);

  //============================================================================
  // actual code being tested
  {
    struct Foo {
      void operator()(Index1D<int> index,
        AccessHandleCollection<int, Range1D<int>> coll
      ) const {
        coll[index].local_access().set_value(42);
      }
    };

    auto tmp = initial_access<int>("hello");
    auto tmp_c = initial_access_collection<int>("world",
      index_range=Range1D<int>(4)
    );

    create_work_while([=]{
      return tmp.get_value() != 73; // should only be false the first time
    }).do_([=]{
      tmp.set_value(73);
      create_concurrent_work<Foo>(tmp_c,
        index_range=Range1D<int>(4)
      );
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_do_out));
  EXPECT_CALL(*mock_runtime, make_next_flow_collection(f_init_c))
    .WillOnce(Return(f_do_out_c));

  EXPECT_REGISTER_USE_COLLECTION(do_use_c, f_init_c, f_do_out_c, Modify, None, 4);

  EXPECT_REGISTER_USE_AND_SET_BUFFER(do_use, f_init, f_do_out, Modify, Modify, value);

  {
    InSequence reg_before_release;

    EXPECT_REGISTER_USE_COLLECTION(do_cont_use_c, f_do_out_c, f_while_out_c, Modify, None, 4);

    EXPECT_RELEASE_USE(while_use_c);

    EXPECT_REGISTER_TASK(do_use, do_use_c);
  }

  EXPECT_RELEASE_USE(while_use);

  EXPECT_RELEASE_USE(do_cont_use_c);

  EXPECT_FLOW_ALIAS(f_do_out, f_while_out);
  EXPECT_FLOW_ALIAS(f_do_out_c, f_while_out_c);

  run_one_task(); // the first while

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  // For the actual collection
  EXPECT_CALL(*mock_runtime, make_next_flow_collection(f_init_c))
    .WillOnce(Return(f_out_coll));
  // For the next while part
  EXPECT_CALL(*mock_runtime, make_next_flow_collection(f_out_coll))
    .WillOnce(Return(f_while_out_c_2));


  // The captured task collection use itself
  EXPECT_REGISTER_USE_COLLECTION(use_coll, f_init_c, f_out_coll, Modify, Modify, 4);

  {
    InSequence reg_before_rel;

    // The continuation use for the task collection
    EXPECT_REGISTER_USE_COLLECTION(use_coll_cont, f_out_coll, f_do_out_c, Modify, None, 4);

    EXPECT_RELEASE_USE(do_use_c);

    EXPECT_CALL(*mock_runtime, register_task_collection_gmock_proxy(
      UseInGetDependencies(ByRef(use_coll))
    ));
  }

  EXPECT_CALL(*mock_runtime, make_forwarding_flow(f_init))
    .WillOnce(Return(f_while_fwd));
  EXPECT_CALL(*mock_runtime, make_next_flow(f_while_fwd))
    .WillOnce(Return(f_while_out_2));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use_2, f_while_fwd, f_while_out_2,
    Modify, Read, value
  );

  // Captured use for second while
  EXPECT_REGISTER_USE_COLLECTION(while_use_2_c, f_out_coll, f_while_out_c_2,
    Modify, None, 4
  );

  {
    InSequence reg_before_release;

    // Continuation use for second while
    EXPECT_REGISTER_USE_COLLECTION(while_cont_use_2_c, f_while_out_c_2, f_do_out_c,
      Modify, None, 4
    );

    EXPECT_RELEASE_USE(use_coll_cont);

    // the second while task
    EXPECT_REGISTER_TASK(while_use_2, while_use_2_c);
  }

  EXPECT_RELEASE_USE(do_use);
  EXPECT_RELEASE_USE(while_cont_use_2_c);

  EXPECT_FLOW_ALIAS(f_while_out_2, f_do_out);
  EXPECT_FLOW_ALIAS(f_while_out_c_2, f_do_out_c);


  run_one_task(); // the do part

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  // SHOULD BE BEFORE VERIFY

  for(int i = 0; i < 4; ++i) {
    values[i] = 0;

    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(f_init_c, i))
      .WillOnce(Return(f_in_idx[i]));
    EXPECT_CALL(*mock_runtime, make_indexed_local_flow(f_out_coll, i))
      .WillOnce(Return(f_out_idx[i]));
    EXPECT_REGISTER_USE_AND_SET_BUFFER(use_idx[i], f_in_idx[i], f_out_idx[i],
      Modify, Modify, values[i]);

    auto created_task = mock_runtime->task_collections.front()->create_task_for_index(i);

    EXPECT_THAT(created_task.get(), UseInGetDependencies(use_idx[i]));

    EXPECT_RELEASE_USE(use_idx[i]);

    created_task->run();

    EXPECT_THAT(values[i], Eq(42));

  }

  EXPECT_RELEASE_USE(use_coll);

  mock_runtime->task_collections.front().reset(nullptr);

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_RELEASE_USE(while_use_2);
  EXPECT_RELEASE_USE(while_use_2_c);

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  run_one_task(); // the second while

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_THAT(value, Eq(73));

}
