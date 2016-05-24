/*
//@HEADER
// ************************************************************************
//
//                          test_member_detector.cc
//                         darma_new
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

#include <darma/impl/meta/member_detector.h>
#include <string>

//using namespace darma_runtime;
//using namespace darma_runtime::meta;
//
//
//struct TestHasPolicy {
//  int policy(int, char, std::string&) { return 0; }
//};
//
//struct TestHasOtherPolicy {
//  void*& policy(int) { return my_ptr; }
//  void* my_ptr;
//};
//
//struct TestHasConstPolicy {
//  int policy(int, char, std::string&) const { return 0; }
//};
//
//struct TestHasBothPolicy {
//  int policy(int, char, std::string&) { return 0; }
//  int policy(int, char, std::string&) const { return 0; }
//};
//
//struct TestNoPolicy { };
//
//struct TestMemberPolicy { int policy; };
//
//struct TestStaticPolicy {
//  static int policy(int, char, std::string&) { return 0; }
//};
//
//struct TestWrongPolicy {
//  int policy(char, int, std::string&) { return 0; }
//  int policy() { return 0; }
//  void policy(int, char, std::string&) { }
//};
//
//DARMA_META_MAKE_MEMBER_DETECTORS(policy);
//
//static_assert(
//  has_member_named_policy<TestHasPolicy>::value,
//  "doesn't work"
//);
//
//static_assert(
//  has_method_named_policy_with_signature<
//    TestHasPolicy, int(int, char, std::string&)
//  >::value,
//  "doesn't work"
//);
//
//static_assert(
//  not has_const_method_named_policy_with_signature<
//    TestHasPolicy, int(int, char, std::string&)
//  >::value,
//  "doesn't work"
//);
//
//static_assert(
//  has_const_method_named_policy_with_signature<
//    TestHasConstPolicy, int(int, char, std::string&)
//  >::value,
//  "doesn't work"
//);
//
//static_assert(
//  not has_method_named_policy_with_signature<
//    TestHasConstPolicy, int(int, char, std::string&)
//  >::value,
//  "doesn't work"
//);
//
//static_assert(
//  has_method_named_policy_with_signature<
//    TestHasBothPolicy, int(int, char, std::string&)
//  >::value and
//  has_const_method_named_policy_with_signature<
//    TestHasBothPolicy, int(int, char, std::string&)
//  >::value,
//  "doesn't work"
//);
//
//static_assert(
//  not has_method_named_policy_with_signature<TestNoPolicy, int(int, char, std::string&)>::value,
//  "doesn't work"
//);
//
//static_assert(
//  has_method_named_policy_with_signature<TestHasOtherPolicy,
//    void*&(int)
//  >::value,
//  "doesn't work"
//);
//
//static_assert(
//  not has_method_named_policy_with_signature<TestWrongPolicy, int(int, char, std::string&)>::value
//  and not has_const_method_named_policy_with_signature<TestWrongPolicy, int(int, char, std::string&)>::value,
//  "doesn't work"
//);
//
//static_assert(
//  not has_method_named_policy_with_signature<
//    int, int(int, char, std::string&)
//  >::value,
//  "doesn't work"
//);
//
//static_assert(
//  not has_method_named_policy_with_signature<
//    TestStaticPolicy, int(int, char, std::string&)
//  >::value,
//  "doesn't work"
//);
//
//static_assert(
//  not has_method_named_policy<TestMemberPolicy>::value,
//  "doesn't work"
//);
//
//static_assert(
//  has_method_named_policy<TestHasPolicy>::value,
//  "doesn't work"
//);
//
////static_assert(
////  not has_static_method_named_policy_with_signature<
////    TestHasPolicy,
////    int(int, char, std::string&)
////  >::value,
////  "doesn't work"
////);
//
//static_assert(
//  has_member_named_policy<
//    TestStaticPolicy
//  >::value,
//  "doesn't work"
//);
//
//static_assert(
//  has_method_named_policy<
//    TestStaticPolicy
//  >::value,
//  "doesn't work"
//);
//
//static_assert(
//  not has_instance_method_named_policy<
//    TestStaticPolicy
//  >::value,
//  "doesn't work"
//);
//
//static_assert(
//  has_static_member_named_policy<
//    TestStaticPolicy
//  >::value,
//  "doesn't work"
//);


#include "../util/empty_main.h"

//static_assert(
//  has_method_named_policy_with_signature<
//    TestStaticPolicy,
//    int(int, char, std::string&)
//  >::value,
//  "doesn't work"
//);

//static_assert(
//  has_static_method_named_policy_with_signature<
//    TestStaticPolicy,
//    int(int, char, std::string&)
//  >::value,
//  "doesn't work"
//);

// NOTE THIS LIMITATION!!!
//static_assert(
//  has_method_named_policy<TestHasBothPolicy>::value,
//  "doesn't work"
//);
