/*
//@HEADER
// ************************************************************************
//
//                      element_range.h.h
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

#ifndef DARMA_INTERFACE_FRONTEND_ELEMENT_RANGE_H
#define DARMA_INTERFACE_FRONTEND_ELEMENT_RANGE_H

#include <cstdlib>

#include "frontend_fwd.h"

#include "serialization_manager.h"
#include "array_concept_manager.h"

namespace darma_runtime {
namespace abstract {
namespace frontend {

/** @todo document this
 *
 */
class ElementRange {
  public:
    virtual ~ElementRange(){}

    // TODO Figure out constness here?
    // TODO figure out if it's reasonable to make this a void*& to allow
    //      assigning of the buffer to the parent (i.e., not when the subset is
    //      not even a shallow copy)
    /** @todo
     *
     */
    virtual void
    setup(void* md_buffer) =0;

    /** @todo
     *
     * @return
     */
    virtual bool
    is_deep_copy() const =0;

    /** @todo
     *
     * @return
     */
    virtual SerializationManager const*
    get_serialization_manager() const =0;

    /** @todo
     *
     * @return
     */
    virtual ArrayConceptManager const*
    get_array_concept_manager() const =0;
};

} // end namespace frontend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_INTERFACE_FRONTEND_ELEMENT_RANGE_H
