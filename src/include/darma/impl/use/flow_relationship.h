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

#ifndef DARMAFRONTEND_FLOW_RELATIONSHIP_H
#define DARMAFRONTEND_FLOW_RELATIONSHIP_H

#include <darma/interface/frontend/flow_relationship.h>

namespace darma_runtime {
namespace detail {

//==============================================================================
// <editor-fold desc="FlowRelationshipImpl"> {{{1

struct FlowRelationshipImpl: abstract::frontend::FlowRelationship
{
  flow_relationship_description_t description_
    = abstract::frontend::FlowRelationship::Insignificant;
  types::flow_t* related_ = nullptr;
  bool related_is_in_ = false;
  types::key_t const* version_key_ = nullptr;
  std::size_t index_ = 0;

  types::anti_flow_t* anti_related_ = nullptr;
  bool related_is_anti_in_ = false;

  FlowRelationshipImpl() = default;
  FlowRelationshipImpl(FlowRelationshipImpl&&) = default;
  FlowRelationshipImpl& operator=(FlowRelationshipImpl&&) = default;

  FlowRelationshipImpl(
    flow_relationship_description_t out_desc,
    types::flow_t* rel,
    bool related_is_in = false,
    types::key_t const* version_key = nullptr,
    std::size_t index = 0,
    types::anti_flow_t* anti_related = nullptr,
    bool anti_rel_is_in = false
  ) : description_(out_desc),
      related_(rel),
      related_is_in_(related_is_in),
      version_key_(version_key),
      index_(index),
      anti_related_(anti_related),
      related_is_anti_in_(anti_rel_is_in)
  { }

  FlowRelationshipImpl
  as_collection_relationship() const {
    return FlowRelationshipImpl(
      description_ | abstract::frontend::FlowRelationship::Collection,
      related_, related_is_in_, version_key_, index_,
      anti_related_, related_is_anti_in_
    );
  }


  flow_relationship_description_t
  description() const override {
    return description_;
  }

  types::flow_t*
  related_flow() const override { return related_; }

  types::anti_flow_t*
  related_anti_flow() const override { return anti_related_; }

  types::key_t const*
  version_key() const override { return version_key_; }

  bool
  use_corresponding_in_flow_as_related() const override {
    return related_is_in_;
  }

  bool
  use_corresponding_in_flow_as_anti_related() const override {
    return related_is_anti_in_;
  }

  std::size_t index() const override { return index_; }

};

// </editor-fold> end FlowRelationshipImpl }}}1
//==============================================================================

namespace flow_relationships {

inline FlowRelationshipImpl
initial_flow() {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Initial,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}

#if _darma_has_feature(mpi_interop)
inline FlowRelationshipImpl
initial_imported_flow() {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::InitialImported,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}
#endif // _darma_has_feature(mpi_interop)

inline FlowRelationshipImpl
null_flow() {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Null,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
same_flow(types::flow_t* rel) {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Same,
    /* related flow = */ rel,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
same_flow_as_in() {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Same,
    /* related flow = */ nullptr,
    /* related_is_in = */ true,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
same_anti_flow(types::anti_flow_t* rel) {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Same,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ rel,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
initial_anti_flow() {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Initial,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}

#if _darma_has_feature(mpi_interop)
inline FlowRelationshipImpl
initial_imported_anti_flow() {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::InitialImported,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}
#endif // _darma_has_feature(mpi_interop)

inline FlowRelationshipImpl
anti_next_of_in_flow() {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Next,
    /* related flow = */ nullptr,
    /* related_is_in = */ true,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
next_flow(types::flow_t* rel) {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Next,
    /* related flow = */ rel,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
next_of_in_flow() {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Next,
    /* related flow = */ nullptr,
    /* related_is_in = */ true,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
forwarding_flow(types::flow_t* rel) {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Forwarding,
    /* related flow = */ rel,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
insignificant_flow() {
  // The default constructed flow relationship is the insignificant flow
  return FlowRelationshipImpl();
}

inline FlowRelationshipImpl
indexed_fetching_flow(
  types::flow_t* rel,
  types::key_t const* version_key,
  std::size_t backend_index
) {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::IndexedFetching,
    /* related flow = */ rel,
    /* related_is_in = */ false,
    /* version key = */ version_key,
    /* index = */ backend_index,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
indexed_fetching_anti_flow(
  types::anti_flow_t* rel,
  types::key_t const* version_key,
  std::size_t backend_index
) {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::IndexedFetching,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ version_key,
    /* index = */ backend_index,
    /* anti_related = */ rel,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
indexed_local_flow(
  types::flow_t* rel,
  std::size_t backend_index
) {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::IndexedLocal,
    /* related flow = */ rel,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ backend_index,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
indexed_local_anti_flow(
  types::anti_flow_t* rel,
  std::size_t backend_index
) {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::IndexedLocal,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ backend_index,
    /* anti_related = */ rel,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
indexed_flow(
  types::flow_t* rel,
  std::size_t backend_index
) {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Indexed,
    /* related flow = */ rel,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ backend_index,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
indexed_anti_flow(
  types::anti_flow_t* rel,
  std::size_t backend_index
) {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Indexed,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ backend_index,
    /* anti_related = */ rel,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
forwarding_anti_flow(
  types::anti_flow_t* rel
) {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Forwarding,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ rel,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
next_anti_flow(
  types::anti_flow_t* rel
) {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Next,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ rel,
    /* anti_rel_is_in = */ false
  );
}

inline FlowRelationshipImpl
next_anti_flow_of_anti_in() {
  return FlowRelationshipImpl(
    abstract::frontend::FlowRelationship::Next,
    /* related flow = */ nullptr,
    /* related_is_in = */ false,
    /* version key = */ nullptr,
    /* index = */ 0,
    /* anti_related = */ nullptr,
    /* anti_rel_is_in = */ true
  );
}

} // end namespace flow_relationships

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_FLOW_RELATIONSHIP_H
