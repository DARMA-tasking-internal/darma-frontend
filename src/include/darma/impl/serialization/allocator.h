/*
//@HEADER
// ************************************************************************
//
//                      allocator.h
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

#ifndef DARMA_IMPL_SERIALIZATION_ALLOCATOR_H
#define DARMA_IMPL_SERIALIZATION_ALLOCATOR_H

#include <memory>
#include <cassert>

#include <darma/impl/util/compressed_pair.h>

#include <darma/interface/frontend/memory_requirement_details.h>

namespace darma_runtime {

namespace serialization {

namespace detail {

template <
  abstract::frontend::MemoryRequirementDetails::memory_type_hint_t type_hint,
  abstract::frontend::MemoryRequirementDetails
    ::memory_accessibility_importance_hint_t read_importance,
  abstract::frontend::MemoryRequirementDetails
    ::memory_accessibility_importance_hint_t write_importance
>
struct StaticMemoryRequirementDetails
  : abstract::frontend::MemoryRequirementDetails
{
  memory_type_hint_t
  get_type_hint() const override {
    return type_hint;
  }
  memory_accessibility_importance_hint_t
  get_read_accessibility_importance() const override {
    return read_importance;
  }
  memory_accessibility_importance_hint_t
  get_write_accessibility_importance() const override {
    return write_importance;
  }
};

using DefaultMemoryRequirementDetails = StaticMemoryRequirementDetails<
  abstract::frontend::MemoryRequirementDetails::MemoryTypeHint::CPUMemory,
  abstract::frontend::MemoryRequirementDetails
    ::MemoryAccessibilityImportanceHint::Normal,
  abstract::frontend::MemoryRequirementDetails
    ::MemoryAccessibilityImportanceHint::Normal
>;

} // end namespace detail

/** @brief An allocator that calls DARMA backend runtime allocate() and
 *  deallocate() methods and defers to some other allocator for the rest
 *  of the allocation functionality (by default, defers to std::allocator<T>)
 */
template <typename T, typename BaseAllocator = std::allocator<T>>
struct darma_allocator
  : std::allocator_traits<BaseAllocator>::template rebind_alloc<T>
{
  private:

    using base_t = typename std::allocator_traits<BaseAllocator>::template rebind_alloc<T>;

    using base_traits_t = std::allocator_traits<base_t>;

  public:

    using const_void_pointer =
      typename std::allocator_traits<base_t>::const_void_pointer;
    using size_type = typename std::allocator_traits<base_t>::size_type;
    using pointer =
      typename std::allocator_traits<base_t>::pointer;

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using is_always_equal = std::false_type;

    template <typename U>
    struct rebind { using other = darma_allocator<U>; };

    pointer
    allocate(size_type n, const_void_pointer _ignored=nullptr);

    void
    deallocate(pointer ptr, size_type n) noexcept;

};

namespace detail {

template <typename T>
struct is_darma_allocator : std::false_type { };

template <typename... Args>
struct is_darma_allocator<darma_allocator<Args...>> : std::true_type { };

} // end namespace detail

} // end namespace serialization

} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_ALLOCATOR_H
