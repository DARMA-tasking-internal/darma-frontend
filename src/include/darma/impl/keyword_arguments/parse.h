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

#include <cassert>
#include <type_traits>
#include <darma/impl/util/static_assertions.h>

#include <tinympl/stl_integer_sequence.hpp>
#include <tinympl/select.hpp>
#include <tinympl/bool.hpp>

#include "kwarg_expression.h"

// TODO figure out how to replace "tags" with "arguments" in error message?!?
// TODO readable errors for variadic positional arguments

namespace _darma__errors {
template <typename... Args>
struct __________no_matching_function_call_with__ { };
template <typename Arg>
struct ____positional_argument____ { };
template <typename Tag>
struct ____keyword_argument____ {
  template <typename Arg>
  struct _equals_ { };
};
struct __________candidate_overloads_are__ { };
template <typename... Args>
struct _____overload_candidate_with__ { };

template <typename Tag>
struct ___expected_positional_or_keyword_ {
  template <typename Arg>
  struct _convertible_to_ { };
  struct _with_deduced_type { };
};

template <typename Tag>
struct ___expected_keyword_argument_only_ {
  template <typename Arg>
  struct _convertible_to_ { };
  struct _with_deduced_type { };
};

struct ___expected_positional_argument_only_ {
  template <typename Arg>
  struct _convertible_to_ { };
  struct _with_deduced_type { };
};

} // end namespace _darma__errors

namespace darma_runtime {

namespace detail {

//==============================================================================

struct converted_parameter { };

struct deduced_parameter { };

struct variadic_arguments_begin_tag { };

//==============================================================================

template <typename ParameterType>
struct _argument_description_base {

  // TODO finish this (maybe?)
  //template <typename ConvertedParam, typename KWArg>
  //using converter_is_valid_archetype = meta::callable_traits<
  //  typename ConvertedParam::


  template <typename T>
  using _type_is_compatible = tinympl::select_first_t<
    //==========================================================================
    std::is_same<ParameterType, deduced_parameter>,
    /* => */ std::true_type,
    //==========================================================================
    std::is_same<ParameterType, converted_parameter>,
    /* => */ std::true_type,
    //==========================================================================
    std::true_type,
    /* => */ std::is_convertible<T, ParameterType>
  >;

};

//==============================================================================

template <typename ParameterType, typename KWArgTag, bool Optional=false>
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
    using is_optional = tinympl::bool_<Optional>;
    using can_be_positional = std::true_type;
    using can_be_keyword = std::true_type;
    using is_converted = std::is_same<ParameterType, converted_parameter>;

    static_assert(not Optional,
      "Optional positional arguments not yet implemented"
    );

    template <typename Argument>
    using argument_is_compatible = tinympl::select_first_t<
      is_kwarg_expression_with_tag<
        std::decay_t<Argument>, KWArgTag
      >,
      /* => */ typename base_t::template _type_is_compatible<_value_type_if_kwarg<Argument>>,
      std::true_type,
      /* default => */ typename base_t::template _type_is_compatible<Argument>
    >;

    using _pretty_printed_error_t = std::conditional_t<
      std::is_same<ParameterType, deduced_parameter>::value,
      typename _darma__errors::___expected_positional_or_keyword_<KWArgTag>::_with_deduced_type,
      typename _darma__errors::___expected_positional_or_keyword_<
        KWArgTag
      >::template _convertible_to_<ParameterType>
    >;

};
template <typename ParameterType, typename KWArgTag, bool Optional=false>
using _positional_or_keyword = positional_or_keyword_argument<ParameterType, KWArgTag, Optional>;

//==============================================================================

template <typename ParameterType, bool Optional=false>
struct positional_only_argument : _argument_description_base<ParameterType> {

  public:
    using is_optional = tinympl::bool_<Optional>;
    using base_t = _argument_description_base<ParameterType>;
    using can_be_positional = std::true_type;
    using can_be_keyword = std::false_type;
    using tag = /* some value that will never accidentally match*/ tinympl::int_<42>;
    using is_converted = std::is_same<ParameterType, converted_parameter>;

    static_assert(not Optional,
      "Optional positional arguments not yet implemented"
    );

    template <typename Argument>
    using argument_is_compatible = typename base_t::template _type_is_compatible<Argument>;

    using _pretty_printed_error_t = std::conditional_t<
      std::is_same<ParameterType, deduced_parameter>::value,
      _darma__errors::___expected_positional_argument_only_::_with_deduced_type,
      _darma__errors::___expected_positional_argument_only_::template _convertible_to_<ParameterType>
    >;
};
template <typename ParameterType, bool Optional=false>
using _positional = positional_only_argument<ParameterType, Optional>;

//==============================================================================

template <typename ParameterType, typename KWArgTag, bool Optional=false>
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

    using is_optional = tinympl::bool_<Optional>;
    using can_be_positional = std::false_type;
    using can_be_keyword = std::true_type;
    using is_converted = std::is_same<ParameterType, converted_parameter>;

