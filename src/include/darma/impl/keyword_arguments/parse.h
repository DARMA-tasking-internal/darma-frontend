/*
//@HEADER
// ************************************************************************
//
//                      parse.h
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

#ifndef DARMA_IMPL_KEYWORD_ARGUMENTS_PARSE_H
#define DARMA_IMPL_KEYWORD_ARGUMENTS_PARSE_H

#include <type_traits>

#include <tinympl/stl_integer_sequence.hpp>
#include <tinympl/select.hpp>
#include <tinympl/bool.hpp>

#include "kwarg_expression.h"

namespace darma_runtime {

namespace detail {

struct deduced_parameter {
  template <typename T>
  T&& get_argument(T&& val) { return std::forward<T>(val); }
};

template <typename ParameterType>
struct _argument_description_base {
  template <typename T>
  using _type_is_compatible = std::conditional_t<
    std::is_same<ParameterType, deduced_parameter>::value,
    std::true_type,
    std::is_convertible<T, ParameterType>
  >;

};

template <typename ParameterType, typename KWArgTag>
struct positional_or_keyword_argument : _argument_description_base<ParameterType> {
  public:
    using parameter_type = ParameterType;
    using tag = KWArgTag;

  private:
    using base_t = _argument_description_base<ParameterType>;

    template <typename T>
    using _value_type_if_kwarg_archetype = decltype( std::declval<T>().value() );
    template <typename T>
    using _value_type_if_kwarg = meta::detected_t<
      _value_type_if_kwarg_archetype, T
    >;

  public:
    using can_be_positional = std::true_type;
    using can_be_keyword = std::true_type;

    template <typename Argument>
    using argument_is_compatible = tinympl::select_first_t<
      is_kwarg_expression_with_tag<
        std::decay_t<Argument>, KWArgTag
      >,
      /* => */ typename base_t::template _type_is_compatible<_value_type_if_kwarg<Argument>>,
      std::true_type,
      /* default => */ typename base_t::template _type_is_compatible<Argument>
    >;

};

template <typename ParameterType>
struct positional_only_argument : _argument_description_base<ParameterType> {

  public:
    using base_t = _argument_description_base<ParameterType>;
    using can_be_positional = std::true_type;
    using can_be_keyword = std::false_type;
    using tag = /* some value that will never accidentally match*/ tinympl::int_<42>;

    template <typename Argument>
    using argument_is_compatible = typename base_t::template _type_is_compatible<Argument>;
};

template <typename ParameterType, typename KWArgTag>
struct keyword_only_argument : _argument_description_base<ParameterType> {
  private:
    using base_t = _argument_description_base<ParameterType>;

    template <typename T>
    using _value_type_if_kwarg_archetype = decltype(std::declval<T>().value());
    template <typename T>
    using _value_type_if_kwarg = meta::detected_t<
      _value_type_if_kwarg_archetype, T
    >;

  public:
    using parameter_type = ParameterType;
    using tag = KWArgTag;

    using can_be_positional = std::false_type;
    using can_be_keyword = std::true_type;

    template <typename Argument>
    using argument_is_compatible = typename base_t::template _type_is_compatible<
      _value_type_if_kwarg<Argument>
    >;

};

namespace _impl {

template <typename WrappedIndex, typename Arg>
struct arg_with_index {
  using index_t = WrappedIndex;
  static constexpr auto index = WrappedIndex::value;
  using argument_type = Arg;
  using type = Arg;
  using decayed_type = std::decay_t<Arg>;
};

template <typename ArgsWithIdxs, typename ArgDescsWithIdxs>
struct _overload_desc_is_valid_impl {

  //==============================================================================
  // <editor-fold desc="make the template parameters visible to the outside for debugging">

  using _args_with_idxs = ArgsWithIdxs;
  using _arg_descs_with_idxs = ArgDescsWithIdxs;


  // </editor-fold> end make the template parameters visible to the outside for debugging
  //==============================================================================


  //============================================================================
  // <editor-fold desc="unary helpers">

  template <typename dpair>
  using _is_positional_description = typename dpair::type::can_be_positional;

  template <typename dpair>
  using _is_kwarg_description = typename dpair::type::can_be_keyword;

  template <typename dpair>
  using _is_pos_or_kwarg_description = tinympl::bool_<
    dpair::type::can_be_keyword::value
    and dpair::type::can_be_positional::value
  >;

