/*
//@HEADER
// ************************************************************************
//
//                      allreduce.h
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

#ifndef DARMA_IMPL_COLLECTIVE_ALLREDUCE_H
#define DARMA_IMPL_COLLECTIVE_ALLREDUCE_H

#include <darma_types.h>

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(simple_collectives)

#include <tinympl/bool.hpp>

#include <darma/impl/collective/collective_fwd.h>

#include <darma/impl/keyword_arguments/macros.h>
#include <darma/impl/keyword_arguments/keyword_argument_name.h>
#include <darma/impl/keyword_arguments/check_allowed_kwargs.h>
#include <darma/impl/keyword_arguments/get_kwarg.h>
#include <darma/impl/keyword_arguments/parse.h>
#include <darma/interface/frontend/collective_details.h>

#include <darma/impl/handle.h> // is_access_handle
#include <darma/impl/use.h> // HandleUse
#include <darma/impl/capture.h> // make_captured_use_holder
#include <darma/impl/task/task.h> // TaskBase


#include "details.h"



namespace darma_runtime {


namespace detail {

// Umm... this will probably need to change if the value types of InHandle and
// OutHandle differ...
template <typename Op, typename InputHandle, typename OutputHandle>
using _get_collective_details_t = SimpleCollectiveDetails<
  std::conditional_t<
    std::is_same<Op, op_not_given>::value,
    typename darma_runtime::default_reduce_op<
      typename std::decay_t<OutputHandle>::value_type
    >::type,
    Op
  >,
  typename std::decay_t<OutputHandle>::value_type
>;


template<typename Op>
struct all_reduce_impl {

  size_t piece_ = abstract::frontend::CollectiveDetails::unknown_contribution();
  size_t n_pieces_ = abstract::frontend::CollectiveDetails::unknown_contribution();
  #if _darma_has_feature(task_collection_token)
  types::task_collection_token_t token_;
  #endif // _darma_has_feature(task_collection_token)

  all_reduce_impl()
  {
    #if _darma_has_feature(task_collection_token)
    auto* running_task =
      detail::safe_static_cast<darma_runtime::detail::TaskBase*>(
        abstract::backend::get_backend_context()->get_running_task()
      );
    if(running_task->parent_token_available) {
      token_ = running_task->token_;
    }
    #endif
  }

  all_reduce_impl(
    size_t piece, size_t n_pieces
    #if _darma_has_feature(task_collection_token)
    , types::task_collection_token_t token
    #endif // _darma_has_feature(task_collection_token)
  ) : piece_(piece), n_pieces_(n_pieces)
      #if _darma_has_feature(task_collection_token)
      , token_(token)
      #endif // _darma_has_feature(task_collection_token)
  { }



  template <
    typename InputHandle,
    typename OutputHandle
  >
  inline
  std::enable_if_t<
    is_access_handle<std::decay_t<InputHandle>>::value
      and is_access_handle<std::decay_t<OutputHandle>>::value
  >
  operator()(
    InputHandle&& input, OutputHandle&& output, types::key_t const& tag,
    size_t piece = abstract::frontend::CollectiveDetails::unknown_contribution(),
    size_t n_pieces = abstract::frontend::CollectiveDetails::unknown_contribution()
  ) const {

    DARMA_ASSERT_MESSAGE(
      input.get_current_use()->use()->scheduling_permissions_ != frontend::Permissions::None,
      "allreduce() called on handle that can't schedule at least Read usage on "
        "data"
    );
    DARMA_ASSERT_MESSAGE(
      output.get_current_use()->use()->scheduling_permissions_ != frontend::Permissions::None
      and output.get_current_use()->use()->scheduling_permissions_ != frontend::Permissions::Read,
      "allreduce() called on handle that can't schedule at least Write usage on "
        "data"
    );
    // TODO disallow same handle

    if(piece == abstract::frontend::CollectiveDetails::unknown_contribution()) {
      piece = piece_;
    }
    if(n_pieces == abstract::frontend::CollectiveDetails::unknown_contribution()) {
      n_pieces = n_pieces_;
    }

    auto* backend_runtime = abstract::backend::get_backend_runtime();

    // This is a read capture of the InputHandle and a write capture of the
    // output handle

    auto input_use_holder = detail::make_captured_use_holder(
      input.var_handle_,
      /* requested_scheduling_permissions */
      frontend::Permissions::None,
      /* requested_immediate_permissions */
      frontend::Permissions::Read,
      input.get_current_use()
    );

    auto output_use_holder = detail::make_captured_use_holder(
      output.var_handle_,
      /* requested_scheduling_permissions */
      frontend::Permissions::None,
      /* requested_immediate_permissions */
      // TODO change this to Write once that is implemented
      frontend::Permissions::Modify,
      output.get_current_use()
    );

    _get_collective_details_t<
      Op, std::decay_t<InputHandle>, std::decay_t<OutputHandle>
    > details(piece, n_pieces
        #if _darma_has_feature(task_collection_token)
        , token_
        #endif // _darma_has_feature(task_collection_token)
    );

    backend_runtime->allreduce_use(
      input_use_holder->relinquish_into_destructible_use(),
      output_use_holder->relinquish_into_destructible_use(),
      &details, tag
    );

  }

  //============================================================================

  template <
    typename InOutHandle
  >
  inline
  std::enable_if_t<
    is_access_handle<std::decay_t<InOutHandle>>::value
  >
  operator()(
    InOutHandle&& in_out, types::key_t const& tag,
    size_t piece = abstract::frontend::CollectiveDetails::unknown_contribution(),
    size_t n_pieces = abstract::frontend::CollectiveDetails::unknown_contribution()
  ) const {

    DARMA_ASSERT_MESSAGE(
      in_out.get_current_use()->use()->scheduling_permissions_ != frontend::Permissions::None,
      "allreduce() called on handle that can't schedule at least read usage on "
      "data (most likely because it was already released"
    );
    DARMA_ASSERT_MESSAGE(
      in_out.get_current_use()->use()->scheduling_permissions_ == frontend::Permissions::Modify,
      "Can't do an allreduce capture of a handle as in_out without Modify"
      " scheduling permissions"
    );

    if(piece == abstract::frontend::CollectiveDetails::unknown_contribution()) {
      piece = piece_;
    }
    if(n_pieces == abstract::frontend::CollectiveDetails::unknown_contribution()) {
      n_pieces = n_pieces_;
    }

    auto* backend_runtime = abstract::backend::get_backend_runtime();

    _get_collective_details_t<
      Op, std::decay_t<InOutHandle>, std::decay_t<InOutHandle>
    > details(piece, n_pieces
        #if _darma_has_feature(task_collection_token)
        , token_
        #endif // _darma_has_feature(task_collection_token)
    );

    // This is a mod capture.  Need special behavior if we have modify
    // immediate permissions (i.e., forwarding)

    auto collective_use_holder = detail::make_captured_use_holder(
      in_out.var_handle_base_,
      /* requested_scheduling_permissions */
      frontend::Permissions::None,
      /* requested_immediate_permissions */
      frontend::Permissions::Modify,
      in_out.get_current_use()
    );

    backend_runtime->allreduce_use(
      // Transfer ownership
      collective_use_holder->relinquish_into_destructible_use(),
      &details, tag
    );
  }

};

} // end namespace detail


