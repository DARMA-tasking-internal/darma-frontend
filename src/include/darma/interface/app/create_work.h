/*
//@HEADER
// ************************************************************************
//
//                          create_work.h
//                         dharma_new
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

#ifndef SRC_INCLUDE_DARMA_INTERFACE_APP_CREATE_WORK_H_
#define SRC_INCLUDE_DARMA_INTERFACE_APP_CREATE_WORK_H_

#include <darma/utility/config.h>

namespace darma {

namespace detail {

struct _create_work_uses_lambda_tag { };
} // end namespace detail

// TODO create_work with return value (Issue #107 on GitLab)

#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
struct _create_work_creation_context;
#else
template <
  typename Functor=detail::_create_work_uses_lambda_tag,
  typename... Args
>
void create_work(Args&&... args);
#endif

} // end namespace darma


#include <darma/impl/create_work/create_work.h>
//#include <darma/impl/util.h>


//#define create_work \
//  auto DARMA_CONCAT_TOKEN_(_DARMA__started_, __LINE__) = \
//    ::darma::detail::_start_create_work(); \
//    ::darma::detail::_do_create_work( \
//      std::move(DARMA_CONCAT_TOKEN_(_DARMA__started_, __LINE__)) \
//    ).operator()


#endif /* SRC_INCLUDE_DARMA_INTERFACE_APP_CREATE_WORK_H_ */
