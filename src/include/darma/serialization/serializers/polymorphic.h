/*
//@HEADER
// ************************************************************************
//
//                      polymorphic.h
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

#ifndef DARMAFRONTEND_SERIALIZATION_SERIALIZERS_POLYMORPHIC_H
#define DARMAFRONTEND_SERIALIZATION_SERIALIZERS_POLYMORPHIC_H

#include <darma/serialization/polymorphic/polymorphic_serializable_object.h>
#include <darma/serialization/pointer_reference_handler.h>

#include <type_traits> // std::is_base_of
#include <memory>

namespace darma_runtime {
namespace serialization {

template <typename T>
struct Serializer_enabled_if<
  std::unique_ptr<T>,
  std::enable_if_t<std::is_base_of<PolymorphicSerializableObject<T>, T>::value>
>
{
  template <typename SizingArchive>
  static void compute_size(std::unique_ptr<T> const& ptr, SizingArchive& ar) {
    ar.add_to_size_raw(ptr->get_packed_size());
  }

  template <typename PackingArchive>
  static void pack(std::unique_ptr<T> const& ptr, PackingArchive& ar) {
    auto ptr_ar = PointerReferenceSerializationHandler<>::make_packing_archive_referencing(ar);
    ptr->pack(*reinterpret_cast<char const**>(&ptr_ar.data_pointer_reference()));
  }

  template <typename UnpackingArchive>
  static void unpack(void* allocated, UnpackingArchive& ar) {

    auto ptr_ar = PointerReferenceSerializationHandler<>::make_unpacking_archive_referencing(ar);

    // TODO allocator support? (Can't do this unless there's a deleter that references the same allocator)
    new (allocated) std::unique_ptr<T>(
      PolymorphicSerializableObject<T>::unpack(
        *reinterpret_cast<char const**>(&ptr_ar.data_pointer_reference())
      )
    );

  }

};


} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_SERIALIZATION_SERIALIZERS_POLYMORPHIC_H
