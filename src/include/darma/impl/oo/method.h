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

#include <darma/impl/oo/oo_fwd.h>
#include <darma/impl/oo/field.h>

#include <darma/impl/handle.h> // is_access_handle

#include <darma/impl/oo/method_runnable.h>
#include <darma/impl/serialization/serialization_fwd.h> // unpack_constructor_tag_t

namespace darma_runtime {


namespace oo {

namespace detail {

template <template <class...> class decorator>
struct outermost_decorator {
  template <typename... Args>
  using apply = tinympl::identity<decorator<Args...>>;
  template <typename... Args>
  using outermost_enclosing_decorator_t = decorator<Args...>;
};

template <typename T>
struct is_outermost_decorator : std::false_type { };

template <template <class...> class decorator>
struct is_outermost_decorator<outermost_decorator<decorator>> : std::true_type { };

template <typename outermost_enclosing, typename... args_lists>
struct subfields_path {
  template <typename... Args>
  using subfields = subfields_path<outermost_enclosing, args_lists..., tinympl::vector<Args...>>;
  template <typename... SubFieldArgs>
  using subfield = subfields<SubFieldArgs...>;
  using args_lists_vector = tinympl::vector<args_lists...>;
  using enclosing_decorator = outermost_enclosing;
};

template <typename enclosing, typename... Args>
struct oo_method_decorator {
  public:
    using args_vector_t = tinympl::vector<Args...>;

  private:
    using _tag_args_vector_indices = typename tinympl::variadic::find_all_if<
      detail::is_oo_name_tag, Args...
    >::type;
    template <typename int_const>
    using _arg_at = tinympl::at<int_const::value, args_vector_t>;

  public:
    using tags_vector_t = typename tinympl::transform<_tag_args_vector_indices,
      _arg_at, tinympl::vector
    >::type;

    template <typename... SubFieldArgs>
    using subfields = subfields_path<enclosing, tinympl::vector<Args...>,
      tinympl::vector<SubFieldArgs...>
    >;

    template <typename... SubFieldArgs>
    using subfield = subfields<SubFieldArgs...>;

};

} // end namespace detail

template <typename... Args>
struct reads_
  : detail::oo_method_decorator<detail::outermost_decorator<reads_>, Args...>
{ };

template <typename... Args>
struct reads_value_
  : detail::oo_method_decorator<detail::outermost_decorator<reads_value_>,Args...>
{ };

template <typename... Args>
struct modifies_
  : detail::oo_method_decorator<detail::outermost_decorator<modifies_>,Args...>
{ };

template <typename... Args>
struct modifies_value_
  : detail::oo_method_decorator<detail::outermost_decorator<modifies_value_>, Args...>
{ };

namespace detail {

template <typename Method> struct deferred_method_call;

template <typename tag>
struct make_is_field_tag {
  template <typename T>
  using apply = std::is_same<typename T::tag, tag>;
};

template <typename tag, typename of_class>
using _get_type_for_tag = tinympl::identity< typename tinympl::at_t<
  tinympl::find_if<
    typename of_class::darma_class::helper_t::fields_with_types,
    make_is_field_tag<tag>::template apply
  >::type::value,
  typename of_class::darma_class::helper_t::fields_with_types
>::value_type >;


template <template <class...> class decorator, typename OfClass, typename args_vector_t>
struct decorated_with {

  template <typename int_const>
  using _arg_at = tinympl::at<int_const::value, args_vector_t>;

  template <typename T>
  using _get_tag = tinympl::identity<typename T::tag>;

  template <typename T>
  using _get_tags_vector = tinympl::identity<typename T::tags_vector_t>;

  using indices = typename tinympl::find_all_if<
    args_vector_t,
    tinympl::make_is_instantiation_of<decorator>::template apply_t
  >::type;
  using decorated_vector = typename tinympl::transform<
    indices, _arg_at, tinympl::vector>::type;

