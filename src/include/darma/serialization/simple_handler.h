/*
//@HEADER
// ************************************************************************
//
//                      simple_handler.h
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

#ifndef DARMAFRONTEND_SIMPLE_HANDLER_H
#define DARMAFRONTEND_SIMPLE_HANDLER_H

#include <darma/impl/util/not_a_type.h>

#include "simple_archive.h"
#include "simple_handler_fwd.h"
#include "pointer_reference_handler_fwd.h"

namespace darma_runtime {
namespace serialization {

/// A simple, allocator-aware serialization handler that only works with stateless allocators
template <typename Allocator>
struct SimpleSerializationHandler {

  private:

    using this_t = SimpleSerializationHandler<Allocator>;

    using char_allocator_t =
      typename std::allocator_traits<Allocator>::template rebind_alloc<char>;

    template <typename Archive>
    static inline void _apply_compute_size_recursively(Archive& ar) { }

    template <typename Archive, typename T, typename... Ts>
    static inline void
    _apply_compute_size_recursively(Archive& ar, T const& obj, Ts const&... rest) {
      // invoke the customization point as an unqualified name, allowing ADL
      darma_compute_size(obj, ar);
      this_t::_apply_compute_size_recursively(ar, rest...);
    };

    template <typename Archive>
    static inline void _apply_pack_recursively(Archive& ar) { }

    template <typename Archive, typename T, typename... Ts>
    static inline void
    _apply_pack_recursively(Archive& ar, T const& obj, Ts const&... rest) {
      // invoke the customization point as an unqualified name, allowing ADL
      darma_pack(obj, ar);
      this_t::_apply_pack_recursively(ar, rest...);
    };

    static_assert(std::is_empty<Allocator>::value,
      "SimpleSerializationHandler only works with stateless Allocators"
    );

    using sizing_archive_t = SimpleSizingArchive;
    using unpacking_archive_t = SimpleUnpackingArchive<Allocator>;
    using serialization_buffer_t = DynamicSerializationBuffer<char_allocator_t>;

    // Not part of the interface; only applicable to SimpleSerializationHandler
    // Used by PointerReferenceSerializationHandler to adapt from archive types
    template <typename T, typename CompatiblePackingOrUnpackingArchive>
    /* requires requires(CompatiblePackingOrUnpackingArchive a) { a._data_spot() => char*& } */
    static T*&
    _data_spot_reference_as(
      CompatiblePackingOrUnpackingArchive& ar
    ) {
      return static_cast<T*&>(*reinterpret_cast<T**>(&ar._data_spot()));
    }

    // Not part of the interface; only applicable to SimpleSerializationHandler
    // Used by PointerReferenceSerializationHandler to adapt from archive types
    template <typename T, typename CompatiblePackingOrUnpackingArchive>
    /* requires
     *   requires(CompatiblePackingOrUnpackingArchive a) { a._data_spot() => char*&; }
     *   || requires(CompatiblePackingOrUnpackingArchive a) { a._data_spot() => char const*&; }
     * */
    static std::add_const_t<T>*&
    _const_data_spot_reference_as(
      CompatiblePackingOrUnpackingArchive& ar
    ) {
      return static_cast<
        std::add_const_t<T>*&
      >(*reinterpret_cast<T const**>(&const_cast<char const*&>(ar._data_spot())));
    }

    template <typename>
    friend struct PointerReferenceSerializationHandler;

  public:


    //==========================================================================
    // <editor-fold desc="archive creation"> {{{1

    static auto
    make_sizing_archive() {
      return SimpleSizingArchive{};
    }

    // Disabled for now, until I get the time to implement the concepts for
    // SizingArchive and SerializationBuffer so that I can distinguish between
    // this one and the SerializationBuffer overload
    //template <typename CompatibleSizingArchive>
    //static auto
    //make_packing_archive(
    //  CompatibleSizingArchive&& ar,
    //  std::enable_if_t<
    //    std::is_rvalue_reference<CompatibleSizingArchive&&>::value,
    //    darma_runtime::detail::_not_a_type
    //  > = { }
    //) {
    //  auto size = SimpleSerializationHandler::get_size(ar);
    //  ar.size_ = 0; // set size to zero as part of expiring the archive
    //  return make_packing_archive(size);
    //}

    static auto
    make_packing_archive(size_t size) {
      serialization_buffer_t buffer(size);
      return SimplePackingArchive<serialization_buffer_t>(std::move(buffer));
    }

    template <typename SerializationBuffer>
    static auto
    make_packing_archive(
      SerializationBuffer&& buffer,
      // Enforce move semantics on a forwarding reference template
      std::enable_if_t<
        std::is_rvalue_reference<SerializationBuffer&&>::value,
        darma_runtime::detail::_not_a_type
      > = { }
    ) {
      return SimplePackingArchive<std::decay_t<SerializationBuffer>>(std::move(buffer));
    }

    template <typename SerializationBuffer>
    static auto
    make_unpacking_archive(SerializationBuffer const& buffer) {
      return SimpleUnpackingArchive<char_allocator_t>(buffer, char_allocator_t{});
    }

    // </editor-fold> end archive creation }}}1
    //==========================================================================

    static std::size_t get_size(sizing_archive_t& ar) { return ar.size_; }

    template <typename CompatiblePackingArchive>
    /* requires requires(CompatiblePackingArchive a) {
     *   a._data_spot() => NullableType;
     *   a.buffer_;
     * } */
    static auto
    extract_buffer(
      CompatiblePackingArchive&& ar,
      // Enforce move semantics on a forwarding reference template
      std::enable_if_t<
        std::is_rvalue_reference<CompatiblePackingArchive&&>::value,
        darma_runtime::detail::_not_a_type
      > = { }
    ) {
      ar._data_spot() = nullptr;  // As part of expiring the Archive
      return std::move(ar.buffer_);
    }

    template <typename CompatiblePackingOrUnpackingArchive>
    /* requires requires(CompatibleUnpackingArchive a) { a._data_spot() => char*; } */
    static char*
    get_archive_data_pointer(
      CompatiblePackingOrUnpackingArchive const& ar
    ) {
      return ar._data_spot();
    }

    //==========================================================================
    // <editor-fold desc="serialize() overloads"> {{{1

    template <typename... Ts>
    static
    serialization_buffer_t
    serialize(Ts const&... objects) {
      size_t size;
      {
        auto s_ar = this_t::make_sizing_archive();
        this_t::_apply_compute_size_recursively(s_ar, objects...);
        size = this_t::get_size(s_ar);
      }
      auto p_ar = this_t::make_packing_archive(size);
      this_t::_apply_pack_recursively(p_ar, objects...);
      return this_t::extract_buffer(std::move(p_ar));
    }

    // </editor-fold> end serialize() overloads }}}1
    //==========================================================================


    //==========================================================================
    // <editor-fold desc="deserialize() overloads"> {{{1

    template <typename T, typename SerializationBuffer>
    static T deserialize(SerializationBuffer const& buffer) {
      using allocator_t = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
      allocator_t alloc{};
      auto* dest = std::allocator_traits<allocator_t>::allocate(alloc, 1);
      this_t::template deserialize<T>(buffer, dest);
      auto rv = std::move(*dest);
      std::allocator_traits<allocator_t>::destroy(alloc, dest);
      std::allocator_traits<allocator_t>::deallocate(alloc, dest, 1);
      return rv;
    }

    template <typename T, typename SerializationBuffer>
    static void
    deserialize(SerializationBuffer const& buffer, void* destination) {
      auto ar = this_t::make_unpacking_archive(buffer);
      // invoke the customization point as an unqualified name, allowing ADL
      darma_unpack<T>(destination, ar);
    }

    // </editor-fold> end deserialize() overloads }}}1
    //==========================================================================


};



} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_SIMPLE_HANDLER_H
