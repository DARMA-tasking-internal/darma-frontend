/*
//@HEADER
// ************************************************************************
//
//                      SSO_key_serialization.h
//                         DARMA
//              Copyright (C) 2018 Sandia Corporation
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

#ifndef DARMAFRONTEND_SSO_KEY_SERIALIZATION_H
#define DARMAFRONTEND_SSO_KEY_SERIALIZATION_H

#include <darma/key/SSO_key.h>

#include <darma/serialization/nonintrusive.h>
#include <darma/serialization/serializers/arithmetic_types.h>
#include <darma/serialization/serializers/array.h>
#include <darma/serialization/serializers/enum.h>


namespace darma_runtime {
namespace serialization {

template <
  size_t BufferSize,
  typename BackendAssignedKeyType,
  typename PieceSizeOrdinal,
  typename ComponentCountOrdinal
>
struct Serializer<
  darma_runtime::detail::SSOKey<
    BufferSize,
    BackendAssignedKeyType,
    PieceSizeOrdinal,
    ComponentCountOrdinal
  >
> {
  using sso_key_t = darma_runtime::detail::SSOKey<
    BufferSize, BackendAssignedKeyType, PieceSizeOrdinal, ComponentCountOrdinal
  >;
  using key_traits_t = darma_runtime::detail::key_traits<sso_key_t>;

  template <typename ArchiveT>
  static void compute_size(sso_key_t const& val, ArchiveT& ar) {
    DARMA_ASSERT_MESSAGE(
      not key_traits_t::needs_backend_key(val),
      "Cannot compute size of a key that is awaiting a backend assigned value."
        "  (This is a backend implementation bug; contact your backend developer)"
    );
    ar % val.mode;
    switch (val.mode) {
      case darma_runtime::detail::_impl::BackendAssigned:
        ar % val.repr.as_backend_assigned.backend_assigned_key;
        break;
      case darma_runtime::detail::_impl::Short:
        ar % val.repr.as_short.size;
        // This could be smaller...
        ar % val.repr.as_short.data;
        break;
      case darma_runtime::detail::_impl::Long:
        // don't really need to store size since range does it
        ar % val.repr.as_long.size;
        ar.add_to_size_raw(val.repr.as_long.size);
        break;
      case darma_runtime::detail::_impl::NeedsBackendAssignedKey:
        DARMA_ASSERT_UNREACHABLE_FAILURE("NeedsBackendAssignedKey");            // LCOV_EXCL_LINE
        break;                                                                  // LCOV_EXCL_LINE
    }
  }

  template <typename ArchiveT>
  static void pack(sso_key_t const& val, ArchiveT& ar)  {
    DARMA_ASSERT_MESSAGE(
      not key_traits_t::needs_backend_key(val),
      "Cannot pack a key that is awaiting a backend assigned value."
        "  (This is a backend implementation bug; contact your backend developer)"
    );
    ar << val.mode;
    switch (val.mode) {
      case darma_runtime::detail::_impl::BackendAssigned:
        ar << val.repr.as_backend_assigned.backend_assigned_key;
        break;
      case darma_runtime::detail::_impl::Short:
        ar << val.repr.as_short.size;
        // This could be smaller...
        ar << val.repr.as_short.data;
        break;
      case darma_runtime::detail::_impl::Long:
        // don't really need to store size since range does it
        ar << val.repr.as_long.size;
        ar.pack_data_raw(
          (const_cast<char*&>(val.repr.as_long.data)),
          val.repr.as_long.data + val.repr.as_long.size
        );
        break;
      case darma_runtime::detail::_impl::NeedsBackendAssignedKey:
        DARMA_ASSERT_UNREACHABLE_FAILURE("NeedsBackendAssignedKey");            // LCOV_EXCL_LINE
        break;                                                                  // LCOV_EXCL_LINE
    }
  }

  template <typename ArchiveT>
  static void unpack(void* allocated, ArchiveT& ar)  {
    new (allocated) sso_key_t(
      typename sso_key_t::unpack_ctor_tag{}, ar
    );
  }
};

} // end namespace serialization

namespace detail {

template <
  size_t BufferSize, typename BackendAssignedKeyType,
  typename PieceSizeOrdinal, typename ComponentCountOrdinal
>
template <typename Archive>
SSOKey<BufferSize, BackendAssignedKeyType, PieceSizeOrdinal, ComponentCountOrdinal>::SSOKey(
  unpack_ctor_tag,
  Archive& ar
) {
  ar >> mode;
  DARMA_ASSERT_MESSAGE(
    mode != _impl::NeedsBackendAssignedKey,
    "SSOKey unpack got mode NeedsBackendAssignedKey, which is not allowed."
      "  (This is a backend implementation bug; contact your backend developer)"
  );
  switch (mode) {
    case darma_runtime::detail::_impl::BackendAssigned: {
      repr.as_backend_assigned = _backend_assigned{};
      ar >> repr.as_backend_assigned.backend_assigned_key;
      break;
    }
    case darma_runtime::detail::_impl::Short: {
      repr.as_short = _short{};
      ar >> repr.as_short.size;
      // This could be smaller...
      ar >> repr.as_short.data;
      break;
    }
    case darma_runtime::detail::_impl::Long: {
      repr.as_long = _long{};
      // don't really need to store size since range does it
      ar >> repr.as_long.size;
      using alloc_t = typename std::allocator_traits<typename Archive::allocator_type>
      ::template rebind_alloc<char>;
      alloc_t alloc = ar.template get_allocator_as<alloc_t>();
      repr.as_long.data = std::allocator_traits<alloc_t>::allocate(alloc, repr.as_long.size);
      ar.template unpack_data_raw<char>(repr.as_long.data, repr.as_long.size);
      break;
    }
    case darma_runtime::detail::_impl::NeedsBackendAssignedKey: {
      DARMA_ASSERT_UNREACHABLE_FAILURE("NeedsBackendAssignedKey");            // LCOV_EXCL_LINE
      break;                                                                  // LCOV_EXCL_LINE
    }
  }
}

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMAFRONTEND_SSO_KEY_SERIALIZATION_H