    template <typename Argument>
    using argument_is_compatible = typename base_t::template _type_is_compatible<
      _value_type_if_kwarg<Argument>
    >;

    using _pretty_printed_error_t = std::conditional_t<
      std::is_same<ParameterType, deduced_parameter>::value,
      typename _darma__errors::___expected_keyword_argument_only_<KWArgTag>::_with_deduced_type,
      typename _darma__errors::___expected_keyword_argument_only_<
        KWArgTag
      >::template _convertible_to_<ParameterType>
    >;
};
template <typename ParameterType, typename KWArgTag, bool Optional=false>
using _keyword = keyword_only_argument<ParameterType, KWArgTag, Optional>;

//==============================================================================

// TODO finish this
//struct variadic_positional_arguments {
//  using parameter_type = converted_parameter;
//  using tag = /* some value that will never accidentally match*/ tinympl::int_<73>;
//
//  using is_optional = std::false_type;
//  using can_be_positional = std::true_type;
//  using can_be_keyword = std::false_type;
//  using is_converted = std::true_type;
//
//  template <typename Argument>
//  using argument_is_compatible = std::true_type;
//
//  // TODO _pretty_printed_error_t
//  // TODO Handle this parameter type below
//
//};

//==============================================================================

namespace _impl {

template <typename T> struct _optional_impl;

template <typename Parameter>
struct _optional_impl<positional_only_argument<Parameter, false>> {
  using type = positional_only_argument<Parameter, true>;
};

template <typename Parameter, typename KWArgTag>
struct _optional_impl<keyword_only_argument<Parameter, KWArgTag, false>> {
  using type = keyword_only_argument<Parameter, KWArgTag, true>;
};

template <typename Parameter, typename KWArgTag>
struct _optional_impl<positional_or_keyword_argument<Parameter, KWArgTag, false>> {
  using type = positional_or_keyword_argument<Parameter, KWArgTag, true>;
};
} // end namespace _impl

//==============================================================================

template <typename T>
using _optional = typename _impl::_optional_impl<T>::type;

template <typename ParamType, typename KWArgTag, bool Optional=false>
using _optional_keyword = _keyword<ParamType, KWArgTag, true>;

//==============================================================================

namespace _impl {

//==============================================================================

template <typename WrappedIndex, typename Arg>
struct arg_with_index {
  using index_t = WrappedIndex;
  static constexpr auto index = WrappedIndex::value;
  using argument_type = Arg;
  using type = Arg;
  using decayed_type = std::decay_t<Arg>;
};

//==============================================================================

template <typename In_ArgsWithIdxs, typename In_ArgDescsWithIdxs,
  typename In_OptionalArgDescsWithIdxs, bool AllowVariadicArgs=false
>
struct _overload_desc_is_valid_impl {

  using ArgsWithIdxs = In_ArgsWithIdxs;
  using ArgDescsWithIdxs = In_ArgDescsWithIdxs;
  using OptionalArgDescsWithIdxs = In_OptionalArgDescsWithIdxs;

  static constexpr auto variadic_args_allowed = AllowVariadicArgs;

  // placeholder for failed finds
  template <
    // Using strange types in these values with the intent to minimize the
    // possibility of accidentally matching...
    typename IsOptional = std::false_type,
    typename CanBePositional = std::false_type,
    typename CanBeKeyWord = std::false_type,
    typename ParameterType = meta::nonesuch,
    typename TagType = tinympl::identity<meta::nonesuch>
  >
  struct nonesuch_description {
    using parameter_type = ParameterType;
    using tag = TagType;
    using is_optional = IsOptional;
    using can_be_positional = CanBePositional;
    using can_be_keyword = CanBeKeyWord;
    template <typename> using argument_is_compatible = std::false_type;
  };


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
  struct _make_kwarg_desc_tag_matches {
    template <typename argpair>
    using apply = tinympl::bool_<
      std::is_same<typename descpair::type::tag,
        typename is_kwarg_expression<typename argpair::decayed_type>::tag
      >::value
    >;
  };

  template <typename descpair>
  using _matching_kwarg_given = tinympl::any_of<
    ArgsWithIdxs, _make_kwarg_desc_tag_matches<descpair>::template apply
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
        _make_kwarg_desc_tag_matches<descpair>::template apply
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

  template <typename argpair>
  struct _make_given_kwarg_tag_matches {
    template <typename descpair>
    using apply = std::is_same<
      typename descpair::type::tag, typename argpair::decayed_type::tag
    >;
  };

  using _req_and_optional_kwarg_descs_with_idxs = typename ArgDescsWithIdxs
    ::template extend_t<OptionalArgDescsWithIdxs>
    ::template erase_if_not_t<_is_kwarg_description>;

  template <typename argpair>
  struct _req_or_optional_kw_desc_found :
    tinympl::any_of<
      _req_and_optional_kwarg_descs_with_idxs,
      _make_given_kwarg_tag_matches<argpair>::template apply
    >
  { };

  template <typename argpair>
  using _kwarg_description_found = tinympl::and_<
    _is_keyword_arg<argpair>,
    _req_or_optional_kw_desc_found<argpair>
  >;

