/*
//@HEADER
// ************************************************************************
//
//                      use_collection.h
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

#ifndef DARMA_USE_COLLECTION_H
#define DARMA_USE_COLLECTION_H

#include <cstdlib> // std::size_t
#include <vector>
#include <darma/impl/util/optional_boolean.h>

namespace darma_runtime {

namespace abstract {

namespace frontend {

/** @todo document this
 *
 */
class UseCollection {
  public:
    template <typename T>
    using index_iterable = std::vector<T>;  // TODO make a template alias for this in frontend/types.h

    /** @brief Indicates if this `UseCollection` is mapped over a
     * `TaskCollection` that depends on it.
     *
     *  @remark If this `Use` that manages this is not dependency of a
     *  `TaskCollection` (and is not going to be in its lifetime), this will
     *  always return `false`.
     *
     *  @remark If this method returns `false`, the return values of
     *  `local_indices_for` and `task_index_for` are unspecified.
     *
     *  @invariant If the `Use` managing this `UseCollection` requests immediate
     *  permissions other than `None`, this must be mapped (i.e., this method
     *  must return `true`).
     *
     *  @return `true` if the `UseCollection` is mapped across a `TaskCollection`;
     * `false` otherwise.
     */
    virtual bool
    is_mapped() const =0;

    /** @todo document this
     *
     *  @param task_index
     *  @return
     */
    virtual index_iterable<std::size_t>
    local_indices_for(std::size_t task_index) const =0;

    virtual std::size_t
    task_index_for(std::size_t collection_index) const =0;

    /** @todo document this
     *
     *  @param other
     *  @return
     */
    virtual OptionalBoolean
    has_same_mapping_as(UseCollection const* other) const =0;

    virtual std::size_t
    size() const =0;

};

} // end namespace frontend

} // end namespace abstract

} // end namespace darma_runtime

#endif //DARMA_USE_COLLECTION_H
