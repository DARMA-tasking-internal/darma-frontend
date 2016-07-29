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
    using value_type_is_serializable =
      typename value_traits::template is_serializable_with_archive<ArchiveT>;

    static constexpr auto value_type_is_directly_serializable =
      value_traits::is_directly_serializable;

    using vector_t = std::vector<T, Allocator>;

    using allocator_t = Allocator;

    template <typename ArchiveT>
    using enable_if_serializable = std::enable_if_t<
      value_type_is_serializable<ArchiveT>::value
    >;

    template <typename ArchiveT>
    inline enable_if_serializable<ArchiveT>
    _unpack_contiguous_if_possible(
      std::true_type, ArchiveT& ar, vector_t& v, size_t& size
    ) const {
      ar.template unpack_contiguous<value_type>(v.data(), size);
    }

    template <typename ArchiveT>
    inline enable_if_serializable<ArchiveT>
    _unpack_contiguous_if_possible(
      std::false_type, ArchiveT& ar, vector_t& v, size_t& size
    ) const {
      for(size_t i = 0; i < size; ++i) {
        value_traits::unpack(v.data()[i], ar, Allocator());
      }
    }

  public:

    template <typename ArchiveT, typename ParentAllocator>
    enable_if_serializable<ArchiveT>
    unpack(void* allocated, ArchiveT& ar, ParentAllocator&& p_alloc) const {
      // The unpack corresponding to the SerDesRange-based pack
      size_t size = 0;
      ar.unpack_item(size);
      // Construct as a std::vector of char arrays of the right size so that
      // the memory gets allocated but the constructors don't get invoked
      // (the element unpacks will do this)
      std::allocator_traits<std::decay_t<ParentAllocator>>::construct(
        p_alloc, static_cast<
          std::vector<
            std::array<char, sizeof(value_type)>,
            typename std::allocator_traits<Allocator>
              ::template rebind_alloc<std::array<char, sizeof(value_type)>>
          >*
        >(allocated),
        size, ar.template get_unpack_allocator<T>(Allocator())
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
