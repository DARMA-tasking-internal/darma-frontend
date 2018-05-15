/*
//@HEADER
// ************************************************************************
//
//                          create_work.h
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

#ifndef SRC_CREATE_WORK_H_
#define SRC_CREATE_WORK_H_

#include <memory>

#include <darma/utility/config.h>
#include <darma/impl/create_work/create_work_fwd.h>

#include <darma/impl/create_work/permissions_downgrades.h>
#include <darma/impl/create_work/create_work_argument_parser.h>
#include <darma/impl/create_work/create_work_lambda.h>
#include <darma/impl/create_work/create_work_functor.h>

#include <tinympl/variadic/at.hpp>
#include <tinympl/variadic/back.hpp>
#include <tinympl/vector.hpp>
#include <tinympl/splat.hpp>
#include <tinympl/lambda.hpp>

#include <darma/impl/meta/tuple_for_each.h>
#include <darma/impl/meta/splat_tuple.h>

#include <darma/interface/app/create_work.h>

#include <darma/impl/handle_attorneys.h>
#include <darma/interface/app/access_handle.h>
#include <darma/impl/runtime.h>
#include <darma/impl/task/task.h>
#include <darma/impl/util.h>

#include <darma/impl/meta/make_flattened_tuple.h>


namespace darma {


template <
  typename Functor, /*=detail::_create_work_uses_lambda_tag */
  typename... Args
>
void
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
_create_work_creation_context::_darma_create_work_with_line_numbers
#else
create_work
#endif
  (Args&&... args) {

  return detail::_create_work_impl<
    Functor,
    typename tinympl::vector<Args...>::pop_back::type,
    typename tinympl::vector<Args...>::back::type
  >(
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    this
#endif
  )(
    std::forward<Args>(args)...
  );

}

template <typename Functor>
void
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
_create_work_creation_context::_darma_create_work_with_line_numbers
#else
create_work
#endif
() {
  return detail::_create_work_impl<
    Functor,
    typename tinympl::vector<>,
    tinympl::nonesuch
  >(
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    this
#endif
  )._impl();
}

//template <typename... Args>
//auto
//reads(Args&&... args) {
//  auto make_read_only = [](auto&& arg) {
//    return detail::make_permissions_downgrade_description<
//      detail::AccessHandlePermissions::Read,
//      detail::AccessHandlePermissions::Read
//    >(std::forward<decltype(arg)>(arg));
//  };
//  return meta::make_flattened_tuple(
//    make_read_only(std::forward<Args>(args))...
//  );
//}

//template <typename... Args>
//decltype(auto)
//schedule_only(Args&&... args) {
//  auto make_schedule_only = [](auto&& arg) {
//    return detail::make_permissions_downgrade_description<
//      detail::AccessHandlePermissions::NotGiven,
//      detail::AccessHandlePermissions::None
//    >(std::forward<decltype(arg)>(arg));
//  };
//  return meta::make_flattened_tuple(
//    make_schedule_only(std::forward<Args>(args))...
//  );
//}

} // end namespace darma


#endif /* SRC_CREATE_WORK_H_ */