  using _tags_vectors_vector = typename tinympl::transform<
    decorated_vector, _get_tags_vector
  >::type;

  // Do first push-front in case there's only one, second in case empty;
  // then join the constituent vectors
  using tag_vector = tinympl::left_fold_t<
    tinympl::push_front_t<
      tinympl::push_front_t<
        _tags_vectors_vector,
        tinympl::vector<>
      >,
      tinympl::vector<>
    >, tinympl::join
  >;


  template <typename tag>
  using _get_type_for_tag_of_class = _get_type_for_tag<tag, OfClass>;

  using type_vector = typename tinympl::transform<tag_vector,
    _get_type_for_tag_of_class
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

//////////////////////////////////////////////////////////////////////////////

template <typename OfClass, typename... Args>
struct field_slice;

template <typename OfClass, typename... Args>
struct field_slice_in_chain;

template <typename T>
using is_field_slice = typename tinympl::is_instantiation_of<field_slice, T>::type;

//template <typename T>
//struct _as_new_field_slice_in_chain;
//
//template <typename outer_decorator, typename first_tag, typename second_tag, typename... rest_lists>
//struct _as_new_field_slice<subfields_path<
//  outer_decorator, tinympl::vector<first_tag>, tinympl::vector<second_tag>, rest_lists...
//> {
//  template <typename ParentOfClass>
//  struct slice_in_chain {
//
//    template <typename tag, typename of_class>
//    using _get_type_for_tag = tinympl::identity< typename tinympl::at_t<
//      tinympl::find_if<
//        typename of_class::darma_class::helper_t::fields_with_types,
//        make_is_field_tag<tag>::template apply
//      >::type::value,
//      typename of_class::darma_class::helper_t::fields_with_types
//    >::value_type >;
//
//    using child_OfClass = _get_type_for_tag<first_tag, ParentOfClass>;
//
//
//    using type = field_slice_in_chain<
//      subfields_path<outer_decorator, second_tag, rest_lists...>
//    >;
//  };
//};


////////////////////////////////////////////////////////////////////////////////

template <typename OfClass, typename... Args>
struct field_slice_helper {
  using of_class_t = OfClass;
  using _args_vector_t = tinympl::vector<Args...>;

  template <typename int_const>
  using _arg_at = tinympl::at<int_const::value, _args_vector_t>;

  //////////////////////////////////////////////////////////////////////////////

  // TODO: This should propogate the read-only-ness!
  template <typename T>
  using _as_read_only_access_handle_if_not_darma_class = std::conditional<
    is_darma_class<T>::value,
    T, darma_runtime::ReadAccessHandle<T>
  >;

  using _reads_base_classes = typename decorated_with<reads_, of_class_t, _args_vector_t>::template chained_base_classes<
    _as_read_only_access_handle_if_not_darma_class, _public_field_in_chain
  >;
  using _reads_fields = typename decorated_with<reads_, of_class_t, _args_vector_t>::template chained_base_classes<
    _as_read_only_access_handle_if_not_darma_class, _field_tag_with_type
  >;

  template <typename T>
  using _as_access_handle_if_not_darma_class = std::conditional<
    is_darma_class<T>::value,
    T, darma_runtime::AccessHandle<T>
  >;

  using _modifies_base_classes = typename decorated_with<modifies_, of_class_t, _args_vector_t>::template chained_base_classes<
    _as_access_handle_if_not_darma_class, _public_field_in_chain
  >;
  using _modifies_fields = typename decorated_with<modifies_, of_class_t, _args_vector_t>::template chained_base_classes<
    _as_access_handle_if_not_darma_class, _field_tag_with_type
  >;

  template <typename T>
  using _as_const_ref = std::add_lvalue_reference<std::add_const_t<T>>;

  using _reads_value_base_classes = typename decorated_with<reads_value_, of_class_t, _args_vector_t>::template chained_base_classes<
    _as_const_ref, _public_field_in_chain
  >;
  using _reads_value_fields = typename decorated_with<reads_value_, of_class_t, _args_vector_t>::template chained_base_classes<
    _as_const_ref, _field_tag_with_type
  >;

