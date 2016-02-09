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

#include "kwarg_expression_fwd.h"

namespace dharma_runtime { namespace detail {

template <typename T> struct extract_tag;

////////////////////////////////////////////////////////////////////////////////

/* keyword_argument_name                                                 {{{1 */ #if 1 // begin fold


template <typename T, typename Tag, typename Catagory>
class keyword_argument_name
{
  public:
    typedef Tag tag;
    typedef T value_t;
    typedef Catagory catagory_t;
    typedef kwarg_expression<T, keyword_argument_name, false> kwarg_expression_t;
    typedef kwarg_expression<T, keyword_argument_name, true> lvalue_kwarg_expression_t;

    //template <typename... KWArgs>
    //using kwarg_key_expression_t = kwarg_expression<key_expression<KWArgs...>,
    //    keyword_argument_name, false>;

    constexpr keyword_argument_name()
    { }

    kwarg_expression_t
    operator=(T&& val) const {
       return kwarg_expression_t(std::forward<T>(val));
    }

    lvalue_kwarg_expression_t
    operator=(T& val) const {
       return lvalue_kwarg_expression_t(val);
    }

    //template <typename U>
    //typename std::enable_if<
    //  not std::is_base_of<T, U>::value
    //  and not std::is_same<Key, typename std::decay<U>::type>::value
    //  and not m::is_instantiation_of<key_expression, typename std::decay<U>::type>::value,
    //  kwarg_expression_t
    //>::type
    //operator=(U&& val) const {
    //   return kwarg_expression_t(std::forward<U>(val));
    //}

    //template <typename U>
    //typename std::enable_if<
    //  std::is_base_of<Key, typename std::decay<U>::type>::value,
    //  kwarg_key_expression_t<Key>
    //>::type
    //operator=(U&& key) const {
    //   return { std::forward<Key>(key) };
    //}

    //template <typename KWArg1, typename... KWArgs>
    //kwarg_key_expression_t<KWArg1, KWArgs...>
    //operator=(
    //  key_expression<KWArg1, KWArgs...>&& val
    //) const
    //{
    //  return { std::forward<key_expression<KWArg1, KWArgs...>>(val) };
    //}


    //template <typename... KWArgs>
    //kwarg_key_expression_t<KWArgs...>
    //operator()(
    //  KWArgs&&... kwargs
    //) const
    //{
    //  return {
    //    key_expression<KWArgs...>(
    //      std::forward<KWArgs>(kwargs)...
    //    )
    //  };
    //}


};

namespace m = tinympl;
namespace mv = tinympl::variadic;

template <class T, typename Tag, typename Catagory>
struct extract_tag<keyword_argument_name<T, Tag, Catagory>> {
  typedef Tag type;
};

template <class T>
struct is_keyword_argument_name
  : public std::false_type
{ };

template <class T, typename Tag, typename Catagory>
struct is_keyword_argument_name<
  keyword_argument_name<T, Tag, Catagory>
> : public std::true_type
{ };

struct null_tag : public keyword_tag
{
  typedef std::true_type is_tag_t;
};

template <typename T, typename Catagory>
struct unnamed_tag : public keyword_tag
{
  typedef std::true_type is_tag_t;
  typedef T value_t;
};

template<typename T, typename Catagory>
using unnamed_tag_name_t = keyword_argument_name<
   T, unnamed_tag<T, Catagory>, Catagory
>;

template <typename T, typename Catagory>
struct tag_data<unnamed_tag<T, Catagory>>
{
  typedef unnamed_tag_name_t<T, Catagory> keyword_name_t;
  typedef T value_t;
  typedef Catagory keyword_catagory_t;
  static constexpr bool has_default_value = false;                                                  \
};

typedef keyword_argument_name<
  std::nullptr_t, null_tag, keyword_catagory_argument
> null_argument_name_t;
namespace {
static constexpr null_argument_name_t null_argument_name;
} // end anonymous namespace used to avoid duplicate symbol errors

template<>
struct tag_data<null_argument_name_t>
{
  typedef null_argument_name_t keyword_name_t;
  typedef std::nullptr_t value_t;
  typedef keyword_catagory_argument keyword_catagory_t;
  static constexpr bool has_default_value = false;
};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


}} // end namespace dharma_mockup::detail


#endif /* KEYWORD_ARGUMENTS_KEYWORD_ARGUMENT_NAME_H_ */
