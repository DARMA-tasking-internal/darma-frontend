/*
//@HEADER
// ************************************************************************
//
//                      SerializationPolicy.h
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

#ifndef DARMA_INTERFACE_BACKEND_SERIALIZATION_POLICY_H
#define DARMA_INTERFACE_BACKEND_SERIALIZATION_POLICY_H

#include <cstdlib>
#include <algorithm>

#include <darma/impl/feature_testing_macros.h>

namespace darma_runtime {
namespace abstract {
namespace backend {

/** @todo document this
 *
 */
struct SerializationPolicy {
  public:

    /** @todo document this
     *
     * @param data_begin
     * @param n_bytes
     * @return
     */
    virtual std::size_t
    packed_size_contribution_for_blob(
      void const* data_begin, std::size_t n_bytes
    ) const {
      // default implementation just does an indirect pack
      // (so return the size here so that it gets included in
      // the indirect buffer's allocation)
      return n_bytes;
    }

    /** @todo document this
     *
     * @param indirect_pack_buffer
     * @param data_begin
     * @param n_bytes
     */
    virtual void
    pack_blob(
      void*& indirect_pack_buffer,
      void const* data_begin,
      std::size_t n_bytes
    ) const {
      // default implementation just does an indirect pack/unpack
      std::copy(
        static_cast<char const*>(data_begin),
        static_cast<char const*>(data_begin) + n_bytes,
        static_cast<char*>(indirect_pack_buffer)
      );
      reinterpret_cast<char*&>(indirect_pack_buffer) += n_bytes;
    }

    /** @todo document this
     *
     * @param indirect_packed_buffer
     * @param dest
     * @param n_bytes
     */
    virtual void
    unpack_blob(
      void*& indirect_packed_buffer,
      void* dest,
      std::size_t n_bytes
    ) const {
      // default implementation just does an indirect pack/unpack
      std::copy(
        static_cast<char*>(indirect_packed_buffer),
        static_cast<char*>(indirect_packed_buffer) + n_bytes,
        static_cast<char*>(dest)
      );
      reinterpret_cast<char*&>(indirect_packed_buffer) += n_bytes;

    }

};

} // end namespace backend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_INTERFACE_BACKEND_SERIALIZATION_POLICY_H
