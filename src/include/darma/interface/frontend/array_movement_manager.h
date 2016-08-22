/*
//@HEADER
// ************************************************************************
//
//                      array_movement_manager.h
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

#ifndef DARMA_INTERFACE_FRONTEND_ARRAY_MOVEMENT_MANAGER_H
#define DARMA_INTERFACE_FRONTEND_ARRAY_MOVEMENT_MANAGER_H

#include <darma/interface/backend/serialization_policy.h>
#include <darma/interface/frontend/index_range.h>

namespace darma_runtime {
namespace abstract {
namespace frontend {

/** @todo document this
 *
 */
class ArrayMovementManager {
  public:

    /** @todo
     *
     * @param obj
     * @param offset
     * @param n_elem
     * @param ser_pol
     * @return
     */
    virtual size_t
    get_packed_size(
      void const* obj,
      size_t offset, size_t n_elem,
      backend::SerializationPolicy* ser_pol
    ) const =0;

    /** @todo
     *
     * @param obj
     * @param idx_range
     * @param ser_pol
     * @return
     */
    virtual size_t
    get_packed_size(
      void const* obj,
      IndexRange const* idx_range,
      backend::SerializationPolicy* ser_pol
    ) const {
      // If this method is unimplemented, range must be contiguous
      assert(idx_range->contiguous());
      return get_packed_size(
        obj, idx_range->offset(), idx_range->size(), ser_pol
      );
    }

    /** @todo
     *
     * @param obj
     * @param buffer
     * @param offset
     * @param n_elem
     * @param ser_pol
     */
    virtual void
    pack_elements(
      void const* obj, void* buffer,
      size_t offset, size_t n_elem,
      backend::SerializationPolicy* ser_pol
    ) const =0;

    /** @todo
     *
     * @param obj
     * @param buffer
     * @param idx_range
     * @param ser_pol
     */
    virtual void
    pack_elements(
      void const* obj, void* buffer,
      IndexRange const* idx_range,
      backend::SerializationPolicy* ser_pol
    ) const {
      // If this method is unimplemented, range must be contiguous
      assert(idx_range->contiguous());
      pack_elements(
        obj, buffer, idx_range->offset(), idx_range->size(), ser_pol
      );
    }

    /** @todo
     *
     * @param obj
     * @param buffer
     * @param offset
     * @param n_elem
     * @param ser_pol
     */
    virtual void
    unpack_elements(
      void* obj, void const* buffer,
      size_t offset, size_t n_elem,
      backend::SerializationPolicy* ser_pol
    ) const =0;

    /** @todo
     *
     * @param obj
     * @param buffer
     * @param idx_range
     * @param ser_pol
     */
    virtual void
    unpack_elements(
      void* obj, void const* buffer,
      IndexRange const* idx_range,
      backend::SerializationPolicy* ser_pol
    ) const {
      // If this method is unimplemented, range must be contiguous
      assert(idx_range->contiguous());
      unpack_elements(
        obj, buffer, idx_range->offset(), idx_range->size(), ser_pol
      );
    }
};

} // end namespace frontend
} // end namespace abstract
} // end namespace darma_runtime


#endif //DARMA_INTERFACE_FRONTEND_ARRAY_MOVEMENT_MANAGER_H