  template <typename T>
  using _as_nonconst_ref = std::add_lvalue_reference<std::remove_const_t<T>>;

  using _modifies_value_base_classes = typename decorated_with<modifies_value_, of_class_t, _args_vector_t>::template chained_base_classes<
    _as_nonconst_ref, _public_field_in_chain
  >;
  using _modifies_value_fields = typename decorated_with<modifies_value_, of_class_t, _args_vector_t>::template chained_base_classes<
    _as_nonconst_ref, _field_tag_with_type
  >;

  //////////////////////////////////////////////////////////////////////////////

  using _subfield_paths = typename tinympl::transform<
    typename tinympl::find_all_if<
      _args_vector_t,
      tinympl::make_is_instantiation_of<subfields_path>::template apply
    >::type, _arg_at, tinympl::vector
  >::type;

  // now we need to gather the subfield paths that belong to a single (first level) tag
  template <typename tag>
  struct subfield_paths_for_tag {

    template <typename path>
    using path_has_tag_in_first = std::integral_constant<bool,
      tinympl::count<
        tinympl::front_t<typename path::args_lists_vector>, tag
      >::value >= 1
    >;

    template <typename path>
    struct make_path_enclosing_for_splat {
      template <typename... args>
      using apply = subfields_path<typename path::enclosing_decorator, args...>;
    };

    template <typename path>
    using remove_first_path_part = tinympl::splat_to<
      typename tinympl::pop_front<typename path::args_lists_vector>::type,
      make_path_enclosing_for_splat<path>::template apply
    >;

    template <typename int_const>
    using _subfield_path_at = tinympl::at<int_const::value, _subfield_paths>;

    using subfield_paths_unmodified = typename tinympl::transform<
      typename tinympl::find_all_if<
        _subfield_paths, path_has_tag_in_first
      >::type,
      _subfield_path_at, tinympl::vector
    >::type;

    using type = tinympl::vector<tag,
      typename _get_type_for_tag<tag, OfClass>::type,
      typename tinympl::transform<
        subfield_paths_unmodified,
        remove_first_path_part
      >::type
    >;
  };

  template <typename path>
  using first_tag_list = tinympl::front<typename path::args_lists_vector>;

  using _all_subfield_tags = typename tinympl::unique<
    typename tinympl::left_fold<
      tinympl::push_front_t<tinympl::push_front_t<
        typename tinympl::transform<_subfield_paths, first_tag_list>::type,
        tinympl::vector<>
      >, tinympl::vector<>>,
      tinympl::join
    >::type
  >::type;

  using _subfield_paths_gathered = typename tinympl::unique<
    typename tinympl::transform<_all_subfield_tags, subfield_paths_for_tag>::type
  >::type;

  template <typename grouped>
  struct make_subfield_base_classes {
    using tag = tinympl::at_t<0, grouped>;
    using child_class_t = tinympl::at_t<1, grouped>;
    using subfield_paths_list = tinympl::at_t<2, grouped>;

    template <typename path>
    using expand_path = std::conditional<
      tinympl::size<typename path::args_lists_vector>::value == 1,
      typename tinympl::transform<
        tinympl::at_t<0, typename path::args_lists_vector>,
        path::enclosing_decorator::template apply
      >::type,
      tinympl::vector<path>
    >;

    using type = _public_field_in_chain<
      typename tinympl::splat_to<
        typename tinympl::join<
          tinympl::vector<child_class_t>,
          typename tinympl::left_fold<
            tinympl::push_front_t<tinympl::push_front_t<
              typename tinympl::transform<subfield_paths_list, expand_path>::type,
              tinympl::vector<>
            >, tinympl::vector<>>,
            tinympl::join
          >::type
        >::type,
        field_slice
      >::type,
      tag
    >;
  };

