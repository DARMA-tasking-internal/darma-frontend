/*
//@HEADER
// ************************************************************************
//
//                      basic_range_1d.h
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

#ifndef DARMAFRONTEND_INDEXING_BASIC_RANGE_1D_H
#define DARMAFRONTEND_INDEXING_BASIC_RANGE_1D_H

#include <darma/serialization/serialization_traits.h> // is_directly_serializable
#include <darma/serialization/serializers/arithmetic_types.h> // is_directly_serializable<Integer>
#include <darma/serialization/serializers/const.h> // is_directly_serializable<const Integer>

#include <darma/utility/wrap_iterator.h>

#include <cstdint>

namespace darma_runtime {
namespace indexing {

template <typename Integer>
class basic_range_1d {

  private:

    Integer size_;
    Integer offset_;

  public:

    using is_index_range = std::true_type;
    using iterator = utility::integer_wrap_iterator<Integer>;
    using index_type = Integer;

    size_t size() const { return size_; }

    template <typename ArchiveT>
    void serialize(ArchiveT& ar) {
      // Only used if Integer is not directly serializable
      ar | size_ | offset_;
    }

    constexpr iterator begin() const {
      return iterator(offset_);
    }

    constexpr iterator end() const {
      return iterator(offset_ + size_);
    }

};

using range_1d = basic_range_1d<int64_t>

} // end namespace indexing

namespace serialization {

template <typename Integer>
struct is_directly_serializable<indexing::basic_range_1d<Integer>>
  : is_directly_serializable<Integer>
{ };

} // end namespace serialization

} // end namespace darma_runtime

#endif //DARMAFRONTEND_INDEXING_BASIC_RANGE_1D_H
