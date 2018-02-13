/*
//@HEADER
// ************************************************************************
//
//                      record_line_numbers.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_RECORD_LINE_NUMBERS_H
#define DARMAFRONTEND_RECORD_LINE_NUMBERS_H

#include <cstdint>

#include <darma/utility/config.h>
#include <darma/interface/app/create_work.h>

namespace darma_runtime {

#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS

struct _create_work_creation_context {
  const char* file;
  const char* func;
  size_t line;
  _create_work_creation_context(const char* file, size_t line, const char* func)
    : file(file), line(line), func(func)
  { }
  template <
    typename Functor=detail::_create_work_uses_lambda_tag,
    typename... Args
  >
  void
  _darma_create_work_with_line_numbers(Args&&... args);
  template <
    typename Functor
  >
  void
  _darma_create_work_with_line_numbers();
};

struct _create_work_while_creation_context : _create_work_creation_context {

  using _create_work_creation_context::_create_work_creation_context;

  template <
    typename Functor=tinympl::nonesuch,
    typename... Args
  >
  auto
  _darma_create_work_while_with_line_numbers(Args&&... args);
};

struct _create_work_if_creation_context : _create_work_creation_context {

  using _create_work_creation_context::_create_work_creation_context;

  template <
    typename Functor=tinympl::nonesuch,
    typename... Args
  >
  auto
  _darma_create_work_if_with_line_numbers(Args&&... args);
};

#define create_work _create_work_creation_context(__FILE__, __LINE__, __func__)._darma_create_work_with_line_numbers
#define create_work_while _create_work_while_creation_context(__FILE__, __LINE__, __func__)._darma_create_work_while_with_line_numbers
#define create_work_if _create_work_if_creation_context(__FILE__, __LINE__, __func__)._darma_create_work_if_with_line_numbers
#endif

} // end namespace darma_runtime

#endif //DARMAFRONTEND_RECORD_LINE_NUMBERS_H
