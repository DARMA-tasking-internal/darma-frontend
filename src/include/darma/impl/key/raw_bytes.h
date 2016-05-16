/*
//@HEADER
// ************************************************************************
//
//                      raw_bytes.h
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

#ifndef DARMA_IMPL_KEY_RAW_BYTES_H
#define DARMA_IMPL_KEY_RAW_BYTES_H

#include <cstddef>
#include <memory>

#include <darma/impl/key/key_fwd.h>
#include <darma/impl/serialization/nonintrusive.h>

namespace darma_runtime {

namespace detail {

struct raw_bytes {
  public:
    explicit
    raw_bytes(size_t nbytes)
      : size(nbytes),
        data(new char[nbytes])
    { }
    raw_bytes(raw_bytes&&) = default;
    raw_bytes(raw_bytes const& other)
      : size(other.size), data(new char[other.size])
    {
      memcpy(data.get(), other.data.get(), size);
    }
    raw_bytes& operator=(raw_bytes const& other) {
      size = other.size;
      data.reset(new char[other.size]);
      memcpy(data.get(), other.data.get(), size);
      return *this;
    }
    std::unique_ptr<char[]> data = nullptr;
    size_t get_size() const { return size; }
  private:
    size_t size;
};

template <>
struct bytes_convert<raw_bytes> {
  static constexpr bool size_known_statically = false;
  inline size_t
  get_size(raw_bytes const& bytes) const {
      return bytes.get_size();
  }
  inline void
  operator()(raw_bytes const& bytes, void* dest, const size_t n_bytes, const size_t offset) const {
      ::memcpy(dest, bytes.data.get() + offset, n_bytes);
  }
  inline raw_bytes
  get_value(void* data, size_t size) const {
      raw_bytes bytes(size);
      memcpy(bytes.data.get(), data, size);
      return bytes;
  }
};



} // end namespace detail

namespace serialization {

template <>
struct Serializer<darma_runtime::detail::raw_bytes> {
  template <typename Archive>
  void get_size(darma_runtime::detail::raw_bytes const& val, Archive& ar) const {
    ar % val.get_size();
    ar.add_to_size(val.get_size());
  }
  template <typename Archive>
  void pack(darma_runtime::detail::raw_bytes const& val, Archive& ar) const {
    ar << val.get_size();
    ar.pack_contiguous((char*)val.data.get(), (char*)val.data.get() + val.get_size());
  }
  template <typename Archive>
  void unpack(void* allocated, Archive& ar) const {
    size_t size = 0;
    ar >> size;
    darma_runtime::detail::raw_bytes* val = new (allocated) darma_runtime::detail::raw_bytes(size);
    ar.template unpack_contiguous<char>(val->data.get(), size);
  }
};

} // end namespace serialization

} // end namespace darma_runtime

#endif //DARMA_IMPL_KEY_RAW_BYTES_H
