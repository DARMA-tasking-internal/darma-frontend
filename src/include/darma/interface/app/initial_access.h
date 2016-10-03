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
#include <darma/impl/util.h>
#include <darma/impl/flow_handling.h>

namespace darma_runtime {

template <
  typename T=void,
  typename... KeyExprParts
>
AccessHandle<T>
initial_access(
  KeyExprParts&&... parts
) {
  static_assert(detail::only_allowed_kwargs_given<
    >::template apply<KeyExprParts...>::type::value,
    "Unknown keyword argument given to initial_access"
  );
  types::key_t key = detail::access_expr_helper<KeyExprParts...>().get_key(
    std::forward<KeyExprParts>(parts)...
  );

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

} // end namespace darma_runtime


#endif /* SRC_INCLUDE_DARMA_INTERFACE_APP_INITIAL_ACCESS_H_ */
