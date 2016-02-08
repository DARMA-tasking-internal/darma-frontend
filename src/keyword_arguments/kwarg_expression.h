/*
//@HEADER
// ************************************************************************
//
//                          kwarg_expression.h
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

#ifndef KEYWORD_ARGUMENTS_KWARG_EXPRESSION_H_
#define KEYWORD_ARGUMENTS_KWARG_EXPRESSION_H_

#include "keyword_argument_name.h"

namespace dharma_runtime { namespace detail {

////////////////////////////////////////////////////////////////////////////////

/* kwarg_expression                                                      {{{1 */ #if 1 // begin fold

template <class T>
struct is_kwarg_expression
  : public std::false_type
{ };

template <
  typename T,
  typename KWArgName,
  bool in_rhs_is_lvalue
>
class kwarg_expression
{
  public:
    typedef typename KWArgName::tag tag;
    typedef KWArgName name_t;
    typedef typename KWArgName::catagory_t catagory_t;
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
          sentinal_type_
        >::type = sentinal_type_()
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

template <typename T, typename KWArgName, bool rhs_is_lvalue>
struct is_kwarg_expression<kwarg_expression<T, KWArgName, rhs_is_lvalue>>
  : public std::true_type
{ };

template <class T, class Tag>
struct is_kwarg_expression_with_tag
  : public std::false_type
{ };

template <class T, typename Tag, typename Catagory, typename U, bool rhs_is_lvalue>
struct is_kwarg_expression_with_tag<
  kwarg_expression<
    T, keyword_argument_name<U, Tag, Catagory>, rhs_is_lvalue
  >,
  Tag
> : public std::true_type
{ };


template <class T>
struct is_tag
  : public std::is_base_of<detail::keyword_tag, T>::type { };

template <class T>
struct extract_tag {
  static_assert(is_tag<T>::value, "invalid keyword tag type");
  typedef T type;
};

template <typename T, typename Enable/*=void*/>
struct extract_type_if_tag {
  // If it's not a tag, then it's the type we want
  typedef T type;
};

template <typename T>
struct extract_type_if_tag<
  T, typename std::enable_if<is_tag<T>::value>::type
>
{
  typedef typename tag_data<T>::value_t type;
};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


}} // end namespace dharma_mockup::detail



#endif /* KEYWORD_ARGUMENTS_KWARG_EXPRESSION_H_ */