  using _subfield_base_classes = typename tinympl::transform<
    _subfield_paths_gathered, make_subfield_base_classes
  >::type;

  template <typename field_in_chain>
  using get_field_descriptor = tinympl::identity<
    _field_tag_with_type<typename field_in_chain::type, typename field_in_chain::tag>
  >;

  using _subfield_fields = typename tinympl::transform<
    _subfield_base_classes, get_field_descriptor
  >::type;

  //////////////////////////////////////////////////////////////////////////////

  using base_class = typename chain_base_classes<
    typename tinympl::join<
      _reads_base_classes, _modifies_base_classes,
      _reads_value_base_classes, _modifies_value_base_classes,
      _subfield_base_classes
    >::type
  >::type;

  using fields = typename tinympl::join<
    _reads_fields, _modifies_fields,
    _reads_value_fields, _modifies_value_fields,
    _subfield_fields
  >::type;

};

template <typename OfClass, typename... Args>
struct field_slice : field_slice_helper<OfClass, Args...>::base_class
{
  using helper_t = field_slice_helper<OfClass, Args...>;
  using base_t = typename helper_t::base_class;

  field_slice() = default;
  field_slice(field_slice const&) = default;
  field_slice(field_slice&&) = default;

  // Forward to base class if it's not a copy or move constructor
  template <typename T,
    typename = std::enable_if_t<
      not std::is_same<std::decay_t<T>, field_slice>::value
        and is_chained_base_class<std::decay_t<T>>::value
    >
  >
  constexpr inline explicit
  field_slice(T&& val)
    : base_t(std::forward<T>(val))
  { }

  // Forward the unpacking constructor
  template <typename ArchiveT>
  constexpr inline explicit
  field_slice(
    serialization::unpack_constructor_tag_t,
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

  template <typename ArchiveT>
  void unpack(ArchiveT& ar) {
    assert(false); // Should never get called; it should just be the unpacking constructor
  }

};

template <typename OfClass, typename... Args>
struct field_slice_in_chain
{
  using helper_t = field_slice_helper<OfClass, Args...>;

  template <typename Base>
  struct link_in_chain
    : helper_t::base_class, Base
  {
    using helper_base_t = typename helper_t::base_class;

    link_in_chain() = default;
    link_in_chain(link_in_chain const&) = default;
    link_in_chain(link_in_chain&&) = default;

    // Forward to base class if it's not a copy or move constructor
    template <typename T,
      typename = std::enable_if_t<
        not std::is_same<std::decay_t<T>, link_in_chain>::value
          and is_chained_base_class<std::decay_t<T>>::value
      >
    >
    constexpr inline explicit
    link_in_chain(T&& val)
      : helper_base_t(std::forward<T>(val)),
        Base(std::forward<T>(val))
    { }

    // Forward the unpacking constructor
    template <typename ArchiveT>
    constexpr inline explicit
    link_in_chain(
      serialization::unpack_constructor_tag_t,
      ArchiveT& ar
    ) : helper_base_t(serialization::unpack_constructor_tag, ar),
        Base(serialization::unpack_constructor_tag, ar)
    { }

  };

};

////////////////////////////////////////////////////////////////////////////////

template <typename OfClass, typename DarmaMethodT, typename... Args>
struct darma_method_helper {

  using of_class_t = OfClass;
  using _args_vector_t = tinympl::vector<Args...>;

  using field_slice_t = field_slice_in_chain<OfClass, Args...>;

  static constexpr std::conditional_t<
    std::is_convertible<oo_sentinel_value_t, of_class_t>::value,
    oo_sentinel_value_t,
    __________you_forgot___using__darma_class__darma_class___in_definition_of_<of_class_t>
  > _detect_forgotten_using_darma_class = { };


