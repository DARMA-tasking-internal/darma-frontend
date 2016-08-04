/*
//@HEADER
// ************************************************************************
//
//                      array_concept_manager.h
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

#ifndef DARMA_INTERFACE_FRONTEND_ARRAY_CONCEPT_MANAGER_H
#define DARMA_INTERFACE_FRONTEND_ARRAY_CONCEPT_MANAGER_H

#include "element_range.h"

namespace darma_runtime {
namespace abstract {
namespace frontend {

/** @todo document this
 *
 */
class ArrayConceptManager {
  public:

    /** @todo
     *
     * @param obj
     * @return
     */
    virtual size_t
    n_elements(void const* obj) const =0;

    /** @todo
     *
     * @param obj
     * @param offset
     * @param n_elem
     * @return
     */
    virtual ElementRange
    get_element_range(
      void const* obj,
      size_t offset, size_t n_elem
    ) const =0;

    /** @todo
     *
     * @param obj
     * @param range
     * @param offset
     * @param size
     */
    virtual void
    set_element_range(
      void* obj,
      ElementRange const& range,
      size_t offset, size_t size
    ) const =0;

    /** @todo
     *
     * @return
     */
    virtual SerializationManager const*
    get_element_serialization_manager() const =0;
};

} // end namespace frontend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_INTERFACE_FRONTEND_ARRAY_CONCEPT_MANAGER_H
