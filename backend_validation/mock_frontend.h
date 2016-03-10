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


class FakeSerializationManager
  : public darma_runtime::abstract::frontend::SerializationManager
{
  public:

    size_t
    get_metadata_size(
      const void* const deserialized_data
    ) const override {
      if(replace_get_metadata_size) {
        return (*replace_get_metadata_size)(this, deserialized_data);
      }
      else {
        return get_metadata_size_return;
      }
    }
    std::function<size_t(SerializationManager const*, const void* const)>* replace_get_metadata_size = nullptr;
    size_t get_metadata_size_return = sizeof(double);

};

class MockSerializationManager
  : public darma_runtime::abstract::frontend::SerializationManager
{
  public:
    MOCK_CONST_METHOD1(
      get_metadata_size,
      size_t(const void* const deserialized_data)
    );

    void
    delegate_to_fake() {
      using ::testing::_;
      using ::testing::Invoke;
      ON_CALL(*this, get_metadata_size(_))
        .WillByDefault(Invoke(&fake, &FakeSerializationManager::get_metadata_size));
    }

    FakeSerializationManager fake;

};


class FakeDependencyHandle
  : public darma_runtime::abstract::frontend::DependencyHandle<
      darma_runtime::types::key_t,
      darma_runtime::types::version_t
    >
{
  public:
    typedef darma_runtime::types::key_t key_t;
    typedef darma_runtime::types::version_t version_t;

    const key_t&
    get_key() const override {
      if(replace_get_key) {
        return (*replace_get_key)(this);
      }
      else {
        return get_key_return;
      }
    }
    std::function<const key_t&(const FakeDependencyHandle*)>* replace_get_key = nullptr;
    key_t get_key_return;

    const version_t&
    get_version() const override {
      if(replace_get_version) {
        return (*replace_get_version)(this);
      }
      else {
        return get_version_return;
      }

    }
    std::function<const version_t&(const FakeDependencyHandle*)>* replace_get_version = nullptr;
    version_t get_version_return;

    void
    set_version(const version_t& v) override {
      if(replace_set_version) {
        return (*replace_set_version)(this, v);
      }
      else {
        get_version_return = v;
        version_is_pending_return = false;
      }

    }
    std::function<void(FakeDependencyHandle*, const version_t&)>* replace_set_version = nullptr;

    bool
    version_is_pending() const override {
      if(replace_version_is_pending) {
        return (*replace_version_is_pending)(this);
      }
      else {
        return version_is_pending_return;
      }
    }
    std::function<bool(const FakeDependencyHandle*)>* replace_version_is_pending = nullptr;
    bool version_is_pending_return = false;

    darma_runtime::abstract::frontend::SerializationManager*
    get_serialization_manager() override {
      if(replace_get_serialization_manager) {
        return (*replace_get_serialization_manager)(this);
      }
      else {
        return get_serialization_manager_return;
      }
    }
    std::function<darma_runtime::abstract::frontend::SerializationManager*(FakeDependencyHandle*)>* replace_get_serialization_manager = nullptr;
    darma_runtime::abstract::frontend::SerializationManager* get_serialization_manager_return = nullptr;

    void
    satisfy_with_data_block(
      darma_runtime::abstract::backend::DataBlock* const data
    ) override {
      if(replace_satisfy_with_data_block) {
        (*replace_satisfy_with_data_block)(this, data);
      }
      else {
        get_data_block_return = data;
        is_satisfied_return = true;
      }
    }
    std::function<void(FakeDependencyHandle*, darma_runtime::abstract::backend::DataBlock* const)>* replace_satisfy_with_data_block = nullptr;

    darma_runtime::abstract::backend::DataBlock*
    get_data_block() const override {
      if(replace_get_data_block) {
        return (*replace_get_data_block)(this);
      }
      else {
        return get_data_block_return;
      }

    }
    std::function<darma_runtime::abstract::backend::DataBlock*(const FakeDependencyHandle*)>* replace_get_data_block = nullptr;
    darma_runtime::abstract::backend::DataBlock* get_data_block_return = nullptr;

    bool
    is_satisfied() const override {
      if(replace_is_satisfied) {
        return (*replace_is_satisfied)(this);
      }
      else {
        return is_satisfied_return;
      }
    }
    std::function<bool(const FakeDependencyHandle*)>* replace_is_satisfied = nullptr;
    bool is_satisfied_return = false;

    void
    allow_writes() override {
      if(replace_allow_writes) {
        return (*replace_allow_writes)(this);
      }
      else {
        is_writable_return = true;
      }
    }
    std::function<void(FakeDependencyHandle*)>* replace_allow_writes = nullptr;

    bool
    is_writable() const override {
      if(replace_is_writable) {
        return (*replace_is_writable)(this);
      }
      else {
        return is_writable_return;
      }
    }
    std::function<bool(const FakeDependencyHandle*)>* replace_is_writable = nullptr;
    bool is_writable_return;

};

