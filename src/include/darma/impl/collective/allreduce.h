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

#include <tinympl/bool.hpp>

#include <darma/impl/keyword_arguments/macros.h>
#include <darma/impl/keyword_arguments/keyword_argument_name.h>
#include <darma/impl/keyword_arguments/check_allowed_kwargs.h>
#include <darma/impl/keyword_arguments/get_kwarg.h>
#include <darma/interface/frontend/collective_details.h>

#include <darma/impl/handle.h> // is_access_handle
#include <darma/impl/use.h> // HandleUse


#include "details.h"


DeclareDarmaTypeTransparentKeyword(collectives, input);
DeclareDarmaTypeTransparentKeyword(collectives, output);
DeclareDarmaTypeTransparentKeyword(collectives, in_out);
DeclareDarmaTypeTransparentKeyword(collectives, piece);
DeclareDarmaTypeTransparentKeyword(collectives, n_pieces);
DeclareDarmaTypeTransparentKeyword(collectives, tag);

namespace darma_runtime {


namespace detail {

struct op_not_given { };

struct argument_not_given { };

template <size_t pos, typename Tag, typename... Args>
decltype(auto)
_get_potentially_positional_handle_arg(
  std::true_type, /* has kwarg version */
  std::false_type, /* can't give both positional and kwarg versions */
  Args&&... args
) {
  return detail::get_typeless_kwarg<Tag>(std::forward<Args>(args)...);
}

template <size_t pos, typename Tag, typename... Args>
decltype(auto)
_get_potentially_positional_handle_arg(
  std::false_type /* no kwarg given */,
  std::true_type, /* 2 or more positional args given */
  Args&&... args
) {
  return std::get<pos>(
    detail::get_positional_arg_tuple(std::forward<Args>(args)...)
  );
}

template <size_t pos, typename Tag, typename... Args>
auto
_get_potentially_positional_handle_arg(
  std::false_type /* no kwarg given */,
  std::false_type, /* no positional given */
  Args&&... args
) {
  return argument_not_given{};
}

template <size_t pos, typename Tag, typename... Args>
auto
_get_potentially_positional_handle_arg(
  std::true_type /* kwarg given */,
  std::true_type, /* positional given */
  Args&&... args
) {
  // TODO better error message
  static_assert(not bool(pos+1) /* always false, shouldn't get here */,
    "invalid argument / keyword argument combination for allreduce"
  );
  return argument_not_given{};
}

struct all_reduce_impl {

  // TODO generalize the use/flow handling here to be used by other collectives

  template <
    typename Op,
    typename InputHandle,
    typename OutputHandle
  >
  static inline
  std::enable_if_t<
    is_access_handle<std::decay_t<InputHandle>>::value
      and is_access_handle<std::decay_t<OutputHandle>>::value
  >
  _do_allreduce(
    InputHandle&& input, OutputHandle&& output, argument_not_given,
    size_t piece, size_t n_pieces, types::key_t const& tag
  ) {
    DARMA_ASSERT_MESSAGE(
      input.current_use_->use.scheduling_permissions_ != HandleUse::None,
      "allreduce() called on handle that can't schedule at least read usage on "
        "data (most likely because it was already released"
    );
    DARMA_ASSERT_MESSAGE(
      output.current_use_->use.scheduling_permissions_ != HandleUse::None,
      "allreduce() called on handle that can't schedule at least read usage on "
        "data (most likely because it was already released"
    );
    // TODO !!!!! write this


    // This is a read capture of the InputHandle and a write capture of the
    // output handle

    // First do the read capture of the input handle

    abort();

  };

