/*
//@HEADER
// ************************************************************************
//
//                      test_kwargs.cc.cc
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


#include <darma/impl/keyword_arguments/get_kwarg.h>
#include <darma/impl/meta/splat_tuple.h>

#include "blabbermouth.h"

typedef EnableCTorBlabbermouth<String, Copy, Move> BlabberMouth;
static MockBlabbermouthListener* listener;

using namespace darma_runtime;
using namespace darma_runtime::detail;
using namespace darma_runtime::meta;

////////////////////////////////////////////////////////////////////////////////

class TestKeywordArguments
  : public ::testing::Test
{
  protected:

    virtual void SetUp() {
      using namespace ::testing;
      listener_ptr = std::make_unique<StrictMock<MockBlabbermouthListener>>();
      BlabberMouth::listener = listener_ptr.get();
      listener = BlabberMouth::listener;
    }

    virtual void TearDown() {
      listener_ptr.reset();
    }

    std::unique_ptr<::testing::StrictMock<MockBlabbermouthListener>> listener_ptr;

};

////////////////////////////////////////////////////////////////////////////////

void _test(BlabberMouth& val) { std::cout << val.data << std::endl; }
void _test(BlabberMouth&& val) { std::cout << val.data << std::endl; }

void _test_lvalue_only(BlabberMouth& val) { std::cout << val.data << std::endl; }
void _test_rvalue_only(BlabberMouth&& val) { std::cout << val.data << std::endl; }

template <typename... Args>
void test_lvalue_only(Args&&... args) {
  meta::splat_tuple(
    get_positional_arg_tuple(std::forward<Args>(args)...), _test_lvalue_only
  );
}

template <typename... Args>
void test_rvalue_only(Args&&... args) {
  meta::splat_tuple(
    get_positional_arg_tuple(std::forward<Args>(args)...), _test_rvalue_only
  );
}

TEST_F(TestKeywordArguments, rvalue_only) {
  EXPECT_CALL(*listener, string_ctor()).Times(1);
  testing::internal::CaptureStdout();

  test_rvalue_only(BlabberMouth("hello"));

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "hello\n"
  );
}

TEST_F(TestKeywordArguments, lvalue_only) {
  EXPECT_CALL(*listener, string_ctor()).Times(1);
  testing::internal::CaptureStdout();

  BlabberMouth b("hello");
  test_lvalue_only(b);

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "hello\n"
  );
}
