/*
//@HEADER
// ************************************************************************
//
//                          helpers.h
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

#ifndef DARMA_BACKEND_VALIDATION_HELPERS_H
#define DARMA_BACKEND_VALIDATION_HELPERS_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef TEST_BACKEND_INCLUDE
#  include TEST_BACKEND_INCLUDE
#endif

// satisfy subsequent of subsequent, each at one less depth
#ifdef DARMA_SERIAL_BACKEND
#  define SERIAL_DISABLED_TEST_F(test, name) TEST_F(test, DISABLED_##name)
#else
#  define SERIAL_DISABLED_TEST_F(test, name) TEST_F(test, name)
#endif

#include "mock_frontend.h"
#include "main.h"

using namespace darma_runtime;
using namespace mock_frontend;
using namespace ::testing;

// some helper functions

template <typename MockDep, typename Lambda, bool needs_read, bool needs_write, bool IsNice=false>
void register_one_dep_capture(MockDep* captured, Lambda&& lambda) {
  typedef typename std::conditional<IsNice, NiceMock<MockTask>, MockTask>::type task_t;

  auto new_task = std::make_unique<task_t>();
  EXPECT_CALL(*new_task, get_dependencies())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(MockTask::handle_container_t{ captured }));
  EXPECT_CALL(*new_task, needs_read_data(Eq(captured)))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(needs_read));
  EXPECT_CALL(*new_task, needs_write_data(Eq(captured)))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(needs_write));
  EXPECT_CALL(*new_task, run())
    .Times(Exactly(1))
    .WillOnce(Invoke(std::forward<Lambda>(lambda)));
  detail::backend_runtime->register_task(std::move(new_task));
}

template <typename MockDep, typename Lambda, bool IsNice=false>
void register_read_only_capture(MockDep* captured, Lambda&& lambda) {
  register_one_dep_capture<MockDep, Lambda, true, false, IsNice>(captured, std::forward<Lambda>(lambda));
}

template <typename MockDep, typename Lambda, bool IsNice=false>
void register_write_only_capture(MockDep* captured, Lambda&& lambda) {
  register_one_dep_capture<MockDep, Lambda, false, true, IsNice>(captured, std::forward<Lambda>(lambda));
}

template <typename MockDep, typename Lambda, bool IsNice=false>
void register_read_write_capture(MockDep* captured, Lambda&& lambda) {
  register_one_dep_capture<MockDep, Lambda, true, true, IsNice>(captured, std::forward<Lambda>(lambda));
}

template <typename Lambda, bool IsNice=false>
void register_nodep_task(Lambda&& lambda) {
  typedef typename std::conditional<false, NiceMock<MockTask>, MockTask>::type task_t;

  auto new_task = std::make_unique<task_t>();
  EXPECT_CALL(*new_task, get_dependencies())
    .Times(AtLeast(1));
  EXPECT_CALL(*new_task, run())
    .Times(Exactly(1))
    .WillOnce(Invoke(std::forward<Lambda>(lambda)));
  detail::backend_runtime->register_task(std::move(new_task));
}

template <typename T, bool IsNice, bool ExpectNewAlloc, typename Version, typename... KeyParts>
std::shared_ptr<typename std::conditional<IsNice, NiceMock<MockDependencyHandle>,
    MockDependencyHandle>::type>
make_handle(
  Version v, KeyParts&&... kp
) {
  typedef typename std::conditional<IsNice, NiceMock<MockSerializationManager>,
      MockSerializationManager>::type ser_man_t;
  typedef typename std::conditional<IsNice, NiceMock<MockDependencyHandle>,
      MockDependencyHandle>::type handle_t;

  // Deleted in accompanying MockDependencyHandle shared pointer deleter
  auto ser_man = new ser_man_t;
  EXPECT_CALL(*ser_man, get_metadata_size(IsNull()))
    .Times(AtLeast(ExpectNewAlloc ? 1 : 0))
    .WillRepeatedly(Return(sizeof(T)));

  auto new_handle = std::shared_ptr<handle_t>(
    new handle_t(),
    [=](handle_t* to_delete){
      delete to_delete;
      delete ser_man;
    }
  );
  new_handle->get_key_return = detail::key_traits<MockDependencyHandle::key_t>::maker()(
      std::forward<KeyParts>(kp)...);
  new_handle->get_version_return = v;

  EXPECT_CALL(*new_handle, get_key())
    .Times(AtLeast(1));
  EXPECT_CALL(*new_handle, get_version())
    .Times(AtLeast(1));
  EXPECT_CALL(*new_handle, get_serialization_manager())
    .Times(AnyNumber())
    .WillRepeatedly(Return(ser_man));
  if (ExpectNewAlloc){
    // because this is a new allocation and will not be satisfied by
    // a predecessor, it MUST be writable at some point
    EXPECT_CALL(*new_handle, allow_writes())
      .Times(Exactly(1));
  }

  return new_handle;
}

template <typename T, bool IsNice, typename Version, typename... KeyParts>
std::shared_ptr<typename std::conditional<IsNice, NiceMock<MockDependencyHandle>,
    MockDependencyHandle>::type>
make_fetching_handle(
  Version v, KeyParts&&... kp
) {
  typedef typename std::conditional<IsNice, NiceMock<MockSerializationManager>,
      MockSerializationManager>::type ser_man_t;
  typedef typename std::conditional<IsNice, NiceMock<MockDependencyHandle>,
      MockDependencyHandle>::type handle_t;

  // Deleted in accompanying MockDependencyHandle shared pointer deleter
  auto ser_man = new ser_man_t;
  EXPECT_CALL(*ser_man, get_metadata_size(IsNull()))
    .Times(AtLeast(0))
    .WillRepeatedly(Return(sizeof(T)));

  auto new_handle = std::shared_ptr<handle_t>(
    new handle_t(),
    [=](handle_t* to_delete){
      delete to_delete;
      delete ser_man;
    }
  );
  new_handle->get_key_return = detail::key_traits<MockDependencyHandle::key_t>::maker()(
      std::forward<KeyParts>(kp)...);
  new_handle->get_version_return = v;
  new_handle->version_is_pending_return = true;

  EXPECT_CALL(*new_handle, get_key())
    .Times(AtLeast(1));
  EXPECT_CALL(*new_handle, allow_writes())
    .Times(Exactly(0));
  EXPECT_CALL(*new_handle, set_version(_))
    .Times(Exactly(1));
  EXPECT_CALL(*new_handle, get_serialization_manager())
    .Times(AnyNumber())
    .WillRepeatedly(Return(ser_man));

  return new_handle;
}

#endif
