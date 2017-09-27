/*
//@HEADER
// ************************************************************************
//
//                      access_handle_serialization.h
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

#ifndef DARMAFRONTEND_IMPL_ACCESS_HANDLE_ACCESS_HANDLE_SERIALIZATION_H
#define DARMAFRONTEND_IMPL_ACCESS_HANDLE_ACCESS_HANDLE_SERIALIZATION_H

#include <darma/impl/feature_testing_macros.h>

#include <darma/interface/app/access_handle.h>

namespace darma_runtime {

//==============================================================================
// <editor-fold desc="Serialization"> {{{1

#if _darma_has_feature(task_migration)
namespace serialization {

template <typename... Args>
struct Serializer<AccessHandle<Args...>>
{
    // TODO update this for all of the special members now present in AccessHandle
  private:
    using AccessHandleT = AccessHandle<Args...>;

    bool handle_is_serializable_assertions(AccessHandleT const& val) const
    {
      // The handle has to be set up and valid
      assert(val.var_handle_.get() != nullptr);
      // The only AccessHandle objects that should ever be migrated are ones that
      // are already registered as part of a task.
      assert(val.current_use_base_->is_registered == true);
      // Also, since this has to be before the task runs, whether or not the use
      // can establish an alias should be exactly determined by whether or not it
      // has modify scheduling permissions (and less than modify immediate permissions)
      assert(
        ((
          val.current_use_->use->scheduling_permissions_
            == ::darma_runtime::frontend::Permissions::Modify
            and val.current_use_->use->immediate_permissions_
              != ::darma_runtime::frontend::Permissions::Modify
        ) and val.current_use_->could_be_alias)
          or (not(
            val.current_use_->use->scheduling_permissions_
              == ::darma_runtime::frontend::Permissions::Modify
              and val.current_use_->use->immediate_permissions_
                != ::darma_runtime::frontend::Permissions::Modify
          ) and not val.current_use_->could_be_alias)
      );
      // captured_as_ should always be normal here
      assert(val.current_use_->use->suspended_out_flow_ == nullptr);
      return true;
    }


  public:
    template <typename ArchiveT>
    void compute_size(AccessHandleT const& val, ArchiveT& ar) const
    {

      assert(handle_is_serializable_assertions(val));

      ar % val.var_handle_->get_key();
      ar % val.current_use_->use->scheduling_permissions_;
      ar % val.current_use_->use->immediate_permissions_;

      auto* backend_runtime = abstract::backend::get_backend_runtime();
      // TODO if we add operator==() to the requirements of flow_t, we don't have to pack the outflow when it's the same as the inflow
      ar.add_to_size_indirect(
        backend_runtime->get_packed_flow_size(val.current_use_->use->in_flow_)
      );
      ar.add_to_size_indirect(
        backend_runtime->get_packed_flow_size(val.current_use_->use->out_flow_)
      );
#if _darma_has_feature(anti_flows)
      ar.add_to_size_indirect(
        backend_runtime->get_packed_anti_flow_size(val.current_use_->use->anti_in_flow_)
      );
      ar.add_to_size_indirect(
        backend_runtime->get_packed_anti_flow_size(val.current_use_->use->anti_out_flow_)
      );
#endif // _darma_has_feature(anti_flows)

    }

    template <typename ArchiveT>
    void pack(AccessHandleT const& val, ArchiveT& ar) const
    {

      assert(handle_is_serializable_assertions(val));

      ar << val.var_handle_->get_key();
      ar << val.current_use_->use->scheduling_permissions_;
      ar << val.current_use_->use->immediate_permissions_;

      using detail::DependencyHandle_attorneys::ArchiveAccess;
      auto* backend_runtime = abstract::backend::get_backend_runtime();
      // TODO if we add operator==() to the requirements of flow_t, we don't have to pack the outflow when it's the same as the inflow
      backend_runtime->pack_flow(
        val.current_use_->use->in_flow_,
        ArchiveAccess::get_spot(ar)
      );
      backend_runtime->pack_flow(
        val.current_use_->use->out_flow_,
        ArchiveAccess::get_spot(ar)
      );
#if _darma_has_feature(anti_flows)
      backend_runtime->pack_anti_flow(
        val.current_use_->use->anti_in_flow_,
        ArchiveAccess::get_spot(ar)
      );
      backend_runtime->pack_anti_flow(
        val.current_use_->use->anti_out_flow_,
        ArchiveAccess::get_spot(ar)
      );
#endif // _darma_has_feature(anti_flows)

    }

    template <typename ArchiveT>
    void unpack(void* allocated, ArchiveT& ar) const
    {
      // Call an unpacking constructor
      new(allocated) AccessHandleT(serialization::unpack_constructor_tag, ar);
    }
};

} // end namespace serialization
#endif // _darma_has_feature(task_migration)

// </editor-fold> end Serialization }}}1
//==============================================================================

} // end namespace darma_runtime

#endif //DARMAFRONTEND_IMPL_ACCESS_HANDLE_ACCESS_HANDLE_SERIALIZATION_H
