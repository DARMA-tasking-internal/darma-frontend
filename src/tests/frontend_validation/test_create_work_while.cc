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
  use_t* initial_use = nullptr;
  use_t* while_use = nullptr;
  use_t* outer_cont_use = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, initial_use, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_while_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, f_while_out, Modify, Read, value);
  EXPECT_REGISTER_USE(outer_cont_use, f_while_out, f_null, Modify, None);

  EXPECT_RELEASE_USE(initial_use);

  EXPECT_REGISTER_TASK(while_use);

  EXPECT_FLOW_ALIAS(f_while_out, f_null);
  EXPECT_RELEASE_USE(outer_cont_use);

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

TEST_F_WITH_PARAMS(TestCreateWorkWhile, basic_same_one_iteration,
  ::testing::Combine(::testing::Bool(), ::testing::Bool()),
  std::tuple<bool, bool>
) {
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
  use_t* while_use_2_cont = nullptr;
  use_t* do_cont_use = nullptr;
  use_t* use_initial = nullptr;
  use_t* use_outer_cont = nullptr;

  int value = 0;

  bool while_as_functor = std::get<0>(GetParam());
  bool do_as_functor = std::get<1>(GetParam());

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_initial, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_while_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, f_while_out, Modify, Read, value);
  EXPECT_REGISTER_USE(use_outer_cont, f_while_out, f_null, Modify, None);

  EXPECT_RELEASE_USE(use_initial);

  EXPECT_REGISTER_TASK(while_use);

  EXPECT_FLOW_ALIAS(f_while_out, f_null);
  EXPECT_RELEASE_USE(use_outer_cont);

  //============================================================================
  // actual code being tested
  {

    auto tmp = initial_access<int>("hello");

    struct WhileFunctor {
      bool operator()(AccessHandle<int> t) {
        return t.get_value() != 73; // should only be false the first time
      }
    };

    struct DoFunctor {
      void operator()(AccessHandle<int> t) {
        t.set_value(73);
      }
    };

    if(not while_as_functor and not do_as_functor) {
      // CASE 0
      create_work_while([=]{
        return tmp.get_value() != 73; // should only be false the first time
      }).do_([=]{
        tmp.set_value(73);
      });
    }
    else if(not while_as_functor and do_as_functor) {
      // CASE 1
      create_work_while([=]{
        return tmp.get_value() != 73; // should only be false the first time
      }).do_<DoFunctor>(tmp);
    }
    else if(while_as_functor and not do_as_functor) {
      // CASE 2
      create_work_while<WhileFunctor>(tmp).do_([=]{
        tmp.set_value(73);
      });
    }
    else if(while_as_functor and do_as_functor) {
      // CASE 3
      create_work_while<WhileFunctor>(tmp).do_<DoFunctor>(tmp);
    }
    else {
      FAIL() << "huh?"; // LCOV_EXCL_LINE
    }


  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_do_out));

  {
    InSequence reg_before_release;

    EXPECT_REGISTER_USE_AND_SET_BUFFER(do_use, f_init, f_do_out, Modify, Modify, value);
    EXPECT_REGISTER_USE(do_cont_use, f_do_out, f_while_out, Modify, None);

    EXPECT_RELEASE_USE(while_use);

    EXPECT_RELEASE_USE(do_cont_use);

    EXPECT_REGISTER_TASK(do_use);

  }

  // TODO get rid of this and the corresponding make_next
  EXPECT_FLOW_ALIAS(f_do_out, f_while_out);

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

    EXPECT_REGISTER_USE(while_use_2_cont, f_while_out_2, f_do_out, Modify, None);

    EXPECT_RELEASE_USE(do_use);

    // TODO get rid of this and the corresponding make_next
    EXPECT_FLOW_ALIAS(f_while_out_2, f_do_out);

    EXPECT_RELEASE_USE(while_use_2_cont);

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
  use_t* init_use = nullptr;
  use_t* do_init_use = nullptr;
  use_t* while_use = nullptr;
  use_t* do_sched_use = nullptr;
  use_t* do_cont_use = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, init_use, make_key("hello"));
  EXPECT_INITIAL_ACCESS(f_init_2, f_null_2, do_init_use, make_key("world"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init_2))
    .WillOnce(Return(f_while_out_2));

