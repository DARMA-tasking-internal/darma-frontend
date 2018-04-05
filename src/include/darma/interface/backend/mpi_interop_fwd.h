/*
//@HEADER
// ************************************************************************
//
//                       mpi_interop_fwd.h
//                            darma
//              Copyright (C) 2017 NTESS, LLC
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

#ifndef SRC_BACKEND_MPI_INTEROP_FWD_H_
#define SRC_BACKEND_MPI_INTEROP_FWD_H_

#include <functional> // std::function
#include <utility> // std::forward
#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(mpi_interop)

#include <darma_types.h>

namespace darma_runtime {

namespace backend {

using namespace darma_runtime::types;

runtime_context_token_t
create_runtime_context(darma_runtime::types::MPI_Comm);

piecewise_collection_token_t
register_piecewise_collection(
  runtime_context_token_t,
  std::shared_ptr<darma_runtime::abstract::frontend::Handle>,
  size_t
);

persistent_collection_token_t
register_persistent_collection(
  runtime_context_token_t,
  std::shared_ptr<darma_runtime::abstract::frontend::Handle>,
  size_t
);

void
register_piecewise_collection_piece(
  runtime_context_token_t,
  piecewise_collection_token_t,
  size_t,
  void*,
  std::function<void(void const*, void*)>,
  std::function<void(void const*, void*)>
);

void run_distributed_region(
  runtime_context_token_t,
  std::function<void()>
);

void
run_distributed_region_worker(runtime_context_token_t);

} // end namespace backend

} // end namespace darma_runtime

#endif

#endif /* SRC_BACKEND_MPI_INTEROP_FWD_H_ */