  template <typename kwargs_with_idxs>
  struct _make_no_duplicates_mfn {
    private:
      template <typename kwargpair>
      struct _make_given_kwarg_matches_given_kwarg {
        template <typename kwargpair2>
        using apply = std::is_same<
          typename kwargpair::decayed_type::tag,
          typename kwargpair2::decayed_type::tag
        >;
      };
    public:
      template <typename kwargpair>
      using apply = tinympl::bool_<
        tinympl::count_if<
          kwargs_with_idxs,
          _make_given_kwarg_matches_given_kwarg<kwargpair>::template apply
        >::value == 1
      >;
  };

  template <typename kwargpair>
  struct _get_optional_kwarg_description {
    using type = typename OptionalArgDescsWithIdxs::template get_first_if_or_default<
      _make_given_kwarg_tag_matches<kwargpair>::template apply,
      arg_with_index<
        std::integral_constant<size_t, 0>,
        nonesuch_description</* is_optional = */std::false_type>
      >
    >::type::type;
  };

  template <typename kwargpair,
    // optional argument used as convenience alias
    typename optkwargdesc = typename _get_optional_kwarg_description<kwargpair>::type
  >
  using _given_kwarg_compatible_or_not_optional = std::conditional_t<
    optkwargdesc::is_optional::value,
    typename optkwargdesc::template argument_is_compatible<typename kwargpair::type>,
    std::true_type
  >;

  template <typename argpair>
  using _is_converted_param = typename argpair::type::is_converted;

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

  static constexpr auto n_described_positional_given =
    n_positional_given < first_kwarg_only_desc ?
      n_positional_given : first_kwarg_only_desc;

  static constexpr auto first_kwarg_given = tinympl::find_if<
    ArgsWithIdxs, _is_keyword_arg
  >::value;

  static constexpr auto n_variadic_given =
    n_positional_given - n_described_positional_given;

  static constexpr auto variadics_given = n_variadic_given > 0;

  static constexpr auto variadics_given_but_not_allowed =
    variadics_given and not variadic_args_allowed;

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

  static constexpr auto too_many_positional = not variadic_args_allowed and
    n_positional_given > n_positional_args_good;

  // Sanity check: if too many positional is false and
  // kwarg_given_before_last_positional is also false, the number of positional
  // arguments given should be less than or equal to the number of "good"
  // positional args (the less than happens when only positional arguments are given)
  static_assert(
    too_many_positional or kwarg_given_before_last_positional or variadic_args_allowed
      or n_positional_given <= n_positional_args_good,
    "metaprogramming logic error"
  );

  // At this point we can assume that all of the positional arguments are
  // "allowed" to be positional, so we can just check to see if the type
  // is allowed
  static constexpr auto all_positional_args_convertible = std::conditional_t<
    too_many_positional or kwarg_given_before_last_positional,
    // if there are too many positional arguments, then we've already failed,
    // so just put false here
    std::false_type,
    tinympl::all_of<
      typename ArgsWithIdxs::template erase<
        n_described_positional_given, ArgsWithIdxs::size
      >::type,
      _pos_arg_is_convertible
    >
  >::value;


  // The rest should be given as keyword arguments.  If any arg desc after
  // n_pos_given is not given as a keyword argument, this overload doesn't match

  using _req_kwarg_descs_with_idxs = typename std::conditional_t<
    (
      (too_many_positional or kwarg_given_before_last_positional)
      and n_positional_given > 0
    ),
    // if there are too many positional arguments, then we've already failed,
    // so just put an empty vector here
    tinympl::identity<tinympl::vector<>>,
    typename ArgDescsWithIdxs
      ::template erase<0, n_described_positional_given>
  >::type;

  using _kwargs_with_idxs = typename std::conditional_t<
    too_many_positional or kwarg_given_before_last_positional,
    // if there are too many positional arguments, then we've already failed,
    // so just put an empty vector here
    tinympl::identity<tinympl::vector<>>,
    typename ArgsWithIdxs::template erase<0, n_positional_given>
  >::type;

  static constexpr auto all_kwargs_found = tinympl::all_of<
    _req_kwarg_descs_with_idxs,
    _matching_kwarg_given
  >::value;

  static constexpr auto all_required_kwargs_convertible = tinympl::all_of<
    _req_kwarg_descs_with_idxs,
    _given_kwarg_type_compatible
  >::value;

  static constexpr auto given_optional_kwargs_convertible = tinympl::all_of<
    _kwargs_with_idxs,
    _given_kwarg_compatible_or_not_optional
  >::value;

  static constexpr auto all_given_kwargs_allowed = tinympl::all_of<
    _kwargs_with_idxs,
    _kwarg_description_found
  >::value;

  static constexpr auto no_keyword_argument_given_twice = tinympl::and_<
    tinympl::bool_<not kwarg_given_before_last_positional>,
    tinympl::all_of<
      _kwargs_with_idxs,
      _make_no_duplicates_mfn<_kwargs_with_idxs>::template apply
    >
  >::type::value;

