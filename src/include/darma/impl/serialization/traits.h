/*
//@HEADER
// ************************************************************************
//
//                          traits.h
//                          DARMA
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

#ifndef DARMA_IMPL_SERIALIZATION_TRAITS_H_
#define DARMA_IMPL_SERIALIZATION_TRAITS_H_

#include <tinympl/logical_and.hpp>

#include <darma/impl/serialization/serialization_fwd.h>

namespace darma_runtime {

namespace serialization {

namespace detail {

using std::declval;
using std::decay_t;
using meta::is_detected;
using std::remove_reference_t;
using std::remove_const_t;
using std::add_const_t;

// TODO check things like is_polymorphic (and do something about it)
// TODO check for const-incorrect or reference incorrect implementations and give helpful error messages in static_asserts

template <typename T, typename Enable>
struct serializability_traits {

  private:

    // Define some type aliases that aid in correctly detecting traits for deduced
    // types that may have qualifiers on them not recognized by the template
    // specialization pattern matcher

    // TODO make this behave specially for the case of const char* and friends (e.g., const char[9])
    // (since these could be deduced types of arguments to, e.g., the functor interface,
    // and are most definitely serializable)

    using _T = remove_const_t<T>;
    using _const_T = add_const_t<T>;
    // For use as a non-intrusive interface template parameter, constness and
    // references should be stripped off so that the specialization pattern matching
    // works.  (that way, we don't need separate specializations for, e.g., std::string
    // and const std::string)
    using _clean_T = remove_const_t<remove_reference_t<T>>;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    // <editor-fold desc="Detection of presence of intrusive serialization methods">

    //----------------------------------------
    // <editor-fold desc="serialize()">

  private:
    // remove_reference_t<ArchiveT>& is to force the argument to be an lvalue reference
    template <typename U, typename ArchiveT>
    using has_intrusive_serialize_archetype =
      decltype( declval<U>().serialize( declval<remove_reference_t<ArchiveT>&>() ) );

    template <typename ArchiveT>
    using has_const_incorrect_serialize =
      meta::is_detected<has_intrusive_serialize_archetype, _const_T, ArchiveT>;

  public:
    template <typename ArchiveT>
    using has_intrusive_serialize =
      meta::is_detected<has_intrusive_serialize_archetype, _T, ArchiveT>;

    // </editor-fold>
    //----------------------------------------

    //----------------------------------------
    // <editor-fold desc="compute_size()">

  private:
    template <typename U>
    using has_intrusive_compute_size_archetype = decltype( declval<U>().compute_size( ) );
    template <typename U, typename ArchiveT>
    using has_intrusive_compute_size_with_archive_archetype = decltype(
      declval<U>().compute_size( declval<remove_reference_t<ArchiveT>&>() )
    );

    // For better error messages via static_asserts
    using has_const_incorrect_compute_size = is_detected<has_intrusive_compute_size_archetype, _T>;
    template <typename ArchiveT>
    using has_const_incorrect_compute_size_with_archive =
      meta::is_detected<has_intrusive_compute_size_with_archive_archetype, _T, ArchiveT>;

  public:
    using has_intrusive_compute_size =
      meta::is_detected<has_intrusive_compute_size_archetype, _const_T>;
    template <typename ArchiveT>
    using has_intrusive_compute_size_with_archive =
      meta::is_detected<has_intrusive_compute_size_with_archive_archetype, _const_T, ArchiveT>;

    // </editor-fold>
    //----------------------------------------

    //----------------------------------------
    // <editor-fold desc="pack()">

  private:
    template <typename U, typename ArchiveT>
    using has_intrusive_pack_archetype =
      decltype( declval<U>().pack( declval<remove_reference_t<ArchiveT>&>() ) );

    template <typename U, typename ArchiveT>
    using has_const_incorrect_pack = is_detected<has_intrusive_pack_archetype, _T, ArchiveT>;

  public:
    template <typename ArchiveT>
    using has_intrusive_pack = is_detected<has_intrusive_pack_archetype, _const_T, ArchiveT>;

    // </editor-fold>
    //----------------------------------------

    //----------------------------------------
    // <editor-fold desc="unpack()">

  private:
    template <typename U, typename ArchiveT>
    using has_intrusive_unpack_archetype =
      decltype( declval<U>().unpack( declval<remove_reference_t<ArchiveT>&>() ) );

    template <typename U, typename ArchiveT>
    using has_const_incorrect_unpack = is_detected<has_intrusive_unpack_archetype, _const_T, ArchiveT>;

  public:
    template <typename ArchiveT>
    using has_intrusive_unpack = is_detected<has_intrusive_pack_archetype, _T, ArchiveT>;

    // </editor-fold>
    //----------------------------------------

    // </editor-fold>
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    // <editor-fold desc="Detection of presence of nonintrusive serialization methods (for a given ArchiveT)">

    //----------------------------------------
    // <editor-fold desc="compute_size()">

  private:

    template <typename U, typename ArchiveT>
    using has_nonintrusive_compute_size_archetype = decltype(
      declval<Serializer<U>>().compute_size(
        declval<const U&>(), declval<remove_const_t<ArchiveT>&>()
      )
    );

  public:

    template <typename ArchiveT>
    using has_nonintrusive_compute_size =
      is_detected<has_nonintrusive_compute_size_archetype, _clean_T, ArchiveT>;

    // </editor-fold>
    //----------------------------------------

    //----------------------------------------
    // <editor-fold desc="pack()">

  private:

    template <typename U, typename ArchiveT>
    using has_nonintrusive_pack_archetype = decltype(
      declval<Serializer<U>>().pack(
        declval<const U&>(), declval<remove_const_t<ArchiveT>&>()
      )
    );

  public:

    template <typename ArchiveT>
    using has_nonintrusive_pack =
      is_detected<has_nonintrusive_pack_archetype, _clean_T, ArchiveT>;

    // </editor-fold>
    //----------------------------------------

    //----------------------------------------
    // <editor-fold desc="unpack()">

  private:

    template <typename U, typename ArchiveT>
    using has_nonintrusive_unpack_archetype = decltype(
      declval<Serializer<U>>().unpack(
        declval<void*>(), declval<remove_const_t<ArchiveT>&>()
      )
    );

  public:

    template <typename ArchiveT>
    using has_nonintrusive_unpack =
      is_detected<has_nonintrusive_unpack_archetype, _clean_T, ArchiveT>;

    // </editor-fold>
    //----------------------------------------

    // </editor-fold>
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    // <editor-fold desc="is_serializable_with_archive">

    // Use of tinympl::logical_and here should short-circuit and save a tiny bit of compilation time
    template <typename ArchiveT>
    using is_serializable_with_archive = tinympl::logical_and<
      has_nonintrusive_compute_size<ArchiveT>,
      has_nonintrusive_pack<ArchiveT>,
      has_nonintrusive_unpack<ArchiveT>
    >;

    typedef Serializer<_clean_T> serializer;

    // </editor-fold>
    ////////////////////////////////////////////////////////////////////////////////

};

} // end namespace detail

} // end namespace serialization

} // end namespace darma_runtime

#define STATIC_ASSERT_SERIALIZABLE_WITH_ARCHIVE(type, artype, ...) \
  static_assert( \
    ::darma_runtime::serialization::detail::serializability_traits<type>::template has_nonintrusive_compute_size<artype>::value, \
    __VA_ARGS__ ":  Cannot generate valid Serializer::compute_size() method for type/archive combination" \
  ); \
  static_assert( \
     ::darma_runtime::serialization::detail::serializability_traits<type>::template has_nonintrusive_pack<artype>::value, \
      __VA_ARGS__ ":  Cannot generate valid Serializer::pack() method for type/archive combination" \
  ); \
  static_assert( \
    ::darma_runtime::serialization::detail::serializability_traits<type>::template has_nonintrusive_unpack<artype>::value, \
    __VA_ARGS__ ":  Cannot generate valid Serializer::unpack() method for type/archive combination" \
  );



#endif /* DARMA_IMPL_SERIALIZATION_TRAITS_H_ */
