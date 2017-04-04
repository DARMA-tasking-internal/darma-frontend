/*
//@HEADER
// ************************************************************************
//
//                      async_access_traits.h
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

#ifndef DARMA_IMPL_ASYNC_ACCESSIBLE_ASYNC_ACCESS_TRAITS_H
#define DARMA_IMPL_ASYNC_ACCESSIBLE_ASYNC_ACCESS_TRAITS_H

#include <type_traits>

#include <tinympl/detection.hpp>
#include <tinympl/bool.hpp>

#include <darma/impl/handle_fwd.h>
#include <darma/impl/index_range/mapping.h>

namespace darma_runtime {

template <typename T>
struct async_accessible_traits {

  public:

    TINYMPL_PUBLIC_DETECTED_TYPE(value_type, value_type, T);

  //============================================================================
  // <editor-fold desc="permissions"> {{{1

  public:

    TINYMPL_PUBLIC_DETECTED_VALUE_WITH_DEFAULT(
      required_immediate_permissions, required_immediate_permissions,
      T::traits::permissions_traits::required_immediate_permissions,
      detail::AccessHandlePermissions::NotGiven
    );

    TINYMPL_PUBLIC_DETECTED_VALUE_WITH_DEFAULT(
      required_scheduling_permissions, required_scheduling_permissions,
      T::traits::permissions_traits::required_scheduling_permissions,
      detail::AccessHandlePermissions::NotGiven
    );

    TINYMPL_PUBLIC_DETECTED_VALUE_WITH_DEFAULT(
      static_immediate_permissions, static_immediate_permissions,
      T::traits::permissions_traits::static_immediate_permissions,
      detail::AccessHandlePermissions::NotGiven
    );

    TINYMPL_PUBLIC_DETECTED_VALUE_WITH_DEFAULT(
      static_scheduling_permissions, static_scheduling_permissions,
      T::traits::permissions_traits::static_scheduling_permissions,
      detail::AccessHandlePermissions::NotGiven
    );

  // </editor-fold> end permissions }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="index_range"> {{{1

  private:

    template <typename U>
    using _get_index_range_archetype = std::decay_t<
      decltype(std::declval<U>().get_index_range())
    >;

  public:

    static constexpr auto has_index_range = tinympl::is_detected<
      _get_index_range_archetype, T
    >::value;

    using index_range = tinympl::detected_t<
      _get_index_range_archetype, T
    >;

  // </editor-fold> end index_range }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="mapped_with and mapped_type"> {{{1

  private:

    template <typename U, typename MappingT>
    using _mapped_with_archetype = decltype(
      std::declval<U>().mapped_with(std::declval<MappingT>())
    );

  public:

    // This should eventually be a variable template, once we're allowed to have
    // them in DARMA.  For now, the *_c extension (c is for "constant") indicates
    // something that will be a variable template named without the _c once that
    // feature is supported
    template <typename MappingT>
    using can_be_mapped_with_c = tinympl::bool_<
      tinympl::is_detected<_mapped_with_archetype, T, MappingT>::value
    >;

    template <typename MappingT>
    using mapped_type = tinympl::detected_t<
      _mapped_with_archetype, T, MappingT
    >;

  // </editor-fold> end mapped_with and mapped_type }}}1
  //============================================================================


    // TODO finish this and use it


};

} // end namespace darma_runtime

#endif //DARMA_IMPL_ASYNC_ACCESSIBLE_ASYNC_ACCESS_TRAITS_H
