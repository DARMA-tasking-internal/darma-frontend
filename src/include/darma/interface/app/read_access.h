/*
//@HEADER
// ************************************************************************
//
//                          read_access.h
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

#ifndef SRC_INCLUDE_DARMA_INTERFACE_APP_READ_ACCESS_H_
#define SRC_INCLUDE_DARMA_INTERFACE_APP_READ_ACCESS_H_

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(arbitrary_publish_fetch)

#include <darma/interface/app/access_handle.h>
#include <darma/impl/handle_attorneys.h>
#include <darma/impl/flow_handling.h>
#include <darma/impl/compatibility.h>
#include <darma/impl/keyword_arguments/check_allowed_kwargs.h>


namespace darma_runtime {

namespace detail {

template <typename U>
struct _read_access_helper {
  template <typename... Args>
  decltype(auto)
  operator()(
    types::key_t&& version_key,
    darma_runtime::detail::variadic_arguments_begin_tag,
    Args&&... args
  ) const {
    auto backend_runtime = darma_runtime::abstract::backend::get_backend_runtime();
    auto var_h = darma_runtime::detail::make_shared<darma_runtime::detail::VariableHandle<U>>(
      darma_runtime::make_key(std::forward<decltype(args)>(args)...)
    );

    using namespace darma_runtime::detail::flow_relationships;
    using namespace darma_runtime::abstract::frontend;

    auto rv = darma_runtime::ReadAccessHandle<U>(
      var_h,
      std::make_shared<detail::UseHolder>(
        detail::HandleUse(
          var_h,
          Use::Read, Use::None,
          detail::HandleUseBase::FlowRelationshipImpl(
            abstract::frontend::FlowRelationship::Fetching,
            /* related flow = */ nullptr,
            /* related_is_in = */ false,
            /* version key = */ &version_key,
            /* index = */ 0
#if _darma_has_feature(anti_flows)
            , /* anti_related = */ nullptr,
            /* anti_rel_is_in = */ false
#endif // _darma_has_feature(anti_flows)
          ),
          //FlowRelationship::Fetching, nullptr,
          null_flow()
          //FlowRelationship::Null, nullptr, false,
        ), true, false
      )
    );
    rv.current_use_->could_be_alias = true;
    return std::move(rv);

  }
};

} // end namespace detail

template <
  typename U=void,
  typename... KeyExprParts
>
DARMA_ATTRIBUTE_DEPRECATED_WITH_MESSAGE(
  "arbitrary publish fetch is being removed very soon"
)
auto
read_access(
  KeyExprParts&&... parts
) {
  using namespace darma_runtime::detail;
  using parser = detail::kwarg_parser<
    variadic_positional_overload_description<
      _optional_keyword<converted_parameter, keyword_tags_for_publication::version>
    >
  >;
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<KeyExprParts...>;

  return parser()
    .with_converters(
      [](auto&&... parts) {
        return darma_runtime::make_key(std::forward<decltype(parts)>(parts)...);
      }
    )
    .with_default_generators(
      keyword_arguments_for_publication::version=[]{ return make_key(); }
    )
    .parse_args(std::forward<KeyExprParts>(parts)...)
    .invoke(detail::_read_access_helper<U>{});
}

} // end namespace darma_runtime

#endif // _darma_has_feature(arbitrary_publish_fetch)

#endif /* SRC_INCLUDE_DARMA_INTERFACE_APP_READ_ACCESS_H_ */
