/*
//@HEADER
// ************************************************************************
//
//                      legacy_runtime.h
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

#ifndef DARMA_INTERFACE_BACKEND_LEGACY_RUNTIME_H
#define DARMA_INTERFACE_BACKEND_LEGACY_RUNTIME_H

#include <darma_types.h>

#include "runtime.h"

namespace darma_runtime {
namespace abstract {
namespace backend {

/**
 *  @ingroup abstract
 *  @class Runtime
 *  @brief Abstract class implemented by the backend containing much of the
 *  runtime.
 *
 *  @note Thread safety of all methods in this class should be handled by the
 *  backend implementaton; two threads must be allowed to call any method in
 *  this class simultaneously.
 *
 */
class LegacyFlowsFromMethodsRuntime : public Runtime {
  public:

    //==========================================================================
    // <editor-fold desc="Flow handling">

    /** @brief Make an initial Flow to be associated with the handle given as an
     *  argument.
     *
     *  The initial Flow will be used as the return value of u->get_in_flow()
     *  for the first Use* u registered with write privileges that returns
     *  handle for u->get_handle().
     *
     *  @remark In the sequential semantic (C++) frontend, this will usually
     *  derive from calls to initial_access in the application code.
     *
     *  @param handle A handle encapsulating a type and unique name (variable)
     *  for which the Flow will represent the initial state (upon a subsequent
     *  call to register_use())
     *
     *  @return The flow for the frontend to use as described
     *
     *
     */
    virtual types::flow_t
    make_initial_flow(
      std::shared_ptr<frontend::Handle const> const& handle
    ) =0;

#if _darma_has_feature(create_concurrent_work)
    /** @brief Similar to make_initial_flow, but for a Use that manages a
     *  collection
     *
     *  @sa Use::manages_collection()
     *  @sa UseCollection
     *
     *  @param handle Analogously to make_initial_flow(), the argument is handle
     *  encapsulating a type and unique name (variable) for which the Flow will
     *  represent the initial state (upon a subsequent call to register_use())
     *
     *  @return The flow for the frontend to use as described
     */
    virtual types::flow_t
    make_initial_flow_collection(
      std::shared_ptr<frontend::Handle const> const& handle
    ) =0;
#endif

    /** @brief Make a null Flow to be associated with the handle given as an
     *  argument.
     *
     *  A null usage as a return value of u->get_out_flow() for some Use* u is
     *  intended to indicate that the data associated with that Use has no
     *  subsequent consumers and can safely be deleted when other Uses are
     *  released.  See release_use().
     *
     *  @param handle The handle variable that will be associated with the Flow
     *  in the corresponding call to register_use()
     *
     *  @return The flow for the frontend to use as described
     *
     */
    virtual types::flow_t
    make_null_flow(
      std::shared_ptr<frontend::Handle const> const& handle
    ) =0;

#if _darma_has_feature(create_concurrent_work)
    /** @brief Analogue of make_null_flow() for a use that manages a collection
     *
     *  @sa make_null_flow()
     *
     *  @param handle The handle variable that will be associated with the Flow
     *  in the corresponding call to register_use()
     *
     *  @return The flow for the frontend to use as described
     */
    virtual types::flow_t
    make_null_flow_collection(
      std::shared_ptr<frontend::Handle const> const& handle
    ) =0;
#endif

    /** @todo update this
     *  @brief Make a new Flow that receives forwarded changes from
     *  another input Flow, the latter of which is associated with a Use on
     *  which Modify immediate permissions were requested and already granted
     *  (via a backend call to Task::run() on the Task object that that Use
     *  was uniquely associated with).
     *
     *  Flows are registered and released indirectly through calls to
     *  register_use()/release_use().  The input Flow to make_forwarding_flow()
     *  must have been registered through a register_use() call, but not yet
     *  released through a release_use() call.
     *  make_forwarding_flow() can be called at most once with a given input.
     *
     *  @param from An already initialized and registered flow returned from
     *  `make_*_flow`
     *
     *  @param purpose An enum indicating the relationship between the flows
     *  (purpose of the function). This information is provided for optimization
     *  purposes. In the current specification, this enum will always be
     *  ForwardingChanges
     *
     *  @return A new Flow object indicating that new data is the produced by
     *  immediate modifications to the data from the Flow given as a parameter
     */
    virtual types::flow_t
    make_forwarding_flow(
      types::flow_t& from
    ) =0;

#if _darma_has_feature(arbitrary_publish_fetch)
    virtual types::flow_t
    make_fetching_flow(
      std::shared_ptr<handle_t const> const& handle,
      types::key_t const& version_key
    ) =0;
#endif

#if _darma_has_feature(create_concurrent_work)
    /** @todo document this
     *
     * @remark Parameter must be a value returned from one of the
     * `make_*_flow_collection()` methods
     *
     * @param from
     * @return
     */
    virtual types::flow_t
    make_indexed_local_flow(
      types::flow_t& from,
      size_t backend_index
    ) =0;

