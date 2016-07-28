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

#include <darma/interface/backend/allocation_policy.h>

namespace darma_runtime {

namespace serialization {

/** @brief a abstract::backend::AllocationPolicy-aware allocator
 *
 */
template <typename T, typename BaseAllocator = std::allocator<T>>
struct darma_allocator
  : BaseAllocator
{
  private:

    abstract::backend::AllocationPolicy* policy_ = nullptr;

    using base_t = BaseAllocator;

  public:

    using const_void_pointer =
      typename std::allocator_traits<base_t>::const_void_pointer;
    using size_type = std::allocator_traits<base_t>::size_type;
    using pointer =
      typename std::allocator_traits<base_t>::pointer;

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using is_always_equal = std::false_type;

    template <typename U>
    struct rebind { using other = darma_allocator<U>; };

    explicit
    darma_allocator(abstract::backend::AllocationPolicy* policy)
      : policy_(policy)
    { }

    pointer
    allocate(size_type n, const_void_pointer _ignored=nullptr) {
      if(policy_) policy_->allocate( sizeof(T) * n );
      else this->base_t::allocate(n, _ignored);
    }

    void
    deallocate(pointer ptr, size_type n) noexcept {
      if(policy_) policy_->deallocate( ptr, sizeof(T) * n );
      else this->base_t::deallocate(ptr, n);
    }

};

} // end namespace serialization

} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_ALLOCATOR_H
