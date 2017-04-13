/*
//@HEADER
// ************************************************************************
//
//                      create_concurrent_work.h
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

#ifndef DARMA_IMPL_TASK_COLLECTION_CREATE_CONCURRENT_WORK_H
#define DARMA_IMPL_TASK_COLLECTION_CREATE_CONCURRENT_WORK_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(create_concurrent_work)
#include <darma/impl/task_collection/task_collection.h>

namespace darma_runtime {

template <typename Functor, typename... Args>
void create_concurrent_work(Args&&... args) {
  using namespace darma_runtime::detail;
  using darma_runtime::keyword_tags_for_create_concurrent_work::index_range;
  using parser = kwarg_parser<
  variadic_positional_overload_description<
    _keyword<deduced_parameter, index_range>
  >
  // TODO other overloads
  >;

  // This is on one line for readability of compiler error; don't respace it please!
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

  parser()
    .parse_args(std::forward<Args>(args)...)
    .invoke([](
      auto&& index_range,
      darma_runtime::detail::variadic_arguments_begin_tag,
      auto&&... args
    ){
      using task_collection_impl_t = typename detail::make_task_collection_impl_t<
        Functor, std::decay_t<decltype(index_range)>, decltype(args)...
      >::type;

      auto task_collection = std::make_unique<task_collection_impl_t>(
        std::forward<decltype(index_range)>(index_range),
        std::forward<decltype(args)>(args)...
      );

      auto* backend_runtime = abstract::backend::get_backend_runtime();
      backend_runtime->register_task_collection(
        std::move(task_collection)
      );

    });

}

} // end namespace darma_runtime
#endif // _darma_has_feature(create_concurrent_work)

#endif //DARMA_IMPL_TASK_COLLECTION_CREATE_CONCURRENT_WORK_H
