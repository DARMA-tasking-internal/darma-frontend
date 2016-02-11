/*
//@HEADER
// ************************************************************************
//
//                          get_kwarg.h
//                         dharma_new
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

#ifndef SRC_KEYWORD_ARGUMENTS_GET_KWARG_H_
#define SRC_KEYWORD_ARGUMENTS_GET_KWARG_H_

#include <tuple>

#include <tinympl/find_if.hpp>
#include <tinympl/variadic/find_all_if.hpp>
#include <tinympl/lambda.hpp>
#include <tinympl/bind.hpp>
#include <tinympl/size.hpp>
#include <tinympl/vector.hpp>
#include <tinympl/string.hpp>
#include <tinympl/identity.hpp>


#include "../meta/sentinal_type.h"

#include "kwarg_expression.h"
#include "keyword_tag.h"

// TODO validate arg/kwarg expression in terms of rules
// TODO errors on unknown/unused keyword arguments
// TODO extract arguments given as either positional or keyword

namespace dharma_runtime { namespace detail {

namespace m = tinympl;
namespace mv = tinympl::variadic;
namespace mp = tinympl::placeholders;

namespace _get_kwarg_impl {

template <typename Tag, typename... Args>
using var_tag_spot = mv::find_if<
  m::lambda<is_kwarg_expression_with_tag<mp::_, Tag>>::template apply,
  Args...
>;

template <typename Tag, typename Seq>
using seq_tag_spot = m::find_if<
  Seq, m::lambda<is_kwarg_expression_with_tag<mp::_, Tag>>::template apply
>;

} // end namespace _get_kwarg_impl

////////////////////////////////////////////////////////////////////////////////

/* get_typed_kwarg                                                       {{{1 */ #if 1 // begin fold

template <typename Tag>
struct get_typed_kwarg {
  static_assert(tag_data<Tag>::has_type, "Can't get_typed_kwarg from untyped tag");

  typedef typename tag_data<Tag>::value_t return_t;

