/*
//@HEADER
// ************************************************************************
//
//                      serialization_buffer.h
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

#ifndef DARMAFRONTEND_SERIALIZATION_BUFFER_H
#define DARMAFRONTEND_SERIALIZATION_BUFFER_H

#include <darma/impl/util/compressed_pair.h>

#include <memory>
#ifndef DARMA_SERIALIZATION_NO_EXCEPTIONS
#  include <stdexcept>
#endif

namespace darma_runtime {
namespace serialization {

template <typename Allocator=std::allocator<char>>
struct DynamicSerializationBuffer {

    static_assert(
      sizeof(typename std::allocator_traits<Allocator>::value_type) == sizeof(char),
      "Allocator given to DynamicSerialziationBuffer must allocate objects of size 1 byte"
    );

  public:

    explicit
    DynamicSerializationBuffer(
      size_t size
    ) : end_(
          std::piecewise_construct,
          std::forward_as_tuple(nullptr),
          std::forward_as_tuple(Allocator{})
        ),
        begin_(std::allocator_traits<Allocator>::allocate(allocator(), size))
    {
      end_.first() = begin_ + size;
    }

    DynamicSerializationBuffer(
      size_t size,
      Allocator& alloc
    ) : end_(
          std::piecewise_construct,
          std::forward_as_tuple(nullptr),
          std::forward_as_tuple(alloc)
        ),
        begin_(std::allocator_traits<Allocator>::allocate(alloc, size))
    {
      end_.first() = begin_ + size;
    }

    DynamicSerializationBuffer(
      DynamicSerializationBuffer&& other
    ) : begin_(other.begin_),
        end_(std::move(other.end_))
    {
      other.begin_ = other.end_.first() = nullptr;
    }

    char* data() { return begin_; }
    char const* data() const { return begin_; }

    size_t capacity() const { return end_.first() - begin_; }

    Allocator& allocator() { return end_.second(); }
    Allocator const& allocator() const { return end_.second(); }

    ~DynamicSerializationBuffer() {
      if(begin_ != nullptr) {
        std::allocator_traits<Allocator>::deallocate(
          allocator(), begin_, end_.first() - begin_
        );
      }
    }

  private:
    // end_ must be first so that the allocator can be used to initialize begin_;
    darma_runtime::detail::compressed_pair<char*, Allocator> end_ = nullptr;
    char* begin_ = nullptr;
};

struct NonOwningSerializationBuffer {

    NonOwningSerializationBuffer(
      char* begin,
      size_t size
    ) : begin_(begin),
        end_(begin_ + size)
    { }

    char* data() { return begin_; }
    char const* data() const { return begin_; }

    size_t capacity() const { return end_ - begin_; }

  private:
    char* begin_ = nullptr;
    char* end_ = nullptr;
};

template <size_t N>
struct FixedSizeSerializationBuffer {

    template <typename Allocator /* ignored*/>
    FixedSizeSerializationBuffer(
      size_t size,
      Allocator const& /*ignored*/
    ) {
#ifndef DARMA_SERIALIZATION_NO_EXCEPTIONS
      if(size > N) {
        throw std::length_error(
          "dynamically requested size exceeds static size of FixedSizeSerializationBuffer"
        );
      }
#endif
    }

    char* data() { return data_; }
    char const* data() const { return data_; }

    size_t capacity() const { return N; }

  private:
    char data_[N];
};

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_SERIALIZATION_BUFFER_H
