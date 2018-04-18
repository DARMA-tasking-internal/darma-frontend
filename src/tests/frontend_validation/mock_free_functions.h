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

extern std::unique_ptr<mock_backend::MockMPIBackend> mock_mpi_backend;

namespace darma_runtime {

namespace backend {

using namespace darma_runtime::types;

inline runtime_context_token_t create_runtime_context(darma_runtime::types::MPI_Comm comm) {
   return mock_mpi_backend->create_runtime_context(comm);
}

inline piecewise_collection_token_t register_piecewise_collection(runtime_context_token_t context_token,
  std::shared_ptr<darma_runtime::abstract::frontend::Handle> handle,
  size_t size_range
) {
  return mock_mpi_backend->register_piecewise_collection(context_token, handle, size_range);
}

inline persistent_collection_token_t register_persistent_collection(runtime_context_token_t context_token,
  std::shared_ptr<darma_runtime::abstract::frontend::Handle> handle,
  size_t size_range
) {
  return mock_mpi_backend->register_persistent_collection(context_token, handle, size_range);
}

inline void
register_piecewise_collection_piece(
  runtime_context_token_t context_token,
  piecewise_collection_token_t piecewise_token,
  size_t index,
  void* pData,
  std::function<void(void const*, void*)> copy_callback,
  std::function<void(void const*, void*)> copy_back_callback
) {
  return mock_mpi_backend->register_piecewise_collection_piece(
    context_token, 
    piecewise_token,
    index,
    pData, 
    copy_callback,
    copy_back_callback
  );
}

inline void
run_distributed_region(
  runtime_context_token_t context_token,
  std::function<void()> foo
) { 
  mock_mpi_backend->run_distributed_region(context_token, foo);
}

inline void run_distributed_region_worker(runtime_context_token_t context_token) { 
  mock_mpi_backend->run_distributed_region_worker(context_token);
}

inline void
release_piecewise_collection(
  runtime_context_token_t context_token,
  piecewise_collection_token_t piecewise_token
) { 
  mock_mpi_backend->release_piecewise_collection(
    context_token,
    piecewise_token
  );
}

inline void
release_persistent_collection(
  runtime_context_token_t context_token,
  persistent_collection_token_t persistent_token
) { 
  mock_mpi_backend->release_persistent_collection(
    context_token,
    persistent_token
  );
}

} // end namespace backend

} // end namespace mock_backend

#endif /* SRC_TESTS_FRONTEND_VALIDATION_MOCK_BACKEND_H_ */