    /** @todo document this
     *
     * @remark Parameter must be a value returned from one of the
     * `make_*_flow_collection()` methods
     *
     * @param from
     * @return
     */
    virtual types::flow_t
    make_indexed_fetching_flow(
      types::flow_t& from,
      types::key_t const& version_key,
      size_t backend_index
    ) =0;

#if _darma_has_feature(commutative_access_handles)
    virtual types::flow_t
    make_indexed_flow(
      types::flow_t& from,
      size_t backend_index
    ) {
      return from; // filler, override if needed
    }
#endif
#endif

    /** @todo update this
     *
     *  @brief Make a flow that will be logically (not necessarily immediately)
     *  subsequent to another Flow
     *
     *  Calls to make_next_flow() indicate a producer-consumer relationship
     *  between Flows. make_next_flow() indicates that an operation consumes
     *  Flow* from and produces the returned Flow*. A direct subsequent
     *  relationship should not be inferred here; the direct subsequent of input
     *  flow `from` will only be the output flow within the same Use if no other
     *  subsequents of the input use are created and registered in its lifetime.
     *  Flows are registered and released indirectly through calls to
     *  register_use()/release_use(). Flow instances cannot be shared across Use
     *  instances. The input to make_next_flow() must have been registered with
     *  register_use(), but not yet released through release_use().
     *  make_next_flow() can be called at most once with a given input.
     *
     *  @param from    The flow consumed by an operation to produce the Flow
     *  returned by make_next_flow()
     *
     *  @param purpose An enum indicating the purpose of the next flow.  This
     *  information is provided for optimization purposes
     *
     *  @return A new Flow object indicating that new data will be produced by
     *  the data incoming from the Flow given as a parameter
     */
    virtual types::flow_t
    make_next_flow(
      types::flow_t& from
    ) =0;

#if _darma_has_feature(create_concurrent_work)
    /** @todo document this
     *
     * @param from
     * @return
     */
    virtual types::flow_t
    make_next_flow_collection(
      types::flow_t& from
    ) =0;
#endif


    /** @todo document this
     */
    virtual void
    establish_flow_alias(
      types::flow_t& from,
      types::flow_t& to
    ) =0;

    // </editor-fold> end flow handling
    //==========================================================================

    // Legacy support for register_use and release_use

    virtual void
    legacy_register_use(frontend::DependencyUse* use_in_legacy_form) =0;

    virtual void
    legacy_release_use(frontend::DependencyUse* use_in_legacy_form) =0;

