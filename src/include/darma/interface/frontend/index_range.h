/*
//@HEADER
// ************************************************************************
//
//                      index_range.h
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

#ifndef DARMA_INTERFACE_FRONTEND_INDEX_RANGE_H
#define DARMA_INTERFACE_FRONTEND_INDEX_RANGE_H

#include <cstdlib> // size_t

#include <darma_types.h>

#include "polymorphic_serializable_object.h"

namespace darma_runtime {
namespace abstract {
namespace frontend {

// TODO deprecate this
class Index { };

// TODO deprecate this
struct IndexIterator {
  virtual Index& operator*() =0;
  virtual IndexIterator& operator++() =0;
  virtual ~IndexIterator() = default;
};

/** @todo
 *
 */
class IndexRange
  : public PolymorphicSerializableObject<IndexRange>
{
  public:
    /** @todo
     *
     * @return
     */
    virtual size_t size() const =0;

};

// TODO deprecate this
class CompactIndexRange : public IndexRange
{
  public:


    /** @todo
     *
     * @return
     */
    virtual size_t offset() const =0;

    /** @todo
     *
     * @return
     */
    virtual bool contiguous() const =0;

    /** @todo
     *
     * @return
     */
    virtual bool strided() const =0;

    /** @todo
     *
     * @return
     */
    virtual size_t stride() const =0;

};

// TODO deprecate this
template <typename FromRange, typename ToRange>
struct IndexMapping : PolymorphicSerializableObject<IndexMapping<FromRange, ToRange>> {

  virtual std::unique_ptr<ToRange const>
  map_forward(FromRange const& from) const =0;

  virtual std::unique_ptr<FromRange const>
  map_reverse(ToRange const& from) const =0;

};



} // end namespace frontend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_INTERFACE_FRONTEND_INDEX_RANGE_H
