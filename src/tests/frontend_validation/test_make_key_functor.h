/*
//@HEADER
// ************************************************************************
//
//                          test_make_key_functor.h
//                         darma_new
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef SRC_TESTS_FRONTEND_VALIDATION_TEST_MAKE_KEY_FUNCTOR_H_
#define SRC_TESTS_FRONTEND_VALIDATION_TEST_MAKE_KEY_FUNCTOR_H_

#include <darma/impl/make_key_functor.h>
#include <darma/impl/keyword_arguments/parse.h>

namespace darma_runtime {

template <typename... Args> 
auto
check_test_make_key_functor(
  Args... args
) {

  using namespace darma_runtime::detail;
  using parser = detail::kwarg_parser<
    overload_description<
      _keyword<converted_parameter, keyword_tags_for_publication::version>
    >
  >;

  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

  return parser()
         .with_converters(_make_key_functor{})
         .parse_args(std::forward<Args>(args)...)
         .invoke([](types::key_t version){return version;});
 
}

} // End namespace darma_runtime

#endif