  static constexpr auto value =
    not (
      kwarg_given_before_last_positional
        or too_many_positional
        or variadics_given_but_not_allowed
    )
    and all_positional_args_convertible
    and all_kwargs_found
    and all_required_kwargs_convertible
    and given_optional_kwargs_convertible
    and all_given_kwargs_allowed
    and no_keyword_argument_given_twice
  ;

  using type = tinympl::bool_<value>;

  //============================================================================

  // This could probably be done faster...
  template <typename descpair>
  using _converter_index = typename tinympl::count_if<
    typename ArgDescsWithIdxs
      ::template extend_t<OptionalArgDescsWithIdxs>
      ::template erase<
        descpair::index,
        ArgDescsWithIdxs::size + OptionalArgDescsWithIdxs::size
      >::type,
    _is_converted_param
  >::type;


  template <
    typename ArgDescPair,
    typename ArgForwarded,
    typename ConvertersTuple
  >
  decltype(auto)
  _get_kwarg_value_impl(
    /* a parameter with a converter */
    std::true_type,
    /* not a multi-arg */
    std::false_type,
    ArgForwarded&& arg,
    ConvertersTuple&& converters
  ) const {
    return std::get<_converter_index<ArgDescPair>::value>(
      std::forward<ConvertersTuple>(converters)
    )(
      std::forward<ArgForwarded>(arg).value()
    );
  }

  template <
    typename ArgDescPair,
    typename ArgForwarded,
    typename ConvertersTuple
  >
  decltype(auto)
  _get_kwarg_value_impl(
    /* a parameter with a converter */
    std::true_type,
    /* a multi-arg */
    std::true_type,
    ArgForwarded&& arg,
    ConvertersTuple&& converters
  ) const {
    return std::forward<ArgForwarded>(arg).value_converted(
      std::get<_converter_index<ArgDescPair>::value>(
        std::forward<ConvertersTuple>(converters)
      )
    );
  }

  template <
    typename ArgDescPair,
    typename ArgForwarded,
    typename ConvertersTuple
  >
  decltype(auto)
  _get_kwarg_value_impl(
    /* not a parameter with a converter */
    std::false_type,
    /* not a multi-arg */
    std::false_type,
    ArgForwarded&& arg,
    ConvertersTuple&& converters
  ) const {
    return std::forward<ArgForwarded>(arg).value();
  }

  //============================================================================

  template <
    typename ArgDescPair,
    typename ArgForwarded,
    typename ConvertersTuple
  >
  decltype(auto)
  _get_kwarg_value(
    ArgForwarded&& arg,
    ConvertersTuple&& converters
  ) const {
    return _get_kwarg_value_impl<
      ArgDescPair, ArgForwarded, ConvertersTuple
    >(
      typename ArgDescPair::type::is_converted{},
      typename tinympl::is_instantiation_of<
        multiarg_typeless_kwarg_expression, std::decay_t<ArgForwarded>
      >::type{},
      std::forward<ArgForwarded>(arg),
      std::forward<ConvertersTuple>(converters)
    );

  }

  //============================================================================

  template <size_t OptionalPosition,
    typename DefaultKWArgsTuple,
    typename ConvertersTuple,
    typename ForwardedArgsTuple
  >
  decltype(auto)
  _get_invoke_optional_arg(
    /* argument given by user */
    std::true_type,
    DefaultKWArgsTuple&& defaults,
    ConvertersTuple&& converters,
    ForwardedArgsTuple&& tup
  ) const {
    using arg_desc_t = typename OptionalArgDescsWithIdxs::template at_t<OptionalPosition>;
    static constexpr auto user_arg_desc_idx = tinympl::find_if<
      ArgsWithIdxs,
      _make_kwarg_desc_tag_matches<arg_desc_t>::template apply
    >::value;
    return _get_kwarg_value<arg_desc_t>(
      std::get<user_arg_desc_idx>(
        std::forward<ForwardedArgsTuple>(tup)
      ),
      std::forward<ConvertersTuple>(converters)
    );
  };

  template <size_t OptionalPosition,
    typename DefaultKWArgsTuple,
    typename ConvertersTuple,
    typename ForwardedArgsTuple
  >
  decltype(auto)
  _get_invoke_optional_arg(
    /* fall back to default */
    std::false_type,
    DefaultKWArgsTuple&& defaults,
    ConvertersTuple&& converters,
    ForwardedArgsTuple&& tup
  ) const {
    using arg_desc_t = typename OptionalArgDescsWithIdxs::template at_t<OptionalPosition>;
    static constexpr auto default_desc_idx = tinympl::find_if<
      typename tinympl::zip<
        tinympl::vector,
        arg_with_index,
        std::make_index_sequence<
          std::tuple_size<std::decay_t<DefaultKWArgsTuple>>::value
        >,
        std::decay_t<DefaultKWArgsTuple>
      >::type,
      _make_kwarg_desc_tag_matches<arg_desc_t>::template apply
    >::value;
    return _get_kwarg_value<arg_desc_t>(
      std::get<default_desc_idx>(
        std::forward<DefaultKWArgsTuple>(defaults)
      ),
      std::forward<ConvertersTuple>(converters)
    );

  };

