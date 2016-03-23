/*
//@HEADER
// ************************************************************************
//
//                          kwarg_expression.h
//                         darma_mockup
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

#ifndef KEYWORD_ARGUMENTS_KWARG_EXPRESSION_H_
#define KEYWORD_ARGUMENTS_KWARG_EXPRESSION_H_

#include <tinympl/string.hpp>
#include <tinympl/range_c.hpp>

#include "keyword_argument_name.h"
#include "../meta/sentinal_type.h"
#include "../meta/move_if.h"
#include "../meta/detection.h"

#include "kwarg_expression_fwd.h"

namespace darma_runtime { namespace detail {

////////////////////////////////////////////////////////////////////////////////

/* kwarg_expression                                                      {{{1 */ #if 1 // begin fold

/* TODO deprecate kwarg_expression.  typeless_kwarg_expression works much better and much more generally */
template <
  typename T, typename KWArgName, bool in_rhs_is_lvalue
>
class kwarg_expression
{
  public:
    typedef typename KWArgName::tag tag;
    typedef KWArgName name_t;
    typedef T value_t;

    static constexpr bool rhs_is_lvalue = in_rhs_is_lvalue;

    typedef typename std::conditional<rhs_is_lvalue,
        T&, T
    >::type actual_value_t;

    kwarg_expression() = delete;

    kwarg_expression(actual_value_t&& val) : val_(
        meta::move_if<not rhs_is_lvalue, T>()(
            std::forward<actual_value_t>(val)
        )
    ) {  }

    kwarg_expression(kwarg_expression&& other)
      : val_(meta::move_if<not rhs_is_lvalue, T>()(
          std::forward<actual_value_t>(other.val_))
        )
    { }

    template <typename U>
    explicit
    kwarg_expression(
        U&& val,
        typename std::enable_if<
          not std::is_base_of<T, U>::value
          and not is_kwarg_expression<
            typename std::decay<U>::type
          >::value,
          meta::sentinal_type
        >::type = meta::sentinal_type()
    ) : val_(std::forward<U>(val))
    { }

    actual_value_t value() & {
      return meta::move_if<not rhs_is_lvalue, T>()(
          std::forward<actual_value_t>(val_)
      );
    }

    actual_value_t value() && {
      return meta::move_if<not rhs_is_lvalue, T>()(
          std::forward<actual_value_t>(val_)
      );
    }

  private:
    actual_value_t val_;
};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* typeless_kwarg_expression                                             {{{1 */ #if 1 // begin fold

template <
  typename Rhs, typename KWArgName
>
class typeless_kwarg_expression
{
  public:
    typedef typename KWArgName::tag tag;
    typedef KWArgName name_t;

    constexpr
    typeless_kwarg_expression(Rhs&& rhs)
      : rhs_(std::forward<Rhs>(rhs))
    { }

    inline constexpr Rhs&&
    value() const {
      return std::forward<Rhs>(rhs_);
    }

    template <typename T>
    inline constexpr T
    value_as() const {
      return std::forward<Rhs>(rhs_);
    }

  private:

    Rhs&& rhs_;
};


namespace m = tinympl;

template <
  typename KWArgName, typename... Args
>
class multiarg_typeless_kwarg_expression {
  public:
    typedef typename KWArgName::tag tag;
    typedef KWArgName name_t;

    constexpr
    multiarg_typeless_kwarg_expression(Args&&... rhs_args)
      : rhs_args_(std::forward<Args>(rhs_args)...)
    { }

    template <typename T>
    inline constexpr T
    value_as() const {
      return value_as<T>(
        m::range_c<size_t, 0, sizeof...(Args)>()
      );
    }

  private:

    std::tuple<Args&&...> rhs_args_;

    template <typename T, size_t... Idxs>
    inline constexpr T
    value_as(tinympl::basic_string<size_t, Idxs...>) const {
      return { std::get<Idxs>(std::forward<std::tuple<Args&&...>>(rhs_args_))... };
    }

    template <typename Converter, size_t... Idxs>
    decltype(
      std::declval<Converter>()(
        std::get<Idxs>(std::declval<std::tuple<Args&&...>>())...
      )
    )
    value_converted(Converter&& conv, m::basic_string<size_t, Idxs...>) const {
      return conv(std::get<Idxs>(rhs_args_)...);
    }

  public:

    template <typename Converter>
    inline constexpr auto
    value_converted(Converter&& conv) const
      -> decltype(value_converted(
           std::forward<Converter>(conv), m::range_c<size_t, 0, sizeof...(Args)>())
         )
    {
      return value_converted(
        std::forward<Converter>(conv),
        m::range_c<size_t, 0, sizeof...(Args)>()
      );
    }
};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* is_kwarg_expression and is_kwarg_expression_with_tag                  {{{1 */ #if 1 // begin fold

template <class T>
struct is_kwarg_expression
  : public std::false_type
{
  typedef meta::nonesuch tag;
};

template <typename T, typename KWArgName, bool rhs_is_lvalue>
struct is_kwarg_expression<kwarg_expression<T, KWArgName, rhs_is_lvalue>>
  : public std::true_type
{
  typedef typename KWArgName::tag tag;
};

template <typename T, typename KWArgName>
struct is_kwarg_expression<typeless_kwarg_expression<T, KWArgName>>
  : public std::true_type
{
  typedef typename KWArgName::tag tag;
};

template <typename KWArgName, typename... Args>
struct is_kwarg_expression<multiarg_typeless_kwarg_expression<KWArgName, Args...>>
  : public std::true_type
{
  typedef typename KWArgName::tag tag;
};

template <class T, class Tag>
struct is_kwarg_expression_with_tag
  : public std::false_type
{ };

template <class T, typename Tag, typename U, bool rhs_is_lvalue>
struct is_kwarg_expression_with_tag<
  kwarg_expression<T, keyword_argument_name<U, Tag>, rhs_is_lvalue>,
  Tag
> : public std::true_type
{ };

template <class T, typename Tag, bool rhs_is_lvalue>
struct is_kwarg_expression_with_tag<
  kwarg_expression<T, typeless_keyword_argument_name<Tag>, rhs_is_lvalue>,
  Tag
> : public std::true_type
{ };

template <class T, typename Tag>
struct is_kwarg_expression_with_tag<
  typeless_kwarg_expression<T, typeless_keyword_argument_name<Tag>>,
  Tag
> : public std::true_type
{ };

template <typename Tag, class... Ts>
struct is_kwarg_expression_with_tag<
  multiarg_typeless_kwarg_expression<typeless_keyword_argument_name<Tag>, Ts...>,
  Tag
> : public std::true_type
{ };


/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////

}} // end namespace darma_mockup::detail



#endif /* KEYWORD_ARGUMENTS_KWARG_EXPRESSION_H_ */
