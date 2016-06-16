/*
//@HEADER
// ************************************************************************
//
//                      method.h
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

#ifndef DARMA_IMPL_OO_METHOD_H
#define DARMA_IMPL_OO_METHOD_H

#include <tinympl/vector.hpp>
#include <tinympl/find_all_if.hpp>

#include <darma/impl/oo/method.h>

namespace darma_runtime {


namespace oo {

template <typename Tag, typename... Args>
struct reads_ {
  using tag = Tag;
  using args_vector_t = tinympl::vector<Args...>;
};

template <typename Tag, typename... Args>
struct reads_value_ {
  using tag = Tag;
  using args_vector_t = tinympl::vector<Args...>;
};

template <typename Tag, typename... Args>
struct modifies_ {
  using tag = Tag;
  using args_vector_t = tinympl::vector<Args...>;
};

template <typename Tag, typename... Args>
struct modifies_value_ {
  using tag = Tag;
  using args_vector_t = tinympl::vector<Args...>;
};

namespace detail {

template <typename OfClass, typename... Args>
struct darma_method_helper {

  using _args_vector_t = tinympl::vector<Args...>;

  template <typename T>
  using _get_tag = tinympl::identity<typename T::tag>;

  template <typename int_const>
  using _arg_at = tinympl::at<int_const::value, _args_vector_t>;

  template <template <class...> class decorator>
  struct decorated_with {
    using indices = typename tinympl::find_all_if<
      _args_vector_t,
      tinympl::make_is_instantiation_of<decorator>::template apply_t
    >::type;
    using decorated_vector = typename tinympl::transform<
      indices, _arg_at, tinympl::vector>::type;

    using tag_vector = typename tinympl::transform<decorated_vector, _get_tag>::type;

    template <typename tag>
    struct make_is_field_tag {
      template <typename T>
      using apply = std::is_same<typename T::tag, tag>;
    };

    template <typename tag>
    using _get_type_for_tag = tinympl::identity< typename tinympl::at_t<
      tinympl::find_if<
        typename OfClass::darma_class::helper_t::fields_with_types,
        make_is_field_tag<tag>::template apply
      >::type::value,
      typename OfClass::darma_class::helper_t::fields_with_types
    >::value_type >;

    using type_vector = typename tinympl::transform<tag_vector,
      _get_type_for_tag
    >::type;

    template <
      template <class...> class transform_type,
      template <class...> class wrapper_template
    >
    struct _make_get_wrapped_type_tag_pair {
      template <typename int_const>
      using apply = tinympl::identity<
        wrapper_template<
          typename transform_type<tinympl::at_t<int_const::value, type_vector>>::type,
          tinympl::at_t<int_const::value, tag_vector>
        >
      >;
    };

    template <
      template <class...> class transform_type,
      template <class...> class wrapper_template
    >
    using chained_base_classes = typename tinympl::transform<
      typename tinympl::make_range_c<size_t, 0, tinympl::size<tag_vector>::value>::type,
      _make_get_wrapped_type_tag_pair<transform_type, wrapper_template>::template apply,
      tinympl::vector
    >::type;

  };


  template <typename T>
  using _as_read_only_access_handle = tinympl::identity<darma_runtime::ReadAccessHandle<T>>;

  using _reads_base_classes = typename decorated_with<reads_>::template chained_base_classes<
    _as_read_only_access_handle, _private_field_in_chain
  >;

  template <typename T>
  using _as_access_handle = tinympl::identity<darma_runtime::AccessHandle<T>>;

  using _modifies_base_classes = typename decorated_with<modifies_>::template chained_base_classes<
    _as_access_handle, _private_field_in_chain
  >;

  template <typename T>
  using _as_const_ref = std::add_lvalue_reference<std::add_const_t<T>>;

  using _reads_value_base_classes = typename decorated_with<reads_value_>::template chained_base_classes<
    _as_const_ref, _private_field_in_chain
  >;

  template <typename T>
  using _as_nonconst_ref = std::add_lvalue_reference<std::remove_const_t<T>>;

  using _modifies_value_base_classes = typename decorated_with<modifies_value_>::template chained_base_classes<
    _as_nonconst_ref, _private_field_in_chain
  >;

  using base_class = typename chain_base_classes<
    typename tinympl::join<
      _reads_base_classes, _modifies_base_classes,
      _reads_value_base_classes, _modifies_value_base_classes
    >::type
  >::type;

};

} // end namespace detail

template <typename OfClass,
  typename... Args
>
struct darma_method
  : detail::darma_method_helper<OfClass, Args...>::base_class
{
  public:
    using helper_t = typename detail::darma_method_helper<OfClass, Args...>;
    using base_t = typename helper_t::base_class;

    // Explicitly default the copy, move, and default constructors
    constexpr inline darma_method() = default;
    constexpr inline darma_method(darma_method const& val) = default;
    constexpr inline darma_method(darma_method&& val) = default;

    // Allow construction from the class that this is a method of
    template <typename OfClassDeduced,
      typename = std::enable_if_t<
        std::is_convertible<OfClassDeduced, OfClass>::value
      >
    >
    constexpr inline explicit
    darma_method(OfClassDeduced&& val)
      : base_t(std::forward<OfClassDeduced>(val))
    { }

};

} // end namespace oo

} // end namespace darma_runtime

#endif //DARMA_IMPL_OO_METHOD_H
