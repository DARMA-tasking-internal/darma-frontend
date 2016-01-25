/*
//@HEADER
// ************************************************************************
//
//                          runtime.h
//                         dharma_mockup
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

#ifndef NEW_RUNTIME_H_
#define NEW_RUNTIME_H_

#include "task_fwd.h"
#include "abstract/backend/runtime.h"

namespace dharma_runtime {

namespace detail {

template <
  typename key_type,
  typename version_type,
  template <typename...> class smart_ptr_template = std::shared_ptr,
  template <typename...> class weak_ptr_template =
      smart_ptr_traits<smart_ptr_template>::template weak_ptr_template
>
class runtime_context {
  public:

    typedef key_type key_t;
    typedef version_type version_t;
    typedef detail::Task<key_t, version_t> task_t;
    typedef smart_ptr_template<task_t> task_ptr;
    typedef weak_ptr_template<task_t> task_weak_ptr;

    runtime_context()
    {
      // TODO initialize rt_;
    }

    task_ptr current_running_task;
    task_weak_ptr current_create_work_context;

    int rank() {
      return backend_runtime->rank();
    }

    int nproc() {
      return backend_runtime->nproc();
    }



    abstract::backend::Runtime<key_type, version_type, smart_ptr_template>* backend_runtime;

};

extern thread_local runtime_context<
  default_key_t, default_version_t, std::shared_ptr, std::weak_ptr> thread_runtime;

} // end namespace backend

} // end namespace dharma_runtime




#endif /* NEW_RUNTIME_H_ */