  template <typename dpair>
  using _is_kwarg_only_description = tinympl::bool_<
    dpair::type::can_be_keyword::value
      and not dpair::type::can_be_positional::value
  >;

  template <typename descpair>
  struct _make_kwarg_tag_matches {
    template <typename argpair>
    using apply = tinympl::bool_<
      std::is_same<typename descpair::type::tag,
        typename is_kwarg_expression<typename argpair::decayed_type>::tag
      >::value
    >;
  };

  template <typename descpair>
  using _matching_kwarg_given = tinympl::any_of<
    ArgsWithIdxs, _make_kwarg_tag_matches<descpair>::template apply
  >;

  template <typename argpair>
  using _is_positional_arg = tinympl::bool_<not is_kwarg_expression<
    typename argpair::decayed_type
  >::value>;

  template <typename argpair>
  using _is_keyword_arg = is_kwarg_expression<
    typename argpair::decayed_type
  >;

  template <typename descpair,
    /* convenience default param used as alias */
    typename argpair=typename ArgsWithIdxs::template at_or_t<
      arg_with_index<std::integral_constant<size_t, 0 /*ignored*/>, meta::nonesuch>,
      tinympl::find_if<ArgsWithIdxs,
        _make_kwarg_tag_matches<descpair>::template apply
      >::value
    >
  >
  using _given_kwarg_type_compatible = tinympl::and_<
    _is_keyword_arg<argpair>,
    typename descpair::type::template argument_is_compatible<typename argpair::type>
  >;


  template <typename argpair>
  using _desc_for = tinympl::at_t<argpair::index, ArgDescsWithIdxs>;

  template <typename argpair>
  using _pos_arg_is_convertible = tinympl::and_<
    _is_positional_arg<argpair>,
    typename _desc_for<argpair>::type
      ::template argument_is_compatible<typename argpair::type>
  >;

  // </editor-fold> end unary helpers
  //============================================================================

  static constexpr auto first_kwarg_only_desc = tinympl::find_if<
    ArgDescsWithIdxs, _is_kwarg_only_description
  >::type::value;

  static constexpr auto first_desc_given_as_kwarg = tinympl::find_if<
    ArgDescsWithIdxs, _matching_kwarg_given
  >::type::value;

  static constexpr auto n_positional_given = tinympl::count_if<
    ArgsWithIdxs, _is_positional_arg
  >::value;

  static constexpr auto first_kwarg_given = tinympl::find_if<
    ArgsWithIdxs, _is_keyword_arg
  >::value;

  // If n_positional_given is greater than the index of the first kwarg given,
  // then there are keyword arguments given before the last positional
  static constexpr auto kwarg_given_before_last_positional =
    n_positional_given > first_kwarg_given;

  // Sanity check: either a kwarg was given before the last positional, or
  // the first kwarg given index should be exactly the same as the number of
  // positional arguments
  static_assert(
    kwarg_given_before_last_positional
    or n_positional_given == first_kwarg_given,
    "metaprogramming logic error"
  );

  // The number of positional arguments that can be given and still have the
  // overload be valid is the minimum of the first argument description index that
  // must be keyword only and the index of the first positional-or-keyword argument
  // that has been given as a keyword argument (either of which could be none of
  // them, so it could be the number of arguments)
  static constexpr auto n_positional_args_good =
    first_kwarg_only_desc < first_desc_given_as_kwarg ?
      first_kwarg_only_desc : first_desc_given_as_kwarg;

  static constexpr auto too_many_positional =
    n_positional_given > n_positional_args_good;

  // Sanity check: if too many positional is false and
  // kwarg_given_before_last_positional is also false, the number of positional
  // arguments given should be less than or equal to the number of "good"
  // positional args (the less than happens when only positional arguments are given)
  static_assert(
    too_many_positional or kwarg_given_before_last_positional
      or n_positional_given <= n_positional_args_good,
    "metaprogramming logic error"
  );

  // At this point we can assume that all of the positional arguments are
  // "allowed" to be positional, so we can just check to see if the type
  // is allowed
  static constexpr auto all_positional_args_convertible = tinympl::all_of<
    typename ArgsWithIdxs::template erase<
      n_positional_given, ArgsWithIdxs::size
    >::type,
    _pos_arg_is_convertible
  >::value;

