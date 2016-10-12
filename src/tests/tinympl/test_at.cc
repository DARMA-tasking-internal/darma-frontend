/*
//@HEADER
// ************************************************************************
//
//                      test_at.cc
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

#include <type_traits> // std::integer_sequence

#include <tinympl/at.hpp>
#include <tinympl/vector.hpp>
#include <tinympl/stl_integer_sequence.hpp>
#include <tinympl/detection.hpp> // tinympl::nonesuch
#include <darma/impl/util/static_assertions.h>

#include <util/empty_main.h>

using namespace tinympl;

STATIC_ASSERT_TYPE_EQ(
  at_t<3, vector<char[0], char[1], char[2], char[3]>>,
  char[3]
);

STATIC_ASSERT_TYPE_EQ(
  at_t<0, vector<char[0], char[1], char[2], char[3]>>,
  char[0]
);

STATIC_ASSERT_TYPE_EQ(
  at_t<1, vector<char[0], char[1], char[2], char[3]>>,
  char[1]
);

STATIC_ASSERT_TYPE_EQ(
  at_t<2, std::make_index_sequence<10>>,
  std::integral_constant<size_t, 2>
);

STATIC_ASSERT_TYPE_EQ(
  variadic::at_t<1, char[0], char[1], char[2], char[3]>,
  char[1]
);

STATIC_ASSERT_TYPE_EQ(
  at_t<0, vector<nonesuch, nonesuch&&, nonesuch&, nonesuch const volatile&>>,
  nonesuch
);

STATIC_ASSERT_TYPE_EQ(
  at_t<1, vector<nonesuch, nonesuch&&, nonesuch&, nonesuch const volatile&>>,
  nonesuch&&
);

STATIC_ASSERT_TYPE_EQ(
  at_t<2, vector<nonesuch, nonesuch&&, nonesuch&, nonesuch const volatile&>>,
  nonesuch&
);

STATIC_ASSERT_TYPE_EQ(
  at_t<3, vector<nonesuch, nonesuch&&, nonesuch&, nonesuch const volatile&>>,
  nonesuch const volatile&
);

STATIC_ASSERT_TYPE_EQ(
  at_t<0, vector<void (nonesuch::*)(nonesuch&&, nonesuch&)>>,
  void (nonesuch::*)(nonesuch&&, nonesuch&)
);

STATIC_ASSERT_TYPE_EQ(
  at_or_t<int, 3, vector<void (nonesuch::*)(nonesuch&&, nonesuch&)>>,
  int
);

STATIC_ASSERT_TYPE_EQ(
  variadic::at_or_t<double, 3, void (nonesuch::*)(nonesuch&&, nonesuch&), int, nonesuch>,
  double
);