#if _darma_has_feature(anti_flows)
  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, nullptr, Read, Read, value);
#else
  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, f_init, Read, Read, value);
#endif // _darma_has_feature(anti_flows)
  EXPECT_REGISTER_USE(do_sched_use, f_init_2, f_while_out_2, Modify, None);
  EXPECT_REGISTER_USE(do_cont_use, f_while_out_2, f_null_2, Modify, None);
  EXPECT_RELEASE_USE(do_init_use);

  EXPECT_REGISTER_TASK(while_use, do_sched_use);

  EXPECT_FLOW_ALIAS(f_init, f_null);
  EXPECT_FLOW_ALIAS(f_while_out_2, f_null_2);
  EXPECT_RELEASE_USE(init_use);
  EXPECT_RELEASE_USE(do_cont_use);

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

TEST_F_WITH_PARAMS(TestCreateWorkWhile, basic_same_four_iterations,
  ::testing::Combine(::testing::Bool(), ::testing::Bool()),
  std::tuple<bool, bool>
) {
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
  use_t* init_use, *cont_use;
  use_t* do_use[] = { nullptr, nullptr, nullptr, nullptr };
  use_t* do_cont_use[] = { nullptr, nullptr, nullptr, nullptr };
  use_t* while_use[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
  use_t* while_cont_use[] = { nullptr, nullptr, nullptr, nullptr, nullptr };

  int value = 0;

  bool while_as_functor = std::get<0>(GetParam());
  bool do_as_functor = std::get<1>(GetParam());

  EXPECT_INITIAL_ACCESS(f_init, f_null, init_use, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_while_out[0]));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use[0], f_init, f_while_out[0], Modify, Read, value);
  EXPECT_REGISTER_USE(cont_use, f_while_out[0], f_null, Modify, None);
  EXPECT_RELEASE_USE(init_use);

  EXPECT_REGISTER_TASK(while_use[0]);

  EXPECT_FLOW_ALIAS(f_while_out[0], f_null);
  EXPECT_RELEASE_USE(cont_use);

  //============================================================================
  // actual code being tested
  {
    struct WhileFunctor {
      bool operator()(AccessHandle<int> t) {
        return t.get_value() < 4; // should only be false the first time
      }
    };

    struct DoFunctor {
      void operator()(AccessHandle<int> t) {
        *t += 1;
      }
    };

    auto tmp = initial_access<int>("hello");

    if(not while_as_functor and not do_as_functor) {
      // CASE 0
      create_work_while([=]{
        return tmp.get_value() < 4; // should do 4 iterations
      }).do_([=]{
        *tmp += 1;
      });
    }
    else if(not while_as_functor and do_as_functor) {
      // CASE 1
      create_work_while([=]{
        return tmp.get_value() < 4; // should do 4 iterations
      }).do_<DoFunctor>(tmp);
    }
    else if(while_as_functor and not do_as_functor) {
      // CASE 2
      create_work_while<WhileFunctor>(tmp).do_([=]{
        *tmp += 1;
      });
    }
    else if(while_as_functor and do_as_functor) {
      // CASE 3
      create_work_while<WhileFunctor>(tmp).do_<DoFunctor>(tmp);
    }
    else {
      FAIL() << "huh?"; // LCOV_EXCL_LINE
    }

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_do_out[0]));

  {
    InSequence reg_before_release;

    EXPECT_REGISTER_USE_AND_SET_BUFFER(do_use[0], f_init, f_do_out[0], Modify, Modify, value);
    EXPECT_REGISTER_USE(do_cont_use[0], f_do_out[0], f_while_out[0], Modify, None);

    EXPECT_RELEASE_USE(while_use[0]);

    EXPECT_REGISTER_TASK(do_use[0]);
  }

  EXPECT_FLOW_ALIAS(f_do_out[0], f_while_out[0]);
  EXPECT_RELEASE_USE(do_cont_use[0]);

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
      EXPECT_REGISTER_USE(while_cont_use[i+1], f_while_out[i+1], f_do_out[i],
        Modify, None
      );

      EXPECT_RELEASE_USE(do_use[i]);

      // TODO get rid of this and the corresponding make_next and just use existing out
      EXPECT_FLOW_ALIAS(f_while_out[i+1], f_do_out[i]);
      EXPECT_RELEASE_USE(while_cont_use[i+1]);

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
        EXPECT_REGISTER_USE(do_cont_use[i + 1],
          f_do_out[i + 1], f_while_out[i+1], Modify, None
        );

        EXPECT_RELEASE_USE(while_use[i + 1]);

        // TODO get rid of this and the corresponding make_next and just use existing out
        EXPECT_FLOW_ALIAS(f_do_out[i+1], f_while_out[i+1]);
        EXPECT_RELEASE_USE(do_cont_use[i+1]);

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
  use_t* while_cont_use_2 = nullptr;
  use_t* init_use = nullptr;
  use_t* init_use_c = nullptr;
  use_t* cont_use = nullptr;
  use_t* cont_use_c = nullptr;

  int value = 0;
  int values[4] = {0, 0, 0, 0};

  EXPECT_INITIAL_ACCESS(f_init, f_null, init_use, make_key("hello"));
  EXPECT_INITIAL_ACCESS_COLLECTION(f_init_c, f_null_c, init_use_c, make_key("world"), 4);

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_while_out));
  EXPECT_CALL(*mock_runtime, make_next_flow_collection(f_init_c))
    .WillOnce(Return(f_while_out_c));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, f_while_out, Modify, Read, value);
  EXPECT_REGISTER_USE_COLLECTION(while_use_c, f_init_c, f_while_out_c, Modify, None, 4);
  EXPECT_REGISTER_USE(cont_use, f_while_out, f_null, Modify, None);
  EXPECT_REGISTER_USE_COLLECTION(cont_use_c, f_while_out_c, f_null_c, Modify, None, 4);
  EXPECT_RELEASE_USE(init_use);
  EXPECT_RELEASE_USE(init_use_c);

  EXPECT_REGISTER_TASK(while_use, while_use_c);

  EXPECT_FLOW_ALIAS(f_while_out, f_null);
  EXPECT_FLOW_ALIAS(f_while_out_c, f_null_c);
  EXPECT_RELEASE_USE(cont_use);
  EXPECT_RELEASE_USE(cont_use_c);

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
  EXPECT_REGISTER_USE(do_cont_use, f_do_out, f_while_out, Modify, None);

  {
    InSequence reg_before_release;

    EXPECT_REGISTER_USE_COLLECTION(do_cont_use_c, f_do_out_c, f_while_out_c, Modify, None, 4);

    EXPECT_RELEASE_USE(while_use_c);

    EXPECT_REGISTER_TASK(do_use, do_use_c);
  }

  EXPECT_RELEASE_USE(while_use);

  EXPECT_FLOW_ALIAS(f_do_out, f_while_out);
  EXPECT_RELEASE_USE(do_cont_use);

  EXPECT_FLOW_ALIAS(f_do_out_c, f_while_out_c);
  EXPECT_RELEASE_USE(do_cont_use_c);

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

  EXPECT_REGISTER_USE(while_cont_use_2, f_while_out_2, f_do_out, Modify, None);

  {
    InSequence reg_before_rel;

    // The continuation use for the task collection
    EXPECT_REGISTER_USE_COLLECTION(use_coll_cont, f_out_coll, f_do_out_c, Modify, None, 4);

    EXPECT_RELEASE_USE(do_use_c);

    EXPECT_CALL(*mock_runtime, register_task_collection_gmock_proxy(
      CollectionUseInGetDependencies(ByRef(use_coll))
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

  EXPECT_FLOW_ALIAS(f_while_out_2, f_do_out);

  EXPECT_RELEASE_USE(while_cont_use_2);

  EXPECT_FLOW_ALIAS(f_while_out_c_2, f_do_out_c);
  EXPECT_RELEASE_USE(while_cont_use_2_c);


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

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkWhile, functor_same_always_false) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_while_out
  );
  use_t* init_use = nullptr;
  use_t* while_use = nullptr;
  use_t* cont_use = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, init_use, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_while_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, f_while_out, Modify, Read, value);
  EXPECT_REGISTER_USE(cont_use, f_while_out, f_null, Modify, None);
  EXPECT_RELEASE_USE(init_use);

  EXPECT_REGISTER_TASK(while_use);

  EXPECT_FLOW_ALIAS(f_while_out, f_null);
  EXPECT_RELEASE_USE(cont_use);

  //============================================================================
  // actual code being tested
  {

    struct WhilePart {
      bool operator()(AccessHandle<int> t) {
        return t.get_value() != 0;
      }
    };
    struct DoPart {
      void operator()(AccessHandle<int> t) {
        t.set_value(73);
      }
    };

    auto tmp = initial_access<int>("hello");

    create_work_while<WhilePart>(tmp).do_<DoPart>(tmp);

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_RELEASE_USE(while_use);

  run_all_tasks();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkWhile, while_nested_read) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_while_out, f_do_out, f_while_2_fwd, f_while_2_out,
    f_do_fwd, f_subtask_out
  );
  use_t* use_init = nullptr;
  use_t* use_while = nullptr;
  use_t* use_while_cont = nullptr;
  use_t* use_do = nullptr;
  use_t* use_do_cont = nullptr;
  use_t* use_while_2 = nullptr;
  use_t* use_while_cont_2 = nullptr;
  use_t* use_subtask = nullptr;
  use_t* use_subtask_cont = nullptr;

  size_t value = 0;

  EXPECT_NEW_INITIAL_ACCESS(f_init, f_null, use_init, make_key("hello"));

  EXPECT_NEW_REGISTER_USE_AND_SET_BUFFER(use_while,
    f_init, Same, &f_init,
    f_while_out, Next, nullptr, true,
    Modify, Read, true, value
  );

  EXPECT_NEW_REGISTER_USE(use_while_cont,
    f_while_out, Same, &f_while_out,
    f_null, Same, &f_null, false,
    Modify, None, false
  );

  EXPECT_NEW_RELEASE_USE(use_init, false);

  EXPECT_REGISTER_TASK(use_while);

  EXPECT_NEW_RELEASE_USE(use_while_cont, true);

  //============================================================================
  // actual code being tested
  {
    auto cur_iter = initial_access<size_t>("hello");

    create_work_while([=]{
      return cur_iter.get_value() < 1;
    }).do_([=]{
      (*cur_iter)++;
      darma::create_work(reads(cur_iter),[=]{
        ASSERT_THAT(cur_iter.get_value(), Eq(1));
      });
    });
  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  //----------------------------------------------------------------------------
  // Expect the do block to be registered inside the while block execution

  EXPECT_NEW_REGISTER_USE_AND_SET_BUFFER(use_do,
    f_init, Same, &f_init,
    f_do_out, Next, nullptr, true,
    Modify, Modify, true, value
  );

  EXPECT_NEW_REGISTER_USE(use_do_cont,
    f_do_out, Same, &f_do_out,
    f_while_out, Same, &f_while_out, false,
    Modify, None, false
  );

  EXPECT_NEW_RELEASE_USE(use_while, false);

  EXPECT_REGISTER_TASK(use_do);

  EXPECT_NEW_RELEASE_USE(use_do_cont, true);

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  //----------------------------------------------------------------------------
  // Expect the next while block to be registered inside the do block execution
  // and the read subtask

#if _darma_has_feature(anti_flows)
  EXPECT_NEW_REGISTER_USE_AND_SET_BUFFER(use_subtask,
    f_do_fwd, Forwarding, &f_init,
    MockFlow(nullptr), Insignificant, nullptr, false,
    Read, Read, true, value
  );
#else
  EXPECT_NEW_REGISTER_USE_AND_SET_BUFFER(use_subtask,
    f_do_fwd, Forwarding, &f_init,
    f_do_fwd, Same, nullptr, true,
    Read, Read, true, value
  );
#endif // _darma_has_feature(anti_flows)

  EXPECT_NEW_REGISTER_USE_AND_SET_BUFFER(use_subtask_cont,
    f_do_fwd, Same, &f_do_fwd,
    f_do_out, Same, &f_do_out, false,
    Modify, Read, false, value
  );

  EXPECT_NEW_RELEASE_USE(use_do, false);

  EXPECT_REGISTER_TASK(use_subtask);

  EXPECT_NEW_REGISTER_USE_AND_SET_BUFFER(use_while_2,
    f_do_fwd, Same, &f_do_fwd,
    f_while_2_out, Next, nullptr, true,
    Modify, Read, true, value
  );

  EXPECT_NEW_REGISTER_USE(use_while_cont_2,
    f_while_2_out, Same, &f_while_2_out,
    f_do_out, Same, &f_do_out, false,
    Modify, None, false
  );

  EXPECT_NEW_RELEASE_USE(use_subtask_cont, false);

  EXPECT_REGISTER_TASK(use_while_2);

  EXPECT_NEW_RELEASE_USE(use_while_cont_2, true);

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  //----------------------------------------------------------------------------
  // Now run the nested read task

  EXPECT_NEW_RELEASE_USE(use_subtask, false);

  run_one_task();

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  //----------------------------------------------------------------------------
  // And run the while

  // This establishes an alias because a subsequent was created for the purposes
  // of schedule modify but there was never an immediate modify subtask created,
  // thus making the two versions equivalent.
  EXPECT_NEW_RELEASE_USE(use_while_2, true);

  run_one_task();

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestCreateWorkWhile, basic_same_one_iter_nested) {
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
  use_t* while_use_2_cont = nullptr;
  use_t* do_cont_use = nullptr;
  use_t* use_initial = nullptr;
  use_t* use_outer_cont = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_initial, make_key("hello"));

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_while_out));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, f_while_out, Modify, Read, value);
  EXPECT_REGISTER_USE(use_outer_cont, f_while_out, f_null, Modify, None);

  EXPECT_RELEASE_USE(use_initial);

  EXPECT_REGISTER_TASK(while_use);

  EXPECT_FLOW_ALIAS(f_while_out, f_null);
  EXPECT_RELEASE_USE(use_outer_cont);

  //============================================================================
  // actual code being tested
  {
    struct Foo {
      Foo() { tmp = initial_access<int>("hello"); }
      AccessHandle<int> tmp;
    };

    Foo f;

    create_work_while([=]{
      return f.tmp.get_value() == 0; // should always be true
    }).do_([=]{
      f.tmp.set_value(73);
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_do_out));

  {
    InSequence reg_before_release;

    EXPECT_REGISTER_USE_AND_SET_BUFFER(do_use, f_init, f_do_out, Modify, Modify, value);
    EXPECT_REGISTER_USE(do_cont_use, f_do_out, f_while_out, Modify, None);

    EXPECT_RELEASE_USE(while_use);

    EXPECT_RELEASE_USE(do_cont_use);

    EXPECT_REGISTER_TASK(do_use);

  }

  // TODO get rid of this and the corresponding make_next
  EXPECT_FLOW_ALIAS(f_do_out, f_while_out);

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

    EXPECT_REGISTER_USE(while_use_2_cont, f_while_out_2, f_do_out, Modify, None);

    EXPECT_RELEASE_USE(do_use);

    // TODO get rid of this and the corresponding make_next
    EXPECT_FLOW_ALIAS(f_while_out_2, f_do_out);

    EXPECT_RELEASE_USE(while_use_2_cont);

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

TEST_F(TestCreateWorkWhile, basic_coll_one_iter_nested) {
  using namespace darma_runtime;
  using namespace ::testing;
  using namespace mock_backend;
  using namespace darma_runtime::keyword_arguments;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(
    f_init, f_null, f_while_out, f_do_out,
    f_while_fwd, f_while_out_2,
    f_init_c, f_null_c, f_while_out_c, f_do_out_c,
    f_while_out_2_c, f_coll_out
  );
  use_t* while_use = nullptr;
  use_t* while_use_c = nullptr;
  use_t* do_use = nullptr;
  use_t* do_use_c = nullptr;
  use_t* while_use_2 = nullptr;
  use_t* while_use_2_c = nullptr;
  use_t* while_use_2_cont = nullptr;
  use_t* while_use_2_cont_c = nullptr;
  use_t* do_cont_use = nullptr;
  use_t* do_cont_use_c = nullptr;
  use_t* use_initial = nullptr;
  use_t* use_initial_c = nullptr;
  use_t* use_outer_cont = nullptr;
  use_t* use_outer_cont_c = nullptr;
  use_t* use_coll = nullptr;
  use_t* use_coll_cont = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, use_initial, make_key("hello"));
  EXPECT_INITIAL_ACCESS_COLLECTION(f_init_c, f_null_c, use_initial_c, make_key("world"), 4);

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_while_out));
  EXPECT_CALL(*mock_runtime, make_next_flow_collection(f_init_c))
    .WillOnce(Return(f_while_out_c));

  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use, f_init, f_while_out, Modify, Read, value);
  EXPECT_REGISTER_USE(use_outer_cont, f_while_out, f_null, Modify, None);
  EXPECT_REGISTER_USE_COLLECTION(while_use_c, f_init_c, f_while_out_c, Modify, None, 4);
  EXPECT_REGISTER_USE_COLLECTION(use_outer_cont_c, f_while_out_c, f_null_c, Modify, None, 4);

  EXPECT_RELEASE_USE(use_initial);
  EXPECT_RELEASE_USE(use_initial_c);

  EXPECT_REGISTER_TASK(while_use, while_use_c);

  EXPECT_FLOW_ALIAS(f_while_out, f_null);
  EXPECT_RELEASE_USE(use_outer_cont);

  EXPECT_FLOW_ALIAS(f_while_out_c, f_null_c);
  EXPECT_RELEASE_USE(use_outer_cont_c);

  //============================================================================
  // actual code being tested
  {
    struct Foo {
      Foo() { tmp_c = initial_access_collection<int>("world", index_range=Range1D<int>(4)); }
      //Foo(Foo const&) = default;
      //Foo(Foo&&) = default;
      AccessHandleCollection<int, Range1D<int>> tmp_c;
    };

    struct Bar {
      void operator()(Index1D<int> idx, AccessHandleCollection<int, Range1D<int>> coll) { }
    };

    Foo f;
    auto tmp = initial_access<int>("hello");

    create_work_while([=]{
      return tmp.get_value() == 0; // should always be true
    }).do_([=]{
      tmp.set_value(73);
      EXPECT_THAT(f.tmp_c.get_index_range().size(), Eq(4));
      // invoke copy ctor
      create_concurrent_work<Bar>(f.tmp_c, index_range=f.tmp_c.get_index_range());
      [](auto p){ }(f.tmp_c);
    });

  }
  //============================================================================

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_next_flow(f_init))
    .WillOnce(Return(f_do_out));
  EXPECT_CALL(*mock_runtime, make_next_flow_collection(f_init_c))
    .WillOnce(Return(f_do_out_c));


  EXPECT_REGISTER_USE_AND_SET_BUFFER(do_use, f_init, f_do_out, Modify, Modify, value);
  EXPECT_REGISTER_USE_COLLECTION(do_use_c, f_init_c, f_do_out_c, Modify, None, 4);

  EXPECT_REGISTER_USE(do_cont_use, f_do_out, f_while_out, Modify, None);
  EXPECT_REGISTER_USE_COLLECTION(do_cont_use_c, f_do_out_c, f_while_out_c, Modify, None, 4);


  EXPECT_RELEASE_USE(while_use);
  EXPECT_RELEASE_USE(while_use_c);


  EXPECT_REGISTER_TASK(do_use, do_use_c);

  // TODO get rid of this and the corresponding make_next
  EXPECT_FLOW_ALIAS(f_do_out, f_while_out);
  EXPECT_RELEASE_USE(do_cont_use);

  EXPECT_FLOW_ALIAS(f_do_out_c, f_while_out_c);
  EXPECT_RELEASE_USE(do_cont_use_c);

  run_one_task(); // the first while

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_CALL(*mock_runtime, make_forwarding_flow(f_init))
    .WillOnce(Return(f_while_fwd));
  EXPECT_CALL(*mock_runtime, make_next_flow(f_while_fwd))
    .WillOnce(Return(f_while_out_2));

  EXPECT_CALL(*mock_runtime, make_next_flow_collection(f_init_c))
    .WillOnce(Return(f_coll_out));
  EXPECT_CALL(*mock_runtime, make_next_flow_collection(f_coll_out))
    .WillOnce(Return(f_while_out_2_c));


  EXPECT_REGISTER_USE_AND_SET_BUFFER(while_use_2, f_while_fwd, f_while_out_2,
    Modify, Read, value
  );
  EXPECT_REGISTER_USE_COLLECTION(while_use_2_c, f_coll_out, f_while_out_2_c,
    Modify, None, 4
  );

  EXPECT_REGISTER_USE(while_use_2_cont, f_while_out_2, f_do_out, Modify, None);
  EXPECT_REGISTER_USE_COLLECTION(while_use_2_cont_c, f_while_out_2_c, f_do_out_c, Modify, None, 4);

  EXPECT_REGISTER_USE_COLLECTION(use_coll, f_init_c, f_coll_out, Modify, Modify, 4);
  EXPECT_REGISTER_USE_COLLECTION(use_coll_cont, f_coll_out, f_do_out_c, Modify, None, 4);

  EXPECT_RELEASE_USE(do_use);
  EXPECT_RELEASE_USE(do_use_c);
  EXPECT_RELEASE_USE(use_coll_cont);

  // TODO get rid of this and the corresponding make_next
  EXPECT_FLOW_ALIAS(f_while_out_2, f_do_out);
  EXPECT_RELEASE_USE(while_use_2_cont);
  EXPECT_FLOW_ALIAS(f_while_out_2_c, f_do_out_c);
  EXPECT_RELEASE_USE(while_use_2_cont_c);

  EXPECT_REGISTER_TASK(while_use_2, while_use_2_c);

  run_one_task(); // the do part

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_FLOW_ALIAS(f_while_fwd, f_while_out_2);
  EXPECT_RELEASE_USE(while_use_2);
  EXPECT_FLOW_ALIAS(f_coll_out, f_while_out_2_c);
  EXPECT_RELEASE_USE(while_use_2_c);

  run_one_task(); // the second while

  Mock::VerifyAndClearExpectations(mock_runtime.get());

  EXPECT_RELEASE_USE(use_coll);

  mock_runtime->task_collections.clear();

  EXPECT_THAT(value, Eq(73));

}

////////////////////////////////////////////////////////////////////////////////

//TEST_F(TestCreateWorkWhile, nested_in_call_two_iters) {
//  using namespace darma_runtime;
//  using namespace ::testing;
//  using namespace mock_backend;
//  using namespace darma_runtime::keyword_arguments;
//
//
//
//  //============================================================================
//  // actual code being tested
//  {
//    struct Bar {
//      void operator()(Index1D<int> idx, AccessHandleCollection<int, Range1D<int>> coll) {
//
//      }
//    };
//
//    auto tmp = initial_access<int>("hello");
//
//    create_work_while([=]{
//      return tmp.get_value() == 0; // should always be true
//    }).do_([=]{
//      tmp.set_value(73);
//      EXPECT_THAT(f.tmp_c.get_index_range().size(), Eq(4));
//      // invoke copy ctor
//      create_concurrent_work<Bar>(f.tmp_c, index_range=f.tmp_c.get_index_range());
//      [](auto p){ }(f.tmp_c);
//    });
//
//  }
//  //============================================================================
//
//
//}

