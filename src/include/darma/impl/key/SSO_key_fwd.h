/*
//@HEADER
// ************************************************************************
//
//                      SSO_key_fwd.h
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

#ifndef DARMA_IMPL_KEY_SSO_KEY_FWD_H
#define DARMA_IMPL_KEY_SSO_KEY_FWD_H

#include <cstdint>
#include <cstdlib>

namespace darma_runtime {
namespace detail {

namespace _impl {

typedef enum {
  Long = (uint8_t)0,
  Short = (uint8_t)1,
  BackendAssigned = (uint8_t)2,
  NeedsBackendAssignedKey = (uint8_t)3
} sso_key_mode_t;

} // end namespace _impl

template <
  /* default allows it to fit in a cache line */
  std::size_t BufferSize = 64 - sizeof(std::size_t) - 8 - 8,
  typename BackendAssignedKeyType = std::size_t,
  typename PieceSizeOrdinal = uint8_t,
  typename ComponentCountOrdinal = uint8_t
>
class SSOKey;

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_KEY_SSO_KEY_FWD_H
