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

#include "mock_frontend.h"

using namespace darma_runtime;
using namespace mock_frontend;

// some helper functions

template <typename MockDep, typename Lambda, bool needs_read, bool needs_write, bool IsNice=false>
void register_one_dep_capture(MockDep* captured, Lambda&& lambda) {
  using namespace ::testing;
  typedef typename std::conditional<IsNice, ::testing::NiceMock<MockTask>, MockTask>::type task_t;

  auto task_a = std::make_unique<task_t>();
  EXPECT_CALL(*task_a, get_dependencies())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(MockTask::handle_container_t{ captured }));
  EXPECT_CALL(*task_a, needs_read_data(Eq(captured)))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(needs_read));
  EXPECT_CALL(*task_a, needs_write_data(Eq(captured)))
    .Times(AtLeast(1))
    .WillRepeatedly(Return(needs_write));
  EXPECT_CALL(*task_a, run())
    .Times(Exactly(1))
    .WillOnce(Invoke(std::forward<Lambda>(lambda)));
  detail::backend_runtime->register_task(std::move(task_a));
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

template <typename T, bool IsNice, bool ExpectNewAlloc, typename Version, typename... KeyParts>
std::shared_ptr<typename std::conditional<IsNice, ::testing::NiceMock<MockDependencyHandle>, MockDependencyHandle>::type>
make_handle(
  Version v, KeyParts&&... kp
) {
  using namespace ::testing;
  typedef typename std::conditional<IsNice, ::testing::NiceMock<MockDependencyHandle>, MockDependencyHandle>::type handle_t;

  // Deleted in accompanying MockDependencyHandle shared pointer deleter
  auto ser_man = new MockSerializationManager();
  ser_man->get_metadata_size_return = sizeof(T);
  if (ExpectNewAlloc){
    EXPECT_CALL(*ser_man, get_metadata_size(IsNull()))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(sizeof(T)));
  }

  auto h_0 = std::shared_ptr<handle_t>(
    new handle_t(),
    [=](handle_t* to_delete){
      delete to_delete;
      delete ser_man;
    }
  );
  EXPECT_CALL(*h_0, get_key())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(detail::key_traits<MockDependencyHandle::key_t>::maker()(std::forward<KeyParts>(kp)...)));
  EXPECT_CALL(*h_0, get_version())
    .Times(AtLeast(1))
    .WillRepeatedly(ReturnRefOfCopy(v));
  if (ExpectNewAlloc){
    EXPECT_CALL(*h_0, get_serialization_manager())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(ser_man));
    EXPECT_CALL(*h_0, allow_writes())
      .Times(Exactly(1));
  }

  return h_0;
}

#endif
