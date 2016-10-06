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
};

template <typename ArgsWithIdxs, typename ArgDescsWithIdxs>
struct _overload_desc_is_valid_impl {
  private:
    template <typename pair>
    using _is_positional_description = typename pair::type::can_be_positional;

    template <typename descpair>
    struct _make_kwarg_tag_matches {
      template <typename argpair>
      using apply = std::is_same<typename descpair::tag,
        typename is_kwarg_expression<typename argpair::type>::tag
      >;
    };

    template <typename descpair>
    using _matching_kwarg_given = tinympl::any_of<
      ArgsWithIdxs, _make_kwarg_tag_matches<descpair>::template apply
    >;

    template <typename pair>
    using _desc_for = tinympl::at_t<pair::index, ArgDescsWithIdxs>;

    template <typename pair>
    using _is_positional_arg = tinympl::bool_<not is_kwarg_expression<
      typename pair::type
    >::value>;
    template <typename pair>
    using _is_keyword_arg = tinympl::bool_<is_kwarg_expression<typename pair::type>::value>;

    static constexpr auto try_last_allowed_positional = tinympl::find_last_if<
      ArgDescsWithIdxs, _is_positional_description
    >::value;

    static constexpr auto arg_count = ArgDescsWithIdxs::size::value;

    static constexpr auto positional_args_end =
      (try_last_allowed_positional == arg_count) ? 0 : try_last_allowed_positional + 1;

    template <typename WrappedEndIndex>
    using given_kwargs_consumed_before_position = typename tinympl::count_if<
      typename ArgDescsWithIdxs::template erase<WrappedEndIndex::index, arg_count>::type,
      _matching_kwarg_given
    >::type;


    template <typename ArgWithIdx>
    using arg_is_valid = tinympl::select_first_t<
      // if it's a positional argument after the last allowed positional argument,
      // it's not valid
      tinympl::bool_<
        (_is_positional_arg<ArgWithIdx>::value)
          and (ArgWithIdx::index >=
            positional_args_end - given_kwargs_consumed_before_position<typename ArgWithIdx::index_t>::value
          )
      >, /* => */ std::false_type,
      // if it's positional and not off the end, it has to be compatible with
      // the corresponding positional argument description
      _is_positional_arg<ArgWithIdx>,
        /* => */ typename tinympl::at_or_t<positional_only_argument<meta::nonesuch>,
          ArgWithIdx::index + given_kwargs_consumed_before_position<typename ArgWithIdx::index_t>::value,
          ArgDescsWithIdxs
        >::template argument_is_compatible<ArgWithIdx>
      // if it's a keyword argument
        // TODO FINISH THIS
    >;

    // TODO finish this

  public:



};



} // end namespace _impl

template <typename... ArgumentDescriptions>
struct overload_description {
  private:

    template <typename... Args>
    using _make_args_with_indices = typename tinympl::zip<
      tinympl::vector, _impl::arg_with_index,
      std::index_sequence_for<Args...>,
      tinympl::vector<Args...>
    >::type;

  public:

    template <typename... Args>
    using is_valid_for_args = std::conditional_t<
      sizeof...(Args) != sizeof...(ArgumentDescriptions),
      std::false_type,
      _impl::_overload_desc_is_valid_impl<
        _make_args_with_indices<Args...>,
        _make_args_with_indices<ArgumentDescriptions...>
      >
    >;



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

};


} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_KEYWORD_ARGUMENTS_PARSE_H
