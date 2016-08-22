/*
//@HEADER
// ************************************************************************
//
//                      create_condition.h
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

#ifndef DARMA_IMPL_CREATE_CONDITION_H
#define DARMA_IMPL_CREATE_CONDITION_H

#include <tinympl/vector.hpp>

#include <darma/impl/util/smart_pointers.h>

namespace darma_runtime {

namespace detail {

template <typename Lambda, typename... Args>
struct create_condition_impl {
  inline bool
  operator()(Args&&... args, Lambda&& lambda) const {
    // TODO set default capture to read

    auto task = detail::make_unique<TaskBase>(std::forward<Lambda>(lambda));

    for(auto&& reg : task->registrations_to_run) {
      reg();
    }
    task->registrations_to_run.clear();

    for(auto&& post_reg_op : task->post_registration_ops) {
      post_reg_op();
    }
    task->post_registration_ops.clear();

    return abstract::backend::get_backend_runtime()->register_condition_task(
      std::move(task)
    );

  }
};

} // end namespace detail


template <typename... Args>
bool
create_condition(Args&&... args) {
  namespace m = tinympl;
  // Pop off the last type and move it to the front
  typedef typename m::vector<Args...>::back::type lambda_t;
  typedef typename m::vector<Args...>::pop_back::type rest_vector_t;
  typedef typename m::splat_to<
    typename rest_vector_t::template push_front<lambda_t>::type, detail::create_condition_impl
  >::type helper_t;

  static_assert(std::is_convertible<decltype(std::declval<lambda_t>()()), bool>::value,
    "Lambda given to create_condition() must return a value convertible to bool"
  );

  return helper_t()(
    std::forward<Args>(args)...
  );
}

} // end namespace darma_runtime

#endif //DARMA_IMPL_CREATE_CONDITION_H
