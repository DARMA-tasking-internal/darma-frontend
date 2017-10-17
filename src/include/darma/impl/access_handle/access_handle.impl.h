/*
//@HEADER
// ************************************************************************
//
//                      access_handle.impl.h
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

#ifndef DARMAFRONTEND_ACCESS_HANDLE_IMPL_H
#define DARMAFRONTEND_ACCESS_HANDLE_IMPL_H

#include <darma/interface/app/access_handle.h>

#include <darma/impl/access_handle/access_handle_base.impl.h>
#include <darma/impl/access_handle/access_handle_capture_description.h>

namespace darma_runtime {

//==============================================================================

template <typename T, typename Traits>
AccessHandle<T, Traits>::AccessHandle(
  AccessHandle<T, Traits> const& copied_from
) : detail::BasicAccessHandle()
{

  auto result = this->copy_capture_handler_t::handle_copy_construct(
    copied_from
  );

  if(not result.did_capture and not result.argument_is_garbage) {
    // then we need to propagate stuff here, since no capture handler was invoked
    // in other words, we're acting like an assignment
    this->detail::BasicAccessHandle::_do_assignment(copied_from);
    other_private_members_ = copied_from.other_private_members_;
  }

}

//==============================================================================

//------------------------------------------------------------------------------
// <editor-fold desc="DARMA feature: task_migration"> {{{2
#if _darma_has_feature(task_migration)
template <typename T, typename Traits>
template <typename Archive>
void
AccessHandle<T, Traits>::unpack_from_archive(Archive& ar) {

  key_t k = make_key();
  ar >> k;

  var_handle_base_ = detail::make_shared<detail::VariableHandle<T>>(k);

  detail::HandleUse::permissions_t sched = frontend::Permissions::_invalid;
  detail::HandleUse::permissions_t immed = frontend::Permissions::_invalid;

  ar >> sched >> immed;

  auto* backend_runtime = abstract::backend::get_backend_runtime();

  using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;

  // TODO decide if unpacking here breaks encapsulation (it should probably be done in the Use ctor)

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
    std::move(anti_in_flow), std::move(anti_out_flow)
  ));

}

#endif // _darma_has_feature(task_migration)
// </editor-fold> end DARMA feature: task_migration }}}2
//------------------------------------------------------------------------------

//==============================================================================

} // end namespace darma_runtime


#endif //DARMAFRONTEND_ACCESS_HANDLE_IMPL_H
