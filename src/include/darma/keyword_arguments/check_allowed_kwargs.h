/*
//@HEADER
// ************************************************************************
//
//                          check_allowed_kwargs.h
//                         dharma_new
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

#ifndef SRC_INCLUDE_DARMA_IMPL_KEYWORD_ARGUMENTS_CHECK_ALLOWED_KWARGS_H_
#define SRC_INCLUDE_DARMA_IMPL_KEYWORD_ARGUMENTS_CHECK_ALLOWED_KWARGS_H_


#include <darma/keyword_arguments/kwarg_expression.h>
#include <darma/keyword_arguments/errors.h>

#include <tinympl/all_of.hpp>
#include <tinympl/any_of.hpp>
#include <tinympl/lambda.hpp>
#include <tinympl/detection.hpp>

namespace darma {
namespace detail {

template <typename... KWTags>
struct only_allowed_kwargs_given {

  template <typename... Args>
  struct apply {
    private:

      template <typename Arg>
      using _allowed_arg = tinympl::variadic::any_of<
        tinympl::lambda<
          std::is_same<typename is_kwarg_expression<Arg>::tag, tinympl::placeholders::_>
        >::template apply_value,
        tinympl::nonesuch,
        KWTags...
      >;

    public:

      typedef typename std::conditional<
          sizeof...(Args) == 0,
          std::true_type,
          tinympl::variadic::all_of<_allowed_arg, Args...>
      >::type::type type;
  };

  template <typename KWTag>
  struct bad_keyword_argument_assert {
    _DARMA__error__::__________Unknown_keyword_argument_<KWTag> bad_keyword_argument;
  };

  template <typename... Args>
  struct static_assert_correct {
    template <typename KWArg>
    using _kwarg_not_found = tinympl::and_<
      is_kwarg_expression<KWArg>,
      tinympl::bool_<
        tinympl::variadic::find<
          typename is_kwarg_expression<KWArg>::tag, KWTags...
        >::value == sizeof...(KWTags)
      >
    >;

    template <typename Arg>
    using _bad_arg = bad_keyword_argument_assert<
      typename is_kwarg_expression<Arg>::tag
    >;

    template <typename Arg>
    using _good_arg = tinympl::identity<Arg>;

    template <typename Arg>
    using _kwarg_test = std::conditional_t<
      _kwarg_not_found<Arg>::value,
      _bad_arg<Arg>,
      _good_arg<Arg>
    >;

    template <typename... Tested>
    static int _do_test(Tested&&...) { };

    using type = decltype(static_assert_correct::_do_test(_kwarg_test<Args>()...));
  };

};


} // end namespace detail
} // end namespace darma


#endif /* SRC_INCLUDE_DARMA_IMPL_KEYWORD_ARGUMENTS_CHECK_ALLOWED_KWARGS_H_ */
