/*
//@HEADER
// ************************************************************************
//
//                          create_work.h
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

#ifndef SRC_INCLUDE_DARMA_INTERFACE_APP_CREATE_WORK_H_
#define SRC_INCLUDE_DARMA_INTERFACE_APP_CREATE_WORK_H_

#include <darma/impl/create_work.h>
#include <darma/impl/keyword_arguments/check_allowed_kwargs.h>

namespace darma_runtime {

template <typename... Args>
typename detail::create_work_parser<Args...>::return_type
create_work(Args&&... args) {
  static_assert(detail::only_allowed_kwargs_given<
    >::template apply<Args...>::type::value,
    "Unknown keyword argument given to create_work()"
  );

  namespace m = tinympl;
  // Pop off the last type and move it to the front
  typedef typename m::vector<Args...>::back::type lambda_t;
  typedef typename m::vector<Args...>::pop_back::type rest_vector_t;
  typedef typename m::splat_to<
    typename rest_vector_t::template push_front<lambda_t>::type, detail::create_work_impl
  >::type helper_t;
  namespace m = tinympl;
  namespace mp = tinympl::placeholders;

  detail::TaskBase* parent_task = dynamic_cast<detail::TaskBase* const>(
    detail::backend_runtime->get_running_task()
  );


  meta::tuple_for_each_filtered_type<
    m::lambda<std::is_same<std::decay<mp::_>, detail::reads_decorator_return>>::template apply
  >(std::forward_as_tuple(std::forward<Args>(args)...), [&](auto&& rdec){
    if(rdec.use_it) {
      for(auto&& h : rdec.handles) {
        parent_task->read_only_handles.emplace(h);
      }
    }
    else {
      for(auto&& h : rdec.handles) {
        parent_task->ignored_handles.emplace(h.get());
      }
    }
  });

  // TODO waits() decorator
  //meta::tuple_for_each_filtered_type<
  //  m::lambda<std::is_same<std::decay<mp::_>, detail::waits_decorator_return>>::template apply
  //>(std::forward_as_tuple(std::forward<Args>(args)...), [&](auto&& rdec){
  //  parent_task->waits_handles.emplace(rdec.handle);
  //});


  return helper_t()(std::forward<Args>(args)...);
}

}


#endif /* SRC_INCLUDE_DARMA_INTERFACE_APP_CREATE_WORK_H_ */
