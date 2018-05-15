/*
//@HEADER
// ************************************************************************
//
//                      element_range.h
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
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

#ifndef DARMA_IMPL_ARRAY_ELEMENT_RANGE_H
#define DARMA_IMPL_ARRAY_ELEMENT_RANGE_H

#include <darma/interface/frontend/element_range.h>
#include <darma/interface/frontend/serialization_manager.h>
#include <darma/interface/frontend/array_concept_manager.h>
#include <darma/interface/frontend/array_movement_manager.h>
#include <darma/impl/serialization/manager.h>

#include "index_decomposition.h"
#include "indexable.h"
#include "concept.h"

namespace darma {
namespace detail {

// Simple implies subset object is always the same type as parent
template <typename T>
class SimpleElementRange
  : public abstract::frontend::ElementRange,
    public serialization::detail::SerializationManagerForType<T>,
    public ArrayConceptManagerForType<T, SimpleElementRange<T>> //,
    //public abstract::frontend::ArrayMovementManager
{
  private:

    using _idx_traits = IndexingTraits<T>;

  public:

    SimpleElementRange(T const& parent, size_t offset, size_t n_elem)
      : parent_(parent), offset_(offset), n_elem_(n_elem)
    { }

    ////////////////////////////////////////////////////////////////////////////
    // <editor-fold desc="abstract::frontend::ElementRange implementation">


    void
    setup(void* md_buffer) override {
      md_buffer_ = md_buffer;
      _idx_traits::make_subobject(md_buffer, parent_, offset_, n_elem_);
    }

    bool
    is_deep_copy() const override {
      return _idx_traits::is_deep_copy(
        *static_cast<T const*>(md_buffer_)
      );
    }

    abstract::frontend::SerializationManager const*
    get_serialization_manager() const override {
      return this;
    }

    abstract::frontend::ArrayConceptManager const*
    get_array_concept_manager() const override {
      return this;
    }

    // TODO array movement manager for a simple element range
//    abstract::frontend::ArrayMovementManager const*
//    get_array_movement_manager() const override {
//      return this;
//    }

    // end abstract::frontend::ElementRange implementation </editor-fold>
    ////////////////////////////////////////////////////////////////////////////

    void* get_buffer() { return md_buffer_; }

    void const* get_buffer() const { return md_buffer_; }

  private:

    void* md_buffer_ = nullptr;

    T const& parent_;

    size_t offset_;
    size_t n_elem_;

};

} // end namespace detail
} // end namespace darma

#endif //DARMA_IMPL_ARRAY_ELEMENT_RANGE_H