  template <
    typename Op,
    typename InOutHandle
  >
  static inline
  std::enable_if_t<
    is_access_handle<std::decay_t<InOutHandle>>::value
  >
  _do_allreduce(
    argument_not_given, argument_not_given, InOutHandle&& in_out,
    size_t piece, size_t n_pieces, types::key_t const& tag
  ) {
    DARMA_ASSERT_MESSAGE(
      in_out.current_use_->use.scheduling_permissions_ != HandleUse::None,
      "allreduce() called on handle that can't schedule at least read usage on "
      "data (most likely because it was already released"
    );
    DARMA_ASSERT_MESSAGE(
      in_out.current_use_->use.scheduling_permissions_ == HandleUse::Permissions::Modify,
      "Can't do an allreduce capture of a handle as in_out without Modify"
      " scheduling permissions"
    );

    auto* backend_runtime = abstract::backend::get_backend_runtime();

    SimpleCollectiveDetails<
      std::conditional_t<
        std::is_same<Op, op_not_given>::value,
        typename darma_runtime::default_reduce_op<
          typename std::decay_t<InOutHandle>::value_type
        >::type,
        Op
      >
    > details(piece, n_pieces);

    // This is a mod capture.  Need special behavior if we have modify
    // immediate permissions (i.e., forwarding)

    switch (in_out.current_use_->use.immediate_permissions_) {
      case HandleUse::None:
      case HandleUse::Read: {
        // Make a next flow to hold the changes
        auto captured_out_flow = backend_runtime->make_next_flow(
          in_out.current_use_->use.in_flow_
        );

        // Make the use to be given to the collective
        HandleUse collective_use(
          in_out.var_handle_.get(),
          in_out.current_use_->use.in_flow_,
          captured_out_flow,
          HandleUse::None,
          HandleUse::Modify
        );
        backend_runtime->register_use(&collective_use);

        // The input flow of the handle in the continuing context is the
        // output flow of the captured use
        in_out.current_use_->use.in_flow_ = captured_out_flow;


        // Call the allreduce
        backend_runtime->allreduce_use(
          &collective_use,
          &collective_use,
          &details, tag
        );

        // Release the use
        backend_runtime->release_use(&collective_use);

        break;
      }
      case HandleUse::Modify: {

        auto collective_in_flow = backend_runtime->make_forwarding_flow(
          in_out.current_use_->use.in_flow_
        );
        auto collective_out_flow = backend_runtime->make_next_flow(
          collective_in_flow
        );

        HandleUse collective_use(
          in_out.var_handle_.get(),
          collective_in_flow, collective_out_flow,
          HandleUse::None, HandleUse::Modify
        );

        backend_runtime->register_use(&collective_use);

        in_out.current_use_->do_release();

        // Call the allreduce
        backend_runtime->allreduce_use(
          &collective_use,
          &collective_use,
          &details, tag
        );

        in_out.current_use_->use.in_flow_ = collective_out_flow;
        // Should be current state (asserted above)
        // in_out.current_use_->use.scheduling_permissions_ = HandleUse::Modify;
        in_out.current_use_->use.immediate_permissions_ = HandleUse::None;
        // out flow remains unchanged

        in_out.current_use_->could_be_alias = true;

        backend_runtime->release_use(&collective_use);

        break;
      }
      default:
        DARMA_ASSERT_NOT_IMPLEMENTED("other handle states for allreduce");
    } // end switch on immediate permissions

  };

};

} // end namespace detail


// TODO generalize the argument parsing here

template <typename Op = detail::op_not_given, typename... KWArgs>
void allreduce(
  KWArgs&&... kwargs
) {
  static_assert(detail::only_allowed_kwargs_given<
      darma_runtime::keyword_tags_for_collectives::input,
      darma_runtime::keyword_tags_for_collectives::output,
      darma_runtime::keyword_tags_for_collectives::in_out,
      darma_runtime::keyword_tags_for_collectives::piece,
      darma_runtime::keyword_tags_for_collectives::n_pieces,
      darma_runtime::keyword_tags_for_collectives::tag
    >::template apply<KWArgs...>::type::value,
    "Unknown keyword argument given to create_work()"
  );

  decltype(auto) positional_args = detail::get_positional_arg_tuple(
    std::forward<KWArgs>(kwargs)...
  );

  // Keyword argument only:
  size_t piece = detail::get_typeless_kwarg_with_default_as<
    darma_runtime::keyword_tags_for_collectives::piece, size_t
  >(
    abstract::frontend::CollectiveDetails::unknown_contribution(),
    std::forward<KWArgs>(kwargs)...
  );

  // Keyword argument only:
  size_t n_pieces = detail::get_typeless_kwarg_with_default_as<
    darma_runtime::keyword_tags_for_collectives::n_pieces, size_t
  >(
    abstract::frontend::CollectiveDetails::unknown_contribution(),
    std::forward<KWArgs>(kwargs)...
  );

  // Keyword argument only:
  auto tag = detail::get_typeless_kwarg_with_converter_and_default<
    darma_runtime::keyword_tags_for_collectives::tag
  >([](auto&&... key_parts) {
      return darma_runtime::make_key(
        std::forward<decltype(key_parts)>(key_parts)...
      );
    },
    types::key_t(),
    std::forward<KWArgs>(kwargs)...
  );

  using n_positional_args_t = typename detail::n_positional_args<KWArgs...>::type;

  // forms to handle: (not counting piece and n_pieces, which are kwarg-only)
  // allreduce(input_arg, output_arg);
  // allreduce(input_arg, output=output_arg);
  // allreduce(input=input_arg, output=output_arg);
  // allreduce(inout_arg);
  // allreduce(in_out=inout_arg)

  using tinympl::bool_;

  decltype(auto) output_handle = detail::_get_potentially_positional_handle_arg<
    1, darma_runtime::keyword_tags_for_collectives::output
  >(
    detail::has_kwarg<
      darma_runtime::keyword_tags_for_collectives::output, KWArgs...
    >(),
    bool_< n_positional_args_t::value >= 2 >(),
    std::forward<KWArgs>(kwargs)...
  );

  decltype(auto) input_handle = detail::_get_potentially_positional_handle_arg<
    0, darma_runtime::keyword_tags_for_collectives::input
  >(
    detail::has_kwarg<
      darma_runtime::keyword_tags_for_collectives::input, KWArgs...
    >(),
    bool_<
      n_positional_args_t::value >= 2
      or (
        n_positional_args_t::value >= 1
        and detail::has_kwarg<
          darma_runtime::keyword_tags_for_collectives::output, KWArgs...
        >::value
      )
    >(),
    std::forward<KWArgs>(kwargs)...
  );

  decltype(auto) in_out_handle = detail::_get_potentially_positional_handle_arg<
    0, darma_runtime::keyword_tags_for_collectives::in_out
  >(
    detail::has_kwarg<
      darma_runtime::keyword_tags_for_collectives::in_out, KWArgs...
    >(),
    bool_<
      n_positional_args_t::value >= 1
    >(),
    std::forward<KWArgs>(kwargs)...
  );

  detail::all_reduce_impl::_do_allreduce<Op>(
    std::forward<decltype(input_handle)>(input_handle),
    std::forward<decltype(output_handle)>(output_handle),
    std::forward<decltype(in_out_handle)>(in_out_handle),
    piece, n_pieces, tag
  );


};



} // end namespace darma_runtime

#endif //DARMA_IMPL_COLLECTIVE_ALLREDUCE_H
