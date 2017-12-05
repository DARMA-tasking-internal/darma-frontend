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

#include "simple_archive.h"

namespace darma_runtime {
namespace serialization {

/// A simple, allocator-aware serialization handler
template <typename Allocator>
struct SimpleSerializationHandler {

  private:

    using char_allocator_t =
    typename std::allocator_traits<Allocator>::template rebind_alloc<char>::type;

  public:

    //==========================================================================
    // <editor-fold desc="serialize() overloads"> {{{1

    template <typename T>
    static
    DynamicSerializationBuffer<char_allocator_t>
    serialize(
      T const& object
    ) {
      return SimpleSerializationHandler::serialize(object, Allocator());

    }

    template <typename T>
    static
    DynamicSerializationBuffer<char_allocator_t>
    serialize(T const& object, Allocator const& alloc) {
      size_t packed_size = 0;
      {
        SimpleSizingArchive ar;
        ar | object;
        packed_size = ar.get_current_size();
      }

      DynamicSerializationBuffer<char_allocator_t> packed_buffer(packed_size, alloc);

      {
        SimplePackingArchive ar(packed_buffer);
        ar | object;
      }

      return packed_buffer;
    }

    template <typename BufferT, typename T>
    static
    BufferT
    serialize(T const& object) {
      return SimpleSerializationHandler::serialize<BufferT, T>(object, Allocator());
    }

    template <typename BufferT, typename T, typename... BufferCtorArgs>
    static
    BufferT
    serialize(T const& object, Allocator const& alloc, BufferCtorArgs&&... args) {
      size_t packed_size = 0;
      {
        SimpleSizingArchive ar;
        ar | object;
        packed_size = ar.get_current_size();
      }

      BufferT packed_buffer(packed_size, alloc, std::forward<BufferCtorArgs>(args)...);

      {
        SimplePackingArchive ar(packed_buffer);
        ar | object;
      }

      return packed_buffer;
    }

    // </editor-fold> end serialize() overloads }}}1
    //==========================================================================


    //==========================================================================
    // <editor-fold desc="deserialize() overloads"> {{{1

    template <typename T, typename Buffer>
    static
    std::enable_if<
      std::is_copy_constructible<T>::value or std::is_move_constructible<T>::value,
      T
    >
    deserialize(Buffer const& buffer) {
      return deserialize(buffer, Allocator());
    }

    template <typename T, typename Buffer>
    static
    std::enable_if<
      std::is_copy_constructible<T>::value or std::is_move_constructible<T>::value,
      T
    >
    deserialize(Buffer const& buffer, Allocator const& alloc) {
      char* rv_as_bytes = std::allocator_traits<char_allocator_t>::allocate(
        alloc, sizeof(T)
      );
      deserialize(buffer, rv_as_bytes, alloc);
      return *reinterpret_cast<T*>(rv_as_bytes);
    }

    template <typename T, typename Buffer>
    static void
    deserialize(Buffer const& buffer, char* destination) {
      deserialize(buffer, destination, Allocator());
    }

    template <typename T, typename Buffer>
    static void
    deserialize(Buffer const& buffer, char* destination, Allocator const& alloc) {
      SimpleUnpackingArchive<Allocator> ar(buffer, alloc);

      using traits = detail::serializability_traits<T>;
      // TODO clean this sequence up w.r.t. reconstruction
      traits::unpack(destination, ar);
    }

    // </editor-fold> end deserialize() overloads }}}1
    //==========================================================================

};



} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_SIMPLE_HANDLER_H
