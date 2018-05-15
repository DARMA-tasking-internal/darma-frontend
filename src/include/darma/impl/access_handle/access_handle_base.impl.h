/*
//@HEADER
// ************************************************************************
//
//                      access_handle_base.impl.h
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

#ifndef DARMAFRONTEND_ACCESS_HANDLE_BASE_IMPL_H
#define DARMAFRONTEND_ACCESS_HANDLE_BASE_IMPL_H

#include <darma/impl/access_handle_base.h>

namespace darma {
namespace detail {

constexpr inline frontend::permissions_t
detail::AccessHandleBase::get_captured_permissions_for(
  detail::AccessHandleBase::capture_op_t op, frontend::permissions_t permissions
) {
  using darma::frontend::Permissions;
  // This used to be a lot more complicated when Commutative/Relaxed/etc
  // were permissions rather than coherence modes.  I've left it here
  // for backwards compatibility, but it may be moved at some point
  switch(op) {
    case CaptureOp::read_only_capture : {
      switch(permissions) {
        case Permissions::Modify :
        case Permissions::Read : {
          return Permissions::Read;
        }
        case Permissions::Write :
        case Permissions::_invalid : // to avoid compiler warnings...
        case Permissions::_notGiven : // to avoid compiler warnings...
        case Permissions::None : {
          // can't do a read_only_capture if you have the above scheduling
          // permissions
          return Permissions::_invalid;
        }
      }
    }
    case CaptureOp::write_only_capture : {
      switch(permissions) {
        case Permissions::Modify :
        case Permissions::Write : {
          return Permissions::Write;
        }
        case Permissions::None :
        case Permissions::_invalid : // to avoid compiler warnings...
        case Permissions::_notGiven : // to avoid compiler warnings...
        case Permissions::Read : {
          // can't do a write_only_capture if you have the above scheduling
          // permissions
          return Permissions::_invalid;
        }
      }
    }
    case CaptureOp::modify_capture : {
      switch(permissions) {
        case Permissions::Modify :
        case Permissions::Write :
        case Permissions::Read : {
          // capture the greatest permissions allowed
          return permissions;
        }
        case Permissions::_invalid : // to avoid compiler warnings...
        case Permissions::_notGiven : // to avoid compiler warnings...
        case Permissions::None : {
          // can't capture with no scheduling permissions
          return Permissions::_invalid;
        }
      }
    }
    case CaptureOp::no_capture : {
      // for completeness
      return Permissions::None;
    }
  }
  // Shouldn't be reachable, but some compilers might warn about no return
  return Permissions::_invalid;                                                 // LCOV_EXCL_LINE
}


//inline void
//AccessHandleBase::call_add_dependency(TaskBase* task) {
//  task->add_dependency(*current_use_base_->use_base);
//}

} // end namespace detail
} // end namespace darma

#endif //DARMAFRONTEND_ACCESS_HANDLE_BASE_IMPL_H
