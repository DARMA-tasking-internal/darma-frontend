/*
//@HEADER
// ************************************************************************
//
//                      create_work_while_functor.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_CREATE_WORK_WHILE_FUNCTOR_H
#define DARMAFRONTEND_CREATE_WORK_WHILE_FUNCTOR_H

#include <tinympl/vector.hpp>

#include <darma/impl/meta/detection.h>

#include "create_work_while_fwd.h"
#include <darma/impl/create_if_then.h>

#include <darma/impl/access_handle_base.h>

#include <darma/impl/util/not_a_type.h>

namespace darma_runtime {
namespace detail {

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_while_do_helper, functor version"> {{{2

template <
  typename WhileHelper,
  typename Functor,
  typename LastArg,
  typename... Args
>
struct _create_work_while_do_helper<
  WhileHelper,
  Functor,
  tinympl::vector<Args...>,
  LastArg
>
{
  using functor_traits_t = functor_call_traits<Functor, Args..., LastArg>;
  using args_tuple_t = typename functor_traits_t::args_tuple_t;
  using args_fwd_tuple_t = std::tuple<Args&& ..., LastArg&&>;
  //using task_option_args_tuple_t = decltype(
  //  std::forward_as_tuple(std::declval<Args&&>()...)
  //);
  using task_option_args_tuple_t = std::tuple<>; // for now

  _not_a_lambda do_lambda;
  args_fwd_tuple_t args_fwd_tup;
  task_option_args_tuple_t task_option_args_tup;
  WhileHelper while_helper;

  static constexpr auto has_lambda_callable = false;

  _create_work_while_do_helper(_create_work_while_do_helper&&) = default;
  _create_work_while_do_helper(_create_work_while_do_helper const&) = delete;

  _create_work_while_do_helper(
    WhileHelper&& while_helper_in,
    Args&& ... args,
    LastArg&& last_arg
  ) : while_helper(std::move(while_helper_in)),
      args_fwd_tup(std::forward<Args>(args)...,
        std::forward<LastArg>(last_arg)
      ) {}

  ~_create_work_while_do_helper()
  {
    auto while_do_task = std::make_unique<
      WhileDoTask<
        typename WhileHelper::callable_t, typename WhileHelper::args_tuple_t,
        WhileHelper::has_lambda_callable,
        Functor, args_tuple_t, has_lambda_callable
      >
    >(
      std::move(*this)
    );

    abstract::backend::get_backend_runtime()->register_task(
      std::move(while_do_task)
    );
  }
};

// </editor-fold> end create_work_while_do_helper, functor version }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_while_helper, functor version"> {{{2

template <typename Functor, typename LastArg, typename... Args>
struct _create_work_while_helper<
  Functor,
  tinympl::vector<Args...>,
  LastArg
>
{

  using callable_t = Functor;
  using functor_traits_t = functor_call_traits<Functor, Args..., LastArg>;
  using args_tuple_t = typename functor_traits_t::args_tuple_t;
  using args_fwd_tuple_t = std::tuple<Args&& ..., LastArg&&>;
  using task_option_args_tuple_t = std::tuple<>; // for now
  //using task_option_args_tuple_t = decltype(
  //  std::forward_as_tuple(std::declval<Args&&>()...)
  //);

  static constexpr auto has_lambda_callable = false;

  _not_a_lambda while_lambda;

  args_fwd_tuple_t args_fwd_tup;
  task_option_args_tuple_t task_option_args_tup;

  _create_work_while_helper(_create_work_while_helper&&) = default;
  _create_work_while_helper(_create_work_while_helper const&) = delete;

  _create_work_while_helper(
    Args&& ... args,
    LastArg&& last_arg
  ) : args_fwd_tup(
    std::forward<Args>(args)...,
    std::forward<LastArg>(last_arg)
  ) {}

  template <typename DoFunctor=meta::nonesuch, typename... DoArgs>
  auto
  do_(DoArgs&& ... args)&&
  {
    return _create_work_while_do_helper<
      _create_work_while_helper, DoFunctor,
      typename tinympl::vector<DoArgs...>::pop_back::type,
      typename tinympl::vector<DoArgs...>::back::type
    >(
      std::move(*this),
      std::forward<DoArgs>(args)...
    );
  }

};

// </editor-fold> end create_work_while_helper, functor version }}}2
//------------------------------------------------------------------------------

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_CREATE_WORK_WHILE_FUNCTOR_H
