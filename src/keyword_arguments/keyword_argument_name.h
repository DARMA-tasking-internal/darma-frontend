/*
//@HEADER
// ************************************************************************
//
//                          keyword_argument_name.h
//                         dharma_mockup
//              Copyright (C) 2015 Sandia Corporation
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

#ifndef KEYWORD_ARGUMENTS_KEYWORD_ARGUMENT_NAME_H_
#define KEYWORD_ARGUMENTS_KEYWORD_ARGUMENT_NAME_H_

#include <tuple>

#include "kwarg_expression_fwd.h"

namespace dharma_runtime { namespace detail {

template <typename T> struct extract_tag;

////////////////////////////////////////////////////////////////////////////////

/* keyword_argument_name                                                 {{{1 */ #if 1 // begin fold

/* TODO deprecate keyword_argument_name.  typeless_keyword_argument_name works much better and much more generally */
template <typename T, typename Tag>
class keyword_argument_name
{
  public:
    typedef Tag tag;
    typedef T value_t;
    typedef kwarg_expression<T, keyword_argument_name, false> kwarg_expression_t;
    typedef kwarg_expression<T, keyword_argument_name, true> lvalue_kwarg_expression_t;


    constexpr keyword_argument_name() { }

    kwarg_expression_t
    operator=(T&& val) const {
       return kwarg_expression_t(std::forward<T>(val));
    }

    lvalue_kwarg_expression_t
    operator=(T& val) const {
       return lvalue_kwarg_expression_t(val);
    }

};

template <class T, typename Tag>
struct extract_tag<keyword_argument_name<T, Tag>> {
  typedef Tag type;
};


/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* typeless_keyword_argument_name                                        {{{1 */ #if 1 // begin fold

template <typename Tag>
class typeless_keyword_argument_name
{
  private:

    template <typename Rhs>
    using kwarg_expr = typeless_kwarg_expression<Rhs, typeless_keyword_argument_name>;
    template <typename... Args>
    using multi_kwarg_expr = multiarg_typeless_kwarg_expression<
      typeless_keyword_argument_name,
      Args...
    >;

  public:

    typedef Tag tag;

    constexpr typeless_keyword_argument_name() { }

    template <typename Rhs>
    inline constexpr kwarg_expr<Rhs>
    operator=(Rhs&& val) const {
      return { std::forward<Rhs>(val) };
    }

    template <typename Rhs>
    inline constexpr kwarg_expr<Rhs>
    operator()(Rhs&& val) const {
      return { std::forward<Rhs>(val) };
    }

    template <typename RhsArg1, typename RhsArg2, typename... RhsArgs>
    inline constexpr multi_kwarg_expr<RhsArg1, RhsArg2, RhsArgs...>
    operator()(RhsArg1&& a1, RhsArg2&& a2, RhsArgs&&... args) const {
      return { std::forward<RhsArg1>(a1), std::forward<RhsArg2>(a2),
        std::forward<RhsArgs>(args)...
      };
    }
};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* keyword_argument_name                                                 {{{1 */ #if 1 // begin fold

template <class T>
struct is_keyword_argument_name
  : public std::false_type
{ };

template <class T, typename Tag>
struct is_keyword_argument_name<
  keyword_argument_name<T, Tag>
> : public std::true_type
{ };

template <typename Tag>
struct is_keyword_argument_name<
  typeless_keyword_argument_name<Tag>
> : public std::true_type
{ };

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* unnamed_tag                                                           {{{1 */ #if 1 // begin fold

template <typename T>
struct unnamed_tag : public keyword_tag
{
  typedef std::true_type is_tag_t;
  typedef T value_t;
};

template<typename T>
using unnamed_tag_name_t = keyword_argument_name<
   T, unnamed_tag<T>
>;

template <typename T>
struct tag_data<unnamed_tag<T>>
{
  typedef unnamed_tag_name_t<T> keyword_name_t;
  typedef T value_t;
  static constexpr bool has_default_value = false;                                                  \
};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* null_tag and null_argument_name                                       {{{1 */ #if 1 // begin fold

struct null_tag : public keyword_tag
{
  typedef std::true_type is_tag_t;
};

typedef keyword_argument_name<std::nullptr_t, null_tag> null_argument_name_t;

namespace {
  static constexpr null_argument_name_t null_argument_name;
} // end anonymous namespace used to avoid duplicate symbol errors

template<>
struct tag_data<null_argument_name_t>
{
  typedef null_argument_name_t keyword_name_t;
  typedef std::nullptr_t value_t;
  static constexpr bool has_default_value = false;
};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////

}} // end namespace dharma_runtime::detail

#endif /* KEYWORD_ARGUMENTS_KEYWORD_ARGUMENT_NAME_H_ */
