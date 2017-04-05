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

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(oo_interface)

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
#include <darma/impl/oo/constructor.h>
#include <darma/impl/oo/errors.h>

#include <darma/interface/app/access_handle.h>

namespace darma_runtime {
namespace oo {

namespace detail {

namespace _impl {

template <typename T>
using _is_darma_class_archetype = decltype(
  _darma__is_darma_class_ADL_helper(std::declval<T&>())
);

template <typename T, typename Context>
using _is_complete_darma_class_from_archetype = typename T::_darma__this_is_a_complete_darma_class;

} // end namespace _impl

// Note that using this metafunction from multiple places with the same Context will
// alway return the value retrieved from the first one!!!
template <typename T, typename Context>
using is_complete_darma_class_from_context = meta::detected_or_t<std::false_type,
  _impl::_is_complete_darma_class_from_archetype, T, Context
>;

} // end namespace detail

template <typename T>
using is_darma_class = meta::detected_or_t<std::false_type,
  detail::_impl::_is_darma_class_archetype, T
>;



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
  using immediate_chain_item_with_cast_to = _immediate_public_method_in_chain<
    OfClass,
    Tag,
    CastTo
  >;
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
    typename tinympl::push_front<
      typename tinympl::push_front<
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
    using apply = tinympl::identity<
      RewrapWith<
        std::conditional_t<
          is_darma_class<typename field_in_chain::type>::value,
          typename field_in_chain::type,
          darma_runtime::AccessHandle<typename field_in_chain::type>
        >,
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

template <typename T>
struct darma_class_instance_delayed {
  using type = std::decay_t<
    decltype(
      _darma__get_associated_constructor(std::declval<T&>())
    )
  >;
};

} // end namespace detail

template <typename T>
using darma_class_instance = typename detail::darma_class_instance_delayed<T>::type;

template <typename ClassName, typename... Args>
struct darma_class
  : detail::darma_class_helper<ClassName, Args...>::base_class
{
  private:

    template <typename T, typename... _Ignored_SFINAE>
    using _constructor_implementation_type = std::remove_reference_t<decltype(
      _darma__get_associated_constructor(std::declval<T&>())
    )>;

    // Clang needs this extra layer of abstraction to make SFINAE work
    // since ClassName##_constructors could be an incomplete class
    template <typename _Tp, typename... _Ignored_SFINAE>
    struct _ctor_impl_exists {
      using type = typename _constructor_implementation_type<_Tp, _Ignored_SFINAE...>::darma_constructor;
    };
    template <typename T, typename... _Ignored_SFINAE>
    using _constructor_implementation_exists = typename _ctor_impl_exists<T, _Ignored_SFINAE...>::type;
    template <typename T, typename... _Ignored_but_needed_for_SFINAE_to_work>
    using constructor_implementation_exists = darma_runtime::meta::is_detected<
      _constructor_implementation_exists, T, _Ignored_but_needed_for_SFINAE_to_work...
    >;

    template <typename T, typename... _Ignored_but_needed_for_SFINAE_to_work>
    using constructor_implementation_type = tinympl::identity<
      darma_runtime::meta::detected_t<
        _constructor_implementation_type, T, _Ignored_but_needed_for_SFINAE_to_work...
      >
    >;

    template <typename T, typename... CTorArgs>
    using _constructor_implementation_callable = decltype(
      std::declval<typename constructor_implementation_type<T, CTorArgs...>::type>()(
        std::declval<CTorArgs>()...
      )
    );
    template <typename T, typename... CTorArgs>
    using constructor_implementation_callable = tinympl::value_identity<
      darma_runtime::meta::is_detected<
        _constructor_implementation_callable, T, CTorArgs...
      >
    >;

    //template <typename T, typename... IgnoredSFINAE>
    //using _default_constructor_implementation_callable = decltype(
    //  std::declval<
    //    typename constructor_implementation_type<T, IgnoredSFINAE...>::type
    //  >().operator()()
    //);
    //template <typename T, typename... IgnoredSFINAE>
    //using default_constructor_implementation_callable = tinympl::value_identity<
    //  darma_runtime::meta::is_detected<
    //    _default_constructor_implementation_callable, T, IgnoredSFINAE...
    //  >
    //>;

  public:

    using _darma__this_is_a_complete_darma_class = std::true_type;

    using helper_t = detail::darma_class_helper<ClassName, Args...>;
    using base_t = typename helper_t::base_class;
    using darma_class_t = darma_class;

    darma_class() = default;
    darma_class(darma_class&&) = default;
    darma_class(darma_class const&) = default;
    darma_class& operator=(darma_class&&) = default;
    darma_class& operator=(darma_class const&) = default;

    // Only used in detection, so shouldn't be implemented
    darma_class(detail::oo_sentinel_value_t const&);

    // Forward to a constructor, if available
    template <typename... CTorArgs,
      typename = std::enable_if_t<
        constructor_implementation_callable<ClassName, CTorArgs...>::value
        and sizeof...(CTorArgs) != 0
        and not std::is_same<
          std::decay_t<tinympl::variadic::at_or_t<meta::nonesuch, 0, CTorArgs...>>,
          ClassName
        >::value
        and not std::is_same<
          std::decay_t<tinympl::variadic::at_or_t<meta::nonesuch, 0, CTorArgs...>>,
          detail::oo_sentinel_value_t
        >::value
      >
    >
    darma_class(CTorArgs&&... args) {
      static_cast<typename constructor_implementation_type<ClassName>::type*>(this)->operator()(
        std::forward<CTorArgs>(args)...
      );
    }

    // This allows copy or move constructors
    template <typename ClassTypeDeduced,
      typename = std::enable_if_t<
        //--------------------------------------------------------------------------------
        // The messy enable_if condition below enables a non-default copy constructor
        // (with a `ClassName const&` parameter) if and only if
        //   ClassName##_constructors::operator()(darma_class_instance<ClassName> const&)
        // is defined (and similarly for the move constructor, as well as the more obscure
        // non-const reference copy constructor and the const move constructor).
        //
        // This rats' nest below is necessary to prevent infinite template recursion.  We
        // use the short-circuiting tinympl::variadic::all_of along with tinympl::delay
        // in the second argument to prevent the second condition from being evaluated
        // unless the first one evaluates to true.  Otherwise, the call to
        // _darma__get_associated_constructor() wrapped by darma_class_instance_delayed
        // re-triggers the generation of this constructor when ClassTypeDeduced
        // is darma_class<ClassType,...> because it's trying to generate the conversion
        // from darma_class<ClassName,...> to ClassName&, (where ClassName inherits
        // this constructor via `using darma_class::darma_class;`) needed to resolve
        // the argument to _darma__get_associated_constructor().  This isn't a problem
        // for the case where std::decay_t<ClassTypeDeduced> is the same as ClassName
        // because no conversion operation generation is necessary, since the argument
        // is an exact match for the formal parameter.
        tinympl::and_<  // note that this and_ is specially implemented to short-circuit
          // condition 0: ignore the sentinel value constructor used for detecting
          // the presence of "using darma_class::darma_class;" in user's implementation
          tinympl::not_<
            std::is_same<std::decay_t<ClassTypeDeduced>, detail::oo_sentinel_value_t>
          >,
          // condition 1:  only generate copy constructor if the decayed type matches the
          // Class itself (exactly; conversion from it's base classes isn't relevant here)
          std::is_same<std::decay_t<ClassTypeDeduced>, ClassName>,
          // condition 2 (only evaluated if condition 1 is true):  there has to be an
          // operator() on ClassName##_constructor which takes one argument that decays
          // to darma_class_instance<ClassName> and has the same constness and reference-ness
          // as ClassTypeDeduced&&.
          tinympl::extract_bool_value_potentially_lazy<
            tinympl::delay<
              constructor_implementation_callable,
              tinympl::identity<ClassName>,
              tinympl::delay<
                tinympl::copy_all_type_properties<ClassTypeDeduced&&>::template apply,
                detail::darma_class_instance_delayed<std::decay_t<ClassTypeDeduced>&>
              >
            >
          >
          // end of condition 2
        >::value
        // end messy rats' nest described above
        //--------------------------------------------------------------------------------
      >
    >
    darma_class(ClassTypeDeduced&& other) {
      static_cast<typename constructor_implementation_type<ClassName>::type*>(this)->operator()(
        static_cast<
          typename tinympl::copy_all_type_properties<ClassTypeDeduced>::template apply<
            typename constructor_implementation_type<ClassName>::type
          >::type
        >(std::forward<ClassTypeDeduced>(other))
      );
    }

    template <typename ArchiveT>
    constexpr inline explicit
    darma_class(
      darma_runtime::serialization::unpack_constructor_tag_t,
      ArchiveT& ar
    ) : base_t(serialization::unpack_constructor_tag, ar)
    { }

    template <typename ArchiveT>
    void compute_size(ArchiveT& ar) const {
      base_t::_darma__compute_size(ar);
    }

    template <typename ArchiveT>
    void pack(ArchiveT& ar) const {
      base_t::_darma__pack(ar);
    }

};

} // end namespace oo

namespace serialization {

namespace detail {

template <typename T>
struct Serializer_enabled_if<T, std::enable_if_t<darma_runtime::oo::is_darma_class<T>::value>> {
  template <typename ArchiveT>
  void
  compute_size(T const& darma_obj, ArchiveT& ar) const {
    static_cast<typename T::darma_class_t const&>(darma_obj).compute_size(ar);
  }
  template <typename ArchiveT>
  void
  pack(T const& darma_obj, ArchiveT& ar) const {
    static_cast<typename T::darma_class_t const&>(darma_obj).pack(ar);
  }
  template <typename ArchiveT>
  void
  unpack(void* allocated, ArchiveT& ar) const {
    new (allocated) T(unpack_constructor_tag, ar);
  }
};

} // end namespace detail

} // end namespace serialization

} // end namespace darma_runtime

#endif // _darma_has_feature(oo_interface)

#endif //DARMA_IMPL_OO_CLASS_H
