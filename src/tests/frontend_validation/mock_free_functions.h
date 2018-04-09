/*
//@HEADER
// ************************************************************************
//
//                       mock_free_functions.h
//                         darma_new
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


#ifndef SRC_TESTS_FRONTEND_VALIDATION_MOCK_FREE_FUNCTIONS_H_
#define SRC_TESTS_FRONTEND_VALIDATION_MOCK_FREE_FUNCTIONS_H_

#include <utility>

#include "mock_backend.h"
#include <darma/interface/app/initial_access.h>
#include <darma/interface/app/read_access.h>
#include <darma/interface/app/create_work.h>
#include <darma/impl/data_store.h>
#include <darma/impl/task_collection/task_collection.h>
#include <darma/impl/task_collection/access_handle_collection.h>
#include <darma/impl/index_range/mapping.h>
#include <darma/impl/array/index_range.h>
#include <darma/impl/task_collection/create_concurrent_work.h>

#include <darma/impl/access_handle/access_handle_collection.impl.h>

namespace darma_runtime {

namespace backend {

using namespace darma_runtime::types;

runtime_context_token_t create_runtime_context(darma_runtime::types::MPI_Comm) {return { 1 };}

piecewise_collection_token_t register_piecewise_collection(runtime_context_token_t,
  std::shared_ptr<darma_runtime::abstract::frontend::Handle>,
  size_t
) {
  return piecewise_collection_token_t();
}

void 
register_piecewise_collection_piece(
  runtime_context_token_t context_token,
  piecewise_collection_token_t piecewise_token,
  size_t index,
  void*,
  std::function<void(void const*, void*)>,
  std::function<void(void const*, void*)>
) {

}

void 
run_distributed_region(
  runtime_context_token_t,
  std::function<void()>
) { }

void run_distributed_region_worker(darma_runtime::types::runtime_context_token_t) { }

void
release_piecewise_collection(
  types::runtime_context_token_t,
  types::piecewise_collection_token_t
) { /* TODO trigger mock here */ }

void
release_persistent_collection(
  types::runtime_context_token_t,
  types::persistent_collection_token_t
) { /* TODO trigger mock here */ }

void conversion_to_ahc(double,darma_runtime::AccessHandleCollection<double,darma_runtime::Range1D<int>>) { }

} // end namespace backend

} // end namespace mock_backend

#endif /* SRC_TESTS_FRONTEND_VALIDATION_MOCK_BACKEND_H_ */
