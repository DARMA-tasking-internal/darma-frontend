/*
//@HEADER
// ************************************************************************
//
//                          mock_frontend.h
//                         dharma_new
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

#ifndef TEST_GTEST_BACKEND_MOCK_FRONTEND_H_
#define TEST_GTEST_BACKEND_MOCK_FRONTEND_H_

#include <functional>

#include <gmock/gmock.h>

#include <darma_types.h>

#include <darma/interface/frontend/task.h>
#include <darma/interface/frontend/serialization_manager.h>
#include <darma/interface/frontend/types.h>
#include <darma/interface/backend/runtime.h>

namespace mock_frontend {

class MockSerializationManager
  : public darma_runtime::abstract::frontend::SerializationManager
{
  public:
    MockSerializationManager() {}

    MOCK_CONST_METHOD0(get_metadata_size, size_t());
    MOCK_CONST_METHOD1(get_packed_data_size, size_t(const void* const));
    MOCK_CONST_METHOD2(pack_data, void(const void* const, void* const));
    MOCK_CONST_METHOD2(unpack_data, void(void* const, const void* const));
    MOCK_CONST_METHOD1(default_construct, void(void*));
    MOCK_CONST_METHOD1(destroy, void(void*));
};

class MockTask
  : public darma_runtime::abstract::frontend::Task<MockTask>
{
  public:
    typedef darma_runtime::types::key_t key_t;
    using use_t = darma_runtime::abstract::frontend::Use;
    typedef darma_runtime::types::handle_container_template<use_t*> handle_container_t;

  private:

    key_t get_name_return;

  public:
    MockTask() { /*this->set_default_behavior();*/ }

    MOCK_METHOD0(run_gmock_proxy, void());

    template < typename T >
    std::enable_if_t<not std::is_same<T, void>::value, T>
    run() {
      return run_gmock_proxy(), T{};
    }

    template < typename T >
    std::enable_if_t<std::is_void<T>::value, void>
    run() {
      run_gmock_proxy();
    }


    MOCK_CONST_METHOD0(get_dependencies, handle_container_t const&());
    MOCK_CONST_METHOD0(get_name, key_t const&());
    MOCK_METHOD1(set_name, void(key_t const&));
    MOCK_CONST_METHOD0(is_migratable, bool());
    MOCK_CONST_METHOD0(get_packed_size, size_t());
    MOCK_CONST_METHOD1(pack, void(void*));

//    void
//    set_default_behavior() {
//      using ::testing::_;
//      using ::testing::Invoke;
//      using ::testing::Return;
//      using ::testing::ReturnRef;
//      using ::testing::ReturnRefOfCopy;
//      using ::testing::SaveArg;
//      ON_CALL(*this, get_dependencies())
//        .WillByDefault(ReturnRefOfCopy(handle_container_t()));
//      ON_CALL(*this, needs_read_data(_))
//        .WillByDefault(Return(false));
//      ON_CALL(*this, needs_write_data(_))
//        .WillByDefault(Return(false));
//      ON_CALL(*this, get_name())
//        .WillByDefault(ReturnRef(get_name_return));
//      ON_CALL(*this, set_name(_))
//        .WillByDefault(SaveArg<0>(&get_name_return));
//      ON_CALL(*this, is_migratable())
//        .WillByDefault(Return(false));
//      ON_CALL(*this, run())
//        .WillByDefault(Invoke([](){}));
//    }
};


class MockHandle
  : public darma_runtime::abstract::frontend::Handle
{
  public:

    typedef darma_runtime::types::key_t key_t;

    MockHandle() { set_default_behavior(); }

    MockHandle(MockHandle&&) = default;

    explicit
    MockHandle(
      key_t const& key
    ) : key_(key), ser_man(
      std::make_shared<::testing::NiceMock<MockSerializationManager>>()
    )
    { set_default_behavior(); }

    template <typename T>
    static MockHandle* create(key_t const& key) {
      using namespace ::testing;
      auto* rv = new NiceMock<MockHandle>(key);
      ON_CALL(*rv->ser_man, get_metadata_size())
        .WillByDefault(Return(sizeof(T)));
      static_assert(std::is_fundamental<T>::value, "Only fundamental types supported in the mock for now");
      ON_CALL(*rv->ser_man, get_packed_data_size(_))
        .WillByDefault(Return(sizeof(T)));
      ON_CALL(*rv->ser_man, pack_data(_, _))
        .WillByDefault(Invoke([](const void* const data, void* const buff){
          std::memcpy(buff, data, sizeof(T));
        }));
      ON_CALL(*rv->ser_man, unpack_data(_, _))
        .WillByDefault(Invoke([](void* const data, const void* const buff){
          std::memcpy(data, buff, sizeof(T));
        }));
      ON_CALL(*rv->ser_man, default_construct(_))
        .WillByDefault(Invoke([](void* alloc){
          new (alloc) T;
        }));
      return rv;
    }


    MOCK_CONST_METHOD0(get_key, key_t const&());
    MOCK_CONST_METHOD0(get_serialization_manager, MockSerializationManager const*());

    void
    set_default_behavior() {
      using namespace ::testing;
      ON_CALL(*this, get_key())
        .WillByDefault(ReturnRef(key_));
      ON_CALL(*this, get_serialization_manager())
        .WillByDefault(Return(ser_man.get()));
    }

    key_t key_;
    std::shared_ptr<MockSerializationManager> ser_man;

};


class MockUse
  : public darma_runtime::abstract::frontend::Use
{
  public:

    using flow_t = darma_runtime::abstract::backend::Flow;

    MockUse() { set_default_behavior(); }

    MockUse(
      MockHandle* handle,
      darma_runtime::abstract::backend::Flow* in_flow,
      darma_runtime::abstract::backend::Flow* out_flow,
      permissions_t scheduling_permissions,
      permissions_t immediate_permissions
    ) : handle_(handle), in_flow_(in_flow),
        out_flow_(out_flow),
        immediate_permissions_(immediate_permissions),
        scheduling_permissions_(scheduling_permissions)
    {
      set_default_behavior();
    }


    MOCK_CONST_METHOD0(get_handle, MockHandle const*());
    MOCK_METHOD0(get_in_flow, flow_t*&());
    MOCK_METHOD0(get_out_flow, flow_t*&());
    MOCK_CONST_METHOD0(immediate_permissions, permissions_t(void));
    MOCK_CONST_METHOD0(scheduling_permissions, permissions_t(void));
    MOCK_METHOD0(get_data_pointer_reference, void*&());

    void
    set_default_behavior() {
      using namespace ::testing;
      ON_CALL(*this, get_handle())
        .WillByDefault(Return(handle_));
      ON_CALL(*this, get_in_flow())
        .WillByDefault(ReturnRef(in_flow_));
      ON_CALL(*this, get_out_flow())
        .WillByDefault(ReturnRef(out_flow_));
      ON_CALL(*this, immediate_permissions())
        .WillByDefault(Return(immediate_permissions_));
      ON_CALL(*this, scheduling_permissions())
        .WillByDefault(Return(scheduling_permissions_));
      ON_CALL(*this, get_data_pointer_reference())
        .WillByDefault(ReturnRef(data_));
    }

    MockHandle* handle_ = nullptr;
    darma_runtime::abstract::backend::Flow* in_flow_;
    darma_runtime::abstract::backend::Flow* out_flow_;
    permissions_t immediate_permissions_;
    permissions_t scheduling_permissions_;
    void* data_;


};

} // end namespace mock_frontend

#include <darma/interface/frontend/detail/crtp_impl.h>

#endif /* TEST_GTEST_BACKEND_MOCK_FRONTEND_H_ */