  //============================================================================

  typedef enum {
    DescribedPositional,
    RequiredKeyword,
    OptionalKeyword,
    VariadicTag,
    VariadicPositional
  } argument_catagory_dispatch_tag;

  //============================================================================

  template <argument_catagory_dispatch_tag Tag>
  using argument_catagory_t = std::integral_constant<argument_catagory_dispatch_tag, Tag>;

  template <size_t Position,
    typename DefaultKWArgsTuple,
    typename ConvertersTuple,
    typename ForwardedArgsTuple
  >
  decltype(auto)
  _get_invoke_arg(
    argument_catagory_t<DescribedPositional>,
    DefaultKWArgsTuple&& defaults,
    ConvertersTuple&& converters,
    ForwardedArgsTuple&& tup
  ) const {
    return std::get<Position>(std::forward<ForwardedArgsTuple>(tup));
  };

  template <size_t Position,
    typename DefaultKWArgsTuple,
    typename ConvertersTuple,
    typename ForwardedArgsTuple
  >
  decltype(auto)
  _get_invoke_arg(
    argument_catagory_t<RequiredKeyword>,
    DefaultKWArgsTuple&& defaults,
    ConvertersTuple&& converters,
    ForwardedArgsTuple&& tup
  ) const {
    using arg_desc_t = typename ArgDescsWithIdxs::template at_t<Position>;
    return _get_kwarg_value<arg_desc_t>(
      std::get<
        tinympl::find_if<
          ArgsWithIdxs,
          _make_kwarg_desc_tag_matches<arg_desc_t>::template apply
        >::value
      >(
        std::forward<ForwardedArgsTuple>(tup)
      ),
      std::forward<ConvertersTuple>(converters)
    );
  };

  template <size_t Position,
    typename DefaultKWArgsTuple,
    typename ConvertersTuple,
    typename ForwardedArgsTuple
  >
  decltype(auto)
  _get_invoke_arg(
    argument_catagory_t<OptionalKeyword>,
    DefaultKWArgsTuple&& defaults,
    ConvertersTuple&& converters,
    ForwardedArgsTuple&& tup
  ) const {
    static constexpr auto optional_position = Position - ArgDescsWithIdxs::size;
    static constexpr auto user_arg_desc_idx = tinympl::find_if<
      ArgsWithIdxs,
      _make_kwarg_desc_tag_matches<
        typename OptionalArgDescsWithIdxs::template at_t<optional_position>
      >::template apply
    >::value;
    return _get_invoke_optional_arg<optional_position>(
      std::integral_constant<bool, (user_arg_desc_idx < ArgsWithIdxs::size)>(),
      std::forward<DefaultKWArgsTuple>(defaults),
      std::forward<ConvertersTuple>(converters),
      std::forward<ForwardedArgsTuple>(tup)
    );
  };

  template <size_t Position,
    typename DefaultKWArgsTuple,
    typename ConvertersTuple,
    typename ForwardedArgsTuple
  >
  decltype(auto)
  _get_invoke_arg(
    argument_catagory_t<VariadicPositional>,
    DefaultKWArgsTuple&& defaults,
    ConvertersTuple&& converters,
    ForwardedArgsTuple&& tup
  ) const {
    static constexpr auto variadic_offset = Position - (ArgDescsWithIdxs::size
      + OptionalArgDescsWithIdxs::size + 1);
    return std::get<n_described_positional_given + variadic_offset>(
      std::forward<ForwardedArgsTuple>(tup)
    );
  };

  template <size_t Position,
    typename DefaultKWArgsTuple,
    typename ConvertersTuple,
    typename ForwardedArgsTuple
  >
  decltype(auto)
  _get_invoke_arg(
    argument_catagory_t<VariadicTag>,
    DefaultKWArgsTuple&& defaults,
    ConvertersTuple&& converters,
    ForwardedArgsTuple&& tup
  ) const {
    return variadic_arguments_begin_tag{};
  };

  //============================================================================

  template <size_t Position,
    typename DefaultKWArgsTuple,
    typename ConvertersTuple,
    typename ForwardedArgsTuple
  >
  decltype(auto)
  get_invoke_arg(
    DefaultKWArgsTuple&& defaults,
    ConvertersTuple&& converters,
    ForwardedArgsTuple&& tup
  ) const {
    using dispatch_tag_t = tinympl::select_first_t<
      tinympl::bool_<(Position < n_described_positional_given)>,
      /* => */ argument_catagory_t<DescribedPositional>,
      tinympl::bool_<(
        Position >= n_described_positional_given
        and Position < ArgDescsWithIdxs::size
      )>,
      /* => */ argument_catagory_t<RequiredKeyword>,
      tinympl::bool_<(
        Position >= ArgDescsWithIdxs::size
        and Position < ArgDescsWithIdxs::size + OptionalArgDescsWithIdxs::size
      )>,
      /* => */ argument_catagory_t<OptionalKeyword>,
      tinympl::bool_<(
        Position == ArgDescsWithIdxs::size + OptionalArgDescsWithIdxs::size
      )>,
      /* => */ argument_catagory_t<VariadicTag>,
      tinympl::bool_<(
        Position > ArgDescsWithIdxs::size + OptionalArgDescsWithIdxs::size
      )>,
      /* => */ argument_catagory_t<VariadicPositional>
    >;
    return _get_invoke_arg<Position>(
      dispatch_tag_t{},
      std::forward<DefaultKWArgsTuple>(defaults),
      std::forward<ConvertersTuple>(converters),
      std::forward<ForwardedArgsTuple>(tup)
    );
  };


};

//==============================================================================


//==============================================================================

template <bool AllowVariadics, typename... ArgumentDescriptions>
struct _overload_description_maybe_variadic {