  template <typename MethodWithTag>
  using _method_with_this = tinympl::identity<
    typename MethodWithTag::template chain_item_with_cast_to<DarmaMethodT>
  >;
  template <typename MethodWithTag>
  using _immediate_method_with_this = tinympl::identity<
    typename MethodWithTag::template immediate_chain_item_with_cast_to<DarmaMethodT>
  >;

  using base_class = typename chain_base_classes<
    typename tinympl::join<
      // least derived base class comes first.  We want immediate versions to be base
      // classes of deferred versions
      typename tinympl::transform<typename of_class_t::helper_t::public_method_tags,
        _immediate_method_with_this
      >::type,
      typename tinympl::transform<typename of_class_t::helper_t::public_method_tags,
        _method_with_this
      >::type,
      // Now include in the chain all of the classes with actual data members
      tinympl::vector<field_slice_t>
    >::type
  >::type;

  using fields = typename field_slice_t::helper_t::fields;

};

template <typename Method> struct deferred_method_call;

} // end namespace detail

template <typename OfClass,
  typename... Args
>
struct darma_method
  : detail::darma_method_helper<OfClass, darma_method<OfClass, Args...>, Args...>::base_class
{
  private:

    template <typename T>
    using _method_t_matches = std::is_same<typename T::method_t::darma_method, darma_method>;
    template <typename T>
    using method_t_matches = darma_runtime::meta::is_detected<_method_t_matches, T>;
    template <typename T>
    using _of_class_t_matches = std::is_same<typename T::of_class_t, OfClass>;
    template <typename T>
    using of_class_t_matches = darma_runtime::meta::is_detected<_of_class_t_matches, T>;

  protected:

    // Aliases allowing deferred recursion
    using deferred_recursive_call = darma_method;
    using deferred_recursive = darma_method;

  public:
    using helper_t = typename detail::darma_method_helper<OfClass, darma_method, Args...>;
    using base_t = typename helper_t::base_class;
    using of_class_t = OfClass;

    // Explicitly default the copy, move, and default constructors
    constexpr inline darma_method() = default;
    constexpr inline darma_method(darma_method const& val) = default;
    constexpr inline darma_method(darma_method&& val) = default;

    // Allow construction from the class that this is a method of
    template <typename OfClassOrMethodDeduced,
      typename = std::enable_if_t<
        (std::is_convertible<OfClassOrMethodDeduced, OfClass>::value
          or of_class_t_matches<std::decay_t<OfClassOrMethodDeduced>>::value)
        and not std::is_same<
          std::decay_t<OfClassOrMethodDeduced>, detail::oo_sentinel_value_t
        >::value
      >
    >
    constexpr inline explicit
    darma_method(OfClassOrMethodDeduced&& val)
      : base_t(std::forward<OfClassOrMethodDeduced>(val))
    { }

    // For detection purposes only; should never be implemented
    darma_method(detail::oo_sentinel_value_t const&);

    // Allow construction from the class that this is a method of
    //template <typename DeferredCallDeduced,
    //  typename = std::enable_if_t<
    //    method_t_matches<std::decay_t<DeferredCallDeduced>>::value
    //  >
    //>
    //constexpr inline explicit
    //darma_method(DeferredCallDeduced&& val)
    //  : base_t(std::forward<DeferredCallDeduced>(val))
    //{ }

};

namespace detail {

template <typename T, typename OfClass>
struct is_darma_method_of_class : std::false_type { };

template <typename OfClass, typename... Args>
struct is_darma_method_of_class<darma_method<OfClass, Args...>, OfClass>
  : std::true_type
{ };

template <typename Method>
struct deferred_method_call_helper {

  std::conditional_t<
    std::is_convertible<oo_sentinel_value_t, Method>::value,
    meta::nonesuch,
    __________you_forgot___using__darma_method__darma_method___in_definition_of_method_for_class_<
      typename tinympl::extract_arg_n<0, Method>::type, typename Method::of_class_t
    >
  > _detect_missing_using_darma_method = { };

