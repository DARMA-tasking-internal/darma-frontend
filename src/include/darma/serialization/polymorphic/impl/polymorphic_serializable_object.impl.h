/*
//@HEADER
// ************************************************************************
//
//                      polymorphic_serialization.h
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMA_IMPL_POLYMORPHIC_SERIALIZATION_H
#define DARMA_IMPL_POLYMORPHIC_SERIALIZATION_H

#include <darma/serialization/polymorphic/registry.h>
#include <darma/serialization/polymorphic/polymorphic_serializable_object.h>

#include <darma/utility/darma_assert.h>
#include <darma/utility/demangle.h>

#include <memory> // unique_ptr

namespace darma {
namespace serialization {

template <typename AbstractType>
std::unique_ptr<AbstractType>
PolymorphicSerializableObject<AbstractType>::unpack(char const*& buffer) {
  // Get the abstract type index that we're looking for
  static const size_t abstract_type_index =
    darma::serialization::detail::get_abstract_type_index<AbstractType>();

  // Get the header
  auto const& header = *reinterpret_cast<
    darma::serialization::detail::SerializedPolymorphicObjectHeader const*
  >(buffer);
  buffer += sizeof(darma::serialization::detail::SerializedPolymorphicObjectHeader);

  // Look through the abstract bases that this object is registered for until
  // we find the entry that corresponds to a callable that unpacks the object
  // as a std::unique_ptr<AbstractType>
  uint8_t i_base = 0;
  size_t concrete_index = std::numeric_limits<size_t>::max();
  for(; i_base < header.n_bases; ++i_base) {
    const auto& entry = *reinterpret_cast<
      darma::serialization::detail::PolymorphicAbstractBasesTableEntry const*
    >(buffer);

    if (abstract_type_index == entry.abstract_index) {
      // We found the one we want to call, so break
      concrete_index = entry.concrete_index;
      break;
    }

    buffer += sizeof(darma::serialization::detail::PolymorphicAbstractBasesTableEntry);
  }
  DARMA_ASSERT_MESSAGE(
    concrete_index != std::numeric_limits<size_t>::max(),
    "No registered unpacker found to unpack a concrete type as abstract type "
      << darma::utility::try_demangle<AbstractType>::name()
  );

  // Make sure we advance the buffer over all of the other base class entries
  buffer += sizeof(darma::serialization::detail::PolymorphicAbstractBasesTableEntry) * (header.n_bases - i_base);

  // get a reference to the static registry
  auto& reg = darma::serialization::detail::get_polymorphic_unpack_registry<AbstractType>();

  // execute the unpack callable on the buffer
  return reg[concrete_index](buffer);
}

} // end namespace serialization
} // end namespace darma

#endif //DARMA_IMPL_POLYMORPHIC_SERIALIZATION_H
