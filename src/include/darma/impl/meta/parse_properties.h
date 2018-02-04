/*
//@HEADER
// ************************************************************************
//
//                      parse_properties.h
//                         DARMA
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

#ifndef DARMA_IMPL_META_PARSE_PROPERTIES_H
#define DARMA_IMPL_META_PARSE_PROPERTIES_H

#include <tinympl/variadic/count_if.hpp>
#include <tinympl/variadic/find_if.hpp>
#include <tinympl/variadic/at.hpp>
#include <tinympl/bool.hpp>

#include <darma/impl/meta/detection.h>

namespace darma_runtime {

namespace meta {

template <
  template <class> class PropertyArchetype,
  bool DefaultValue,
  bool Required = false,
  bool RequireUnique = true
>
struct boolean_template_property_parser {

  private:

    static_assert(RequireUnique, "Non-unique property parsing not yet implemented");

    template <typename Property>
    using _check_property_given = is_detected_value_t<
      PropertyArchetype, Property
    >;

    template <typename Property>
    using _check_property_type = detected_t<
      PropertyArchetype, Property
    >;

  public:

    template <typename... Properties>
    struct parse {
      private:
        static constexpr auto  _num_matching_properties_given =
          tinympl::variadic::count_if<_check_property_given, Properties...>::type::value;

        static_assert(
          not RequireUnique or _num_matching_properties_given <= 1,
          "Multiple values given for template property that is required to be unique"
        );

        static_assert(
          not Required or _num_matching_properties_given >= 1,
          "Missing required template property that is required to be unique"
        );

        static constexpr auto _property_index = tinympl::variadic::find_if<
          _check_property_given, Properties...
        >::type::value;

      public:

        static constexpr auto given = _num_matching_properties_given >= 1;

        static constexpr auto value = std::conditional_t<
          _property_index == sizeof...(Properties),
          tinympl::bool_<DefaultValue>,
          _check_property_type<
            tinympl::variadic::at_or_t<nonesuch, // ignored, out-of-bounds safety only
              _property_index,
              Properties...
            >
          >
        >::value;

        using type = tinympl::bool_<value>;

    };

};


template <typename... Parsers>
struct property_parsing_checker {

  template <typename... Properties>
  struct assert_all_properties_known {

    template <typename Property>
    struct _prop_given {
      template <typename Parser>
      using apply = tinympl::bool_<
        Parser::template parse<Property>::given
      >;
    };

    template <typename Property>
    using _check_given = tinympl::variadic::any_of<
      _prop_given<Property>::template apply, Parsers...
    >;

    static constexpr auto all_properties_known = tinympl::variadic::all_of<
      _check_given, Properties...
    >::type::value;

    static_assert(all_properties_known,
      "Unknown template property given to template with property parameters"
    );

    using type = tinympl::bool_<all_properties_known>;
  };

};

} // end namespace meta

} // end namespace darma_runtime

#endif //DARMA_IMPL_META_PARSE_PROPERTIES_H
