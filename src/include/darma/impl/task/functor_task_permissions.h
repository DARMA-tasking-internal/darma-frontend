/*
//@HEADER
// ************************************************************************
//
//                      functor_task.h
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

#ifndef DARMA_FUNCTOR_PERMISSIONS_H
#define DARMA_FUNCTOR_PERMISSIONS_H

#include <darma/impl/create_work/capture_permissions.h>

namespace darma {
namespace detail {

namespace impl {

template <typename LastArg>
void
_apply_permissions_impl(LastArg&& last_arg) {
   last_arg.do_source_permissions_modifications();
};

template <typename FirstArg, typename... LastArgs>
void
_apply_permissions_impl(FirstArg&& first_arg, LastArgs&&... last_args) {
   first_arg.do_source_permissions_modifications();
   _apply_permissions_impl(std::forward<LastArgs>(last_args)...);
};

} // end namespace impl


template <typename... Args>
void permissions_downgrades(Args&&... deferred_permissions_modifications) {
   impl::_apply_permissions_impl(std::forward<Args>(deferred_permissions_modifications)...);
};

template <typename... Args>
void required_permissions(Args&&... deferred_permissions_modifications) {
   impl::_apply_permissions_impl(std::forward<Args>(deferred_permissions_modifications)...);
};

} // end namespace detail
} // end namespace darma

#endif // DARMAFRONTEND_PERMISSIONS_DOWNGRADES_H
