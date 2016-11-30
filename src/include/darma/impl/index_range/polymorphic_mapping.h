/*
//@HEADER
// ************************************************************************
//
//                      polymorphic_mapping.h
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

#ifndef DARMA_IMPL_INDEX_RANGE_POLYMORPHIC_MAPPING_H
#define DARMA_IMPL_INDEX_RANGE_POLYMORPHIC_MAPPING_H

#include <type_traits>

#include <vector>
#include <darma/impl/util/optional_boolean.h>

#include "mapping.h"

namespace darma_runtime {

template <
  typename FromIndexT, typename ToIndexT=FromIndexT,
  typename FromMultiIndexT=std::vector<FromIndexT>,
  typename ToMultiIndexT=std::vector<ToIndexT>
>
class PolymorphicMapping {

  public:

    using is_index_mapping_t = std::true_type;
    using from_index_type = FromIndexT;
    using from_multi_index_type = FromMultiIndexT;
    using to_index_type = ToIndexT;
    using to_multi_index_type = ToMultiIndexT;

    virtual bool is_one_to_one() const =0;
    static constexpr auto is_always_one_to_one = false;

    virtual bool is_one_to_many() const =0;
    static constexpr auto is_always_one_to_many = false;

    virtual bool is_many_to_one() const =0;
    static constexpr auto is_always_many_to_one = false;

    virtual bool is_many_to_many() const =0;
    static constexpr auto is_always_many_to_many = false;

    virtual to_multi_index_type
    map_forward(from_index_type const&) const =0;

    virtual from_multi_index_type
    map_backward(to_index_type const&) const =0;

    virtual optional_boolean_t
    is_same(PolymorphicMapping const& other) const {
      // We can at least say they are the same if this is true:
      if(std::addressof(*this) == std::addressof(other)) {
        return OptionalBoolean::KnownTrue;
      }
      else {
        return OptionalBoolean::Unknown;
      }
    }

    virtual ~PolymorphicMapping() = default;

};

template <
  typename FromIndexT, typename ToIndexT=FromIndexT
>
class PolymorphicOneToOneMapping
  : public PolymorphicMapping<FromIndexT, ToIndexT, FromIndexT, ToIndexT>
{

  public:

    using is_index_mapping_t = std::true_type;
    using from_index_type = FromIndexT;
    using from_multi_index_type = FromIndexT;
    using to_index_type = ToIndexT;
    using to_multi_index_type = ToIndexT;

    bool is_one_to_one() const override { return true; }
    static constexpr auto is_always_one_to_one = true;

    bool is_one_to_many() const override { return false; }
    static constexpr auto is_always_one_to_many = false;

    bool is_many_to_one() const override { return false; }
    static constexpr auto is_always_many_to_one = false;

    bool is_many_to_many() const override { return false; }
    static constexpr auto is_always_many_to_many = false;

    virtual ~PolymorphicOneToOneMapping() = default;
};

// TODO fill in other possible types


} // end namespace darma_runtime

#endif //DARMA_IMPL_INDEX_RANGE_POLYMORPHIC_MAPPING_H
