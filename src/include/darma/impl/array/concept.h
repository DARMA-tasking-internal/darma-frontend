/*
//@HEADER
// ************************************************************************
//
//                      concept.h
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

#ifndef DARMA_IMPL_ARRAY_CONCEPT_H
#define DARMA_IMPL_ARRAY_CONCEPT_H

#include <darma/interface/frontend/array_concept_manager.h>
#include <darma/impl/util/smart_pointers.h>

#include "indexable.h"

namespace darma {
namespace detail {

template <typename T, typename ElementRangeT>
class ArrayConceptManagerForType
  : public abstract::frontend::ArrayConceptManager
{
  private:

    using _idx_traits = IndexingTraits<T>;

  public:

    size_t
    n_elements(void const* obj) const override {
      return _idx_traits::n_elements(*static_cast<T const*>(obj));
    }

    types::unique_ptr_template<abstract::frontend::ElementRange>
    get_element_range(
      void const* obj,
      size_t offset, size_t n_elem
    ) const override;

    void
    set_element_range(
      void* obj,
      abstract::frontend::ElementRange const& range,
      size_t offset, size_t size
    ) const override {
      void const* sub_object =
        static_cast<ElementRangeT const&>(range).get_buffer();

      _idx_traits::set_element_range(
        *static_cast<T*>(obj),
        *static_cast<typename _idx_traits::subset_object_type const*>(
          sub_object
        ),
        offset, size
      );
    }


};


} // end namespace detail
} // end namespace darma

#endif //DARMA_IMPL_ARRAY_CONCEPT_H
