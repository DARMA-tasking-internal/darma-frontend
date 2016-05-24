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

#include <gmock/gmock.h>

#include <darma/interface/frontend/task.h>
#include <darma/interface/frontend/serialization_manager.h>
#include <darma/interface/frontend/dependency_handle.h>
#include <darma/interface/frontend/types.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/backend/data_block.h>
#include <darma_types.h>
#include <functional>

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
    MOCK_CONST_METHOD0(allocate_data, void*());
};

class MockDependencyHandle
  : public darma_runtime::abstract::frontend::DependencyHandle<
      darma_runtime::types::key_t,
      darma_runtime::types::version_t
    >
{
  public:
    typedef darma_runtime::types::key_t key_t;
    typedef darma_runtime::types::version_t version_t;

    key_t get_key_return;
    version_t get_version_return;
    bool version_is_pending_return = false;
    bool is_satisfied_return = false;
    bool is_writable_return = false;
    darma_runtime::abstract::backend::DataBlock* get_data_block_return = nullptr;

    MockDependencyHandle(){
      get_key_return = darma_runtime::make_key("key_not_specified");
      this->set_default_behavior();
    }

    MOCK_CONST_METHOD0(get_key, key_t const&());
    MOCK_CONST_METHOD0(get_version, version_t const&());
    MOCK_METHOD1(set_version, void(version_t const& v));
    MOCK_CONST_METHOD0(version_is_pending, bool());
    MOCK_METHOD0(get_serialization_manager,
      darma_runtime::abstract::frontend::SerializationManager*()
    );
    MOCK_METHOD1(satisfy_with_data_block,
      void(darma_runtime::abstract::backend::DataBlock* data)
    );
    MOCK_CONST_METHOD0(get_data_block,
      darma_runtime::abstract::backend::DataBlock*()
    );
    MOCK_CONST_METHOD0(is_satisfied, bool());
    MOCK_METHOD0(allow_writes, void());
    MOCK_CONST_METHOD0(is_writable, bool());

    void
    set_default_behavior() {
      using ::testing::_;
      using ::testing::Invoke;
      using ::testing::ReturnRef;
      using ::testing::ReturnPointee;
      ON_CALL(*this, get_key())
        .WillByDefault(ReturnRef(get_key_return));
      ON_CALL(*this, get_version())
        .WillByDefault(ReturnRef(get_version_return));
      ON_CALL(*this, set_version(_))
        .WillByDefault(Invoke([this](version_t const &v){
           get_version_return = v; version_is_pending_return = false; }));
      ON_CALL(*this, version_is_pending())
        .WillByDefault(ReturnPointee(&version_is_pending_return));
      ON_CALL(*this, satisfy_with_data_block(_))
        .WillByDefault(Invoke([this](darma_runtime::abstract::backend::DataBlock *db){
           get_data_block_return = db; is_satisfied_return = true; }));
      ON_CALL(*this, get_data_block())
        .WillByDefault(ReturnPointee(&get_data_block_return));
      ON_CALL(*this, is_satisfied())
        .WillByDefault(ReturnPointee(&is_satisfied_return));
      ON_CALL(*this, allow_writes())
        .WillByDefault(Invoke([this](){ is_writable_return = true; }));
      ON_CALL(*this, is_writable())
        .WillByDefault(ReturnPointee(&is_writable_return));
    }

    void
    increase_version_depth(int cnt) {
      for (int i=0; i<cnt; i++)
        get_version_return.push_subversion();
    }
};

class MockTask
  : public darma_runtime::abstract::frontend::Task
{
  public:
    typedef darma_runtime::types::key_t key_t;
    typedef darma_runtime::types::version_t version_t;
    typedef darma_runtime::abstract::frontend::DependencyHandle<key_t, version_t> handle_t;
    typedef darma_runtime::types::handle_container_template<handle_t*> handle_container_t;

  private:
    key_t get_name_return;

  public:
    MockTask() { this->set_default_behavior(); }

    MOCK_CONST_METHOD0(get_dependencies, handle_container_t const&());
    MOCK_CONST_METHOD1(needs_read_data, bool(handle_t const* handle));
    MOCK_CONST_METHOD1(needs_write_data, bool(handle_t const* handle));
    MOCK_CONST_METHOD0(get_name, key_t const&());
    MOCK_METHOD1(set_name, void(key_t const& name_key));
    MOCK_CONST_METHOD0(is_migratable, bool());
    MOCK_METHOD0(run, void());
    MOCK_CONST_METHOD0(get_packed_size, size_t());
    MOCK_CONST_METHOD1(pack, void(void*));

    void
    set_default_behavior() {
      using ::testing::_;
      using ::testing::Invoke;
      using ::testing::Return;
      using ::testing::ReturnRef;
      using ::testing::ReturnRefOfCopy;
      using ::testing::SaveArg;
      ON_CALL(*this, get_dependencies())
        .WillByDefault(ReturnRefOfCopy(handle_container_t()));
      ON_CALL(*this, needs_read_data(_))
        .WillByDefault(Return(false));
      ON_CALL(*this, needs_write_data(_))
        .WillByDefault(Return(false));
      ON_CALL(*this, get_name())
        .WillByDefault(ReturnRef(get_name_return));
      ON_CALL(*this, set_name(_))
        .WillByDefault(SaveArg<0>(&get_name_return));
      ON_CALL(*this, is_migratable())
        .WillByDefault(Return(false));
      ON_CALL(*this, run())
        .WillByDefault(Invoke([](){}));
    }
};


} // end namespace mock_frontend

#endif /* TEST_GTEST_BACKEND_MOCK_FRONTEND_H_ */
