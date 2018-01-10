/*
//@HEADER
// ************************************************************************
//
//                      serialization_traits.impl.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_SERIALIZATION_SERIALIZATION_TRAITS_IMPL_H
#define DARMAFRONTEND_SERIALIZATION_SERIALIZATION_TRAITS_IMPL_H

#include <darma/serialization/nonintrusive.h>
#include <darma/serialization/serialization_traits.h>

#include <darma/utility/static_assertions.h>

#include <tinympl/detection.hpp>
#include <tinympl/bool.hpp>
#include <tinympl/logical_and.hpp>
#include <tinympl/logical_or.hpp>

#include <type_traits>

namespace darma_runtime {
namespace serialization {

namespace impl {

template <typename T, typename Archive>
struct get_serializer_style {

  //----------------------------------------------------------------------------
  // <editor-fold desc="serializer_style::intrusive_serialize"> {{{2

  template <typename U, typename UArchive>
  using _intrusive_serialize_archetype = decltype(
    std::declval<U&>().serialize(std::declval<UArchive&>())
  );

  static constexpr auto uses_intrusive_serialize = tinympl::is_detected_exact<void,
    _intrusive_serialize_archetype, T, Archive
  >::value;

  // </editor-fold> end serializer_style::intrusive_serialize }}}2
  //----------------------------------------------------------------------------

  //----------------------------------------------------------------------------
  // <editor-fold desc="serializer_style::intrusive_pack_unpack"> {{{2

  template <typename U, typename UArchive>
  using _intrusive_compute_size_archetype = decltype(
    std::declval<U const&>().compute_size(std::declval<UArchive&>())
  );

  // Needs to be templated so that it doesn't get generated unless Archive::is_sizing() is true
  template <typename U, typename UArchive>
  using _has_intrusive_compute_size = tinympl::is_detected_exact<void,
    _intrusive_compute_size_archetype, U, UArchive
  >;

  template <typename U, typename UArchive>
  using _intrusive_pack_archetype = decltype(
    std::declval<U const&>().pack(std::declval<UArchive&>())
  );

  // Needs to be templated so that it doesn't get generated unless Archive::is_packing() is true
  template <typename U, typename UArchive>
  using _has_intrusive_pack = tinympl::is_detected_exact<void,
    _intrusive_pack_archetype, U, UArchive
  >;

  template <typename U, typename UArchive>
  using _intrusive_unpack_archetype = decltype(
    std::declval<U const&>().unpack(std::declval<void*>(), std::declval<UArchive&>())
  );

  // Needs to be templated so that it doesn't get generated unless Archive::is_unpacking() is true
  template <typename U, typename UArchive>
  using _has_intrusive_unpack = tinympl::is_detected_exact<void,
    _intrusive_unpack_archetype, U, UArchive
  >;

  static constexpr auto uses_intrusive_pack_unpack = tinympl::and_<
    // Only generate (even in unevaluated context) if is_sizing() is true
    tinympl::or_<
      tinympl::bool_<!Archive::is_sizing()>,
      _has_intrusive_compute_size<T, Archive>
    >,
    // Only generate (even in unevaluated context) if is_packing() is true
    tinympl::or_<
      tinympl::bool_<!Archive::is_packing()>,
      _has_intrusive_pack<T, Archive>
    >,
    // Only generate (even in unevaluated context) if is_unpacking() is true
    tinympl::or_<
      tinympl::bool_<!Archive::is_unpacking()>,
      _has_intrusive_unpack<T, Archive>
    >,
    // If all three are false, something has gone horribly wrong, so don't count
    // this as the correct mode
    tinympl::bool_<
      Archive::is_sizing()
      or Archive::is_packing()
      or Archive::is_unpacking()
    >
  >::value;

  // </editor-fold> end serializer_style::intrusive_pack_unpack }}}2
  //----------------------------------------------------------------------------

  //----------------------------------------------------------------------------
  // <editor-fold desc="serializer_style::nonintrusive_pack_unpack"> {{{2

  template <typename U, typename UArchive>
  using _nonintrusive_compute_size_archetype = decltype(
    darma_runtime::serialization::Serializer<U>::compute_size(
      std::declval<U const&>(), std::declval<UArchive&>()
    )
  );

  // Needs to be templated so that it doesn't get generated unless Archive::is_sizing() is true
  template <typename U, typename UArchive>
  using _has_nonintrusive_compute_size = tinympl::is_detected_exact<void,
    _nonintrusive_compute_size_archetype, U, UArchive
  >;

  template <typename U, typename UArchive>
  using _nonintrusive_pack_archetype = decltype(
    darma_runtime::serialization::Serializer<U>::pack(
      std::declval<U const&>(), std::declval<UArchive&>()
    )
  );

  // Needs to be templated so that it doesn't get generated unless Archive::is_packing() is true
  template <typename U, typename UArchive>
  using _has_nonintrusive_pack = tinympl::is_detected_exact<void,
    _nonintrusive_pack_archetype, U, UArchive
  >;

  template <typename U, typename UArchive>
  using _nonintrusive_unpack_archetype = decltype(
    darma_runtime::serialization::Serializer<U>::unpack(
      std::declval<void*>(), std::declval<UArchive&>()
    )
  );

  // Needs to be templated so that it doesn't get generated unless Archive::is_unpacking() is true
  template <typename U, typename UArchive>
  using _has_nonintrusive_unpack = tinympl::is_detected_exact<void,
    _nonintrusive_unpack_archetype, U, UArchive
  >;

  static constexpr auto uses_nonintrusive = tinympl::and_<
    // Only generate (even in unevaluated context) if is_sizing() is true
    tinympl::or_<
      tinympl::bool_<!Archive::is_sizing()>,
      _has_nonintrusive_compute_size<T, Archive>
    >,
    // Only generate (even in unevaluated context) if is_packing() is true
    tinympl::or_<
      tinympl::bool_<!Archive::is_packing()>,
      _has_nonintrusive_pack<T, Archive>
    >,
    // Only generate (even in unevaluated context) if is_unpacking() is true
    tinympl::or_<
      tinympl::bool_<!Archive::is_unpacking()>,
      _has_nonintrusive_unpack<T, Archive>
    >,
    // If all three are false, something has gone horribly wrong, so don't count
    // this as the correct mode
    tinympl::bool_<
      Archive::is_sizing()
        or Archive::is_packing()
        or Archive::is_unpacking()
    >
  >::value;

  // </editor-fold> end serializer_style::nonintrusive }}}2
  //----------------------------------------------------------------------------

  static constexpr auto unserializable =
    not uses_intrusive_serialize and not uses_intrusive_pack_unpack and not uses_nonintrusive;

  static constexpr auto uses_multiple_styles =
    (uses_intrusive_serialize or uses_intrusive_pack_unpack or uses_nonintrusive)
    and not (uses_intrusive_serialize xor uses_intrusive_pack_unpack xor uses_nonintrusive);

};

//==============================================================================
// <editor-fold desc="compute_size() customization point default implementation"> {{{1

template <typename T, typename SizingArchive>
std::enable_if_t<
  darma_runtime::serialization::is_sizable_with_archive<T, SizingArchive>::value
    and impl::get_serializer_style<T, SizingArchive>::uses_intrusive_serialize
>
compute_size_impl(T const& obj, SizingArchive& ar) {
  // need to const cast for serialization, since not all modes are const
  const_cast<T&>(obj).serialize(ar);
};

template <typename T, typename SizingArchive>
std::enable_if_t<
  darma_runtime::serialization::is_sizable_with_archive<T, SizingArchive>::value
    and impl::get_serializer_style<T, SizingArchive>::uses_intrusive_pack_unpack
>
compute_size_impl(T const& obj, SizingArchive& ar) {
  obj.compute_size(ar);
};

template <typename T, typename SizingArchive>
std::enable_if_t<
  darma_runtime::serialization::is_sizable_with_archive<T, SizingArchive>::value
    and impl::get_serializer_style<T, SizingArchive>::uses_nonintrusive
>
compute_size_impl(T const& obj, SizingArchive& ar) {
  darma_runtime::serialization::Serializer<T>::compute_size(obj, ar);
};

template <typename T, typename SizingArchive>
std::enable_if_t<
  not darma_runtime::serialization::is_sizable_with_archive<T, SizingArchive>::value
    and impl::get_serializer_style<T, SizingArchive>::unserializable
>
compute_size_impl(T const& obj, SizingArchive& ar) {
  struct ___serialization_failed_because_type___ { };
  struct ___is_not_sizable_with_archive_of_type___ { };
  struct ___because_no_valid_serialization_specification_styles_are_detected___ { };
  typename _darma__static_failure<
    _____________________________________________________________________,
    ___serialization_failed_because_type___, T,
    ___is_not_sizable_with_archive_of_type___, SizingArchive,
    ___because_no_valid_serialization_specification_styles_are_detected___,
    _____________________________________________________________________
  >::type _failure;
};

template <typename T, typename SizingArchive>
std::enable_if_t<
  not darma_runtime::serialization::is_sizable_with_archive<T, SizingArchive>::value
    and impl::get_serializer_style<T, SizingArchive>::uses_multiple_styles
>
compute_size_impl(T const& obj, SizingArchive& ar) {
  struct ___serialization_failed_because_type___ { };
  struct ___is_not_sizable_with_archive_of_type___ { };
  struct ___because_more_than_on_serialization_specification_style_is_detected___ { };
  typename _darma__static_failure<
    _____________________________________________________________________,
    ___serialization_failed_because_type___, T,
    ___is_not_sizable_with_archive_of_type___, SizingArchive,
    ___because_more_than_on_serialization_specification_style_is_detected___,
    _____________________________________________________________________
  >::type _failure;
};

// </editor-fold> end compute_size() customization point default implementation }}}1
//==============================================================================

//==============================================================================
// <editor-fold desc="pack() customization point default implementation"> {{{1

template <typename T, typename PackingArchive>
std::enable_if_t<
  darma_runtime::serialization::is_packable_with_archive<T, PackingArchive>::value
    and impl::get_serializer_style<T, PackingArchive>::uses_intrusive_serialize
    and not impl::get_serializer_style<T, PackingArchive>::uses_multiple_styles
>
pack_impl(T const& obj, PackingArchive& ar) {
  // need to const cast for serialization, since not all modes are const
  const_cast<T&>(obj).serialize(ar);
};

template <typename T, typename PackingArchive>
std::enable_if_t<
  darma_runtime::serialization::is_packable_with_archive<T, PackingArchive>::value
    and impl::get_serializer_style<T, PackingArchive>::uses_intrusive_pack_unpack
    and not impl::get_serializer_style<T, PackingArchive>::uses_multiple_styles
>
pack_impl(T const& obj, PackingArchive& ar) {
  obj.pack(ar);
};

template <typename T, typename PackingArchive>
std::enable_if_t<
  darma_runtime::serialization::is_packable_with_archive<T, PackingArchive>::value
    and impl::get_serializer_style<T, PackingArchive>::uses_nonintrusive
    and not impl::get_serializer_style<T, PackingArchive>::uses_multiple_styles
>
pack_impl(T const& obj, PackingArchive& ar) {
  darma_runtime::serialization::Serializer<T>::pack(obj, ar);
};

template <typename T, typename PackingArchive>
std::enable_if_t<
  not darma_runtime::serialization::is_packable_with_archive<T, PackingArchive>::value
>
pack_impl(T const& obj, PackingArchive& ar) {
  struct ___serialization_failed_because_type___ { };
  struct ___is_not_packable_with_archive_of_type___ { };
  typename _darma__static_failure<
    _____________________________________________________________________,
    ___serialization_failed_because_type___, T,
    ___is_not_packable_with_archive_of_type___, PackingArchive,
    _____________________________________________________________________
  >::type _failure;
};

// </editor-fold> end pack() customization point default implementation }}}1
//==============================================================================

//==============================================================================
// <editor-fold desc="unpack() customization point default implementation"> {{{1

template <typename T, typename UnpackingArchive>
std::enable_if_t<
  darma_runtime::serialization::is_unpackable_with_archive<T, UnpackingArchive>::value
    and impl::get_serializer_style<T, UnpackingArchive>::uses_intrusive_serialize
    and not impl::get_serializer_style<T, UnpackingArchive>::uses_multiple_styles
    and std::is_default_constructible<T>::value
>
unpack_impl(void* allocated, UnpackingArchive& ar) {
  auto obj_ptr = new (allocated) T();
  obj_ptr->serialize(ar);
};

template <typename T, typename UnpackingArchive>
std::enable_if_t<
  darma_runtime::serialization::is_unpackable_with_archive<T, UnpackingArchive>::value
    and impl::get_serializer_style<T, UnpackingArchive>::uses_intrusive_pack_unpack
    //and not impl::get_serializer_style<T, UnpackingArchive>::uses_multiple_styles
>
unpack_impl(void* allocated, UnpackingArchive& ar) {
  T::unpack(allocated, ar);
};

template <typename T, typename UnpackingArchive>
std::enable_if_t<
  darma_runtime::serialization::is_unpackable_with_archive<T, UnpackingArchive>::value
    and impl::get_serializer_style<T, UnpackingArchive>::uses_nonintrusive
    //and not impl::get_serializer_style<T, UnpackingArchive>::uses_multiple_styles
>
unpack_impl(void* allocated, UnpackingArchive& ar) {
  darma_runtime::serialization::Serializer<T>::unpack(allocated, ar);
}

template <typename T, typename UnpackingArchive>
std::enable_if_t<
  darma_runtime::serialization::is_unpackable_with_archive<T, UnpackingArchive>::value
    and impl::get_serializer_style<T, UnpackingArchive>::uses_intrusive_serialize
    and not impl::get_serializer_style<T, UnpackingArchive>::uses_multiple_styles
    and not std::is_default_constructible<T>::value
>
unpack_impl(void* allocated, UnpackingArchive& ar) {
  struct ___serialization_failed_because_type___ { };
  struct ___is_not_unpackable_with_archive_of_type___ { };
  struct ___because_intrusive_serialize_is_given_for_non_default_constructible_type___ { };
  typename _darma__static_failure<
    _____________________________________________________________________,
    ___serialization_failed_because_type___, T,
    ___is_not_unpackable_with_archive_of_type___, UnpackingArchive,
    ___because_intrusive_serialize_is_given_for_non_default_constructible_type___,
    _____________________________________________________________________
  >::type _failure;
};

template <typename T, typename UnpackingArchive>
std::enable_if_t<
  not darma_runtime::serialization::is_unpackable_with_archive<T, UnpackingArchive>::value
    and impl::get_serializer_style<T, UnpackingArchive>::uses_multiple_styles
>
unpack_impl(void* allocated, UnpackingArchive& ar) {
  // TODO this should produce a compile-time readable error
  struct ___serialization_failed_because_type___ { };
  struct ___is_not_unpackable_with_archive_of_type___ { };
  struct ___because_more_than_on_serialization_specification_style_is_detected___ { };
  typename _darma__static_failure<
    _____________________________________________________________________,
    ___serialization_failed_because_type___, T,
    ___is_not_unpackable_with_archive_of_type___, UnpackingArchive,
    ___because_more_than_on_serialization_specification_style_is_detected___,
    _____________________________________________________________________
  >::type _failure;
}

template <typename T, typename UnpackingArchive>
std::enable_if_t<
  not darma_runtime::serialization::is_unpackable_with_archive<T, UnpackingArchive>::value
    and impl::get_serializer_style<T, UnpackingArchive>::unserializable
>
unpack_impl(void* allocated, UnpackingArchive& ar) {
  struct ___serialization_failed_because_type___ { };
  struct ___is_not_unpackable_with_archive_of_type___ { };
  struct ___because_no_valid_serialization_specification_styles_are_detected___ { };
  typename _darma__static_failure<
    _____________________________________________________________________,
    ___serialization_failed_because_type___, T,
    ___is_not_unpackable_with_archive_of_type___, UnpackingArchive,
    ___because_no_valid_serialization_specification_styles_are_detected___,
    _____________________________________________________________________
  >::type _failure;
}

// </editor-fold> end unpack() customization point default implementation }}}1
//==============================================================================

} // end namespace impl


//==============================================================================
// <editor-fold desc="metafunction customization point default implementations"> {{{1

template <typename T, typename SizingArchive>
struct is_sizable_with_archive_enabled_if<
  T, SizingArchive, std::enable_if_t<
    SizingArchive::is_sizing()
      and not impl::get_serializer_style<T, SizingArchive>::uses_multiple_styles
      and not impl::get_serializer_style<T, SizingArchive>::unserializable
  >
> : std::true_type
{ };

template <typename T, typename PackingArchive>
struct is_packable_with_archive_enabled_if<
  T, PackingArchive, std::enable_if_t<
    PackingArchive::is_packing()
      and not impl::get_serializer_style<T, PackingArchive>::uses_multiple_styles
      and not impl::get_serializer_style<T, PackingArchive>::unserializable
  >
> : std::true_type
{ };

template <typename T, typename UnpackingArchive>
struct is_unpackable_with_archive_enabled_if<
  T, UnpackingArchive, std::enable_if_t<
    UnpackingArchive::is_unpacking()
      and not impl::get_serializer_style<T, UnpackingArchive>::uses_multiple_styles
      and not impl::get_serializer_style<T, UnpackingArchive>::unserializable
  >
> : std::true_type
{ };

// </editor-fold> end metafunction customization point default implementations }}}1
//==============================================================================

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_SERIALIZATION_SERIALIZATION_TRAITS_IMPL_H
