/*
//@HEADER
// ************************************************************************
//
//                      sizing_archive.h
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

#ifndef DARMAFRONTEND_SIZING_ARCHIVE_H
#define DARMAFRONTEND_SIZING_ARCHIVE_H

#include <darma/serialization/serialization_traits.h>
#include <darma/serialization/serialization_buffer.h>
#include <darma/serialization/simple_handler_fwd.h>

#include <cstddef>

namespace darma_runtime {
namespace serialization {

class SimpleSizingArchive {
  protected:

    std::size_t size_ = 0;

    SimpleSizingArchive() = default;

    template <typename>
    friend struct SimpleSerializationHandler;

  private:

    template <typename T>
    inline auto& _ask_serializer_for_size(T const& obj) & {
      darma_compute_size(obj, *this);
      return *this;
    }

  public:

    static constexpr bool is_sizing() { return true; }
    static constexpr bool is_packing() { return false; }
    static constexpr bool is_unpacking() { return false; }

    void add_to_size_raw(size_t size) {
      size_ += size;
    }

    template <typename T>
    inline auto& operator|(T const& obj) & {
      return _ask_serializer_for_size(obj);
    }

    template <typename T>
    inline auto& operator%(T const& obj) & {
      return _ask_serializer_for_size(obj);
    }

};

template <typename SerializationBuffer=DynamicSerializationBuffer<std::allocator<char>>>
class SimplePackingArchive {
  protected:

    char* data_spot_ = nullptr;
    SerializationBuffer buffer_;

    template <typename BufferT>
    explicit SimplePackingArchive(BufferT&& buffer)
      : data_spot_(buffer.data()), buffer_(std::forward<BufferT>(buffer))
    { }

    template <typename>
    friend struct SimpleSerializationHandler;

  private:

    template <typename T>
    inline auto& _ask_serializer_to_pack(T const& obj) & {
      darma_pack(obj, *this);
      return *this;
    }

  public:

    static constexpr bool is_sizing() { return false; }
    static constexpr bool is_packing() { return true; }
    static constexpr bool is_unpacking() { return false; }

    template <typename InputIterator>
    void pack_data_raw(InputIterator begin, InputIterator end) {
      using value_type =
        std::remove_const_t<std::remove_reference_t<decltype(*begin)>>;
      // std::copy(begin, end, reinterpret_cast<value_type*>(data_spot_));
      // Use memcpy, since copy invokes the assignment operator, and "raw"
      // implies that this isn't necessary
      auto size = std::distance(begin, end) * sizeof(value_type);
      std::memcpy(data_spot_, static_cast<void const*>(begin), size);
      data_spot_ += size;
    }

    template <typename T>
    inline auto& operator|(T const& obj) & {
      return _ask_serializer_to_pack(obj);
    }

    template <typename T>
    inline auto& operator<<(T const& obj) & {
      return _ask_serializer_to_pack(obj);
    }

};

template <typename Allocator=std::allocator<char>>
class SimpleUnpackingArchive {
  protected:

    darma_runtime::detail::compressed_pair<char const*, Allocator> data_spot_;

    template <typename BufferT>
    explicit SimpleUnpackingArchive(
      BufferT const& buffer, Allocator const& alloc
    ) : data_spot_(
          std::piecewise_construct,
          std::forward_as_tuple(buffer.data()),
          std::forward_as_tuple(alloc)
        )
    { }

    template <typename>
    friend struct SimpleSerializationHandler;

  private:

    template <typename T>
    inline auto& _ask_serializer_to_unpack(T const& obj) & {
      darma_unpack<T>(obj, *this);
      return *this;
    }

  public:

    using allocator_type = Allocator;

    // TODO Generate an iterator? (Maybe not for all types of archives)

    static constexpr bool is_sizing() { return false; }
    static constexpr bool is_packing() { return false; }
    static constexpr bool is_unpacking() { return true; }

    template <typename RawDataType, typename OutputIterator>
    void unpack_data_raw(OutputIterator dest, size_t n_items) {
      // Check that OutputIterator is an output iterator
      //static_assert(meta::is_output_iterator<OutputIterator>::value,
      //  "OutputIterator must be an output iterator."
      //);

      //std::copy(
      //  reinterpret_cast<RawDataType*>(data_spot_.first()),
      //  reinterpret_cast<RawDataType*>(data_spot_.first()) + n_items,
      //  dest
      //);

      std::memcpy(
        static_cast<void*>(dest),
        data_spot_.first(),
        n_items * sizeof(RawDataType)
      );

      data_spot_.first() += n_items * sizeof(RawDataType);
    }



    template <typename T>
    inline auto& operator|(T const& obj) & {
      return _ask_serializer_to_unpack(obj);
    }

    template <typename T>
    inline auto& operator>>(T const& obj) & {
      return _ask_serializer_to_unpack(obj);
    }

    template <typename T>
    inline T unpack_next_item_as() & {
      using allocator_t = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
      auto alloc = allocator_t(get_allocator());
      auto* storage = std::allocator_traits<allocator_t>::allocate(
        alloc, 1
      );
      darma_unpack<T>(storage, *this);
      return *reinterpret_cast<T*>(storage);
    }

    template <typename T>
    inline void unpack_next_item_at(void* allocated) & {
      darma_unpack<T>(allocated, *this);
    }

    auto const& get_allocator() const {
      return data_spot_.second();
    }
    auto& get_allocator() {
      return data_spot_.second();
    }

};

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_SIZING_ARCHIVE_H
