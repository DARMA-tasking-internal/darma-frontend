/*
//@HEADER
// ************************************************************************
//
//                          darma_types.h
//                         dharma_new
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

#ifndef BACKENDS_SERIAL_DARMA_TYPES_H_
#define BACKENDS_SERIAL_DARMA_TYPES_H_

#define DARMA_BACKEND_SPMD_NAME_PREFIX "spmd"


#include <utility> // std::pair

namespace darma_runtime { namespace types {
  using cpu_set_t = /* not implemented */ int; // just a placeholder for now
}} // end namespace darma_runtime::types

namespace threads_backend {
  // forward declaration
  struct InnerFlow;
} // end namespace threads_backend

#include <darma/interface/defaults/pointers.h>

namespace darma_runtime { namespace types {
typedef types::shared_ptr_template<::threads_backend::InnerFlow> flow_t;
}} // end namespace darma_runtime::types


//#include <darma/impl/key/simple_key.h>
#include <darma/impl/key/SSO_key_fwd.h>

#ifndef DARMA_KEY_STACK_SIZE
#define DARMA_KEY_STACK_SIZE 64
#endif

namespace darma_runtime { namespace types {
typedef types::shared_ptr_template<::threads_backend::InnerFlow> flow_t;
}} // end namespace darma_runtime::types

namespace darma_runtime { namespace types {
  typedef darma_runtime::detail::SSOKey<
    DARMA_KEY_STACK_SIZE - 16,
    uint64_t
  > key_t;
}} // end namespace darma_runtime::types

#include <darma/impl/key/SSO_key.h>

#endif /* BACKENDS_SERIAL_DARMA_TYPES_H_ */