class MockDependencyHandle
  : public darma_runtime::abstract::frontend::DependencyHandle<
      darma_runtime::types::key_t,
      darma_runtime::types::version_t
    >
{
  private:
    typedef darma_runtime::types::key_t key_t;
    typedef darma_runtime::types::version_t version_t;
  public:
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
    delegate_to_fake() {
      using ::testing::_;
      using ::testing::Invoke;
      ON_CALL(*this, get_key())
        .WillByDefault(Invoke(&fake, &FakeDependencyHandle::get_key));
      ON_CALL(*this, get_version())
        .WillByDefault(Invoke(&fake, &FakeDependencyHandle::get_version));
      ON_CALL(*this, set_version(_))
        .WillByDefault(Invoke(&fake, &FakeDependencyHandle::set_version));
      ON_CALL(*this, version_is_pending())
        .WillByDefault(Invoke(&fake, &FakeDependencyHandle::version_is_pending));
      ON_CALL(*this, get_serialization_manager())
        .WillByDefault(Invoke(&fake, &FakeDependencyHandle::get_serialization_manager));
      ON_CALL(*this, satisfy_with_data_block(_))
        .WillByDefault(Invoke(&fake, &FakeDependencyHandle::satisfy_with_data_block));
      ON_CALL(*this, get_data_block())
        .WillByDefault(Invoke(&fake, &FakeDependencyHandle::get_data_block));
      ON_CALL(*this, is_satisfied())
        .WillByDefault(Invoke(&fake, &FakeDependencyHandle::is_satisfied));
      ON_CALL(*this, allow_writes())
        .WillByDefault(Invoke(&fake, &FakeDependencyHandle::allow_writes));
      ON_CALL(*this, is_writable())
        .WillByDefault(Invoke(&fake, &FakeDependencyHandle::is_writable));
    }

    FakeDependencyHandle fake;

};



class FakeTask
  : public darma_runtime::abstract::frontend::Task<
      darma_runtime::types::key_t,
      darma_runtime::types::version_t,
      darma_runtime::types::handle_container_template
    >
{
  public:

    typedef darma_runtime::types::key_t key_t;
    typedef darma_runtime::types::version_t version_t;
    typedef darma_runtime::abstract::frontend::DependencyHandle<key_t, version_t> handle_t;
    typedef darma_runtime::types::handle_container_template<handle_t*> handle_container_t;

    handle_container_t get_dependencies_return;
    std::function<const handle_container_t&(const FakeTask*)>* replace_get_dependencies = nullptr;

    const handle_container_t&
    get_dependencies() const override {
      if(replace_get_dependencies) {
        return (*replace_get_dependencies)(this);
      }
      else {
        return get_dependencies_return;
      }
    }


    bool needs_read_data_return = false;
    std::function<bool(const FakeTask*, const handle_t*)>* replace_needs_read_data = nullptr;

    bool needs_read_data(const handle_t* handle) const override {
      if(replace_needs_read_data) {
        return (*replace_needs_read_data)(this, handle);
      }
      else {
        return needs_read_data_return;
      }
    }


    bool needs_write_data_return = false;
    std::function<bool(const FakeTask*, const handle_t*)>* replace_needs_write_data = nullptr;

    bool needs_write_data(const handle_t* handle) const override {
      if(replace_needs_write_data) {
        return (*replace_needs_write_data)(this, handle);
      }
      else {
        return needs_write_data_return;
      }
    }


    key_t get_name_return;
    std::function<const key_t&(const FakeTask*)>* replace_get_name = nullptr;

    const key_t&
    get_name() const override {
      if(replace_get_name) {
        return (*replace_get_name)(this);
      }
      else {
        return get_name_return;
      }
    }


    std::function<void(FakeTask*, const key_t&)>* replace_set_name = nullptr;

    void set_name(const key_t& name_key) override {
      if(replace_get_name) {
        (*replace_set_name)(this, name_key);
      }
      else {
        get_name_return = name_key;
      }
    }

    bool is_migratable_return = false;
    std::function<bool(const FakeTask*)>* replace_is_migratable = nullptr;

    bool
    is_migratable() const override {
      if(replace_is_migratable) {
        return (*replace_is_migratable)(this);
      }
      else {
        return is_migratable_return;
      }
    }

    std::function<void(const FakeTask*)>* replace_run = nullptr;

    void run() override {
      if(replace_run) {
        (*replace_run)(this);
      }
    }

};


