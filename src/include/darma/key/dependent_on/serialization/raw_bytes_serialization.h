/*
//@HEADER
// ************************************************************************
//
//                      raw_bytes_serialization.h
//                         DARMA
//              Copyright (C) 2018 Sandia Corporation
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

#ifndef DARMAFRONTEND_RAW_BYTES_SERIALIZATION_H
#define DARMAFRONTEND_RAW_BYTES_SERIALIZATION_H

#include <darma/serialization/nonintrusive.h>

#include <darma/key/raw_bytes.h>

namespace darma {

namespace serialization {

template <>
struct Serializer<darma::detail::raw_bytes> {
  template <typename Archive>
  void get_size(darma::detail::raw_bytes const& val, Archive& ar) const {
    ar % val.get_size();
    ar.add_to_size_raw(val.get_size());
  }
  template <typename Archive>
  void pack(darma::detail::raw_bytes const& val, Archive& ar) const {
    ar << val.get_size();
    ar.pack_data_raw((char*)val.data.get(), (char*)val.data.get() + val.get_size());
  }
  template <typename Archive>
  void unpack(void* allocated, Archive& ar) const {
    size_t size = 0;
    ar >> size;
    auto* val = new (allocated) darma::detail::raw_bytes(size);
    ar.template unpack_data_raw<char>(val->data.get(), size);
  }
};

} // end namespace serialization

} // end namespace darma

#endif //DARMAFRONTEND_RAW_BYTES_SERIALIZATION_H
