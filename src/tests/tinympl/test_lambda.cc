/*
//@HEADER
// ************************************************************************
//
//                          test_lambda.hpp
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


#include <tinympl/lambda.hpp>

#include <util/empty_main.h>

#include "metatest_helpers.h"

#include <tinympl/vector.hpp>
#include <tinympl/delay.hpp>

#include <string>
#include <vector>
#include <type_traits>
#include <map>
#include <queue>

using namespace tinympl;
using namespace tinympl::placeholders;

meta_assert(
  std::is_same<
    typename lambda<std::vector<_>>::template apply<int>::type,
    std::vector<int>
  >::value
);

meta_assert(
  std::is_same<
    typename lambda<
      vector<
        std::vector<_2>,
        _1
      >
    >::template apply<int, double>::type,
    vector<std::vector<double>, int>
  >::value
);

meta_assert(
  std::is_same<
    typename lambda<
      std::map<_1, _2>
    >::template apply<int, double>::type,
    std::map<int, double>
  >::value
);


//meta_assert(
//  undelay<
//    lambda<delay<std::is_same, std::decay<placeholders::_>, identity<int>>>::template apply
//  >::template apply<int const volatile&>::type::value
//);

meta_assert(
  lambda<std::is_same<std::decay<placeholders::_>, int>>::template apply<int const volatile&>::type::value
);
