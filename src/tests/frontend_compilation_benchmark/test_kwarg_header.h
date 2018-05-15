/*
//@HEADER
// ************************************************************************
//
//                      test_kwarg_header.h
//                         DARMA
//              Copyright (C) 2018 Sandia Corporation
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

#ifndef DARMA_TEST_KWARG_LAMBDA_H 
#define DARMA_TEST_KWARG_LAMBDA_H

#include <cstdlib> // size_t

#include <darma/impl/keyword_arguments/parse.h>
#include <darma/impl/capture.h>
#include <darma/impl/keyword_arguments/macros.h>

DeclareDarmaTypeTransparentKeyword(compilation, arg1);
DeclareDarmaTypeTransparentKeyword(compilation, arg2);
DeclareDarmaTypeTransparentKeyword(compilation, arg3);
DeclareDarmaTypeTransparentKeyword(compilation, arg4);
DeclareDarmaTypeTransparentKeyword(compilation, arg5);
DeclareDarmaTypeTransparentKeyword(compilation, arg6);
DeclareDarmaTypeTransparentKeyword(compilation, arg7);
DeclareDarmaTypeTransparentKeyword(compilation, arg8);

DeclareStandardDarmaKeywordArgumentAliases(compilation, arg1);
DeclareStandardDarmaKeywordArgumentAliases(compilation, arg2);
DeclareStandardDarmaKeywordArgumentAliases(compilation, arg3);
DeclareStandardDarmaKeywordArgumentAliases(compilation, arg4);
DeclareStandardDarmaKeywordArgumentAliases(compilation, arg5);
DeclareStandardDarmaKeywordArgumentAliases(compilation, arg6);
DeclareStandardDarmaKeywordArgumentAliases(compilation, arg7);
DeclareStandardDarmaKeywordArgumentAliases(compilation, arg8);

namespace darma {

namespace test_detail {

struct default_arg1_functor {
   explicit default_arg1_functor() {};
   unsigned long operator()() {return 1ul;}
};

struct default_arg2_functor {
   explicit default_arg2_functor() {};
   unsigned long operator()() {return 2ul;}
};

struct default_arg3_functor {
   explicit default_arg3_functor() {};
   unsigned long operator()() {return 3ul;}
};

struct default_arg4_functor {
   explicit default_arg4_functor() {};
   unsigned long operator()() {return 4ul;}
};

struct default_arg5_functor {
   explicit default_arg5_functor() {};
   unsigned long operator()() {return 5ul;}
};

struct default_arg6_functor {
   explicit default_arg6_functor() {};
   unsigned long operator()() {return 6ul;}
};

struct default_arg7_functor {
   explicit default_arg7_functor() {};
   unsigned long operator()() {return 7ul;}
};

struct default_arg8_functor {
   explicit default_arg8_functor() {};
   unsigned long operator()() {return 8ul;}
};

struct foo {
   explicit foo() {};
   void operator()(size_t arg1) {};
   void operator()(size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5, size_t arg6, size_t arg7, size_t arg8) {};
};

} // end namespace test_detail

// First test -- Lambda
template <typename... Args>
void 
foo_kwarg_lambda_test_one(Args&&... args) {

   using namespace darma::detail;
   using parser = detail::kwarg_parser<
      overload_description<
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg1>
         >
      >;

   using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<>;
 
   parser()
      .with_default_generators(
         keyword_arguments_for_compilation::arg1=[]{ return 1ul; }
      )
      .parse_args()
      .invoke(test_detail::foo{});
}

// Second test -- Lambda
template <typename... Args>
void
foo_kwarg_lambda_test_two(Args&&... args) {

   using namespace darma::detail;
   using parser = detail::kwarg_parser<
      overload_description<
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg1>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg2>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg3>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg4>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg5>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg6>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg7>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg8>
         >
      >;

   using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<>;

   parser()
      .with_default_generators(
         keyword_arguments_for_compilation::arg1=[]{ return 1ul; },
         keyword_arguments_for_compilation::arg2=[]{ return 2ul; },
         keyword_arguments_for_compilation::arg3=[]{ return 3ul; },
         keyword_arguments_for_compilation::arg4=[]{ return 4ul; },
         keyword_arguments_for_compilation::arg5=[]{ return 5ul; },
         keyword_arguments_for_compilation::arg6=[]{ return 6ul; },
         keyword_arguments_for_compilation::arg7=[]{ return 7ul; },
         keyword_arguments_for_compilation::arg8=[]{ return 8ul; }
      )
      .parse_args()
      .invoke(test_detail::foo{});
}

// First test -- Functor
template <typename... Args>
void
foo_kwarg_functor_test_one(Args&&... args) {

   using namespace darma::detail;
   using parser = detail::kwarg_parser<
      overload_description<
         _optional_keyword<std::size_t, keyword_tags_for_publication::n_readers>
         >
      >;

   using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<>;

   parser()
      .with_default_generators(
         keyword_arguments_for_publication::n_readers=test_detail::default_arg1_functor{}
      )
      .parse_args()
      .invoke(test_detail::foo{});
}

// Second test -- Functor
template <typename... Args>
void
foo_kwarg_functor_test_two(Args&&... args) {

   using namespace darma::detail;
   using parser = detail::kwarg_parser<
      overload_description<
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg1>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg2>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg3>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg4>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg5>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg6>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg7>,
         _optional_keyword<std::size_t, keyword_tags_for_compilation::arg8>
         >
      >;

   using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<>;

   parser()
      .with_default_generators(
         keyword_arguments_for_compilation::arg1=test_detail::default_arg1_functor{},
         keyword_arguments_for_compilation::arg2=test_detail::default_arg2_functor{},
         keyword_arguments_for_compilation::arg3=test_detail::default_arg3_functor{},
         keyword_arguments_for_compilation::arg4=test_detail::default_arg4_functor{},
         keyword_arguments_for_compilation::arg5=test_detail::default_arg5_functor{},
         keyword_arguments_for_compilation::arg6=test_detail::default_arg6_functor{},
         keyword_arguments_for_compilation::arg7=test_detail::default_arg7_functor{},
         keyword_arguments_for_compilation::arg8=test_detail::default_arg8_functor{}
      )
      .parse_args()
      .invoke(test_detail::foo{});
}

} // end namespace darma

#endif 
