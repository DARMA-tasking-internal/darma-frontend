/*
//@HEADER
// ************************************************************************
//
//                         mpi_interop.h
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

#ifndef SRC_BACKEND_MPI_INTEROP_H_
#define SRC_BACKEND_MPI_INTEROP_H_

#include <darma/interface/backend/mpi_interop_fwd.h>

#if _darma_has_feature(mpi_interop)

#include <darma_types.h>

namespace darma {

namespace backend {

template <
  typename Serializable,
  typename CopyOutCallback,
  typename CopyInCallback
>
void
register_piecewise_collection_piece(
  types::runtime_context_token_t context_token,
  types::piecewise_collection_token_t collection_token,
  size_t piece_index,
  Serializable& piece,
  CopyOutCallback&& copy_out = nullptr,
  CopyInCallback&& copy_in = nullptr
) {
    register_piecewise_collection_piece(context_token,
    collection_token,
    piece_index,
    (void*)&piece,
    std::function<void(void const*, void*)>(std::forward<CopyOutCallback>(copy_out)),
    std::function<void(void const*, void*)>(std::forward<CopyInCallback>(copy_in))
  );
}

template <typename Callable>
void
run_distributed_region(
  types::runtime_context_token_t context_token,
  Callable&& callable
) {
  run_distributed_region(
    context_token,
    std::function<void()>(std::forward<Callable>(callable))
  );
}

} // end namespace backend

} // end namespace darma

#endif

#endif /* SRC_BACKEND_MPI_INTEROP_H_ */
