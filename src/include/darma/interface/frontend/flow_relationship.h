/*
//@HEADER
// ************************************************************************
//
//                      flow_relationship.h
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

#ifndef DARMA_INTERFACE_FRONTEND_FLOW_RELATIONSHIP_H
#define DARMA_INTERFACE_FRONTEND_FLOW_RELATIONSHIP_H

#include <darma_types.h>

namespace darma_runtime {
namespace abstract {
namespace frontend {

class FlowRelationship {
  public:

    typedef enum FlowRelationshipTag {
      Same = 1,
      Initial = 2,
      Null = 4,
      Next = 8,
      Forwarding = 16,
      Indexed = 32,
      Local = 64, // Currently not valid by itself, only with Indexed
      Fetching = 128, // Currently not valid by itself
      Collection = 256 // Currently not valid by itself
#if _darma_has_feature(anti_flows)
      , Anti = 512
      , Insignificant = 1024
#endif // _darma_has_feature(anti_flows)
      // TODO Move flow packing here
      // , Packed = 256 // Packed stands alone, mostly; it doesn't bitwise-or with
                   // other flags except for Collection
      //,
    } flow_relationship_tag_t;

    using flow_relationship_description_t =
      std::underlying_type_t<flow_relationship_tag_t>;

    static constexpr auto IndexedLocal =
      flow_relationship_description_t(Indexed)
        | flow_relationship_description_t(Local);
    static constexpr auto IndexedFetching =
      flow_relationship_description_t(Indexed)
        | flow_relationship_description_t(Fetching);
    static constexpr auto InitialCollection =
      flow_relationship_description_t(Initial)
        | flow_relationship_description_t(Collection);
    static constexpr auto NullCollection =
      flow_relationship_description_t(Null)
        | flow_relationship_description_t(Collection);
    static constexpr auto NextCollection =
      flow_relationship_description_t(Next)
        | flow_relationship_description_t(Collection);
    static constexpr auto SameCollection =
      flow_relationship_description_t(Same)
        | flow_relationship_description_t(Collection);
#if _darma_has_feature(anti_flows)
    static constexpr auto InsignificantCollection =
      flow_relationship_description_t(Insignificant)
        | flow_relationship_description_t(Collection);
    static constexpr auto AntiNext =
      flow_relationship_description_t(Anti)
        | flow_relationship_description_t(Next);
    static constexpr auto AntiIndexedFetching =
      flow_relationship_description_t(Anti)
        | flow_relationship_description_t(IndexedFetching);
    static constexpr auto AntiNextCollection =
      flow_relationship_description_t(AntiNext)
        | flow_relationship_description_t(Collection);
#endif // _darma_has_feature(anti_flows)

    virtual flow_relationship_description_t description() const =0;

    virtual types::flow_t* const related_flow() const =0;

#if _darma_has_feature(anti_flows)
    // Only non-null if it's an "AntiFlowRelationship" (not a real class,
    // but conceptually speaking)
    virtual types::anti_flow_t* const related_anti_flow() const =0;

    virtual bool use_corresponding_in_flow_as_anti_related() const =0;
#endif // _darma_has_feature(anti_flows)

    virtual bool use_corresponding_in_flow_as_related() const =0;

    virtual types::key_t const* version_key() const =0;

    virtual std::size_t index() const =0;
};

} // end namespace frontend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_INTERFACE_FRONTEND_FLOW_RELATIONSHIP_H
