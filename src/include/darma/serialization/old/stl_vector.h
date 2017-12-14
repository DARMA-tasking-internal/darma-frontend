/*
//@HEADER
// ************************************************************************
//
//                      vector.h
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

#ifndef DARMA_IMPL_SERIALIZATION_STL_VECTOR_H
#define DARMA_IMPL_SERIALIZATION_STL_VECTOR_H

#include <vector>

#include "nonintrusive.h"
#include "traits.h"

namespace darma_runtime {

namespace serialization {

// Faster specialization for std::vector specifically
template <typename T, typename Allocator>
struct Serializer<std::vector<T, Allocator>>
  // Inherit from the general container specialization
  : detail::Serializer_enabled_if<std::vector<T, Allocator>>
{
  private:

    using value_type = T;
    using value_traits = detail::serializability_traits<T>;

    template <typename ArchiveT>
    using value_type_is_unpackable =
      typename value_traits::template is_unpackable_with_archive<ArchiveT>;

    static constexpr auto value_type_is_directly_serializable =
      value_traits::is_directly_serializable;

    using vector_t = std::vector<T, Allocator>;

    using allocator_t = Allocator;

    template <typename ArchiveT>
    using enable_if_unpackable = std::enable_if_t<
      value_type_is_unpackable<ArchiveT>::value
    >;

    template <typename ArchiveT>
    inline enable_if_unpackable<ArchiveT>
    _unpack_contiguous_if_possible(
      std::true_type, ArchiveT& ar, vector_t& v, size_t& size
    ) const {
      ar.template unpack_direct<value_type>(v.data(), size);
    }

    template <typename ArchiveT>
    inline enable_if_unpackable<ArchiveT>
    _unpack_contiguous_if_possible(
      std::false_type, ArchiveT& ar, vector_t& v, size_t& size
    ) const {
      for(size_t i = 0; i < size; ++i) {
        value_traits::unpack(&(v.data()[i]), ar, Allocator());
      }
    }

  public:

    template <typename ArchiveT>
    enable_if_unpackable<ArchiveT>
    unpack(void* allocated, ArchiveT& ar) const {
      return unpack(allocated, ar, darma_allocator<std::vector<T, Allocator>>());
    }

    template <typename ArchiveT, typename ParentAllocator>
    enable_if_unpackable<ArchiveT>
    unpack(void* allocated, ArchiveT& ar, ParentAllocator&& p_alloc) const {
      // The unpack corresponding to the SerDesRange-based pack
      size_t size = 0;
      ar.unpack_item(size);

      // Construct as a std::vector of char arrays of the right size so that
      // the memory gets allocated but the constructors don't get invoked
      // (the element unpacks will do this)
      using value_type_as_bytes = std::array<char, sizeof(value_type)>;
      using allocator_of_bytes = typename std::allocator_traits<Allocator>
        ::template rebind_alloc<value_type_as_bytes>;
      using vector_of_bytes = std::vector<
        value_type_as_bytes, allocator_of_bytes
      >;
      using parent_allocator = std::decay_t<ParentAllocator>;
      // Don't rebind the parent allocator to vector_of_bytes, since the
      // Allocator concept implies that the type of the argument pointer
      // (rather than the allocator's value_type) should be used for
      // construction.
      // TODO handle all of the propagate_on_* typedefs from the standard
      std::allocator_traits<parent_allocator>::construct(
        p_alloc, static_cast<vector_of_bytes*>(allocated),
        size, Allocator()
      );

      vector_t& v = *static_cast<vector_t*>(allocated);
      _unpack_contiguous_if_possible(
        std::integral_constant<bool, value_type_is_directly_serializable>(),
        ar, v, size
      );
    }

};


} // end namespace serialization

} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_STL_VECTOR_H
