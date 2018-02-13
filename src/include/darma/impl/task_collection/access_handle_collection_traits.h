/*
//@HEADER
// ************************************************************************
//
//                      access_handle_collection_traits.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMA_IMPL_TASK_COLLECTION_ACCESS_HANDLE_COLLECTION_TRAITS_H
#define DARMA_IMPL_TASK_COLLECTION_ACCESS_HANDLE_COLLECTION_TRAITS_H


#include <darma/impl/access_handle/access_handle_traits.h>

#include <darma/utility/optional_boolean.h>

namespace darma_runtime {

namespace detail {

template <typename T>
struct is_access_handle_collection : std::false_type { };

// TODO special members (e.g., for allocator)

namespace ahc_traits {

template <
  OptionalBoolean IsOuter = OptionalBoolean::Unknown,
  typename AHSemanticTraits = access_handle_semantic_traits<>,
  OptionalBoolean IsMapped = OptionalBoolean::Unknown

>
struct semantic_traits {
  // TODO decide if this is the same as the AccessHandle could_be_outermost_scope or whatever
  static constexpr auto is_outer = IsOuter;
  using handle_semantic_traits = AHSemanticTraits;
  static constexpr auto is_mapped = IsMapped;

  template <OptionalBoolean NewIsMapped>
  struct with_is_mapped {
    using type = semantic_traits<
      is_outer,
      handle_semantic_traits,
      NewIsMapped
    >;
  };

  template <typename OtherTraits>
  using is_convertible_from = tinympl::bool_<
    // is_outer must be compatible
    (
      is_outer == OptionalBoolean::Unknown
        or OtherTraits::is_outer == OptionalBoolean::Unknown
        or is_outer == OptionalBoolean::KnownFalse
        or (is_outer == OptionalBoolean::KnownTrue
          and OtherTraits::is_outer != OptionalBoolean::KnownFalse
        )
    )
    // is_mapped must be compatible:  Basically, we cant copy a KnownTrue
    // mapped AHC into a mapped KnownFalse AHC
    and (
      is_mapped == OptionalBoolean::Unknown
        or OtherTraits::is_mapped == OptionalBoolean::Unknown
        or is_mapped == OptionalBoolean::KnownTrue
        or (
          is_mapped == OptionalBoolean::KnownFalse // redundant, but here for clarity
            and OtherTraits::is_mapped == OptionalBoolean::KnownFalse
        )
    )
  >;

};

} // end namespace ahc_traits

template <typename T, typename IndexRangeT,
  typename PermissionsTraits = access_handle_permissions_traits<>,
  typename SemanticTraits = ahc_traits::semantic_traits<>,
  typename AllocationTraits = access_handle_allocation_traits<T>
>
struct access_handle_collection_traits {
  public:
    using permissions_traits = PermissionsTraits;
    using semantic_traits = SemanticTraits;
    using allocation_traits = AllocationTraits;

    template <typename NewPermissionsTraits>
    struct with_permissions_traits {
      using type = access_handle_collection_traits<T, IndexRangeT,
        NewPermissionsTraits,
        semantic_traits,
        allocation_traits
      >;
    };

    template <typename NewSemanticTraits>
    struct with_semantic_traits {
      using type = access_handle_collection_traits<T, IndexRangeT,
        permissions_traits,
        NewSemanticTraits,
        allocation_traits
      >;
    };

    template <typename NewAllocationTraits>
    struct with_allocation_traits {
      using type = access_handle_collection_traits<T, IndexRangeT,
        permissions_traits,
        semantic_traits,
        NewAllocationTraits
      >;
    };

  private:
    template <typename AHC>
    struct _is_convertible_from_ahc_impl {
      using type = typename tinympl::and_<
        typename permissions_traits
          ::template is_convertible_from<typename AHC::traits::permissions_traits>,
        typename semantic_traits
          ::template is_convertible_from<typename AHC::semantic_traits>,
        typename allocation_traits
          ::template is_convertible_from<typename AHC::allocation_traits>
      >::type;
      static constexpr auto value = type::value;
    };

  public:
    template <typename AccessHandleCollectionT>
    using is_convertible_from_access_handle_collection = typename tinympl::and_<
      is_access_handle_collection<AccessHandleCollectionT>,
      _is_convertible_from_ahc_impl<AccessHandleCollectionT>
    >::type;

    template <typename OtherTraits>
    using is_convertible_from = typename tinympl::and_<
      typename permissions_traits
        ::template is_convertible_from<typename OtherTraits::permissions_traits>,
      typename semantic_traits
        ::template is_convertible_from<typename OtherTraits::semantic_traits>,
      typename allocation_traits
        ::template is_convertible_from<typename OtherTraits::allocation_traits>
    >::type;

};

template <typename T, typename IndexRange, typename... Flags>
struct make_access_handle_collection_traits {
  private:
    template <typename Traits, typename Modifier>
    using _add_trait = typename Modifier::template modified_traits<Traits>;

  public:

    using type = typename tinympl::variadic::left_fold<
      _add_trait, access_handle_collection_traits<T, IndexRange>, Flags...
    >::type;

    template <typename Traits>
    struct from_traits {
      using type = typename tinympl::variadic::left_fold<
        _add_trait, Traits, Flags...
      >::type;
    };

};

template <typename T, typename IndexRange, typename... Flags>
using make_access_handle_collection_traits_t = typename
  make_access_handle_collection_traits<T, IndexRange, Flags...>::type;


} // end namespace detail

namespace advanced {
namespace access_handle_collection_traits {

template <typename NewPermissionsTraits>
using permissions_traits = ::darma_runtime::advanced
  ::access_handle_traits::permissions_traits<NewPermissionsTraits>;

namespace internal {

template <OptionalBoolean new_is_mapped>
struct is_mapped
{
  static constexpr auto value = new_is_mapped;
  template <typename Traits>
  using modified_traits = typename Traits::template with_semantic_traits<
    typename Traits::semantic_traits
      ::template with_is_mapped<new_is_mapped>::type
  >;
  using is_access_handle_trait_flag = std::true_type;
};


} // end namespace internal

} // end namespace access_handle_collection_traits

} // end namespace advanced

} // end namespace darma_runtime

#endif //DARMA_IMPL_TASK_COLLECTION_ACCESS_HANDLE_COLLECTION_TRAITS_H
