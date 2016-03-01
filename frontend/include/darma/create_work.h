/*
//@HEADER
// ************************************************************************
//
//                          create_work.h
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

#ifndef SRC_CREATE_WORK_H_
#define SRC_CREATE_WORK_H_

#include <memory>

#include <tinympl/variadic/back.hpp>
#include <tinympl/vector.hpp>
#include <tinympl/splat.hpp>

#include "runtime.h"

#include "task.h"

namespace darma_runtime {

namespace detail {

namespace mv = tinympl::variadic;

template <typename... Args>
struct create_work_parser {
  typedef /* TODO ??? */ void return_type;
  typedef typename mv::back<Args...>::type lambda_type;
};

template <typename... Args>
struct reads_decorator_parser {
  typedef /* TODO */ int return_type;
};

template <typename... Args>
struct waits_decorator_parser {
  typedef /* TODO */ int return_type;
};

template <typename... Args>
struct writes_decorator_parser {
  typedef /* TODO */ int return_type;
};

template <typename... Args>
struct reads_writes_decorator_parser {
  typedef /* TODO */ int return_type;
};


template <typename Lambda, typename... Args>
struct create_work_impl {
  typedef create_work_parser<Args..., Lambda> parser;
  typedef abstract::backend::runtime_t runtime_t;
  typedef runtime_t::key_t key_t;
  typedef runtime_t::version_t version_t;
  typedef runtime_t::task_t abstract_task_t;
  typedef detail::Task<Lambda> task_t;
  typedef detail::TaskBase task_base_t;

  inline typename parser::return_type
  operator()(Args&&... args, Lambda&& lambda) const && {
    return detail::backend_runtime->register_task(
      std::make_unique<task_t>(
        std::forward<Lambda>(lambda)
      )
    );
  }
};


} // end namespace detail


template <typename... Args>
typename detail::reads_decorator_parser<Args...>::return_type
reads(Args&&... args) {
  // TODO implement this
  return typename detail::reads_decorator_parser<Args...>::return_type();
}

template <typename... Args>
typename detail::waits_decorator_parser<Args...>::return_type
waits(Args&&... args) {
  // TODO implement this
  return typename detail::waits_decorator_parser<Args...>::return_type();
}

template <typename... Args>
typename detail::writes_decorator_parser<Args...>::return_type
writes(Args&&... args) {
  // TODO implement this
  return typename detail::writes_decorator_parser<Args...>::return_type();
}

template <typename... Args>
typename detail::reads_writes_decorator_parser<Args...>::return_type
reads_writes(Args&&... args) {
  // TODO implement this
  return typename detail::reads_writes_decorator_parser<Args...>::return_type();
}

template <typename... Args>
typename detail::create_work_parser<Args...>::return_type
create_work(Args&&... args) {
  namespace m = tinympl;
  // Pop off the last type and move it to the front
  typedef typename m::vector<Args...>::back::type lambda_t;
  typedef typename m::vector<Args...>::pop_back::type rest_vector_t;
  typedef typename m::splat_to<
    typename rest_vector_t::template push_front<lambda_t>::type, detail::create_work_impl
  >::type helper_t;

  return helper_t()(std::forward<Args>(args)...);
}


} // end namespace darma_runtime



#endif /* SRC_CREATE_WORK_H_ */