class MockTask
  : public darma_runtime::abstract::frontend::Task<
      darma_runtime::types::key_t,
      darma_runtime::types::version_t,
      darma_runtime::types::handle_container_template
    >
{
  private:
    typedef darma_runtime::types::key_t key_t;
    typedef darma_runtime::types::version_t version_t;
    typedef darma_runtime::abstract::frontend::DependencyHandle<key_t, version_t> handle_t;
    typedef darma_runtime::types::handle_container_template<handle_t*> handle_container_t;

  public:
    MOCK_CONST_METHOD0(get_dependencies, handle_container_t const&());
    MOCK_CONST_METHOD1(needs_read_data, bool(handle_t const* handle));
    MOCK_CONST_METHOD1(needs_write_data, bool(handle_t const* handle));
    MOCK_CONST_METHOD0(get_name, key_t const&());
    MOCK_METHOD1(set_name, void(key_t const& name_key));
    MOCK_CONST_METHOD0(is_migratable, bool());
    MOCK_METHOD0(run, void());

    void
    delegate_to_fake() {
      using ::testing::_;
      using ::testing::Invoke;
      ON_CALL(*this, get_dependencies())
        .WillByDefault(Invoke(&fake, &FakeTask::get_dependencies));
      ON_CALL(*this, needs_read_data(_))
        .WillByDefault(Invoke(&fake, &FakeTask::needs_read_data));
      ON_CALL(*this, needs_write_data(_))
        .WillByDefault(Invoke(&fake, &FakeTask::needs_write_data));
      ON_CALL(*this, get_name())
        .WillByDefault(Invoke(&fake, &FakeTask::get_name));
      ON_CALL(*this, set_name(_))
        .WillByDefault(Invoke(&fake, &FakeTask::set_name));
      ON_CALL(*this, is_migratable())
        .WillByDefault(Invoke(&fake, &FakeTask::is_migratable));
      ON_CALL(*this, run())
        .WillByDefault(Invoke(&fake, &FakeTask::run));
    }

    FakeTask fake;
};


} // end namespace mock_frontend

#ifndef DARMA_THREAD_LOCAL_BACKEND_RUNTIME
#  define DARMA_THREAD_LOCAL_BACKEND_RUNTIME
#endif

namespace darma_runtime {

namespace detail {

template <typename __ignored = void>
abstract::backend::runtime_t*&
_gen_backend_runtime_ptr() {
  static_assert(std::is_same<__ignored, void>::value, "");
  static DARMA_THREAD_LOCAL_BACKEND_RUNTIME abstract::backend::runtime_t* rv;
  return rv;
}

//extern
DARMA_THREAD_LOCAL_BACKEND_RUNTIME
abstract::backend::runtime_t*& backend_runtime = _gen_backend_runtime_ptr<>();

} // end namespace backend

} // end namespace darma_runtime

#endif /* TEST_GTEST_BACKEND_MOCK_FRONTEND_H_ */