  template <typename... Args>
  return_t
  operator()(Args&&... args) {
    static constexpr size_t spot = _get_kwarg_impl::var_tag_spot<Tag, Args...>::value;
    // TODO error message readability and compile-time debugability
    static_assert(spot < sizeof...(Args), "missing required keyword argument");
    return std::get<spot>(std::forward_as_tuple(args...)).value();
  }

};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* get_typed_kwarg_with_default                                          {{{1 */ #if 1 // begin fold

namespace _get_kwarg_impl {

template <
  typename Tag, typename ArgsTuple, typename ForwardedArgsTuple, typename ReturnType,
  typename Enable=void /* => kwarg found */
>
struct _default_select_impl {
  static constexpr size_t spot = seq_tag_spot<Tag, ArgsTuple>::value;
  template <typename ReturnTypeCastable>
  ReturnType
  operator()(ReturnTypeCastable&& default_val, ForwardedArgsTuple&& tup) const {
    return std::get<spot>(std::forward<ForwardedArgsTuple>(tup)).value();
  }
};

template <typename Tag, typename ArgsTuple, typename ForwardedArgsTuple, typename ReturnType>
struct _default_select_impl<
  Tag, ArgsTuple, ForwardedArgsTuple, ReturnType,
  std::enable_if_t<seq_tag_spot<Tag, ArgsTuple>::value == m::size<ArgsTuple>::value>
> {
  template <typename ReturnTypeCastable>
  ReturnType
  operator()(ReturnTypeCastable&& default_val, ForwardedArgsTuple&& tup) const {
    return default_val;
  }
};

} // end namespace _get_kwarg_impl

template <typename Tag>
struct get_typed_kwarg_with_default {
  static_assert(tag_data<Tag>::has_type, "Can't get_typed_kwarg from untyped tag");

  typedef typename tag_data<Tag>::value_t return_t;

  template <typename ReturnTCastable, typename... Args>
  inline constexpr return_t
  operator()(ReturnTCastable&& default_val, Args&&... args) const {
    return _get_kwarg_impl::_default_select_impl<Tag,
        std::tuple<Args...>,
        decltype(std::forward_as_tuple(args...)), return_t
    >()(std::forward<ReturnTCastable>(default_val), std::forward_as_tuple(args...));
  }

};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* get_typeless_kwarg_as                                                 {{{1 */ #if 1 // begin fold

namespace _get_kwarg_impl {

template <typename Tag, typename AsType>
struct _typeless_kwarg_as_getter {
  template <typename... Args>
  inline constexpr AsType
  operator()(Args&&... args) const {
    constexpr size_t spot = _get_kwarg_impl::var_tag_spot<Tag, Args...>::value;
    // TODO error message readability and compile-time debugability
    static_assert(spot < sizeof...(Args), "missing required keyword argument");
    return std::get<spot>(std::forward_as_tuple(args...)).template value_as<AsType>();
  }
};

} // end namespace _get_kwarg_impl

template <typename Tag, typename AsType, typename... Args>
inline constexpr AsType
get_typeless_kwarg_as(
  Args&&... args
) {
  return _get_kwarg_impl::_typeless_kwarg_as_getter<Tag, AsType>()(
    std::forward<Args>(args)...
  );
}

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* get_typeless_kwarg_with_default_as                                    {{{1 */ #if 1 // begin fold

namespace _get_kwarg_impl {

template <
  typename Tag, typename ArgsTuple, typename ForwardedArgsTuple, typename ReturnType,
  typename Enable=void /* => kwarg found */
>
struct _typeless_default_select_impl {
  static constexpr size_t spot = seq_tag_spot<Tag, ArgsTuple>::value;
  template <typename ReturnTypeCastable>
  inline constexpr ReturnType
  operator()(ReturnTypeCastable&& default_val, ForwardedArgsTuple&& tup) const {
    return std::get<spot>(std::forward<ForwardedArgsTuple>(tup)).template value_as<ReturnType>();
  }
};

template <typename Tag, typename ArgsTuple, typename ForwardedArgsTuple, typename ReturnType>
struct _typeless_default_select_impl<
  Tag, ArgsTuple, ForwardedArgsTuple, ReturnType,
  std::enable_if_t<seq_tag_spot<Tag, ArgsTuple>::value == m::size<ArgsTuple>::value>
> {
  template <typename ReturnTypeCastable>
  inline constexpr ReturnType
  operator()(ReturnTypeCastable&& default_val, ForwardedArgsTuple&& tup) const {
    return std::forward<ReturnTypeCastable>(default_val);
  }
};

template <typename Tag, typename AsType>
struct _typeless_kwarg_with_default_as_getter {
  template <typename AsTypeConvertible, typename... Args>
  inline constexpr AsType
  operator()(AsTypeConvertible&& default_val, Args&&... args) const {
    return _get_kwarg_impl::_typeless_default_select_impl<
      Tag, std::tuple<Args...>,
      decltype(std::forward_as_tuple(args...)), AsType
    >()(
      std::forward<AsTypeConvertible>(default_val),
      std::forward_as_tuple(args...)
    );
  }
};

} // end namespace _get_kwarg_impl

template <typename Tag, typename AsType, typename AsTypeConvertible, typename... Args>
inline constexpr
AsType  //std::enable_if_t<std::is_literal_type<AsType>::value, AsType>
get_typeless_kwarg_with_default_as(
  AsTypeConvertible&& default_val,
  Args&&... args
) {
  return _get_kwarg_impl::_typeless_kwarg_with_default_as_getter<Tag, AsType>()(
    std::forward<AsTypeConvertible>(default_val),
    std::forward<Args>(args)...
  );
}


/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* get_typeless_kwarg_with_converter                                     {{{1 */ #if 1 // begin fold

namespace _get_kwarg_impl {

namespace _tkwcg_helper {

template <typename Tag,
  typename ArgsVector,
  typename Converter,
  typename Enable=void /* not multi_kwarg_expr */
>
struct _impl {
  using return_t = decltype(
    std::declval<Converter>()(
      std::declval<
        typename m::at<seq_tag_spot<Tag, ArgsVector>::value, ArgsVector>::type
      >().value()
    )
  );
  template <typename... Args>
  inline constexpr return_t
  operator()(Converter&& conv, Args&&... args) const {
    constexpr size_t spot = _get_kwarg_impl::var_tag_spot<Tag, Args...>::value;
    // TODO error message readability and compile-time debugability
    static_assert(spot < sizeof...(Args), "missing required keyword argument");
    return conv(std::get<spot>(std::forward_as_tuple(args...)).value());
  }
};

template <typename Tag,
  typename ArgsVector,
  typename Converter
>
struct _impl<Tag, ArgsVector, Converter,
  typename std::enable_if<m::is_instantiation_of<
    multiarg_typeless_kwarg_expression,
    typename std::decay<
      typename m::at<seq_tag_spot<Tag, ArgsVector>::value, ArgsVector>::type
    >::type
  >::value>::type
> {
  using return_t = decltype(
    std::declval<
      typename m::at< seq_tag_spot<Tag, ArgsVector>::value, ArgsVector>::type
    >().value_converted(std::declval<Converter>())
  );
  template <typename... Args>
  inline constexpr return_t
  operator()(Converter&& conv, Args&&... args) const {
    constexpr size_t spot = _get_kwarg_impl::var_tag_spot<Tag, Args...>::value;
    // TODO error message readability and compile-time debugability
    static_assert(spot < sizeof...(Args), "missing required keyword argument");
    return std::get<spot>(std::forward_as_tuple(args...)).value_converted(
      std::forward<Converter>(conv)
    );
  }
};

} // end namespace _tkwcg_helper

template <typename Tag>
struct _typeless_kwarg_with_converter_getter {
  template <typename Converter, typename... Args>
  using helper_t = typename _tkwcg_helper::_impl<Tag, m::vector<Args...>, Converter>;
  template <typename Converter, typename... Args>
  using return_t = typename helper_t<Converter, Args...>::return_t;

  template <typename Converter, typename... Args>
  return_t<Converter, Args...>
  operator()(Converter&& conv, Args&&... args) const {
    return helper_t<Converter, Args...>()(
      std::forward<Converter>(conv),
      std::forward<Args>(args)...
    );
  }
};

} // end namespace _get_kwarg_impl

template <typename Tag, typename Converter, typename... Args>
inline constexpr
typename
_get_kwarg_impl::_typeless_kwarg_with_converter_getter<Tag>::template return_t<Converter, Args...>
get_typeless_kwarg_with_converter(
  Converter&& conv,
  Args&&... args
) {
  return _get_kwarg_impl::_typeless_kwarg_with_converter_getter<Tag>()(
    std::forward<Converter>(conv),
    std::forward<Args>(args)...
  );
}


/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* get_typeless_kwarg_with_converter_and_default                         {{{1 */ #if 1 // begin fold

namespace _get_kwarg_impl {

template <
  typename Tag, typename ArgsTuple,
  typename Enable=void /* => kwarg found */
>
struct _default_conv_select_impl {
  static constexpr size_t spot = seq_tag_spot<Tag, ArgsTuple>::value;

  template <typename Converter, typename DefaultType, typename... Args>
  using helper_t = _tkwcg_helper::_impl<Tag, m::vector<Args...>, Converter>;
  template <typename Converter, typename DefaultType, typename... Args>
  using return_t = typename helper_t<Converter, DefaultType, Args...>::return_t;

  template <typename Converter, typename DefaultConvertible, typename... Args>
  inline constexpr return_t<Converter, DefaultConvertible, Args...>
  operator()(Converter&& conv, DefaultConvertible&&, Args&&... args) const {
    return helper_t<Converter, DefaultConvertible, Args...>()(
      std::forward<Converter>(conv),
      std::forward<Args>(args)...
    );
  }
};

template <typename Tag, typename ArgsTuple>
struct _default_conv_select_impl<
  Tag, ArgsTuple,
  std::enable_if_t<seq_tag_spot<Tag, ArgsTuple>::value == m::size<ArgsTuple>::value>
> {
  template <typename Converter, typename DefaultType, typename... Args>
  using return_t = DefaultType;

  template <typename Converter, typename DefaultConvertible, typename... Args>
  inline constexpr return_t<Converter, DefaultConvertible, Args...>
  operator()(Converter&&, DefaultConvertible&& def_val, Args&&...) const {
    return std::forward<DefaultConvertible>(def_val);
  }
};

template <typename Tag>
struct _typeless_kwarg_with_converter_and_default_getter {

  template <typename Converter, typename DefaultType, typename... Args>
  using return_t = typename
    _default_conv_select_impl<Tag, std::tuple<Args...>>::template return_t<Converter, DefaultType, Args...>;

  template <typename Converter, typename DefaultType, typename... Args>
  inline constexpr
  return_t<Converter, DefaultType, Args...>
  operator()(Converter&& conv, DefaultType&& def_val, Args&&... args) const {
    return _default_conv_select_impl<Tag, std::tuple<Args...>>()(
      std::forward<Converter>(conv),
      std::forward<DefaultType>(def_val),
      std::forward<Args>(args)...
    );
  }
};

} // end namespace _get_kwarg_impl

template <typename Tag, typename Converter, typename Default, typename... Args>
inline constexpr
typename
_get_kwarg_impl::_typeless_kwarg_with_converter_and_default_getter<Tag>::template return_t<
  Converter, Default, Args...
>
get_typeless_kwarg_with_converter_and_default(
  Converter&& conv, Default&& def_val,
  Args&&... args
) {
  return _get_kwarg_impl::_typeless_kwarg_with_converter_and_default_getter<Tag>()(
    std::forward<Converter>(conv),
    std::forward<Default>(def_val),
    std::forward<Args>(args)...
  );
}


/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* get_positional_args_tuple                                    {{{1 */ #if 1 // begin fold

namespace _get_kwarg_impl {

// TODO verify that all positional args come before all keyword args

struct _positional_arg_tuple_getter {
  private:
    template <typename... Args>
    using _pos_arg_spots = typename mv::find_all_if_not<
      is_kwarg_expression, Args...
    >::type;

  public:

    template <typename... Args>
    using return_t = typename m::transform<
      _pos_arg_spots<Args...>,
      m::lambda<mv::types_only::at<mp::_, Args&&...>>::template apply,
      std::tuple
    >::type;

    template <typename... Args>
    inline constexpr return_t<Args...>
    operator()(Args&&... args) const {
      return _impl(
        _pos_arg_spots<Args...>(),
        std::forward<Args>(args)...
      );
    }

  private:
    template <typename... Args, size_t... Spots>
    inline constexpr return_t<Args...>
    _impl(m::vector_c<std::size_t, Spots...>, Args&&... args) const {
      return std::forward_as_tuple(
        std::get<Spots>(std::forward_as_tuple(std::forward<Args>(args)...))...
      );
    }
};

} // end namespace _get_kwarg_impl

template <typename... Args>
inline constexpr
_get_kwarg_impl::_positional_arg_tuple_getter::return_t<Args...>
get_positional_arg_tuple(Args&&... args)
{
  return _get_kwarg_impl::_positional_arg_tuple_getter()(std::forward<Args>(args)...);
}


/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////

}} // end namespace dharma_runtime::detail



#endif /* SRC_KEYWORD_ARGUMENTS_GET_KWARG_H_ */
