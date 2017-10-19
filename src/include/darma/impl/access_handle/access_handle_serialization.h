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
      assert(val.var_handle_base_.get() != nullptr);
      // Suspended out flow shouldn't exist
      assert(val.get_current_use()->use()->suspended_out_flow_ == nullptr);
      return true;
    }


  public:
    template <typename ArchiveT>
    void compute_size(AccessHandleT const& val, ArchiveT& ar) const
    {

      assert(handle_is_serializable_assertions(val));

      ar % val.var_handle_base_->get_key();
      ar % val.get_current_use()->use()->scheduling_permissions_;
      ar % val.get_current_use()->use()->immediate_permissions_;
      ar % val.get_current_use()->use()->coherence_mode_;

      auto* backend_runtime = abstract::backend::get_backend_runtime();
      ar.add_to_size_indirect(
        backend_runtime->get_packed_flow_size(val.get_current_use()->use()->in_flow_)
      );
      ar.add_to_size_indirect(
        backend_runtime->get_packed_flow_size(val.get_current_use()->use()->out_flow_)
      );
      ar.add_to_size_indirect(
        backend_runtime->get_packed_anti_flow_size(val.get_current_use()->use()->anti_in_flow_)
      );
      ar.add_to_size_indirect(
        backend_runtime->get_packed_anti_flow_size(val.get_current_use()->use()->anti_out_flow_)
      );

    }

    template <typename ArchiveT>
    void pack(AccessHandleT const& val, ArchiveT& ar) const
    {
      assert(handle_is_serializable_assertions(val));

      ar << val.var_handle_base_->get_key();
      ar << val.get_current_use()->use()->scheduling_permissions_;
      ar << val.get_current_use()->use()->immediate_permissions_;
      ar << val.get_current_use()->use()->coherence_mode_;

      using detail::DependencyHandle_attorneys::ArchiveAccess;
      auto* backend_runtime = abstract::backend::get_backend_runtime();
      // TODO if we add operator==() to the requirements of flow_t, we don't have to pack the outflow when it's the same as the inflow
      backend_runtime->pack_flow(
        val.get_current_use()->use()->in_flow_,
        ArchiveAccess::get_spot(ar)
      );
      backend_runtime->pack_flow(
        val.get_current_use()->use()->out_flow_,
        ArchiveAccess::get_spot(ar)
      );
      backend_runtime->pack_anti_flow(
        val.get_current_use()->use()->anti_in_flow_,
        ArchiveAccess::get_spot(ar)
      );
      backend_runtime->pack_anti_flow(
        val.get_current_use()->use()->anti_out_flow_,
        ArchiveAccess::get_spot(ar)
      );
    }

    template <typename ArchiveT>
    void unpack(void* allocated, ArchiveT& ar) const
    {
      // Call an unpacking constructor
      new(allocated) AccessHandleT(serialization::unpack_constructor_tag, ar);
    }
};

} // end namespace serialization

template <typename T, typename Traits>
template <typename Archive>
void
AccessHandle<T, Traits>::unpack_from_archive(Archive& ar) {

  key_t k = make_key();
  ar >> k;

  var_handle_base_ = detail::make_shared<detail::VariableHandle<T>>(k);

  // TODO decide if unpacking here breaks encapsulation (it should probably be done in Use ctor)

  frontend::permissions_t sched = frontend::Permissions::_invalid;
  frontend::permissions_t immed = frontend::Permissions::_invalid;
  frontend::coherence_mode_t coherence_mode = frontend::CoherenceMode::Invalid;

  ar >> sched >> immed >> coherence_mode;

  auto* backend_runtime = abstract::backend::get_backend_runtime();

  using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;

  auto in_flow = backend_runtime->make_unpacked_flow(
    ArchiveAccess::get_const_spot(ar)
  );

  // Note that the backend function advances the underlying pointer, so the
  // pointer returned by get_spot is different in the call below from the
  // call above
  auto out_flow = backend_runtime->make_unpacked_flow(
    ArchiveAccess::get_const_spot(ar)
  );

  auto anti_in_flow = backend_runtime->make_unpacked_anti_flow(
    ArchiveAccess::get_const_spot(ar)
  );

  auto anti_out_flow = backend_runtime->make_unpacked_anti_flow(
    ArchiveAccess::get_const_spot(ar)
  );

  set_current_use(base_t::use_holder_t::recreate_migrated(
    var_handle_base_,
    sched, immed,
    std::move(in_flow), std::move(out_flow),
    std::move(anti_in_flow), std::move(anti_out_flow),
    coherence_mode
  ));

}
#endif // _darma_has_feature(task_migration)

// </editor-fold> end Serialization }}}1
//==============================================================================

} // end namespace darma_runtime

#endif //DARMAFRONTEND_IMPL_ACCESS_HANDLE_ACCESS_HANDLE_SERIALIZATION_H
