/*
//@HEADER
// ************************************************************************
//
//                      policy_aware_archive.h
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

#ifndef DARMA_IMPL_SERIALIZATION_POLICY_AWARE_ARCHIVE_H
#define DARMA_IMPL_SERIALIZATION_POLICY_AWARE_ARCHIVE_H

#include <type_traits>

#include <tinympl/bool.hpp>

#include "archive.h"
#include <darma/interface/backend/allocation_policy.h>
#include <darma/impl/serialization/allocator.h>

namespace darma_runtime {
namespace serialization {

class PolicyAwareArchive
  : public detail::ArchivePassthroughMixin<PolicyAwareArchive>,
    public detail::ArchiveRangesMixin<PolicyAwareArchive,
      detail::ArchiveOperatorsMixin<PolicyAwareArchive>
    >
{
  private:

    template <typename T>
    using traits = detail::serializability_traits<T>;
    template <typename T, bool additional_condition>
    using enable_if_serializable_and = std::enable_if_t<
      traits<T>::template is_serializable_with_archive<PolicyAwareArchive>::value
      and additional_condition
    >;

    template <typename T>
    using enable_if_serializable = enable_if_serializable_and<T, true>;

    template <typename T, typename AllocatorT>
    using enable_if_serializable_and_not_darma_allocator = enable_if_serializable_and<T,
      not detail::is_darma_allocator<AllocatorT>::value
    >;
    template <typename T, typename AllocatorT>
    using enable_if_serializable_and_is_darma_allocator = enable_if_serializable_and<T,
      detail::is_darma_allocator<AllocatorT>::value
    >;

    abstract::backend::SerializationPolicy const* serialization_policy_;

    darma_allocator<void> template_allocator_;

    void* start;
    void* spot;

  public:

    //------------------------------------------------------------------------//

    PolicyAwareArchive(
      abstract::backend::SerializationPolicy* ser_pol,
      abstract::backend::AllocationPolicy* alloc_policy
    ) : serialization_policy_(ser_pol),
      template_allocator_(alloc_policy)
    { }

    explicit
    PolicyAwareArchive(
      abstract::backend::SerializationPolicy* ser_pol
    ) : PolicyAwareArchive(ser_pol, nullptr)
    { }


    //------------------------------------------------------------------------//

    // Need an AllocationPolicy-aware unpack_item

    template <typename T>
    enable_if_serializable<T>
    unpack_item(void* val) {
      traits<T>::unpack( val, *this, get_unpack_allocator<T>());
    }

    template <typename T, typename AllocatorT>
    enable_if_serializable_and_not_darma_allocator<T, AllocatorT>
    unpack_item(void* val, AllocatorT&& in_alloc) {
      traits<T>::unpack( val, *this,
        get_unpack_allocator<T>(std::forward<AllocatorT>(in_alloc))
      );
    };

    template <typename T, typename AllocatorT>
    enable_if_serializable_and_is_darma_allocator<T, AllocatorT>
    unpack_item(void* val, AllocatorT&& in_alloc) {
      // if the other's allocation policy is different, just use the other one,
      // since someone who did this probably knows what they're doing
      traits<T>::unpack( val, *this, std::forward<AllocatorT>(in_alloc) );
    };

    // Deduced overloads

    template <typename T>
    enable_if_serializable<T>
    unpack_item(T& val) {
      traits<T>::unpack(
        const_cast<void*>(static_cast<const void*>(&val)),
        *this, get_unpack_allocator<T>()
      );
    }

    template <typename T, typename AllocatorT>
    enable_if_serializable_and_not_darma_allocator<T, AllocatorT>
    unpack_item(T& val, AllocatorT&& in_alloc) {
      traits<T>::unpack(
        const_cast<void*>(static_cast<const void*>(&val)),
        *this, get_unpack_allocator<T>(std::forward<AllocatorT>(in_alloc))
      );
    };

    template <typename T, typename AllocatorT>
    enable_if_serializable_and_is_darma_allocator<T, AllocatorT>
    unpack_item(T& val, AllocatorT&& in_alloc) {
      // if the other's allocation policy is different, just use the other one,
      // since someone who did this probably knows what they're doing
      traits<T>::unpack(
        const_cast<void*>(static_cast<const void*>(&val)),
        *this, std::forward<AllocatorT>(in_alloc)
      );
    };

    //------------------------------------------------------------------------//

    inline void
    add_to_size_indirect(size_t size) {
      assert(is_sizing());
      spot = static_cast<char*>(spot) + size;
    }

    //------------------------------------------------------------------------//

    template <typename ContiguousIterator>
    inline void
    add_to_size_direct(
      ContiguousIterator begin, size_t N
    ) {
      assert(is_sizing());
      using value_type = typename std::iterator_traits<ContiguousIterator>::value_type;
      size_t to_add = serialization_policy_->packed_size_contribution_for_blob(
        static_cast<void const*>(std::addressof(*begin)), N*sizeof(value_type)
      );
      spot = static_cast<char*>(spot) + to_add;
    }

    //------------------------------------------------------------------------//

    template <typename InputIterator>
    inline void pack_indirect(InputIterator begin, InputIterator end) {
      using value_type = typename std::iterator_traits<InputIterator>::value_type;
      size_t packed_size = std::distance(begin, end) * sizeof(value_type);
      std::copy(begin, end,
        static_cast<value_type*>(spot)
      );
      spot = static_cast<char*>(spot) + packed_size;
    }

    //------------------------------------------------------------------------//

    template <typename DirectlySerializableType, typename OutputIterator>
    inline void unpack_indirect(OutputIterator out_begin, size_t n_items) {
      using value_type = typename std::iterator_traits<OutputIterator>::value_type;
      size_t n_bytes = sizeof(DirectlySerializableType) * n_items;
      std::move(
        static_cast<DirectlySerializableType*>(spot),
        reinterpret_cast<DirectlySerializableType*>(
          static_cast<char*>(spot) + sizeof(DirectlySerializableType) * n_items
        ),
        out_begin
      );
      spot = static_cast<char*>(spot) + n_bytes;
    }

    //------------------------------------------------------------------------//

    template <typename ContiguousIterator>
    inline void pack_direct(ContiguousIterator begin, ContiguousIterator end) {
      assert(is_packing());

      using value_type = typename std::iterator_traits<ContiguousIterator>::value_type;

      serialization_policy_->pack_blob(spot,
        static_cast<void const*>(std::addressof(*begin)),
        std::distance(begin, end) * sizeof(value_type)
      );
    }

    //------------------------------------------------------------------------//

    template <typename DirectlySerializableType, typename ContiguousOutputIterator>
    inline void unpack_direct(ContiguousOutputIterator dest, size_t n_items) {
      // Check that ContiguousOutputIterator is an output iterator
      static_assert(meta::is_output_iterator<ContiguousOutputIterator>::value,
        "OutputIterator must be an output iterator."
      );
      assert(is_unpacking());

      using value_type = typename std::iterator_traits<ContiguousOutputIterator>::value_type;

      serialization_policy_->unpack_blob(spot,
        static_cast<void*>(std::addressof(*dest)),
        n_items * sizeof(value_type)
      );
    }

    //------------------------------------------------------------------------//

    template <typename T,
      typename=std::enable_if_t<
        sizeof(darma_allocator<T>) == sizeof(darma_allocator<void>)
      >
    >
    auto&
    get_unpack_allocator() {
      return *reinterpret_cast<darma_allocator<T>*>(&template_allocator_);
    }

    template <typename T,
      typename=std::enable_if_t<
        sizeof(darma_allocator<T>) != sizeof(darma_allocator<void>)
      >
    >
    auto
    get_unpack_allocator() {
      // Not safe to reinterpret-cast (could be specialized); need to create a
      // new instance
      return darma_allocator<T>(template_allocator_.policy());
    }

    template <typename T, typename WrappedAllocator>
    decltype(auto) get_unpack_allocator(WrappedAllocator&& alloc) {
      return _get_unpack_allocator_impl<T>(
        tinympl::bool_<
          sizeof(darma_allocator<T, WrappedAllocator>) == sizeof(darma_allocator<void>)
            and std::is_empty<WrappedAllocator>::value
        >(),
        std::integral_constant<bool,
          detail::is_darma_allocator<std::decay_t<WrappedAllocator>>::value
        >(),
        std::forward<WrappedAllocator>(alloc)
      );
    }

  private:

    template <typename T, typename WrappedAllocator, typename _IgnoredCondition>
    auto& _get_unpack_allocator_impl(
      std::true_type, _IgnoredCondition,
      WrappedAllocator&& alloc
    ) {
      return *reinterpret_cast<darma_allocator<T, WrappedAllocator>*>(&template_allocator_);
    }

    template <typename T, typename WrappedAllocator>
    auto _get_unpack_allocator_impl(
      std::false_type, std::true_type,
      WrappedAllocator&& alloc
    ) {
      // we just need a copy with a different policy
      WrappedAllocator rv = std::forward<WrappedAllocator>(alloc);
      rv.set_policy(template_allocator_.policy());
      return rv;
    }

    template <typename T, typename WrappedAllocator>
    auto _get_unpack_allocator_impl(
      std::false_type, std::false_type,
      WrappedAllocator&& alloc
    ) {
      // worst case, we have to make completely new one
      return darma_allocator<T>(template_allocator_.policy(),
        std::forward<WrappedAllocator>(alloc)
      );
    }

    friend class Serializer_attorneys::ArchiveAccess;
    friend class darma_runtime::detail::DependencyHandle_attorneys::ArchiveAccess;
};

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_POLICY_AWARE_ARCHIVE_H
