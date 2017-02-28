/*
//@HEADER
// ************************************************************************
//
//                      string.h
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

#ifndef DARMA_IMPL_SERIALIZATION_STRING_H
#define DARMA_IMPL_SERIALIZATION_STRING_H

namespace darma_runtime {
namespace serialization {

template <
  typename CharT, typename Traits, typename Allocator
>
struct Serializer<
  std::basic_string<CharT, Traits, Allocator>
> {
  using string_t = std::basic_string<CharT, Traits, Allocator>;
  template <typename ArchiveT>
  size_t compute_size(string_t const& val, ArchiveT& ar) const {
    ar.incorporate_size(size_t());
    ar.add_to_size_direct(val.data(), val.size());
  }

  template <typename ArchiveT>
  void pack(string_t const& val, ArchiveT& ar) const {
    ar << val.size();
    ar.pack_direct(val.data(), val.data() + val.size());
  }

  template <typename ArchiveT, typename Allocator>
  void unpack(void* allocated, ArchiveT& ar, Allocator const& alloc) const {
    size_t size = 0;
    ar >> size;
    auto* val = new (allocated) std::string(size, '\0', alloc);
    ar.template unpack_direct<CharT>(
      const_cast<CharT*>(val->data()),
      size
    );
  }

};

namespace detail {

namespace _impl {

template <typename T>
using _has_char_traits_length_archetype = decltype(
std::char_traits<T>::length(std::declval<T>())
);
template <typename T>
using _has_char_traits_length = meta::is_detected_convertible<
  size_t, _has_char_traits_length_archetype, T
>;

} // end namespace _impl

template <typename T>
struct Serializer_enabled_if<
  T*,
  std::enable_if_t<_impl::_has_char_traits_length<T>::value>
> {
  template <typename ArchiveT>
  size_t compute_size(T* const val, ArchiveT& ar) const {
    ar.incorporate_size(size_t());
    ar.add_to_size_direct(val, std::char_traits<T>::length(val));
  }

  template <typename ArchiveT>
  void pack(T* const val, ArchiveT& ar) const {
    size_t length = std::char_traits<T>::length(val);
    ar << length;
    ar.pack_direct(val, val + length);
  }

  template <typename ArchiveT>
  void unpack(void* allocated, ArchiveT& ar) const {
    size_t length = 0;
    ar >> length;
    ar.template unpack_direct<T>(allocated, length);
  }
};

template <typename T>
struct Serializer_enabled_if<
  T const*,
  std::enable_if_t<_impl::_has_char_traits_length<T>::value>
>: Serializer<T*> {
};

} // end namespace detail
} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_STRING_H