template <typename Op = detail::op_not_given, typename... KWArgs>
void allreduce(
  KWArgs&&... kwargs
) {

  using namespace darma_runtime::detail;
  using parser = detail::kwarg_parser<
    overload_description<
      _positional_or_keyword<deduced_parameter, keyword_tags_for_collectives::input>,
      _positional_or_keyword<deduced_parameter, keyword_tags_for_collectives::output>,
      _optional_keyword<converted_parameter, keyword_tags_for_collectives::tag>,
      _optional_keyword<size_t, keyword_tags_for_collectives::piece>,
      _optional_keyword<size_t, keyword_tags_for_collectives::n_pieces>
    >,
    overload_description<
      _positional_or_keyword<deduced_parameter, keyword_tags_for_collectives::in_out>,
      _optional_keyword<converted_parameter, keyword_tags_for_collectives::tag>,
      _optional_keyword<size_t, keyword_tags_for_collectives::piece>,
      _optional_keyword<size_t, keyword_tags_for_collectives::n_pieces>
    >
  >;

  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<KWArgs...>;

  parser()
    .with_default_generators(
      keyword_arguments_for_collectives::tag=[]{ return make_key(); },
      keyword_arguments_for_collectives::piece=[]{
        return abstract::frontend::CollectiveDetails::unknown_contribution();
      },
      keyword_arguments_for_collectives::n_pieces=[] {
        return abstract::frontend::CollectiveDetails::unknown_contribution();
      }
    )
    .with_converters(
      [](auto&&... key_parts) {
        return make_key(std::forward<decltype(key_parts)>(key_parts)...);
      }
    )
    .parse_args(
      std::forward<KWArgs>(kwargs)...
    )
    .invoke(detail::all_reduce_impl<Op>());

};

} // end namespace darma_runtime

#endif // _darma_has_feature(simple_collectives)

#endif //DARMA_IMPL_COLLECTIVE_ALLREDUCE_H
