/*
//@HEADER
// ************************************************************************
//
//                      unique_ptr.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_IMPL_SERIALIZATION_UNIQUE_PTR_H
#define DARMAFRONTEND_IMPL_SERIALIZATION_UNIQUE_PTR_H

#include <memory>

#include "nonintrusive.h"
#include "traits.h"

namespace darma_runtime {
namespace serialization {

// TODO handle deleter
template <typename T>
struct Serializer<std::unique_ptr<T>>
{
  private:

    using value_type = T;
    using value_traits = detail::serializability_traits<T>;

    template <typename ArchiveT>
    using value_type_is_sizable =
      typename value_traits::template is_sizable_with_archive<ArchiveT>;

    template <typename ArchiveT>
    using value_type_is_packable =
      typename value_traits::template is_packable_with_archive<ArchiveT>;

    template <typename ArchiveT>
    using value_type_is_unpackable =
      typename value_traits::template is_unpackable_with_archive<ArchiveT>;

  public:

    template <typename ArchiveT>
    std::enable_if_t<value_type_is_sizable<ArchiveT>::value>
    compute_size(std::unique_ptr<T> const& val, ArchiveT& ar) {
      ar % bool(val);
      if(val) {
        ar % (*val.get());
      }
    }

    template <typename ArchiveT>
    std::enable_if_t<value_type_is_packable<ArchiveT>::value>
    pack(std::unique_ptr<T> const& val, ArchiveT& ar) {
      ar << bool(val);
      if(val) {
        ar << (*val.get());
      }
    }

    template <typename ArchiveT, typename AllocatorT>
    std::enable_if_t<value_type_is_packable<ArchiveT>::value>
    unpack(void* allocated, ArchiveT& ar, AllocatorT&& alloc) {
      bool is_nonnull;
      ar >> is_nonnull;
      if(is_nonnull) {
        using value_type_as_bytes = std::array<char, sizeof(value_type)>;
        using allocator_of_bytes = typename std::allocator_traits<std::decay_t<AllocatorT>>
          ::template rebind_alloc<value_type_as_bytes>;
        using allocator_of_bytes_traits = std::allocator_traits<allocator_of_bytes>;
        value_type_as_bytes* value_allocated = allocator_of_bytes_traits::allocate(
          // TODO potentially unpack this instead
          allocator_of_bytes(),
          1
        );

        auto& val = value_traits::reconstruct(value_allocated, ar, alloc);
        ar >> val;
        new (allocated) std::unique_ptr<T>(reinterpret_cast<value_type*>(
          value_allocated
        ));
      }
    }

};


} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_IMPL_SERIALIZATION_UNIQUE_PTR_H
