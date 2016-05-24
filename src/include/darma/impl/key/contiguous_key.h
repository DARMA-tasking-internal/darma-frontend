/*
//@HEADER
// ************************************************************************
//
//                      contiguous_key.h
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

#ifndef DARMA_IMPL_CONTIGUOUS_KEY_H
#define DARMA_IMPL_CONTIGUOUS_KEY_H

#include <cstdint>
#include <cstddef>
#include <utility>
#include "segmented_key.h"
namespace darma_runtime {
namespace detail {

class ContiguousAllocationKey {
  private:
    uint8_t n_pieces_;
    uint8_t size_;

    template <typename BytesConverter, typename Value>
    inline size_t part_size(
      BytesConverter&& bytes_converter,
      Value&& val) {
      size_t data_size = bytes_converter.get_size(std::forward<decltype(val)>(val));
      data_size += sizeof(KeySegmentPieceMetadata);
      return data_size;
    }

    char* data_start() const {
      return const_cast<char*>(reinterpret_cast<const char*>(this) + sizeof(ContiguousAllocationKey));
    }

    template <
      typename T,
      typename = typename std::enable_if_t<!std::is_same<std::decay_t<T>, ContiguousAllocationKey*>::value>
    >
    char* append(T&& val, char* data_spot){
      typedef std::remove_cv_t<std::remove_reference_t<T>> actual_type;
      bytes_convert<actual_type> bc;
      KeySegmentPieceMetadata* md = reinterpret_cast<KeySegmentPieceMetadata*>(data_spot);
      data_spot += sizeof(KeySegmentPieceMetadata);
      uint8_t data_size = bc.copy_bytes(data_spot, val);
      md->piece_size = data_size;
      data_spot += data_size;
      return data_spot;
    }

    //template <
    //  typename T,
    //  typename = typename std::enable_if_t<std::is_same<std::decay_t<T>, ContiguousAllocationKey*>::value>
    //>
    char* append(ContiguousAllocationKey* key, char* data_spot) {
      ::memcpy(data_spot, key->data_start(), key->size());
      n_pieces_ += key->n_pieces() - 2; //I already counted 1 for this
      size_ -= sizeof(KeySegmentPieceMetadata); //I already counted this
      return data_spot + key->size();
    }

  public:
    ContiguousAllocationKey() = default;

    uint8_t
    size() const {
      return size_;
    }

    uint8_t
    n_pieces() const {
      return n_pieces_;
    }

    template <typename... Args>
    ContiguousAllocationKey(
      size_t data_size,
      char* data_block,
      Args&&... args
    ) : size_(data_size), n_pieces_(sizeof...(args)){
      char* data_spot = data_block;
      meta::tuple_for_each(
        std::forward_as_tuple(std::forward<Args>(args)...),
        [&](auto&& val) {
          data_spot = append(val, data_spot);
        }
      );
      //printf("%p %d %p %p\n", data_start(), size_, data_start() + size_, data_spot);
      assert((data_start() + size()) == data_spot);
    }

    template <typename... Args>
    void print(std::ostream& os){
      os << "Key N=" << sizeof...(Args) << " Bytes=" << (int)size_ << ": ";
      char* end = PrintArray<Args...>()(data_start(), os);
      if (end != data_start() + size_){
        std::cerr << "wrong number/type of template arguments passed to key::print" << std::endl;
        assert(end == (data_start() + size_));
      }
    }

};


} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_CONTIGUOUS_KEY_H
