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

#include <darma/interface/app/access_handle.h>
#include <darma/impl/handle_attorneys.h>
#include <darma/impl/flow_handling.h>
#include <darma/impl/keyword_arguments/check_allowed_kwargs.h>


namespace darma_runtime {

template <
  typename U=void,
  typename... KeyExprParts
>
AccessHandle<U>
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
    .invoke([](
      types::key_t&& version_key,
      variadic_arguments_begin_tag,
      auto&&... args
    ) -> decltype(auto) {
      auto backend_runtime = abstract::backend::get_backend_runtime();
      auto var_h = detail::make_shared<detail::VariableHandle<U>>(
        darma_runtime::make_key(std::forward<decltype(args)>(args)...)
      );
      auto in_flow = detail::make_flow_ptr(
        backend_runtime->make_fetching_flow( var_h, std::move(version_key) )
      );
      auto out_flow = detail::make_flow_ptr(
        backend_runtime->make_null_flow( var_h )
      );

      return detail::access_attorneys::for_AccessHandle::construct_access<U>(
        var_h, in_flow, out_flow,
        detail::HandleUse::Read, detail::HandleUse::None
      );

    });
}

template <
  typename U=void,
  typename... KeyExprParts
>
AccessHandle<U>
acquire_ownership(
  KeyExprParts&&... parts
) {
  using _check_kwargs_assert_t = typename detail::only_allowed_kwargs_given<
    keyword_tags_for_publication::version,
    keyword_tags_for_acquire_ownership::from_data_store
  >::template static_assert_correct<KeyExprParts...>::type;

  using namespace darma_runtime::detail;
  using parser = detail::kwarg_parser<
    variadic_positional_overload_description<
      _optional_keyword<converted_parameter, keyword_tags_for_publication::version>,
      _optional_keyword<converted_parameter, keyword_tags_for_acquire_ownership::from_data_store>
    >
  >;
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<KeyExprParts...>;

  return parser()
    .with_converters(
      // version key converter
      [](auto&&... parts) {
        return darma_runtime::make_key(std::forward<decltype(parts)>(parts)...);
      },
      // from_data_store
      [](auto&& ds_arg) {
        return detail::DataStoreAttorney::get_handle(ds_arg);
      }
    )
    .with_default_generators(
      keyword_arguments_for_publication::version=[]{ return make_key(); },
      keyword_arguments_for_acquire_ownership::from_data_store=[]{
        return std::shared_ptr<abstract::backend::DataStoreHandle>(nullptr);
      }
    )
    .parse_args(
      std::forward<KeyExprParts>(parts)...
    )
    .invoke(
      [](
        types::key_t&& version_key,
        std::shared_ptr<abstract::backend::DataStoreHandle> ds_handle,
        variadic_arguments_begin_tag,
        auto&&... args
      ) -> decltype(auto) {

        auto backend_runtime = abstract::backend::get_backend_runtime();
        auto var_h = detail::make_shared<detail::VariableHandle<U>>(
          darma_runtime::make_key(std::forward<decltype(args)>(args)...)
        );
        auto in_flow = detail::make_flow_ptr(
          backend_runtime->make_fetching_flow( var_h, version_key, ds_handle, true )
        );
        auto out_flow = detail::make_flow_ptr(
          backend_runtime->make_null_flow( var_h )
        );
        return detail::access_attorneys::for_AccessHandle::construct_access<U>(
          var_h, in_flow, out_flow,
          detail::HandleUse::Modify, detail::HandleUse::None
        );
      }
    );

}


} // end namespace darma_runtime


#endif /* SRC_INCLUDE_DARMA_INTERFACE_APP_READ_ACCESS_H_ */
