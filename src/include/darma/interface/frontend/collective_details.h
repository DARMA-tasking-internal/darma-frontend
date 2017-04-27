/*
//@HEADER
// ************************************************************************
//
//                      collective_details.h
//                         DARMA
//              Copyright (C) 2016 Sandia Corporation
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

#ifndef DARMA_INTERFACE_FRONTEND_COLLECTIVE_DETAILS_H
#define DARMA_INTERFACE_FRONTEND_COLLECTIVE_DETAILS_H

#include <cstdlib>

#include "reduce_operation.h"
#include <darma/interface/backend/region_context_handle.h>
#include <darma/impl/feature_testing_macros.h>

namespace darma_runtime {
namespace abstract {
namespace frontend {


class CollectiveDetails {
  public:

    /** @deprecated */
    static inline constexpr size_t
    unknown_contribution() {
      return std::numeric_limits<size_t>::max();
    }

    /** @deprecated */
    virtual size_t
    this_contribution() const =0;

    /** @deprecated */
    virtual size_t
    n_contributions() const =0;

    /** @deprecated (though might be added later) */
    virtual bool
    is_indexed() const =0;

    virtual ReduceOp const*
    reduce_operation() const =0;

#if _darma_has_feature(task_collection_token)
    virtual darma_runtime::types::task_collection_token_t const&
    get_task_collection_token() const =0;
#endif // _darma_has_feature(task_collection_token)

};


} // end namespace frontend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_INTERFACE_FRONTEND_COLLECTIVE_DETAILS_H
