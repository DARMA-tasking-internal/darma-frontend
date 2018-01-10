/*
//@HEADER
// ************************************************************************
//
//                      create_work_argument_parser.h
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

#ifndef DARMAFRONTEND_CREATE_WORK_ARGUMENT_PARSER_H
#define DARMAFRONTEND_CREATE_WORK_ARGUMENT_PARSER_H

#include <darma/impl/create_work/create_work_fwd.h>

#include <darma/keyword_arguments/macros.h>
#include <darma/keyword_arguments/parse.h>

#include <darma/impl/task/task_base.h>

#include <darma/interface/app/keyword_arguments/name.h>

#include <darma/interface/backend/types.h>

// TODO move this once it becomes part of the specified interface
DeclareDarmaTypeTransparentKeyword(task_creation, allow_aliasing);
DeclareDarmaTypeTransparentKeyword(task_creation, is_parallel);

namespace darma_runtime {
namespace detail {

using create_work_argument_parser = darma_runtime::detail::kwarg_parser<
  variadic_positional_overload_description<
    _optional_keyword<
      converted_parameter, keyword_tags_for_task_creation::name
    >,
    _optional_keyword<
      converted_parameter, keyword_tags_for_task_creation::allow_aliasing
    >,
    _optional_keyword<
      bool, keyword_tags_for_task_creation::is_parallel
    >
  >
>;

inline auto setup_create_work_argument_parser() {
  return create_work_argument_parser()
    .with_converters(
      [](auto&& ... parts) {
        return darma_runtime::make_key(std::forward<decltype(parts)>(parts)...);
      },
      [](auto&&... aliasing_desc) {
        return std::make_unique<darma_runtime::detail::TaskBase::allowed_aliasing_description>(
          darma_runtime::detail::allowed_aliasing_description_ctor_tag_t(),
          std::forward<decltype(aliasing_desc)>(aliasing_desc)...
        );
      }
    )
    .with_default_generators(
      darma_runtime::keyword_arguments_for_task_creation::name = [] {
        // TODO this should actually return a "pending backend assignment" key
        return darma_runtime::make_key();
      },
      keyword_arguments_for_task_creation::allow_aliasing = [] {
        return std::make_unique<darma_runtime::detail::TaskBase::allowed_aliasing_description>(
          darma_runtime::detail::allowed_aliasing_description_ctor_tag_t(),
          false
        );
      },
      keyword_arguments_for_task_creation::is_parallel = [] { return false; }
    );
}

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_CREATE_WORK_ARGUMENT_PARSER_H
