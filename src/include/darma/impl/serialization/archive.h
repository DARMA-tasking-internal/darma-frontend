/*
//@HEADER
// ************************************************************************
//
//                         archive.h
//                          DARMA
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

#ifndef DARMA_IMPL_SERIALIZATION_ARCHIVE_H_
#define DARMA_IMPL_SERIALIZATION_ARCHIVE_H_

#include <type_traits>
#include <iterator>

#include <tinympl/is_instantiation_of.hpp>
#include <darma/impl/meta/is_container.h>
#include <darma/interface/backend/serialization_policy.h>

#include "serialization_fwd.h"
#include "nonintrusive.h"
#include "range.h"
#include "archive_mixins.h"
#include "archive_passthrough.h"

namespace darma_runtime {

namespace serialization {

////////////////////////////////////////////////////////////////////////////////

// A simple frontend object for interacting with user-defined serializations
// TODO Archives that check for pointer loops and stuff

class SimplePackUnpackArchive
  : public detail::ArchivePassthroughMixin<SimplePackUnpackArchive>,
    public detail::ArchiveRangesMixin<SimplePackUnpackArchive,
      detail::ArchiveOperatorsMixin<SimplePackUnpackArchive>
    >
{
  private:
    // readability alias
    using byte = char;

    byte* start = nullptr;
    byte* spot = nullptr;

  public:

    inline void add_to_size(size_t size) {
      assert(is_sizing());
      spot += size;
    }

    template <typename InputIterator>
    inline void add_to_size_contiguous(
      InputIterator begin, size_t N
    ) {
      assert(is_sizing());
      using value_type = std::remove_reference_t<decltype(*begin)>;
      spot += N * sizeof(value_type);
    }

    template <typename InputIterator>
    inline void pack_contiguous(InputIterator begin, InputIterator end) {
      // Check that InputIterator is an input iterator
      static_assert(std::is_base_of<std::input_iterator_tag,
          typename std::iterator_traits<InputIterator>::iterator_category
        >::value,
        "InputIterator must be an input iterator."
      );
      assert(is_packing());

      using value_type =
        std::remove_const_t<std::remove_reference_t<decltype(*begin)>>;
      std::copy(begin, end, reinterpret_cast<value_type*>(spot));
      const size_t sz = sizeof(value_type);
      spot += std::distance(begin, end) * sizeof(value_type);
    }

    template <typename DirectlySerializableType, typename OutputIterator>
    inline void unpack_contiguous(OutputIterator dest, size_t n_items) {
      // Check that OutputIterator is an output iterator
      static_assert(meta::is_output_iterator<OutputIterator>::value,
        "OutputIterator must be an output iterator."
      );
      assert(is_unpacking());

      std::move(
        reinterpret_cast<DirectlySerializableType*>(spot),
        reinterpret_cast<DirectlySerializableType*>(spot)+n_items,
        dest
      );
      spot += n_items * sizeof(DirectlySerializableType);
    }

  private:

    // TODO get rid of these?!?!?
    friend class Serializer_attorneys::ArchiveAccess;
    friend class darma_runtime::detail::DependencyHandle_attorneys::ArchiveAccess;

};

////////////////////////////////////////////////////////////////////////////////

} // end namespace serialization

} // end namespace darma_runtime

#endif /* DARMA_IMPL_SERIALIZATION_ARCHIVE_H_ */
