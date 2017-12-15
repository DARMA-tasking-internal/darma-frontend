/*
//@HEADER
// ************************************************************************
//
//                      string.h
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

#ifndef DARMAFRONTEND_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_STRING_H
#define DARMAFRONTEND_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_STRING_H

#include <darma/serialization/nonintrusive.h>
#include <darma/serialization/serialization_traits.h>

#include <darma/serialization/serializers/arithmetic_types.h>

#include <string>

#ifndef DARMA_SERIALIZATION_STRING_UNPACK_MAX_STACK_ALLOCATION
#  define DARMA_SERIALIZATION_STRING_UNPACK_STACK_ALLOCATION_MAX 1024
#endif

namespace darma_runtime {
namespace serialization {

// TODO strings with non-standard/non-empty allocators ?
// TODO make a specialization of unpack takes advantage of the fact that (most) output archives can provide an iterator

//==============================================================================

template <typename CharT, typename Traits>
struct Serializer<std::basic_string<CharT, Traits>> {

  static_assert(is_directly_serializable<CharT>::value,
    "CharT of std::basic_string must be directly serializable"
  );

  using string_t = std::basic_string<CharT, Traits>;

  template <typename Archive>
  static void get_packed_size(string_t const& obj, Archive& ar) {
    ar | obj.size();
    ar.add_to_size_raw(sizeof(CharT) * obj.size());
  }

  template <typename Archive>
  static void pack(string_t const& obj, Archive& ar) {
    ar | obj.size();
    ar.pack_data_raw(obj.data(), obj.data() + obj.size());
  }

  template <typename Archive>
  static void unpack(void* allocated, Archive& ar) {
    auto size = ar.template unpack_next_item_as<typename string_t::size_type>();
    auto& obj = *(new (allocated) string_t(size, static_cast<CharT>(0)));
    ar.template unpack_data_raw<CharT const>(
      // This is probably safe everywhere, given that the nonconst version of
      // data() was added to C++17
      const_cast<CharT*>(obj.data()), size
    );
    // If the const cast ever doesn't work, we'd need to do something like this:
    //
    //   auto size = ar.template unpack_next_item_as<typename string_t::size_type>();
    //   if(size <= DARMA_SERIALIZATION_STRING_UNPACK_STACK_ALLOCATION_MAX) {        // [[likely]] {
    //     CharT tmp[size];
    //     ar.template unpack_data_raw<CharT const>(tmp, size);
    //     new (allocated) string_t(tmp, size);
    //   }
    //   else{
    //     using alloc_traits = std::allocator_traits<typename Archive::allocator_type>;
    //     auto& alloc = ar.get_allocator();
    //     auto n_to_alloc =
    //       size * (sizeof(typename alloc_traits::value_type) / sizeof(CharT) + 1);
    //     auto* tmp = alloc_traits::allocate(alloc, n_to_alloc);
    //     ar.template unpack_data_raw<CharT const>(reinterpret_cast<CharT*>(tmp), size);
    //     new (allocated) string_t(tmp, size);
    //     alloc_traits::deallocate(alloc, tmp, n_to_alloc);
    //   }
  }
};

//==============================================================================

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_STRING_H
