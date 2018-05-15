/*
//@HEADER
// ************************************************************************
//
//                      test_any_convertible.cc
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include <darma/impl/meta/any_convertible.h>

void test_val_(int);
void test_const_val_(int const);
void test_lval_ref_(int&);
void test_const_lval_ref_(int const&);
void test_rval_ref_(int&&);
void test_const_rval_ref_(int const&&);

using test_val = decltype(test_val_);
using test_const_val = decltype(test_const_val_);
using test_lval_ref = decltype(test_lval_ref_);
using test_const_lval_ref = decltype(test_const_lval_ref_);
using test_rval_ref = decltype(test_rval_ref_);
using test_const_rval_ref = decltype(test_const_rval_ref_);

struct hard {
  hard() = delete;
  hard(hard const&) = delete;
  hard(hard&&) = delete;
};

// The val and const_val versions of this should never work, no matter what,
// because of the way the constructors are deleted, so we omit them here
void hard_lval_ref_(hard&);
void hard_const_lval_ref_(hard const&);
void hard_rval_ref_(hard&&);
void hard_const_rval_ref_(hard const &&);

using hard_lval_ref = decltype(hard_lval_ref_);
using hard_const_lval_ref = decltype(hard_const_lval_ref_);
using hard_rval_ref = decltype(hard_rval_ref_);
using hard_const_rval_ref = decltype(hard_const_rval_ref_);

struct nocopy {
  nocopy() = delete;
  nocopy(nocopy const&) = delete;
  nocopy(nocopy&&) = default;
};

void nocopy_val_(nocopy);
void nocopy_const_val_(nocopy const);
void nocopy_lval_ref_(nocopy&);
void nocopy_const_lval_ref_(nocopy const&);
void nocopy_rval_ref_(nocopy&&);
void nocopy_const_rval_ref_(nocopy const &&);

using nocopy_val = decltype(nocopy_val_);
using nocopy_const_val = decltype(nocopy_const_val_);
using nocopy_lval_ref = decltype(nocopy_lval_ref_);
using nocopy_const_lval_ref = decltype(nocopy_const_lval_ref_);
using nocopy_rval_ref = decltype(nocopy_rval_ref_);
using nocopy_const_rval_ref = decltype(nocopy_const_rval_ref_);

struct nomove {
  nomove() = delete;
  nomove(nomove const&) = delete;
  nomove(nomove&&) = default;
};

void nomove_val_(nomove);
void nomove_const_val_(nomove const);
void nomove_lval_ref_(nomove&);
void nomove_const_lval_ref_(nomove const&);
void nomove_rval_ref_(nomove&&);
void nomove_const_rval_ref_(nomove const &&);

using nomove_val = decltype(nomove_val_);
using nomove_const_val = decltype(nomove_const_val_);
using nomove_lval_ref = decltype(nomove_lval_ref_);
using nomove_const_lval_ref = decltype(nomove_const_lval_ref_);
using nomove_rval_ref = decltype(nomove_rval_ref_);
using nomove_const_rval_ref = decltype(nomove_const_rval_ref_);

using namespace darma::meta;

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="any_arg">

static_assert(is_callable_with_args<test_val, any_arg>::value,
  "any_arg failed with test_val");
static_assert(is_callable_with_args<test_const_val, any_arg>::value,
  "any_arg failed with test_const_val");
static_assert(is_callable_with_args<test_const_lval_ref, any_arg>::value,
  "any_arg failed with test_const_lval_ref");
static_assert(is_callable_with_args<test_lval_ref, any_arg>::value,
  "any_arg failed with test_lval_ref");
static_assert(is_callable_with_args<test_rval_ref, any_arg>::value,
  "any_arg failed with test_rval_ref");
static_assert(is_callable_with_args<test_const_rval_ref, any_arg>::value,
  "any_arg failed with test_const_rval_ref");

static_assert(is_callable_with_args<hard_const_lval_ref, any_arg>::value,
  "any_arg failed with hard_const_lval_ref");
static_assert(is_callable_with_args<hard_lval_ref, any_arg>::value,
  "any_arg failed with hard_lval_ref");
static_assert(is_callable_with_args<hard_rval_ref, any_arg>::value,
  "any_arg failed with hard_rval_ref");
static_assert(is_callable_with_args<hard_const_rval_ref, any_arg>::value,
  "any_arg failed with hard_const_rval_ref");

static_assert(is_callable_with_args<nocopy_val, any_arg>::value,
  "any_arg failed with nocopy_val");
static_assert(is_callable_with_args<nocopy_const_val, any_arg>::value,
  "any_arg failed with nocopy_const_val");
static_assert(is_callable_with_args<nocopy_const_lval_ref, any_arg>::value,
  "any_arg failed with nocopy_const_lval_ref");
static_assert(is_callable_with_args<nocopy_lval_ref, any_arg>::value,
  "any_arg failed with nocopy_lval_ref");
static_assert(is_callable_with_args<nocopy_rval_ref, any_arg>::value,
  "any_arg failed with nocopy_rval_ref");
static_assert(is_callable_with_args<nocopy_const_rval_ref, any_arg>::value,
  "any_arg failed with nocopy_const_rval_ref");

static_assert(is_callable_with_args<nomove_val, any_arg>::value,
  "any_arg failed with nomove_val");
static_assert(is_callable_with_args<nomove_const_val, any_arg>::value,
  "any_arg failed with nomove_const_val");
static_assert(is_callable_with_args<nomove_const_lval_ref, any_arg>::value,
  "any_arg failed with nomove_const_lval_ref");
static_assert(is_callable_with_args<nomove_lval_ref, any_arg>::value,
  "any_arg failed with nomove_lval_ref");
static_assert(is_callable_with_args<nomove_rval_ref, any_arg>::value,
  "any_arg failed with nomove_rval_ref");
static_assert(is_callable_with_args<nomove_const_rval_ref, any_arg>::value,
  "any_arg failed with nomove_const_rval_ref");

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

// TODO re-enable these once we have them working for ICC

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="ambiguous_if_by_value">

static_assert(not is_callable_with_args<test_val, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with test_val");
static_assert(not is_callable_with_args<test_const_val, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with test_const_val");
static_assert(is_callable_with_args<test_const_lval_ref, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with test_const_lval_ref");
static_assert(is_callable_with_args<test_lval_ref, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with test_lval_ref");
static_assert(is_callable_with_args<test_rval_ref, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with test_rval_ref");
static_assert(is_callable_with_args<test_const_rval_ref, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with test_const_rval_ref");

static_assert(is_callable_with_args<hard_const_lval_ref, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with hard_const_lval_ref");
static_assert(is_callable_with_args<hard_lval_ref, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with hard_lval_ref");
//static_assert(is_callable_with_args<hard_rval_ref, ambiguous_if_by_value>::value,
//  "ambiguous_if_by_value failed with hard_rval_ref");
//static_assert(is_callable_with_args<hard_const_rval_ref, ambiguous_if_by_value>::value,
//  "ambiguous_if_by_value failed with hard_const_rval_ref");

static_assert(not is_callable_with_args<nocopy_val, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with nocopy_val");
static_assert(not is_callable_with_args<nocopy_const_val, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with nocopy_const_val");
static_assert(is_callable_with_args<nocopy_const_lval_ref, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with nocopy_const_lval_ref");
static_assert(is_callable_with_args<nocopy_lval_ref, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with nocopy_lval_ref");
//static_assert(is_callable_with_args<nocopy_rval_ref, ambiguous_if_by_value>::value,
//  "ambiguous_if_by_value failed with nocopy_rval_ref");
//static_assert(is_callable_with_args<nocopy_const_rval_ref, ambiguous_if_by_value>::value,
//  "ambiguous_if_by_value failed with nocopy_const_rval_ref");

static_assert(not is_callable_with_args<nomove_val, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with nomove_val");
static_assert(not is_callable_with_args<nomove_const_val, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with nomove_const_val");
static_assert(is_callable_with_args<nomove_const_lval_ref, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with nomove_const_lval_ref");
static_assert(is_callable_with_args<nomove_lval_ref, ambiguous_if_by_value>::value,
  "ambiguous_if_by_value failed with nomove_lval_ref");
//static_assert(is_callable_with_args<nomove_rval_ref, ambiguous_if_by_value>::value,
//  "ambiguous_if_by_value failed with nomove_rval_ref");
//static_assert(is_callable_with_args<nomove_const_rval_ref, ambiguous_if_by_value>::value,
//  "ambiguous_if_by_value failed with nomove_const_rval_ref");

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="any_nonconst_rvalue_reference">

static_assert(not is_callable_with_args<test_val, any_nonconst_rvalue_reference>::value,
  "any_nonconst_rvalue_reference failed with test_val");
static_assert(not is_callable_with_args<test_const_val, any_nonconst_rvalue_reference>::value,
  "any_nonconst_rvalue_reference failed with test_const_val");
static_assert(not is_callable_with_args<test_const_lval_ref, any_nonconst_rvalue_reference>::value,
  "any_nonconst_rvalue_reference failed with test_const_lval_ref");
static_assert(not is_callable_with_args<test_lval_ref, any_nonconst_rvalue_reference>::value,
  "any_nonconst_rvalue_reference failed with test_lval_ref");
static_assert(is_callable_with_args<test_rval_ref, any_nonconst_rvalue_reference>::value,
  "any_nonconst_rvalue_reference failed with test_rval_ref");
static_assert(not is_callable_with_args<test_const_rval_ref, any_nonconst_rvalue_reference>::value,
  "any_nonconst_rvalue_reference failed with test_const_rval_ref");

//static_assert(not is_callable_with_args<hard_const_lval_ref, any_nonconst_rvalue_reference>::value,
//  "any_nonconst_rvalue_reference failed with hard_const_lval_ref");
static_assert(not is_callable_with_args<hard_lval_ref, any_nonconst_rvalue_reference>::value,
  "any_nonconst_rvalue_reference failed with hard_lval_ref");
//static_assert(is_callable_with_args<hard_rval_ref, any_nonconst_rvalue_reference>::value,
//  "any_nonconst_rvalue_reference failed with hard_rval_ref");
//static_assert(not is_callable_with_args<hard_const_rval_ref, any_nonconst_rvalue_reference>::value,
//  "any_nonconst_rvalue_reference failed with hard_const_rval_ref");

static_assert(not is_callable_with_args<nocopy_val, any_nonconst_rvalue_reference>::value,
  "any_nonconst_rvalue_reference failed with nocopy_val");
static_assert(not is_callable_with_args<nocopy_const_val, any_nonconst_rvalue_reference>::value,
  "any_nonconst_rvalue_reference failed with nocopy_const_val");
//static_assert(not is_callable_with_args<nocopy_const_lval_ref, any_nonconst_rvalue_reference>::value,
//  "any_nonconst_rvalue_reference failed with nocopy_const_lval_ref");
static_assert(not is_callable_with_args<nocopy_lval_ref, any_nonconst_rvalue_reference>::value,
  "any_nonconst_rvalue_reference failed with nocopy_lval_ref");
//static_assert(is_callable_with_args<nocopy_rval_ref, any_nonconst_rvalue_reference>::value,
//  "any_nonconst_rvalue_reference failed with nocopy_rval_ref");
//static_assert(not is_callable_with_args<nocopy_const_rval_ref, any_nonconst_rvalue_reference>::value,
//  "any_nonconst_rvalue_reference failed with nocopy_const_rval_ref");

static_assert(not is_callable_with_args<nomove_val, any_nonconst_rvalue_reference>::value,
  "any_nonconst_rvalue_reference failed with nomove_val");
static_assert(not is_callable_with_args<nomove_const_val, any_nonconst_rvalue_reference>::value,
  "any_nonconst_rvalue_reference failed with nomove_const_val");
//static_assert(not is_callable_with_args<nomove_const_lval_ref, any_nonconst_rvalue_reference>::value,
//  "any_nonconst_rvalue_reference failed with nomove_const_lval_ref");
static_assert(not is_callable_with_args<nomove_lval_ref, any_nonconst_rvalue_reference>::value,
  "any_nonconst_rvalue_reference failed with nomove_lval_ref");
//static_assert(is_callable_with_args<nomove_rval_ref, any_nonconst_rvalue_reference>::value,
//  "any_nonconst_rvalue_reference failed with nomove_rval_ref");
//static_assert(not is_callable_with_args<nomove_const_rval_ref, any_nonconst_rvalue_reference>::value,
//  "any_nonconst_rvalue_reference failed with nomove_const_rval_ref");

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

