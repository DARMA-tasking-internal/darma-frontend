/*
//@HEADER
// ************************************************************************
//
//                      test_functor_migrate.cc
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

#include "test_functor.h"

#include <darma/interface/frontend/unpack_task.h>

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simple_migrate) {
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;

  DECLARE_MOCK_FLOWS(f_init, f_set_42_out, f_null, f_set_42_out_migrated);
  use_t* set_42_use, *hello_use, *migrated_use, *initial_use, *cont_use;
  set_42_use = hello_use = migrated_use = nullptr;

  int value = 0;

  EXPECT_INITIAL_ACCESS(f_init, f_null, initial_use, make_key("hello"));

  EXPECT_LEAF_MOD_CAPTURE_MN_OR_MR_AND_SET_BUFFER(
    f_init, f_set_42_out, set_42_use, f_null, cont_use, value
  );

  EXPECT_RELEASE_USE(initial_use);

  EXPECT_REGISTER_TASK(set_42_use);

  EXPECT_RO_CAPTURE_RN_RR_MN_OR_MR_AND_SET_BUFFER(f_set_42_out, hello_use, value);

  EXPECT_REGISTER_TASK(hello_use);

  EXPECT_FLOW_ALIAS(f_set_42_out, f_null);

  EXPECT_RELEASE_USE(cont_use);

  //============================================================================
  // Code to actually be tested
  {
    struct SetTo42 {
      //void operator()(AccessHandle<int>& val) const { val.set_value(42); }
      void operator()(int& val) const { val = 42; }
    };

    struct HelloWorldNumber {
      void operator()(int const& val) const {
        EXPECT_THAT(val, Eq(42));
      }
    };

    auto tmp = initial_access<int>("hello");

    create_work<SetTo42>(tmp);

    create_work<HelloWorldNumber>(tmp);

  }
  //============================================================================

  EXPECT_RELEASE_USE(set_42_use);

  // run the first task...
  mock_runtime->registered_tasks.front()->run();
  mock_runtime->registered_tasks.pop_front();

  // Now migrate the task...
  auto& task_to_migrate = mock_runtime->registered_tasks.front();

  EXPECT_CALL(*mock_runtime, get_packed_flow_size(f_set_42_out))
    .Times(
#if _darma_has_feature(anti_flows)
      1
#else
      2
#endif // _darma_has_feature(anti_flows)
    ).WillRepeatedly(Return(sizeof(mock_backend::MockFlow)));

#if _darma_has_feature(anti_flows)
  EXPECT_CALL(*mock_runtime, get_packed_flow_size(Eq(nullptr)))
    .Times(1).WillRepeatedly(Return(sizeof(mock_backend::MockFlow)));
#endif // _darma_has_feature(anti_flows)

  size_t task_packed_size = task_to_migrate->get_packed_size();

  char buffer[task_packed_size];

  EXPECT_CALL(*mock_runtime, pack_flow(f_set_42_out, Truly(
    // GCC doesn't like it if I capture buffer by reference, so...
    [buffer=&(buffer[0]), task_packed_size](auto&& flow_buff) {
      // expect that the buffer given here is part of the buffer from above
      return
        (intptr_t(&(buffer[0])) <= intptr_t(flow_buff))
        and (intptr_t(&(buffer[0])+task_packed_size) > intptr_t(flow_buff));
    }
  ))).Times(
#if _darma_has_feature(anti_flows)
    1
#else
    2
#endif // _darma_has_feature(anti_flows)
  ).WillRepeatedly(Invoke([&](auto&& flow, void*& buffer) {
    // memcpy the flow directly into the buffer.  We'll just use it for
    // comparison purposes later to make sure the translation layer is
    // delivering the correct buffer to make_unpacked_flow
    ::memcpy(buffer, (void*)(std::addressof(flow)), sizeof(mock_backend::MockFlow));
    // and advance the buffer
    reinterpret_cast<char*&>(buffer) += sizeof(mock_backend::MockFlow);
  }));

#if _darma_has_feature(anti_flows)
  EXPECT_CALL(*mock_runtime, pack_flow(Eq(nullptr), Truly(
    // GCC doesn't like it if I capture buffer by reference, so...
    [buffer=&(buffer[0]), task_packed_size](auto&& flow_buff) {
      // expect that the buffer given here is part of the buffer from above
      return
        (intptr_t(&(buffer[0])) <= intptr_t(flow_buff))
          and (intptr_t(&(buffer[0])+task_packed_size) > intptr_t(flow_buff));
    }
  ))).Times(1).WillRepeatedly(Invoke([&](auto&& flow, void*& buffer) {
    // memcpy the flow directly into the buffer.  We'll just use it for
    // comparison purposes later to make sure the translation layer is
    // delivering the correct buffer to make_unpacked_flow
    ::memcpy(buffer, (void*)(std::addressof(flow)), sizeof(mock_backend::MockFlow));
    // and advance the buffer
    reinterpret_cast<char*&>(buffer) += sizeof(mock_backend::MockFlow);
  }));
#endif // _darma_has_feature(anti_flows)

  task_to_migrate->pack(buffer);

  EXPECT_CALL(*mock_runtime, make_unpacked_flow(Truly([&](void const* buff){
    return *reinterpret_cast<mock_backend::MockFlow const*>(buff) == f_set_42_out
      || *reinterpret_cast<mock_backend::MockFlow const*>(buff) == nullptr;
  }))).Times(2)
#if _darma_has_feature(anti_flows)
    .WillOnce(Invoke([&](void const*& buff) {
      reinterpret_cast<char const*&>(buff) += sizeof(mock_backend::MockFlow);
      return f_set_42_out_migrated;
    }))
    .WillOnce(Invoke([&](void const*& buff) {
      reinterpret_cast<char const*&>(buff) += sizeof(mock_backend::MockFlow);
      return mock_backend::MockFlow(nullptr);
    }));
#else
    .WillRepeatedly(Invoke([&](void const*& buff) {
      reinterpret_cast<char const*&>(buff) += sizeof(mock_backend::MockFlow);
      return f_set_42_out_migrated;
    }));
#endif // _darma_has_feature(anti_flows)

  EXPECT_CALL(*mock_runtime, reregister_migrated_use(IsUseWithFlows(
#if _darma_has_feature(anti_flows)
    f_set_42_out_migrated, nullptr,
#else
    f_set_42_out_migrated, f_set_42_out_migrated,
#endif // _darma_has_feature(anti_flows)
    use_t::None, use_t::Read
  ))).WillOnce(Invoke([&](auto&& rereg_use) {
      darma_runtime::abstract::frontend::use_cast<
        darma_runtime::abstract::frontend::DependencyUse*
      >(rereg_use)->get_data_pointer_reference() = &value;
      migrated_use = rereg_use;
    }));

  auto migrated_task = darma_runtime::abstract::frontend
    ::PolymorphicSerializableObject<darma_runtime::abstract::frontend::Task>
    ::unpack(buffer);

  ASSERT_THAT(migrated_task->get_dependencies().size(), Eq(1));
  ASSERT_THAT(*migrated_task->get_dependencies().begin(),
    IsUseWithFlows(
  #if _darma_has_feature(anti_flows)
      f_set_42_out_migrated, nullptr,
  #else
      f_set_42_out_migrated, f_set_42_out_migrated,
  #endif // _darma_has_feature(anti_flows)
      use_t::None, use_t::Read
    )
  );

  // Since we need to have f_set_42_out still in existence to make the above
  // expectations work, we need to move the release down here.
  // TODO figure out how to do this test so that task_to_migrate can be released
  // before we unpack the other task
  EXPECT_RELEASE_USE(hello_use);
  // Release the task on the "origin node"
  task_to_migrate = nullptr;

  EXPECT_RELEASE_USE(migrated_use);

  migrated_task->run();

  migrated_task = nullptr;

}

////////////////////////////////////////////////////////////////////////////////