  // The rest should be given as keyword arguments.  If any arg desc after
  // n_pos_given is not given as a keyword argument, this overload doesn't match
  static constexpr auto all_kwargs_found = tinympl::all_of<
    typename ArgDescsWithIdxs::template erase<0, n_positional_given>::type,
    _matching_kwarg_given
  >::value;

  static constexpr auto all_kwargs_convertible = tinympl::all_of<
    typename ArgDescsWithIdxs::template erase<0, n_positional_given>::type,
    _given_kwarg_type_compatible
  >::value;

  static constexpr auto value =
    not (
      kwarg_given_before_last_positional
        or too_many_positional
    )
    and all_positional_args_convertible
    and all_kwargs_found
    and all_kwargs_convertible;

  using type = tinympl::bool_<value>;

  template <size_t Position, typename ForwardedArgsTuple>
  decltype(auto)
  _get_invoke_arg(
    /* given as positional (i.e., position less than n_positional_given) */
    std::true_type,
    ForwardedArgsTuple&& tup
  ) const {
    return std::get<Position>(std::forward<ForwardedArgsTuple>(tup));
  };

  template <size_t Position, typename ForwardedArgsTuple>
  decltype(auto)
  _get_invoke_arg(
    /* given as positional (i.e., position less than n_positional_given) */
    std::false_type,
    ForwardedArgsTuple&& tup
  ) const {
    return std::get<
      tinympl::find_if<
        ArgsWithIdxs,
        _make_kwarg_tag_matches<
          typename ArgDescsWithIdxs::template at_t<Position>
        >::template apply
      >::value
    >(
      std::forward<ForwardedArgsTuple>(tup)
    ).value();
  };

  template <size_t Position, typename ForwardedArgsTuple>
  decltype(auto)
  get_invoke_arg(ForwardedArgsTuple&& tup) const {
    return _get_invoke_arg<Position>(
      typename tinympl::bool_<(Position < n_positional_given)>::type{},
      std::forward<ForwardedArgsTuple>(tup)
    );
  };


};



} // end namespace _impl

template <typename... ArgumentDescriptions>
struct overload_description {

  template <typename... Args>
  using _make_args_with_indices = typename tinympl::zip<
    tinympl::vector, _impl::arg_with_index,
    std::index_sequence_for<Args...>,
    tinympl::vector<Args...>
  >::type;

  template <typename... Args>
  using _helper = _impl::_overload_desc_is_valid_impl<
    _make_args_with_indices<Args...>,
    _make_args_with_indices<ArgumentDescriptions...>
  >;

  template <typename... Args>
  using is_valid_for_args = std::conditional_t<
    sizeof...(Args) != sizeof...(ArgumentDescriptions),
    std::false_type, _helper<Args...>
  >;

  template <typename Callable, typename ForwardedArgsTuple, typename Helper, size_t... Idxs>
  decltype(auto)
  _invoke_impl(Callable&& C,
    std::integer_sequence<size_t, Idxs...>,
    Helper h,
    ForwardedArgsTuple&& tup
  ) const {
    return std::forward<Callable>(C)(
      h.template get_invoke_arg<Idxs>(
        std::forward<ForwardedArgsTuple>(tup)
      )...
    );
  };

  template <typename Callable, typename... Args>
  decltype(auto)
  invoke(Callable&& C, Args&&... args) const {
    return _invoke_impl(
      std::forward<Callable>(C),
      std::index_sequence_for<Args...>(),
      _helper<Args...>{},
      std::forward_as_tuple(std::forward<Args>(args)...)
    );
  };


};

template <typename... OverloadDescriptions>
struct kwarg_parser {
  private:

    template <typename... Args>
    struct _make_overload_is_valid {
      template <typename Overload>
      using apply = typename Overload::template is_valid_for_args<Args...>;
    };



  public:

    template <typename... Args>
    using invocation_is_valid = typename tinympl::variadic::any_of<
      _make_overload_is_valid<Args...>::template apply, OverloadDescriptions...
    >::type;

    template <typename Callable, typename... Args>
    decltype(auto)
    invoke(Callable&& C, Args&&... args) const {
      return tinympl::variadic::at_t<
        tinympl::variadic::find_if<
          _make_overload_is_valid<Args...>::template apply,
          OverloadDescriptions...
        >::value,
        OverloadDescriptions...
      >().invoke(
        std::forward<Callable>(C),
        std::forward<Args>(args)...
      );
    };
};


} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_KEYWORD_ARGUMENTS_PARSE_H
