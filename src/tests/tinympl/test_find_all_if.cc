/*
//@HEADER
// ************************************************************************
//
//                          test_find_all_if.cc
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


#include <tinympl/find_all.hpp>
#include <tinympl/find_all_if.hpp>
#include <util/empty_main.h>

#include <type_traits>

#include <tinympl/vector.hpp>
#include <tinympl/string.hpp>

#include <darma/impl/util/static_assertions.h>

using namespace tinympl;

STATIC_ASSERT_TYPE_EQ(
  typename find_all<
    vector<float, int, int, float, double>,
    float
  >::type,
  std::integer_sequence<size_t, 0, 3>
);

STATIC_ASSERT_TYPE_EQ(
  typename find_all_if<
    vector<float, int, int, float, double>,
    std::is_integral
  >::type,
  std::integer_sequence<size_t, 1, 2>
);


STATIC_ASSERT_TYPE_EQ(
  typename find_all_if<
    vector<float>,
    std::is_integral
  >::type,
  std::integer_sequence<size_t>
);

STATIC_ASSERT_TYPE_EQ(
  typename find_all_if<
    vector<>,
    std::is_integral
  >::type,
  std::integer_sequence<size_t>
);

STATIC_ASSERT_TYPE_EQ(
  typename find_all_if<
    vector<int>,
    std::is_integral
  >::type,
  std::integer_sequence<size_t, 0>
);
