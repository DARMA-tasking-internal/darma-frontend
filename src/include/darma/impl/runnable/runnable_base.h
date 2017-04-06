/*
//@HEADER
// ************************************************************************
//
//                      runnable_base.h
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

#ifndef DARMA_IMPL_RUNNABLE_RUNNABLE_BASE_H
#define DARMA_IMPL_RUNNABLE_RUNNABLE_BASE_H

#include <cstdlib>

#include "runnable_fwd.h"

namespace darma_runtime {
namespace detail {

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="RunnableBase">

class RunnableBase {
  public:
    virtual bool run() =0;

#if _darma_has_feature(task_migration)
    virtual size_t get_index() const =0;
    virtual size_t get_packed_size() const =0;
    virtual void pack(void* allocated) const =0;
#endif // _darma_has_feature(task_migration)

    virtual ~RunnableBase() { }
    bool is_lambda_like_runnable = false;
    virtual void copy_lambda(void* dest) const { /* do nothing unless is_lambda_like */ }
    virtual std::size_t lambda_size() const { return 0; }
    virtual bool needs_resource_pack() const { return false; }
    virtual void set_resource_pack(types::resource_pack_t const&) { assert(false); }
};

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_RUNNABLE_RUNNABLE_BASE_H
