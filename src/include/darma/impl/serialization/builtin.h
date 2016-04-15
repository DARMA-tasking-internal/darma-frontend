/*
//@HEADER
// ************************************************************************
//
//                      builtin.h
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

#ifndef DARMA_IMPL_SERIALIZATION_BUILTIN_H
#define DARMA_IMPL_SERIALIZATION_BUILTIN_H

#include <type_traits>
#include <cstddef>

#include "nonintrusive.h"

namespace darma_runtime {
namespace serialization {

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="Specialization for fundamental types">

template <typename T>
struct Serializer<T,
  std::enable_if_t<std::is_fundamental<std::remove_cv_t<T>>::value>
> {
  template <typename ArchiveT>
  void
  compute_size(T const&, ArchiveT& ar) const {
    if(not ar.is_sizing()) Serializer_attorneys::ArchiveAccess::start_sizing(ar);
    Serializer_attorneys::ArchiveAccess::spot(ar) += sizeof(T);
  }

  template <typename ArchiveT>
  void
  pack(T const& val, ArchiveT& ar) const {
    using Serializer_attorneys::ArchiveAccess;
    if(not ar.is_packing()) ArchiveAccess::start_packing(ar);
    std::memcpy(ArchiveAccess::spot(ar), &val, sizeof(T));
    ArchiveAccess::spot(ar) += sizeof(T);
  }

  template <typename ArchiveT>
  void
  unpack(void* val, ArchiveT& ar) const {
    using Serializer_attorneys::ArchiveAccess;
    if(not ar.is_unpacking()) ArchiveAccess::start_unpacking(ar);
    std::memcpy(val, ArchiveAccess::spot(ar), sizeof(T));
    ArchiveAccess::spot(ar) += sizeof(T);
  }
};

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////


} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_BUILTIN_H