  using _arg_desc_vector = tinympl::vector<ArgumentDescriptions...>;

  //============================================================================
  // <editor-fold desc="Check that all of the optional descs follow required ones">

  template <typename T>
  using _is_required = tinympl::bool_<not T::is_optional::value>;

  template <typename T>
  using _is_optional = typename T::is_optional;

  static constexpr auto first_optional_desc_idx = tinympl::find_if_t<
    _arg_desc_vector, _is_optional
  >::value;

  static constexpr auto no_optional_descs_given =
    first_optional_desc_idx == _arg_desc_vector::size;

  static constexpr auto last_required_desc_idx = tinympl::find_last_if_t<
    _arg_desc_vector, _is_required
  >::value;

  static constexpr auto no_required_descs_given =
    last_required_desc_idx == _arg_desc_vector::size;

  static_assert(
    no_optional_descs_given or no_required_descs_given
      or last_required_desc_idx < first_optional_desc_idx,
    "Optional argument description given after last required argument"
  );

  // </editor-fold> end
  //============================================================================


  //============================================================================
  // <editor-fold desc="Pull out the required and optional arguments">

  using _required_arg_descs =
    typename _arg_desc_vector::template erase_if_not_t<_is_required>;

  using _optional_arg_descs =
    typename _arg_desc_vector::template erase_if_not_t<_is_optional>;

  // </editor-fold> end Pull out the required and optional arguments
  //============================================================================


  template <size_t ToAdd>
  struct _make_add_to_mfn {
    template <typename WrappedIndex>
    struct apply {
      using type = std::integral_constant<
        typename WrappedIndex::value_type,
        WrappedIndex::value + ToAdd
      >;
    };
  };
  template <typename ArgsVector, size_t offset=0>
  using _make_args_with_indices = typename tinympl::zip<
    tinympl::vector, _impl::arg_with_index,
    tinympl::transform_t<
      std::make_index_sequence<ArgsVector::size>,
      _make_add_to_mfn<offset>::template apply
    >,
    ArgsVector
  >::type;

  template <typename... Args>
  using _helper = _impl::_overload_desc_is_valid_impl<
    _make_args_with_indices<tinympl::vector<Args...>>,
    _make_args_with_indices<_required_arg_descs>,
    _make_args_with_indices<_optional_arg_descs, _required_arg_descs::size>,
    AllowVariadics
  >;

  template <typename... Args>
  using is_valid_for_args = typename _helper<Args...>::type;

  template <
    typename DefaultKWArgsTuple,
    typename ConvertersTuple,
    typename Callable,
    typename ForwardedArgsTuple,
    typename Helper,
    size_t... Idxs
  >
  decltype(auto)
  _invoke_impl(
    DefaultKWArgsTuple&& defaults,
    ConvertersTuple&& converters,
    Callable&& C,
    std::integer_sequence<size_t, Idxs...>,
    Helper h,
    ForwardedArgsTuple&& tup
  ) const {
    return std::forward<Callable>(C)(
      h.template get_invoke_arg<Idxs>(
        std::forward<DefaultKWArgsTuple>(defaults),
        std::forward<ConvertersTuple>(converters),
        std::forward<ForwardedArgsTuple>(tup)
      )...
    );
  };

  using _pretty_printed_error_t =
    _darma__errors::_____overload_candidate_with__<
      typename ArgumentDescriptions::_pretty_printed_error_t...
    >;

};

} // end namespace _impl

template <typename... ArgumentDescriptions>
struct overload_description {

  using var_helper_t =
    _impl::_overload_description_maybe_variadic<false, ArgumentDescriptions...>;

  template <
    typename DefaultKWArgsTuple,
    typename ConvertersTuple,
    typename Callable,
    typename ArgsTuple
  >
  decltype(auto)
  invoke(
    DefaultKWArgsTuple&& defaults,
    ConvertersTuple&& converters,
    Callable&& C,
    ArgsTuple&& args
  ) const {
    using helper_t = tinympl::splat_to_t<ArgsTuple, var_helper_t::template _helper>;
    return var_helper_t()._invoke_impl(
      std::forward<DefaultKWArgsTuple>(defaults),
      std::forward<ConvertersTuple>(converters),
      std::forward<Callable>(C),
      std::make_index_sequence<
        helper_t::ArgDescsWithIdxs::size + helper_t::OptionalArgDescsWithIdxs::size
      >(),
      helper_t{},
      std::forward<ArgsTuple>(args)
    );
  };

