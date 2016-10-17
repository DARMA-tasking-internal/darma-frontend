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
#include <darma/impl/keyword_arguments/parse.h>
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

  // TODO generalize the use/flow handling here to be used by other collectives

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
    size_t piece, size_t n_pieces
  ) const {
    DARMA_ASSERT_MESSAGE(
      input.current_use_->use.scheduling_permissions_ != HandleUse::None,
      "allreduce() called on handle that can't schedule at least Read usage on "
        "data"
    );
    DARMA_ASSERT_MESSAGE(
      output.current_use_->use.scheduling_permissions_ != HandleUse::None
      and output.current_use_->use.scheduling_permissions_ != HandleUse::Read,
      "allreduce() called on handle that can't schedule at least Write usage on "
        "data"
    );
    // TODO disallow same handle

    // TODO !!!!! write this
    //DARMA_ASSERT_NOT_IMPLEMENTED("collectives with separate input and output");

    detail::HandleUse* input_use, *output_use;
    input_use = output_use = nullptr;
    bool release_input_use = false;
    bool release_output_use = false;
    auto* backend_runtime = abstract::backend::get_backend_runtime();

    // This is a read capture of the InputHandle and a write capture of the
    // output handle

    // First do the read capture of the input handle
    switch(input.current_use_->use.immediate_permissions_) {
      case HandleUse::None:
      case HandleUse::Read: {

        // Note that scheduling permissions assertion is above, so we don't
        // need to mess with it here

        input_use = new HandleUse(
          input.var_handle_,
          input.current_use_->use.in_flow_,
          input.current_use_->use.in_flow_,
          /* scheduling_permissions= */ HandleUse::None,
          /* immediate_permissions = */ HandleUse::Read
        );

        backend_runtime->register_use(input_use);

        break;
      }
      case HandleUse::Modify: {

        auto inflow = make_forwarding_flow_ptr(
          input.current_use_->use.in_flow_, backend_runtime
        );

        input_use = new HandleUse(
          input.var_handle_,
          inflow, inflow,
          /* scheduling_permissions= */ HandleUse::None,
          /* immediate_permissions = */ HandleUse::Read
        );

        backend_runtime->register_use(input_use);

        input.current_use_->use.immediate_permissions_ = HandleUse::Read;
        input.current_use_->use.in_flow_ = inflow;
        // current_use_->use.out_flow_ and scheduling_permissions_ unchanged
        input.current_use_->could_be_alias = true;

        break;
      }
      default:
        DARMA_ASSERT_NOT_IMPLEMENTED("Collectives with input handles and"
          " different immediate permissions");
    }

    // Now do the write capture of the output handle
    switch(output.current_use_->use.immediate_permissions_) {
      case HandleUse::None:
      case HandleUse::Read: {
        auto write_outflow = make_next_flow_ptr(
          output.current_use_->use.in_flow_, backend_runtime
        );
        // Note that scheduling permissions assertion is above, so we don't
        // need to mess with it here
        output_use = new HandleUse(
          output.var_handle_,
          output.current_use_->use.in_flow_,
          write_outflow,
          /* scheduling_permissions= */ HandleUse::None,
          /* immediate_permissions = */ HandleUse::Write
        );

        backend_runtime->register_use(output_use);

        if(output.current_use_->use.immediate_permissions_ == HandleUse::Read) {
          output.current_use_->do_release();
        }

        output.current_use_->use.in_flow_ = write_outflow;
        // output.current_use_->use.out_flow_ unchanged
        output.current_use_->use.immediate_permissions_ = HandleUse::None;
        output.current_use_->could_be_alias = true;

        break;
      }
      case HandleUse::Write: {
        DARMA_ASSERT_NOT_IMPLEMENTED("Write after write dependency handling"
          " is (perhaps permanently) unimplemented");
        break;
      }
      case HandleUse::Modify: {
        // This is also (kind of) a write-after-write.  We don't need a
        // forwarding flow because we don't need the data to be available

        // TODO doing this doesn't make a lot of sense, so it should probably be
        //      a warning in some "safe" mode or "strict" mode

        auto write_outflow = make_next_flow_ptr(
          output.current_use_->use.in_flow_, backend_runtime
        );

        output_use = new HandleUse(
          output.var_handle_,
          output.current_use_->use.in_flow_,
          write_outflow,
          /* scheduling_permissions= */ HandleUse::None,
          /* immediate_permissions = */ HandleUse::Write
        );

        output.current_use_->do_release();

        output.current_use_->use.immediate_permissions_ = HandleUse::None;
        // output.current_use_->use.scheduling_permissions_ unchanged
        output.current_use_->use.in_flow_ = write_outflow;

        backend_runtime->register_use(output_use);

        break;
      }
      default:
        DARMA_ASSERT_NOT_IMPLEMENTED("Collectives with output handles and"
          " different immediate permissions");
    } // end switch on output immediate permissions

    {
      _get_collective_details_t<
        Op, std::decay_t<InputHandle>, std::decay_t<OutputHandle>
      > details(piece, n_pieces);

      backend_runtime->allreduce_use(
        input_use, output_use, &details, tag
      );

    } // delete details object


    backend_runtime->release_use(input_use);
    delete input_use;
    backend_runtime->release_use(output_use);
    delete output_use;


  }

  template <
    typename InOutHandle
  >
  inline
  std::enable_if_t<
    is_access_handle<std::decay_t<InOutHandle>>::value
  >
  operator()(
    InOutHandle&& in_out, types::key_t const& tag,
    size_t piece, size_t n_pieces
  ) const {
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

    _get_collective_details_t<
      Op, std::decay_t<InOutHandle>, std::decay_t<InOutHandle>
    > details(piece, n_pieces);

    // This is a mod capture.  Need special behavior if we have modify
    // immediate permissions (i.e., forwarding)

    switch (in_out.current_use_->use.immediate_permissions_) {
      case HandleUse::None:
      case HandleUse::Read: {
        // Make a next flow to hold the changes
        auto captured_out_flow = make_next_flow_ptr(
          in_out.current_use_->use.in_flow_, backend_runtime
        );

        // Make the use to be given to the collective
        HandleUse collective_use(
          in_out.var_handle_,
          in_out.current_use_->use.in_flow_,
          captured_out_flow,
          /* scheduling_permissions= */ HandleUse::None,
          /* immediate_permissions = */ HandleUse::Modify
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

        auto collective_in_flow = make_forwarding_flow_ptr(
          in_out.current_use_->use.in_flow_, backend_runtime
        );
        auto collective_out_flow = make_next_flow_ptr(
          collective_in_flow, backend_runtime
        );

        HandleUse collective_use(
          in_out.var_handle_,
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

  }

};

} // end namespace detail


// TODO generalize the argument parsing here

template <typename Op = detail::op_not_given, typename... KWArgs>
void allreduce(
  KWArgs&&... kwargs
) {
  //static_assert(detail::only_allowed_kwargs_given<
  //    darma_runtime::keyword_tags_for_collectives::input,
  //    darma_runtime::keyword_tags_for_collectives::output,
  //    darma_runtime::keyword_tags_for_collectives::in_out,
  //    darma_runtime::keyword_tags_for_collectives::piece,
  //    darma_runtime::keyword_tags_for_collectives::n_pieces,
  //    darma_runtime::keyword_tags_for_collectives::tag
  //  >::template apply<KWArgs...>::type::value,
  //  "Unknown keyword argument given to create_work()"
  //);

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
    .with_defaults(
      keyword_arguments_for_collectives::tag=make_key(),
      keyword_arguments_for_collectives::piece=abstract::frontend::CollectiveDetails::unknown_contribution(),
      keyword_arguments_for_collectives::n_pieces=abstract::frontend::CollectiveDetails::unknown_contribution()
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



  //decltype(auto) positional_args = detail::get_positional_arg_tuple(
  //  std::forward<KWArgs>(kwargs)...
  //);

  //// Keyword argument only:
  //size_t piece = detail::get_typeless_kwarg_with_default_as<
  //  darma_runtime::keyword_tags_for_collectives::piece, size_t
  //>(
  //  abstract::frontend::CollectiveDetails::unknown_contribution(),
  //  std::forward<KWArgs>(kwargs)...
  //);

  //// Keyword argument only:
  //size_t n_pieces = detail::get_typeless_kwarg_with_default_as<
  //  darma_runtime::keyword_tags_for_collectives::n_pieces, size_t
  //>(
  //  abstract::frontend::CollectiveDetails::unknown_contribution(),
  //  std::forward<KWArgs>(kwargs)...
  //);

  //// Keyword argument only:
  //auto tag = detail::get_typeless_kwarg_with_converter_and_default<
  //  darma_runtime::keyword_tags_for_collectives::tag
  //>([](auto&&... key_parts) {
  //    return darma_runtime::make_key(
  //      std::forward<decltype(key_parts)>(key_parts)...
  //    );
  //  },
  //  types::key_t(),
  //  std::forward<KWArgs>(kwargs)...
  //);

  //using n_positional_args_t = typename detail::n_positional_args<KWArgs...>::type;

  //// forms to handle: (not counting piece and n_pieces, which are kwarg-only)
  //// allreduce(input_arg, output_arg);
  //// allreduce(input_arg, output=output_arg);
  //// allreduce(input=input_arg, output=output_arg);
  //// allreduce(inout_arg);
  //// allreduce(in_out=inout_arg)

  //using tinympl::bool_;

  //decltype(auto) output_handle = detail::_get_potentially_positional_handle_arg<
  //  1, darma_runtime::keyword_tags_for_collectives::output
  //>(
  //  detail::has_kwarg<
  //    darma_runtime::keyword_tags_for_collectives::output, KWArgs...
  //  >(),
  //  bool_< n_positional_args_t::value >= 2 >(),
  //  std::forward<KWArgs>(kwargs)...
  //);

  //decltype(auto) input_handle = detail::_get_potentially_positional_handle_arg<
  //  0, darma_runtime::keyword_tags_for_collectives::input
  //>(
  //  detail::has_kwarg<
  //    darma_runtime::keyword_tags_for_collectives::input, KWArgs...
  //  >(),
  //  bool_<
  //    n_positional_args_t::value >= 2
  //    or (
  //      n_positional_args_t::value >= 1
  //      and detail::has_kwarg<
  //        darma_runtime::keyword_tags_for_collectives::output, KWArgs...
  //      >::value
  //    )
  //  >(),
  //  std::forward<KWArgs>(kwargs)...
  //);

  //decltype(auto) in_out_handle = detail::_get_potentially_positional_handle_arg<
  //  0, darma_runtime::keyword_tags_for_collectives::in_out
  //>(
  //  detail::has_kwarg<
  //    darma_runtime::keyword_tags_for_collectives::in_out, KWArgs...
  //  >(),
  //  bool_<
  //    n_positional_args_t::value >= 1
  //    and (
  //      not (
  //        detail::has_kwarg<
  //          darma_runtime::keyword_tags_for_collectives::output, KWArgs...
  //        >::value
  //        or n_positional_args_t::value >= 2
  //      )
  //    )
  //  >(),
  //  std::forward<KWArgs>(kwargs)...
  //);

  //detail::all_reduce_impl::_do_allreduce<Op>(
  //  std::forward<decltype(input_handle)>(input_handle),
  //  std::forward<decltype(output_handle)>(output_handle),
  //  std::forward<decltype(in_out_handle)>(in_out_handle),
  //  piece, n_pieces, tag
  //);


};



} // end namespace darma_runtime

#endif //DARMA_IMPL_COLLECTIVE_ALLREDUCE_H
