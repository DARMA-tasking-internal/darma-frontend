/*
//@HEADER
// ************************************************************************
//
//                      create_work_if.h
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

#ifndef DARMAFRONTEND_CREATE_WORK_IF_H
#define DARMAFRONTEND_CREATE_WORK_IF_H

#include <tinympl/vector.hpp>

#include <darma/impl/config.h>

#include <darma/impl/meta/detection.h>

#include <darma/impl/create_work/create_if_then_fwd.h>

#include <darma/impl/create_work/record_line_numbers.h>

namespace darma_runtime {

template <
  typename Functor
#if !DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
  =meta::nonesuch
#endif
  , typename... Args
>
auto
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
_create_work_if_creation_context::_darma_create_work_if_with_line_numbers(
#else
create_work_if(
#endif
  Args&& ... args
) {
  return detail::_create_work_if_helper<
    Functor,
    typename tinympl::vector<Args...>::safe_pop_back::type,
    typename tinympl::vector<Args...>::template safe_back<meta::nonesuch>::type
  >(
#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
    std::move(*this),
#endif
    std::forward<Args>(args)...
  );
}

} // end namespace darma_runtime

#include <darma/impl/create_work/create_if_then.h>

#endif //DARMAFRONTEND_CREATE_WORK_IF_H
