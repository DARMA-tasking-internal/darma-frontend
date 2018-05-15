/*
//@HEADER
// ************************************************************************
//
//                      SSO_key.impl.h
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

#ifndef DARMAFRONTEND_SSO_KEY_IMPL_H
#define DARMAFRONTEND_SSO_KEY_IMPL_H

#include "SSO_key.h"

namespace darma {
namespace detail {

//==============================================================================
// Implementation of ctor

namespace _impl {

template <typename SSOKeyT, typename ComponentCountOrdinal>
struct _do_add_component {
  char*& buffer;
  SSOKeyT& this_;
  ComponentCountOrdinal actual_component_count = 0;

  _do_add_component(SSOKeyT& in_key, char*& in_buffer)
    : this_(in_key), buffer(in_buffer) { }

  DARMA_FORCE_INLINE
  void _impl() { }

  template <typename T, typename... Ts>
  DARMA_FORCE_INLINE
  void _impl(T&& arg, Ts&&... args) {
    actual_component_count +=
      darma::detail::_impl::sso_key_add(this_, buffer, std::forward<T>(arg));
    _impl(std::forward<Ts>(args)...);
  }

  template <typename Tuple, size_t... Idxs>
  DARMA_FORCE_INLINE
  void add_tuple(
    std::integer_sequence<size_t, Idxs...>,
    Tuple&& tup
  ) {
    _impl(std::get<Idxs>(std::forward<Tuple>(tup))...);
  };
};

} // end namespace _impl

template <
  size_t BufferSize,
  typename BackendAssignedKeyType,
  typename PieceSizeOrdinal,
  typename ComponentCountOrdinal
>
template <typename... Args>
SSOKey<BufferSize, BackendAssignedKeyType, PieceSizeOrdinal, ComponentCountOrdinal>::SSOKey(
  utility::variadic_constructor_tag_t,
  Args&&... args
) {

  static_assert(
    sizeof...(Args) < std::numeric_limits<ComponentCountOrdinal>::max(),
    "Too many components given to SSO Key"
  );
  size_t buffer_size = _impl::_sum(
    bytes_convert<std::remove_reference_t<Args>>().get_size(
      std::forward<Args>(args)
    )...
  ) + sizeof...(Args)
    * (sizeof(PieceSizeOrdinal) + sizeof(bytes_type_metadata))
    + sizeof(ComponentCountOrdinal);
  char* buffer, * buffer_start;
  if (buffer_size < BufferSize) {
    // Employ SSO
    mode = _impl::Short;
    repr.as_short = _short();
    repr.as_short.size = buffer_size;
    buffer = buffer_start = repr.as_short.data;
  } else {
    // use large buffer
    mode = _impl::Long;
    repr.as_long = _long();
    repr.as_long.size = buffer_size;
    repr.as_long.data = buffer = buffer_start = static_cast<char*>(
      abstract::backend::get_backend_memory_manager()->allocate(buffer_size)
    );
  }
  // Skip over the number of components; we'll come back to it
  buffer += sizeof(ComponentCountOrdinal);
  ComponentCountOrdinal actual_component_count = 0;
  // use out-of-line functor to avoid lambda instantiation proliferation and
  // speed up compile times, even though this makes the code a bit less readable
  auto component_adder = _impl::_do_add_component<SSOKey, ComponentCountOrdinal>(*this, buffer);
  component_adder.add_tuple(
    std::index_sequence_for<Args...>{},
    std::forward_as_tuple(std::forward<Args>(args)...)
  );
  *reinterpret_cast<ComponentCountOrdinal*>(buffer_start) =
    component_adder.actual_component_count;
}

template <
  size_t BufferSize,
  typename BackendAssignedKeyType,
  typename PieceSizeOrdinal,
  typename ComponentCountOrdinal
>
SSOKey<BufferSize, BackendAssignedKeyType, PieceSizeOrdinal, ComponentCountOrdinal>::~SSOKey() {
  if (mode == _impl::Long and repr.as_long.data != nullptr) {
    abstract::backend::get_backend_memory_manager()->deallocate(
      repr.as_long.data, repr.as_long.size
    );
  }
};

template <
  size_t BufferSize,
  typename BackendAssignedKeyType,
  typename PieceSizeOrdinal,
  typename ComponentCountOrdinal
>
SSOKey<BufferSize, BackendAssignedKeyType, PieceSizeOrdinal, ComponentCountOrdinal>::SSOKey(SSOKey const& other)
  : mode(other.mode)
{
  switch(mode) {
    case _impl::BackendAssigned: {
      repr.as_backend_assigned = other.repr.as_backend_assigned;
      break;
    }
    case _impl::Short: {
      ::memset(repr.as_short.data, 0, BufferSize);
      repr.as_short.size = other.repr.as_short.size;
      ::memcpy(repr.as_short.data, other.repr.as_short.data, repr.as_short.size);
      break;
    }
    case _impl::Long: {
      repr.as_long.size = other.repr.as_long.size;
      repr.as_long.data = static_cast<char*>(
        abstract::backend::get_backend_memory_manager()->allocate(repr.as_long.size)
      );
      ::memcpy(repr.as_long.data, other.repr.as_long.data, repr.as_long.size);
      break;
    }
    case _impl::NeedsBackendAssignedKey: {
      // nothing to do
      break;
    }
  }
}

} // end namespace detail
} // end namespace darma

#endif //DARMAFRONTEND_SSO_KEY_IMPL_H
