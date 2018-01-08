/*
//@HEADER
// ************************************************************************
//
//                      pointer_reference_archive.h
//                         DARMA
//              Copyright (C) 2018 Sandia Corporation
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

#ifndef DARMAFRONTEND_SERIALIZATION_POINTER_REFERENCE_ARCHIVE_H
#define DARMAFRONTEND_SERIALIZATION_POINTER_REFERENCE_ARCHIVE_H

#include "pointer_reference_handler_fwd.h"

#include <darma/serialization/simple_archive.h>

#include <cassert>

namespace darma_runtime {
namespace serialization {

namespace detail {

struct _not_a_serialization_buffer { };

} // end namespace detail

template <typename SerializationBuffer=detail::_not_a_serialization_buffer>
class PointerReferencePackingArchive {
  private:

    darma_runtime::detail::compressed_pair<char*&, SerializationBuffer> data_spot_;

    template <typename... SerializationBufferCtorArgs>
    PointerReferencePackingArchive(
      char*& ptr, SerializationBufferCtorArgs&&... args
    ) : data_spot_(
          std::piecewise_construct,
          std::forward_as_tuple(ptr),
          std::forward_as_tuple(std::forward<SerializationBufferCtorArgs>(args)...)
        )
    { }

    // Put this here to do the messy casting for interfacing with a void* in one place
    explicit
    PointerReferencePackingArchive(void*& ptr)
      : PointerReferencePackingArchive(*reinterpret_cast<char**>(&ptr))
    { }

    char*& _data_spot() { return data_spot_.first(); }

    template <typename>
    friend struct PointerReferenceSerializationHandler;

  public:

    PointerReferencePackingArchive(PointerReferencePackingArchive&&) = default;

    static constexpr bool is_sizing() { return false; }
    static constexpr bool is_packing() { return true; }
    static constexpr bool is_unpacking() { return false; }

    template <typename ContiguousIterator>
    void pack_data_raw(ContiguousIterator begin, ContiguousIterator end) {
      using value_type =
        std::remove_const_t<std::remove_reference_t<decltype(*begin)>>;
      // Use memcpy instead of std::copy, since copy invokes the assignment
      // operator, and "raw" implies that this isn't necessary
      auto size = std::distance(begin, end) * sizeof(value_type);
      assert(size >= 0);
      std::memcpy(_data_spot(), static_cast<void const*>(begin), size);
      _data_spot() += size;
    }

    template <typename T>
    inline auto& operator|(T const& obj) & {
      return this->operator<<(obj);
    }

    template <typename T>
    inline auto& operator<<(T const& obj) & {
      darma_pack(obj, *this);
      return *this;
    }

    void*& data_pointer_reference() { return *reinterpret_cast<void**>(&data_spot_.first()); }
};

template <typename Allocator = std::allocator<char>>
class PointerReferenceUnpackingArchive {
  public:

    using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<char>;

  private:

    darma_runtime::detail::compressed_pair<char const*&, Allocator> data_spot_;

    explicit
    PointerReferenceUnpackingArchive(char const*& ptr)
      : data_spot_(
          std::piecewise_construct,
          std::forward_as_tuple(ptr),
          std::forward_as_tuple(allocator_type{})
        )
    { }

    // Put this here to do the messy casting for interfacing with a void* in one place
    explicit
    PointerReferenceUnpackingArchive(char*& ptr)
      : PointerReferenceUnpackingArchive(*const_cast<char const**>(&ptr))
    { /* forwarding ctor, must be empty */ }

    // Put this here to do the messy casting for interfacing with a void* in one place
    explicit
    PointerReferenceUnpackingArchive(void const*& ptr)
      : PointerReferenceUnpackingArchive(*reinterpret_cast<char const**>(&ptr))
    { /* forwarding ctor, must be empty */ }

    // Put this here to do the messy casting for interfacing with a void* in one place
    explicit
    PointerReferenceUnpackingArchive(void*& ptr)
      : PointerReferenceUnpackingArchive(*reinterpret_cast<char const**>(const_cast<void const**>(&ptr)))
    { /* forwarding ctor, must be empty */ }

    char const*& _data_spot() { return data_spot_.first(); }

    template <typename>
    friend struct PointerReferenceSerializationHandler;

    template <typename T>
    inline auto& _ask_serializer_to_unpack(
      T& obj,
      std::enable_if_t<
        not std::is_trivially_destructible<T>::value
          and not std::is_array<T>::value,
        darma_runtime::detail::_not_a_type
      > = { }
    ) & {
      auto* buffer = static_cast<void*>(&obj);
      using rebound_alloc = typename std::allocator_traits<allocator_type>::template rebind_alloc<T>;
      rebound_alloc alloc{get_allocator()};
      std::allocator_traits<rebound_alloc>::destroy(alloc, &obj);
      darma_unpack(allocated_buffer_for<T>(buffer), *this);
      return *this;
    }

    template <typename T>
    inline auto& _ask_serializer_to_unpack(
      T& obj,
      std::enable_if_t<
        std::is_trivially_destructible<T>::value,
        darma_runtime::detail::_not_a_type
      > = { }
    ) & {
      auto* buffer = static_cast<void*>(&obj);
      darma_unpack(allocated_buffer_for<T>(buffer), *this);
      return *this;
    }

  public:

    PointerReferenceUnpackingArchive(PointerReferenceUnpackingArchive&&) = default;

    static constexpr bool is_sizing() { return false; }
    static constexpr bool is_packing() { return false; }
    static constexpr bool is_unpacking() { return true; }

    template <typename RawDataType>
    void unpack_data_raw(void* allocated_dest, size_t n_items) {
      // Use memcpy instead of std::copy, since copy invokes the assignment
      // operator, and "raw" implies that this isn't necessary
      size_t size = n_items * sizeof(RawDataType);
      std::memcpy(allocated_dest, data_spot_.first(), size);
      data_spot_.first() += size;
    }

    template <typename T>
    inline auto& operator|(T& obj) & {
      return _ask_serializer_to_unpack(obj);
    }

    template <typename T>
    inline auto& operator>>(T& obj) & {
      return _ask_serializer_to_unpack(obj);
    }

    template <typename T>
    inline T unpack_next_item_as(
      std::enable_if_t<
        (sizeof(T) > DARMA_SERIALIZATION_SIMPLE_ARCHIVE_UNPACK_STACK_ALLOCATION_MAX),
        darma_runtime::detail::_not_a_type_numbered<0>
      > = { }
    ) & {
      using allocator_t = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
      auto alloc = allocator_t(get_allocator());
      auto* storage = std::allocator_traits<allocator_t>::allocate(
        alloc, 1
      );
      darma_unpack(allocated_buffer_for<T>(storage), *this);
      // Use a unique_ptr to delete the temporary storage after returning
      std::unique_ptr<T> raii_holder(*reinterpret_cast<T*>(storage));
      return *raii_holder.get();
    }

    template <typename T>
    inline T unpack_next_item_as(
      std::enable_if_t<
        (sizeof(T) <= DARMA_SERIALIZATION_SIMPLE_ARCHIVE_UNPACK_STACK_ALLOCATION_MAX),
        darma_runtime::detail::_not_a_type_numbered<1>
      > = { }
    ) & {
      char on_stack_buffer[sizeof(T)];
      darma_unpack(allocated_buffer_for<T>(on_stack_buffer), *this);
      // Use a unique_ptr to delete the temporary storage after returning
      auto destroy_but_not_delete = [this](auto* ptr) {
        using allocator_t = typename std::allocator_traits<allocator_type>::template rebind_alloc<T>;
        allocator_t alloc{get_allocator()};
        // Destroy but don't deallocate, since the data is allocated on the stack
        std::allocator_traits<allocator_t>::destroy(alloc, ptr);
      };
      std::unique_ptr<T, decltype(destroy_but_not_delete)> raii_destroy(
        reinterpret_cast<T*>(on_stack_buffer),
        destroy_but_not_delete
      );
      return *raii_destroy.get();
    }

    template <typename T>
    inline void unpack_next_item_at(void* allocated) & {
      darma_unpack(allocated_buffer_for<T>(allocated), *this);
    }

    auto const& get_allocator() const {
      return data_spot_.second();
    }
    auto& get_allocator() {
      return data_spot_.second();
    }

    template <typename NeededAllocatorT>
    NeededAllocatorT const& get_allocator_as() const {
      // Let's hope this is a compatible type
      return data_spot_.second();
    }

    void const*& data_pointer_reference() { return *reinterpret_cast<void const**>(&data_spot_.first()); }

};

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_SERIALIZATION_POINTER_REFERENCE_ARCHIVE_H