  using _pretty_printed_error_t = typename var_helper_t::_pretty_printed_error_t;

};

template <typename... ArgumentDescriptions>
struct variadic_positional_overload_description {

  using var_helper_t =
    _impl::_overload_description_maybe_variadic<true, ArgumentDescriptions...>;

  template <
    typename DefaultKWArgsTuple,
    typename ConvertersTuple,
    typename Callable,
    typename ArgsTuple
  >
  decltype(auto)
  invoke(
    DefaultKWArgsTuple&& defaults,
    ConvertersTuple&& converters,
    Callable&& C,
    ArgsTuple&& args
  ) const {
    using helper_t = tinympl::splat_to_t<ArgsTuple, var_helper_t::template _helper>;
    return var_helper_t()._invoke_impl(
      std::forward<DefaultKWArgsTuple>(defaults),
      std::forward<ConvertersTuple>(converters),
      std::forward<Callable>(C),
      std::make_index_sequence<
        helper_t::ArgDescsWithIdxs::size + helper_t::OptionalArgDescsWithIdxs::size
        + helper_t::n_variadic_given + 1
      >(),
      helper_t{},
      std::forward<ArgsTuple>(args)
    );
  };

  using _pretty_printed_error_t = typename var_helper_t::_pretty_printed_error_t;

};

template <typename... OverloadDescriptions>
struct kwarg_parser {
  private:

    template <typename... Args>
    struct _make_overload_is_valid {
      template <typename Overload>
      using apply = typename Overload::var_helper_t::template is_valid_for_args<Args...>;
    };

    // readability
    template <bool cond, typename A, typename B>
    using ______see_calling_context_below_____ = typename std::conditional_t<cond, A, B>::type;

    template <typename Arg, typename Enable=void>
    struct readable_argument_description {
      using type = _darma__errors::____positional_argument____<Arg>;
    };
    template <typename Arg>
    struct readable_argument_description<
      Arg, std::enable_if_t<is_kwarg_expression<std::decay_t<Arg>>::value>
    > {
      using type = typename _darma__errors::____keyword_argument____<typename Arg::tag>
        ::template _equals_<typename Arg::argument_type>;
    };


  public:

    template <typename... Args>
    using invocation_is_valid = typename tinympl::variadic::any_of<
      _make_overload_is_valid<Args...>::template apply, OverloadDescriptions...
    >::type;

    template <typename... Args>
    using _valid_overload_desc_t = tinympl::variadic::at_t<
      tinympl::variadic::find_if<
        _make_overload_is_valid<Args...>::template apply,
        OverloadDescriptions...
      >::value,
      OverloadDescriptions...
    >;

    template <typename... Args>
    using static_assert_valid_invocation = decltype(
      ______see_calling_context_below_____<
        invocation_is_valid<Args...>::value,
        tinympl::identity<int>,
        _darma__static_failure<
          _____________________________________________________________________,
          _____________________________________________________________________,
          _darma__errors::__________no_matching_function_call_with__<
            typename readable_argument_description<Args>::type...
          >,
          _____________________________________________________________________,
          _____________________________________________________________________,
          _darma__errors::__________candidate_overloads_are__,
          typename OverloadDescriptions::_pretty_printed_error_t...,
          _____________________________________________________________________,
          _____________________________________________________________________
        >
      >());

    template <
      typename DefaultsTuple,
      typename ConvertersTuple,
      typename ArgsTuple,
      typename OverloadDesc
    >
    struct _parsed_invoke {
      DefaultsTuple def_tup;
      ConvertersTuple convs_tup;
      ArgsTuple args_tup;

      _parsed_invoke(
        DefaultsTuple&& indef_tup,
        ConvertersTuple&& inconvs_tup,
        ArgsTuple&& inargs_tup
      ) : def_tup(std::forward<DefaultsTuple>(indef_tup)),
          convs_tup(std::forward<ConvertersTuple>(inconvs_tup)),
          args_tup(std::forward<ArgsTuple>(inargs_tup))
      { }

      template <typename Callable>
      decltype(auto)
      invoke(
        Callable&& callable
      ) && {
        return OverloadDesc().invoke(
          std::move(def_tup),
          std::move(convs_tup),
          std::forward<Callable>(callable),
          std::move(args_tup)
        );
      }
    };



    template <typename DefaultsTuple, typename ConvertersTuple=std::tuple<>>
    struct _with_defaults_invoker;

    template <typename ConvertersTuple, typename DefaultsTuple=std::tuple<>>
    struct _with_converters_invoker {
      ConvertersTuple conv_tup;
      DefaultsTuple def_tup;
      template <typename _Ignored=void,
        typename=std::enable_if_t<
          std::is_void<_Ignored>::value and std::is_same<DefaultsTuple, std::tuple<>>::value
        >
      >
      explicit _with_converters_invoker(ConvertersTuple&& tup)
        : conv_tup(std::forward<ConvertersTuple>(tup)),
          def_tup()
      { }
      explicit _with_converters_invoker(ConvertersTuple&& tup, DefaultsTuple&& defs)
        : conv_tup(std::forward<ConvertersTuple>(tup)),
          def_tup(std::forward<DefaultsTuple>(defs))
      { }

