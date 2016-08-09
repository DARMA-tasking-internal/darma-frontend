/*
//@HEADER
// ************************************************************************
//
//                      element_range.h
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

#ifndef DARMA_IMPL_ARRAY_ELEMENT_RANGE_H
#define DARMA_IMPL_ARRAY_ELEMENT_RANGE_H

#include <darma/interface/frontend/element_range.h>
#include <darma/interface/frontend/serialization_manager.h>
#include <darma/interface/frontend/array_concept_manager.h>

#include "index_decomposition.h"
#include "indexable.h"

namespace darma_runtime {
namespace detail {

// Simple implies subset object is always the same type as parent
template <typename T>
class SimpleElementRange
  : public abstract::frontend::ElementRange,
    public abstract::frontend::SerializationManager,
    public abstract::frontend::ArrayConceptManager
{
  private:

    using _idx_traits = IndexingTraits<T>;

  public:

    SimpleElementRange(T& parent, size_t offset, size_t n_elem)
      : parent_(parent), offset_(offset), n_elem_(n_elem)
    { }

    ////////////////////////////////////////////////////////////////////////////
    // <editor-fold desc="abstract::frontend::ElementRange implementation">


    void
    setup(void* md_buffer) override {
      _idx_traits::make_subobject(md_buffer, parent_, offset_, n_elem_);
    }

    bool
    is_deep_copy() const override {
      // TODO
      abort();
    }

    abstract::frontend::SerializationManager const*
    get_serialization_manager() const {
      return this;
    }

    abstract::frontend::ArrayConceptManager const*
    get_array_concept_manager() const {
      return this;
    }

    // end abstract::frontend::ElementRange implementation </editor-fold>
    ////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////
    // <editor-fold desc="abstract::frontend::SerializationManager implementation">

    size_t
    get_metadata_size() const override {
      return sizeof(T);
    }

    size_t
    get_packed_data_size(
      const void* const object_data
      abstract::backend::SerializationPolicy* ser_policy
    ) const override {
      // TODO
    }

    // end abstract::frontend::SerializationManager implementation </editor-fold>
    ////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////
    // <editor-fold desc="abstract::frontend::ArrayConceptManager implementation">

    // end abstract::frontend::ArrayConceptManager implementation </editor-fold>
    ////////////////////////////////////////////////////////////////////////////

  private:

    T& parent_;

    size_t offset_;
    size_t n_elem_;

};


} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_ARRAY_ELEMENT_RANGE_H
