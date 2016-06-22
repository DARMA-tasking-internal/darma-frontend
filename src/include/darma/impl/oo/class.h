/*
//@HEADER
// ************************************************************************
//
//                      class.h
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

#ifndef DARMA_IMPL_OO_CLASS_H
#define DARMA_IMPL_OO_CLASS_H

#include <type_traits>

#include <tinympl/vector.hpp>
#include <tinympl/is_instantiation_of.hpp>
#include <tinympl/at.hpp>
#include <tinympl/find_all_if.hpp>
#include <tinympl/left_fold.hpp>
#include <tinympl/partition.hpp>
#include <tinympl/lambda.hpp>

#include <darma/impl/oo/field.h>
#include <darma/impl/oo/util.h>

#include <darma/interface/app/access_handle.h>

namespace darma_runtime {
namespace oo {

template <typename... Args>
struct public_methods {
  using args_vector_t = tinympl::vector<Args...>;
};

namespace detail {

template <typename OfClass, typename Tag, typename CastThisTo=OfClass>
struct _public_method_in_chain {
  using tag = Tag;
  template <typename Base>
  using link_in_chain = typename tag::template as_public_method_in_chain<
    OfClass, Base, CastThisTo
  >;
};

template <typename OfClass, typename Tag, typename CastThisTo=OfClass>
struct _immediate_public_method_in_chain {
  using tag = Tag;
  template <typename Base>
  using link_in_chain = typename tag::template as_immediate_public_method_in_chain<
    OfClass, Base, CastThisTo
  >;
};

template <typename OfClass, typename Tag>
struct _public_method_with_tag {
  using tag = Tag;
  using of_class = OfClass;
  template <typename CastTo>
  using chain_item_with_cast_to = _public_method_in_chain<OfClass, Tag, CastTo>;
  template <typename CastTo>
  using immediate_chain_item_with_cast_to = _immediate_public_method_in_chain<OfClass, Tag, CastTo>;
};

template <typename ClassName, typename... Args>
struct darma_class_helper {

  using _args_vector_t = tinympl::vector<Args...>;
  static constexpr auto n_args = sizeof...(Args);

  template <template <class...> class template_tag>
  using _template_tag_index_list = typename tinympl::find_all_if<
    _args_vector_t,
    tinympl::make_is_instantiation_of<template_tag>::template apply_t
  >::type;

  template <typename int_const>
  using _get_template_tag_arg = tinympl::identity<
    typename tinympl::extract_args<
      typename tinympl::variadic::at<int_const::value, Args...>::type
    >::template rebind<tinympl::vector>
  >;

  template <template <class...> class template_tag>
  using _template_tag_args_list = typename tinympl::transform<
    _template_tag_index_list<template_tag>,
    _get_template_tag_arg, tinympl::vector
  >::type;

  template <template <class...> class template_tag>
  using _template_tag_args_vector_joined = typename tinympl::left_fold<
    // prepend two empty lists so that the fold still works even if the args list is empty
    typename tinympl::push_front<typename tinympl::push_front<
      _template_tag_args_list<template_tag>,
      tinympl::vector<>>::type, tinympl::vector<>
    >::type,
    tinympl::join
  >::type;

  ////////////////////////////////////////////////////////////////////////////////
  // <editor-fold desc="Extract private_fields and public_fields list(s)">

  template <template <class...> class RewrapWith>
  struct _make_access_handle_replacer {
    template <typename field_in_chain>
    using apply = tinympl::identity<RewrapWith<
      darma_runtime::AccessHandle<typename field_in_chain::type>,
      typename field_in_chain::tag
    >>;
  };

  using _private_fields_vector = typename tinympl::transform<
    typename tinympl::partition<
      2, _template_tag_args_vector_joined<private_fields>,
      _private_field_in_chain, tinympl::vector
    >::type,
    _make_access_handle_replacer<_private_field_in_chain>::template apply
  >::type;

  using _public_fields_vector = typename tinympl::transform<
    typename tinympl::partition<
      2, _template_tag_args_vector_joined<public_fields>,
      _public_field_in_chain, tinympl::vector
    >::type,
    _make_access_handle_replacer<_public_field_in_chain>::template apply
  >::type;

  using fields_with_types = typename tinympl::join<
    typename tinympl::partition<
      2, _template_tag_args_vector_joined<public_fields>,
      _field_tag_with_type, tinympl::vector
    >::type,
    typename tinympl::partition<
      2, _template_tag_args_vector_joined<private_fields>,
      _field_tag_with_type, tinympl::vector
    >::type
  >::type;


  // </editor-fold>
  ////////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////////
  // <editor-fold desc="Extract public_methods list(s)">

  template <typename Tag>
  using _make_public_method_chain_part = tinympl::identity<
    _public_method_in_chain<ClassName, Tag>
  >;

  using _public_methods_base_class_vector = typename tinympl::transform<
    _template_tag_args_vector_joined<public_methods>,
    _make_public_method_chain_part
  >::type;

  template <typename Tag>
  using _make_public_method_tags_part = tinympl::identity<
    _public_method_with_tag<ClassName, Tag>
  >;

  using public_method_tags = typename tinympl::transform<
    _template_tag_args_vector_joined<public_methods>,
    _make_public_method_tags_part
  >::type;

  // </editor-fold>
  ////////////////////////////////////////////////////////////////////////////////

  using base_class = typename chain_base_classes<
    typename tinympl::join<
      _private_fields_vector,
      _public_fields_vector,
      _public_methods_base_class_vector
    >::type
  >::type;

};

} // end namespace detail


template <typename ClassName, typename... Args>
struct darma_class
  : detail::darma_class_helper<ClassName, Args...>::base_class
{
  private:

  public:

    using helper_t = detail::darma_class_helper<ClassName, Args...>;

    darma_class() = default;

};

} // end namespace oo
} // end namespace darma_runtime

#endif //DARMA_IMPL_OO_CLASS_H