    // Implementations of the new register_use() and release_use() that translate
    // to the old semantics
    virtual void
    register_use(frontend::UsePendingRegistration* use) override
    {
      using namespace darma_runtime::abstract::frontend;
      types::flow_t in_flow;
      auto const& in_rel = use->get_in_flow_relationship();

      switch (in_rel.description()) {
        case FlowRelationship::Same :
        case FlowRelationship::SameCollection : {
          in_flow = *in_rel.related_flow();
          break;
        }
        case FlowRelationship::Initial : {
          in_flow = make_initial_flow(use->get_handle());
          break;
        }
#if _darma_has_feature(arbitrary_publish_fetch)
        case FlowRelationship::Fetching : {
          in_flow = make_fetching_flow(use->get_handle(), *in_rel.version_key());
          break;
        }
#endif //_darma_has_feature(arbitrary_publish_fetch)
        case FlowRelationship::Null : {
          assert(false); // not allowed as in flow
          break;
        }
        case FlowRelationship::Next : {
          in_flow = make_next_flow(*in_rel.related_flow());
          break;
        }
        case FlowRelationship::ForwardingCollection : // for now, just ignored the collection part
        case FlowRelationship::Forwarding : {
          in_flow = make_forwarding_flow(*in_rel.related_flow());
          break;
        }
        case FlowRelationship::InitialCollection : {
          in_flow = make_initial_flow_collection(use->get_handle());
          break;
        }
        case FlowRelationship::NullCollection : {
          assert(false); // not allowed as in flow
          break;
        }
        case FlowRelationship::NextCollection : {
          in_flow = make_next_flow_collection(*in_rel.related_flow());
          break;
        }
        case FlowRelationship::IndexedLocal : {
          in_flow = make_indexed_local_flow(
            *in_rel.related_flow(),
            in_rel.index()
          );
          break;
        }
#if _darma_has_feature(commutative_access_handles)
        case FlowRelationship::Indexed : {
          in_flow = make_indexed_flow(
            *in_rel.related_flow(),
            in_rel.index()
          );
          break;
        }
#endif // _darma_has_feature(commutative_access_handles)
        case FlowRelationship::IndexedFetching : {
          in_flow = make_indexed_fetching_flow(
            *in_rel.related_flow(),
            *in_rel.version_key(),
            in_rel.index()
          );
          break;
        }
        default: {
          assert(false); // not implemented, what did I forget?
          break;
        }
      } // End switch in_flow_relationship description

      auto const& out_rel = use->get_out_flow_relationship();
      types::flow_t* out_related_flow = out_rel.related_flow();
      if(out_rel.use_corresponding_in_flow_as_related()) {
        assert(out_related_flow == nullptr);
        out_related_flow = std::addressof(in_flow);
      }
      types::flow_t out_flow;

      switch (out_rel.description()) {
        case FlowRelationship::Same :
        case FlowRelationship::SameCollection : {
          out_flow = *out_related_flow;
          break;
        }
        case FlowRelationship::Initial : {
          assert(false); // not allowed
          break;
        }
        case FlowRelationship::Null : {
          out_flow = make_null_flow(use->get_handle());
          break;
        }
        case FlowRelationship::Next : {
          out_flow = make_next_flow(*out_related_flow);
          break;
        }
        case FlowRelationship::Forwarding : {
          assert(false); // should be not allowed
          break;
        }
        case FlowRelationship::InitialCollection : {
          assert(false); // should be not allowed
          break;
        }
        case FlowRelationship::NullCollection : {
          out_flow = make_null_flow_collection(use->get_handle());
          break;
        }
        case FlowRelationship::NextCollection : {
          out_flow = make_next_flow_collection(*out_related_flow);
          break;
        }
        case FlowRelationship::IndexedLocal : {
          out_flow = make_indexed_local_flow(
            *out_related_flow,
            out_rel.index()
          );
          break;
        }
#if _darma_has_feature(commutative_access_handles)
        case FlowRelationship::Indexed : {
          out_flow = make_indexed_flow(
            *out_rel.related_flow(),
            out_rel.index()
          );
          break;
        }
#endif // _darma_has_feature(commutative_access_handles)
        case FlowRelationship::IndexedFetching : {
          out_flow = make_indexed_fetching_flow(
            *out_related_flow,
            *out_rel.version_key(),
            out_rel.index()
          );
          break;
        }
#if _darma_has_feature(anti_flows)
        case FlowRelationship::Insignificant :
        case FlowRelationship::InsignificantCollection : {
          // make it null, for now (may not be that way for ever)
          out_flow = types::flow_t(nullptr);
          break;
        }
#endif // _darma_has_feature(anti_flows)
        default: {
          assert(false); // not implemented, what did I forget?
          break;
        }
      } // End switch out_flow_relationship description

      use->set_in_flow(in_flow);
      use->set_out_flow(out_flow);

      // For now, assume (incorrectly) that get_data_pointer_reference() is also
      // valid to call in register_use(), since some old implementations could
      // have been doing that
#if _darma_has_feature(register_all_uses)
      legacy_register_use(
        use_cast<DependencyUse*>(use)
      );
#else // !_darma_has_feature(register_all_uses)
      if(use->will_be_dependency()) {
        legacy_register_use(
          use_cast<DependencyUse*>(use)
        );
      }
#endif // _darma_has_feature(register_all_uses)

    }

    virtual void
    release_use(frontend::UsePendingRelease* use) override
    {
      if(use->establishes_alias()) {
        establish_flow_alias(use->get_in_flow(), use->get_out_flow());
      }

      // For now, assume (incorrectly) that get_data_pointer_reference() is also
      // valid to call in release_use(), since some old implementations could
      // have been doing that (thus, we cast it to a DependencyUse here)
#if _darma_has_feature(register_all_uses)
      legacy_release_use(
        frontend::use_cast<frontend::DependencyUse*>(use)
      );
#else // !_darma_has_feature(register_all_uses)
      if(use->was_dependency()) {
        legacy_release_use(
          use_cast<frontend::DependencyUse*>(use)
        );
      }
#endif // _darma_has_feature(register_all_uses)


    }

    virtual ~LegacyFlowsFromMethodsRuntime() noexcept = default;
};

} // end namespace backend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_INTERFACE_BACKEND_LEGACY_RUNTIME_H
