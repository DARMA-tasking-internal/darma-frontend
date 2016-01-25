/*
//@HEADER
// ************************************************************************
//
//                          runtime.h
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

#ifndef SRC_ABSTRACT_BACKEND_RUNTIME_H_
#define SRC_ABSTRACT_BACKEND_RUNTIME_H_

#include "../frontend/dependency_handle.h"
#include "../frontend/task.h"
#include "../frontend/containment_manager.h"
#include "../frontend/aliasing_manager.h"

namespace dharma_runtime {

namespace abstract {

namespace backend {

template <
  typename Key,
  typename Version,
  template <typename...> class smart_ptr_template = std::shared_ptr
>
class Runtime {

  public:

    typedef abstract::frontend::DependencyHandle<Key, Version> handle_t;
    typedef abstract::frontend::ContainmentManager<Key, Version> containment_manager_t;
    typedef abstract::frontend::AliasingManager<Key, Version> aliasing_manager_t;
    typedef abstract::frontend::Task task_t;
    typedef smart_ptr_template<task_t> task_ptr;
    typedef smart_ptr_template<const task_t> task_const_ptr;
    typedef smart_ptr_template<handle_t> handle_ptr;
    typedef smart_ptr_template<const handle_t> handle_const_ptr;

    virtual void
    register_task(
      const task_ptr& task,
      const task_ptr& parent
    ) =0;

    // Methods for creating handles and registering fetches of those handles

    virtual void
    register_handle(
      const handle_const_ptr& new_handle,
      const handle_const_ptr& prev_data = nullptr,
      bool needs_read_prev_data = true
    ) =0;

    virtual void
    publish_handle(
      const handle_t* const,
      const Key& version_tag,
      const size_t n_additional_fetchers = 1
    ) =0;

    virtual handle_t
    resolve_version_tag(
      const Key& handle_key,
      const Key& version_tag
    ) =0;

    virtual void
    register_fetcher(
      const handle_t* const fetcher_handle
    ) =0;

    virtual void
    release_fetcher(
      const handle_t* const fetcher_handle
    ) =0;

    // Methods for "bare" dependency satisfaction and use.  Not used
    // for task dependencies

    virtual void
    fill_handle(
      handle_t* const to_fill,
      bool needs_write_access = false
    ) =0;

    // Methods for establishing containment and/or aliasing relationships

    virtual void
    establish_containment_relationship(
      const handle_t* const inner_handle,
      const handle_t* const outer_handle,
      containment_manager_t const& manager
    ) =0;

    virtual void
    establish_aliasing_relationship(
      const handle_t* const handle_a,
      const handle_t* const handle_b,
      aliasing_manager_t const& manager
    ) =0;

    // Virtual destructor, since we have virtual methods

    virtual ~Runtime() noexcept { }

};

} // end namespace backend

} // end namespace abstract

} // end namespace dharma_runtime




#endif /* SRC_ABSTRACT_BACKEND_RUNTIME_H_ */