      template <typename... DefaultKWArgs>
      auto
      with_defaults(DefaultKWArgs&&... dkws) && {
        static_assert(std::is_same<DefaultsTuple, std::tuple<>>::value, "internal error");
        return _with_defaults_invoker<
          decltype(std::forward_as_tuple(std::forward<DefaultKWArgs>(dkws)...)),
          ConvertersTuple
        >(
          std::forward_as_tuple(std::forward<DefaultKWArgs>(dkws)...),
          std::move(conv_tup)
        );
      }

      template <typename Callable, typename... Args>
      decltype(auto)
      invoke(Callable&& C, Args&&... args) && {
        return _parsed_invoke<
          DefaultsTuple, ConvertersTuple,
          decltype(std::forward_as_tuple(std::forward<Args>(args)...)),
          _valid_overload_desc_t<Args...>
        >(
          std::move(def_tup),
          std::move(conv_tup),
          std::forward_as_tuple(std::forward<Args>(args)...)
        ).invoke(
          std::forward<Callable>(C)
        );
      };

      template <typename... Args>
      auto
      parse_args(Args&&... args) {
        return _parsed_invoke<
          DefaultsTuple, ConvertersTuple,
          decltype(std::forward_as_tuple(std::forward<Args>(args)...)),
          _valid_overload_desc_t<Args...>
        >(
          std::move(def_tup),
          std::move(conv_tup),
          std::forward_as_tuple(std::forward<Args>(args)...)
        );
      }

    };

    template <typename DefaultsTuple, typename ConvertersTuple /*=std::tuple<>*/>
    struct _with_defaults_invoker {
      DefaultsTuple def_tup;
      ConvertersTuple conv_tup;
      template <typename _Ignored=void,
        typename=std::enable_if_t<
          std::is_void<_Ignored>::value and std::is_same<ConvertersTuple, std::tuple<>>::value
        >
      >
      explicit _with_defaults_invoker(DefaultsTuple&& tup)
        : def_tup(std::forward<DefaultsTuple>(tup)), conv_tup()
      { }
      explicit _with_defaults_invoker(DefaultsTuple&& defs, ConvertersTuple&& tup)
        : conv_tup(std::forward<ConvertersTuple>(tup)),
          def_tup(std::forward<DefaultsTuple>(defs))
      { }

      template <typename... Converters>
      auto
      with_converters(Converters&&... convs) && {
        static_assert(std::is_same<ConvertersTuple, std::tuple<>>::value, "internal error");
        return _with_converters_invoker<
          decltype(std::forward_as_tuple(std::forward<Converters>(convs)...)),
          DefaultsTuple
        >(
          std::forward_as_tuple(std::forward<Converters>(convs)...),
          std::move(def_tup)
        );
      }

      template <typename Callable, typename... Args>
      decltype(auto)
      invoke(Callable&& C, Args&&... args) && {
        return _parsed_invoke<
          DefaultsTuple, ConvertersTuple,
          decltype(std::forward_as_tuple(std::forward<Args>(args)...)),
          _valid_overload_desc_t<Args...>
        >(
          std::move(def_tup),
          std::move(conv_tup),
          std::forward_as_tuple(std::forward<Args>(args)...)
        ).invoke(
          std::forward<Callable>(C)
        );
      }

      template <typename... Args>
      auto
      parse_args(Args&&... args) {
        return _parsed_invoke<
          DefaultsTuple, ConvertersTuple,
          decltype(std::forward_as_tuple(std::forward<Args>(args)...)),
          _valid_overload_desc_t<Args...>
        >(
          std::move(def_tup),
          std::move(conv_tup),
          std::forward_as_tuple(std::forward<Args>(args)...)
        );
      }

    };



    template <typename... DefaultKWArgs>
    auto
    with_defaults(
      DefaultKWArgs&&... dkws
    ) const {
      return _with_defaults_invoker<
        decltype(std::forward_as_tuple(std::forward<DefaultKWArgs>(dkws)...)),
        std::tuple<>
      >(
        std::forward_as_tuple(std::forward<DefaultKWArgs>(dkws)...)
      );
    }

    template <typename... Converters>
    auto
    with_converters(
      Converters&&... convs
    ) const {
      return _with_converters_invoker<
        decltype(std::forward_as_tuple(std::forward<Converters>(convs)...)),
        std::tuple<>
      >(
        std::forward_as_tuple(std::forward<Converters>(convs)...)
      );
    }

    template <typename Callable, typename... Args>
    decltype(auto)
    invoke(Callable&& C, Args&&... args) const {
      return with_defaults().invoke(
        std::forward<Callable>(C),
        std::forward<Args>(args)...
      );
    }

};

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_KEYWORD_ARGUMENTS_PARSE_H
