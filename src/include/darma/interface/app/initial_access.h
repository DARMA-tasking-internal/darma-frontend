/*
//@HEADER
// ************************************************************************
//
//                          initial_access.h
//                         dharma_new
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

#ifndef SRC_INCLUDE_DARMA_INTERFACE_APP_INITIAL_ACCESS_H_
#define SRC_INCLUDE_DARMA_INTERFACE_APP_INITIAL_ACCESS_H_

#include <tinympl/extract_template.hpp>

#include <darma/interface/app/access_handle.h>
#include <darma/impl/handle_attorneys.h>
#include <darma/impl/keyword_arguments/check_allowed_kwargs.h>
#include <darma/impl/keyword_arguments/parse.h>
#include <darma/impl/util.h>
#include <darma/impl/flow_handling.h>

namespace darma_runtime {

namespace detail {

template <typename T>
struct _initial_access_key_helper {

  decltype(auto)
  _impl(darma_runtime::types::key_t const& key) const {
    auto* backend_runtime = abstract::backend::get_backend_runtime();
    auto var_h = detail::make_shared<detail::VariableHandle<T>>(key);
    auto in_flow = detail::make_flow_ptr(
      backend_runtime->make_initial_flow( var_h )
    );
    auto out_flow = detail::make_flow_ptr(
      backend_runtime->make_null_flow( var_h )
    );
    return detail::access_attorneys::for_AccessHandle::construct_access<T>(
      var_h, in_flow, out_flow, detail::HandleUse::Modify, detail::HandleUse::None
    );
  }

  template <typename Arg, typename... Args>
  decltype(auto)
  operator()(variadic_arguments_begin_tag, Arg&& arg, Args&&... args) {
    types::key_t key = darma_runtime::make_key(
      std::forward<Arg>(arg),
      std::forward<decltype(args)>(args)...
    );
    return _impl(key);
  }

  decltype(auto)
  operator()(variadic_arguments_begin_tag) {
    // call default ctor to make a backend-awaiting key
    types::key_t key = darma_runtime::types::key_t();
    return _impl(key);
  }

};

} // end namespace detail

template <
  typename T=void,
  typename... KeyExprParts
>
AccessHandle<T>
initial_access(
  KeyExprParts&&... parts
) {
  using namespace darma_runtime::detail;
  using parser = detail::kwarg_parser<
    variadic_positional_overload_description<>
  >;
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<KeyExprParts...>;

  return parser()
    .parse_args(std::forward<KeyExprParts>(parts)...)
    .invoke(detail::_initial_access_key_helper<T>{});
}

} // end namespace darma_runtime


#endif /* SRC_INCLUDE_DARMA_INTERFACE_APP_INITIAL_ACCESS_H_ */
