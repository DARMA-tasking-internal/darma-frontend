/*
//@HEADER
// ************************************************************************
//
//                      range_1d.h
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

#ifndef DARMA_IMPL_INDEX_RANGE_RANGE_1D_H
#define DARMA_IMPL_INDEX_RANGE_RANGE_1D_H

#include <tinympl/bool.hpp>
#include <tinympl/variadic/count_if.hpp>

#include <darma/impl/meta/detection.h>

#include <darma/indexing/abstract/index_range.h>

#include <darma/serialization/polymorphic/polymorphic_serialization_adapter.h>
#include <darma/impl/meta/parse_properties.h>

namespace darma_runtime {

namespace indexing {

namespace integer_range_1d_options {

template <bool value=true>
struct use_stateless_mapping_to_dense {
  using use_stateless_mapping_to_dense_option_type = tinympl::bool_<value>;
  static constexpr auto use_stateless_mapping_to_dense_option_value = value;
};

} // end namespace integer_range_1d_options

//==============================================================================

namespace detail {

template <typename... Properties>
struct basic_integer_range_1d_property_parser {

  //==============================================================================
  // <editor-fold desc="use_stateless_mapping_to_dense"> {{{1

  private:

    template <typename Property>
    using _check_use_stateless_mapping_to_dense_archetype =
      typename Property::use_stateless_mapping_to_dense_property_type;

    using _use_stateless_mapping_to_dense_parser =
      meta::boolean_template_property_parser<
        _check_use_stateless_mapping_to_dense_archetype,
        /* default = */ false
      >;

  public:

    static constexpr auto use_stateless_mapping_to_dense =
      _use_stateless_mapping_to_dense_parser::template parse<Properties...>::value;

  // </editor-fold> end  }}}1
  //==============================================================================

  //==============================================================================
  // <editor-fold desc="check that all properties are accounted for"> {{{1

  private:

    using _checker = meta::property_parsing_checker<
      _use_stateless_mapping_to_dense_parser
    >;

  public:

    using check_all_properties_known = typename _checker
      ::template assert_all_properties_known<Properties...>::type;


  // </editor-fold> end  }}}1
  //==============================================================================

};

template <typename Integer>
struct basic_integer_index_1d {

};

template <
  typename Integer,
  typename... Properties
>
struct basic_integer_range_1d
  : serialization::PolymorphicSerializationAdapter<
      basic_integer_range_1d<Integer>,
      abstract::frontend::IndexRange
    >
{

  private:

    Integer size_;
    Integer offset_;

    using _properties = basic_integer_range_1d_property_parser<Properties...>;

    using _check_properties = typename _properties::check_all_properties_known;

    static constexpr auto use_stateless_mapping_to_dense =
      _properties::use_stateless_mapping_to_dense;

    // TODO add stride
    //Integer stride_;

    // TODO stateless/stateful mapping to dense
    // TODO simpler index property (i.e., yields just Integer instead)

    basic_integer_range_1d() = delete;

  public:

    using index_type = basic_integer_index_1d<Integer>;
    using is_index_range = std::true_type;

    explicit
    basic_integer_range_1d(Integer size)
      : size_(size), offset_(0)
    { }

    basic_integer_range_1d(Integer begin, Integer end)
      : size_(end - begin), offset_(begin)
    { }


    // TODO finish this

    // Allow serialization without default constructor
    template <typename Archive>
    basic_integer_range_1d&
    reconstruct(void* allocated, Archive&) {
      auto* rv = new (allocated) basic_integer_range_1d(0);
      return *rv;
    }

    template <typename Archive>
    void serialize(Archive& ar) {
      ar | size_ | offset_;
    }

    size_t size() const override { return size_; }

};

} // end namespace detail

using index_range_1d = detail::basic_integer_range_1d<int64_t>;

using index_1d = detail::basic_integer_index_1d<int64_t>;

} // end namespace indexing


} // end namespace darma_runtime

#endif //DARMA_IMPL_INDEX_RANGE_RANGE_1D_H
