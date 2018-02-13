/*
//@HEADER
// ************************************************************************
//
//                      access_handle.impl.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

  // If we didn't do a capture and the argument isn't garbage (e.g., from
  // lambda serdes process), then we need to do the normal copy constructor
  // operations
  if(not result.did_capture and not result.argument_is_garbage) {
    // then we need to propagate stuff here, since no capture handler was invoked
    // in other words, we're acting like an assignment
    this->detail::BasicAccessHandle::_do_assignment(copied_from);
    other_private_members_ = copied_from.other_private_members_;
  }

}

//==============================================================================

} // end namespace darma_runtime


#endif //DARMAFRONTEND_ACCESS_HANDLE_IMPL_H