  template <typename Field>
  using _access_handle_wrapped_base_class = tinympl::select_first<
      darma_runtime::detail::is_access_handle<typename Field::value_type>
    ,  /* : */ _public_field_in_chain<typename Field::value_type, typename Field::tag>
    /* =============== */
    , tinympl::and_<
        std::is_lvalue_reference<typename Field::value_type>,
        std::is_const<std::remove_reference_t<typename Field::value_type>>
      >
    , /* : */ _public_field_in_chain<
        ReadAccessHandle< std::decay_t<typename Field::value_type> >,
        typename Field::tag
      >
    /* =============== */
    , tinympl::and_<
        std::is_lvalue_reference< typename Field::value_type >,
        tinympl::not_<std::is_const<std::remove_reference_t<typename Field::value_type>>>
      >
    , /* : */ _public_field_in_chain<
        AccessHandle< std::decay_t<typename Field::value_type> >,
        typename Field::tag
      >
    /* =============== */
    , is_darma_class< std::decay_t<typename Field::value_type> >
    , /* : */ _public_field_in_chain<
        typename Field::value_type,
        typename Field::tag
      >
    /* =============== */
    , is_field_slice< std::decay_t<typename Field::value_type> >
    , /* : */ _public_field_in_chain<
        typename Field::value_type,
        typename Field::tag
      >
  >;

  using base_class = typename chain_base_classes<typename tinympl::transform<
    typename Method::darma_method::helper_t::fields, _access_handle_wrapped_base_class
  >::type>::type;

};

template <typename Method>
struct deferred_method_call
  : deferred_method_call_helper<Method>::base_class
{
  using helper_t = deferred_method_call_helper<Method>;
  using base_t = typename helper_t::base_class;
  using tag_t = decltype( _darma__get_associated_method_template_tag(std::declval<Method&>()) );
  using of_class_t = typename Method::darma_method::helper_t::of_class_t;
  using method_t = Method;

  deferred_method_call() = default;

  // Allow construction from the class that this is a method of
  template <typename OfClassDeduced,
    typename = std::enable_if_t<
      std::is_convertible<OfClassDeduced, of_class_t>::value
      or is_darma_method_of_class<
        std::decay_t<OfClassDeduced>, of_class_t
      >::value
    >
  >
  constexpr inline explicit
  deferred_method_call(OfClassDeduced&& val)
    : base_t(std::forward<OfClassDeduced>(val))
  { }

  template <typename ArchiveT>
  constexpr inline explicit
  deferred_method_call(
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


  template <typename... Args>
  void run(Args&&... args) {
    Method(std::move(*this))(std::forward<Args>(args)...);
  }

};

template <typename Method, typename ClassOrCallingMethodT, typename... Args>
inline void
_create_deferred_method_call(ClassOrCallingMethodT&& cls, Args&&... args) {
  auto t = darma_runtime::detail::make_unique<darma_runtime::detail::TaskBase>();
  darma_runtime::detail::TaskBase* parent_task = static_cast<darma_runtime::detail::TaskBase* const>(
    darma_runtime::abstract::backend::get_backend_context()->get_running_task()
  );
  parent_task->current_create_work_context = t.get();
  // This should trigger the captures to happen in the access handle copy constructors
  t->set_runnable(std::make_unique<MethodRunnable<
    deferred_method_call<Method>, Args...
  >>(
    std::forward<ClassOrCallingMethodT>(cls),
    std::forward<Args>(args)...
  ));
  parent_task->current_create_work_context = nullptr;

  for(auto&& reg : t->registrations_to_run) {
    reg();
  }
  t->registrations_to_run.clear();

  for(auto&& post_reg_op : t->post_registration_ops) {
    post_reg_op();
  }
  t->post_registration_ops.clear();

  darma_runtime::abstract::backend::get_backend_runtime()->register_task(
    std::move(t)
  );
};

} // end namespace detail

} // end namespace oo

} // end namespace darma_runtime

#endif //DARMA_IMPL_OO_METHOD_H
