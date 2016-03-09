/*
//@HEADER
// ************************************************************************
//
//                          spmd.h
//                         darma_new
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

#ifndef SPMD_H_
#define SPMD_H_

#include <memory>

#include <darma/impl/runtime.h>
#include <darma/interface/backend/runtime.h>
#include <darma/impl/darma_assert.h>
#include <darma/impl/task.h>


namespace darma_runtime {

typedef size_t darma_rank_t;

inline void
darma_init(
  int& argc,
  char**& argv
) {
  std::unique_ptr<typename darma_runtime::abstract::backend::runtime_t::task_t> moved_from =
      std::make_unique<detail::TopLevelTask>();
  abstract::backend::darma_backend_initialize(
    argc, argv, detail::backend_runtime, std::move(moved_from)
  );
}

inline darma_rank_t
darma_spmd_rank() {
  DARMA_ASSERT_NOT_NULL(detail::backend_runtime);
  DARMA_ASSERT_NOT_NULL(detail::backend_runtime->get_running_task());

  DARMA_ASSERT_EQUAL(
    std::string(detail::backend_runtime->get_running_task()->get_name().component<0>().as<std::string>()),
    DARMA_BACKEND_SPMD_NAME_PREFIX
  );
  return detail::backend_runtime
      ->get_running_task()->get_name().component<1>().as<darma_rank_t>();
}

inline darma_rank_t
darma_spmd_size() {
  DARMA_ASSERT_EQUAL(
    std::string(detail::backend_runtime->get_running_task()->get_name().component<0>().as<std::string>()),
    DARMA_BACKEND_SPMD_NAME_PREFIX
  );
  return detail::backend_runtime
      ->get_running_task()->get_name().component<2>().as<darma_rank_t>();
}

inline void
darma_finalize() {
  detail::backend_runtime->finalize();
}

} // end namespace darma_runtime

#endif /* SPMD_H_ */
