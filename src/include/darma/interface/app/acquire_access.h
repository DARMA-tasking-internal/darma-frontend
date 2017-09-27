/*
//@HEADER
// ************************************************************************
//
//                      acquire_access.h
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

#ifndef DARMAFRONTEND_ACQUIRE_ACCESS_H
#define DARMAFRONTEND_ACQUIRE_ACCESS_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(unmanaged_data)

#include <darma/impl/keyword_arguments/parse.h>

namespace darma_runtime {
namespace experimental {

template <typename T, typename... Args>
auto
acquire_access(
  T* unmanaged_data_ptr,
  Args&&... args
) {
  using namespace darma_runtime::detail;
  using parser = detail::kwarg_parser<
    variadic_positional_overload_description<>
  >;
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<KeyExprParts...>;

  return parser()
    .parse_args(std::forward<Args>(args)...)
    .invoke([&](
        variadic_arguments_begin_tag,
        auto&&... key_expr_parts
      ) {
        auto key = sizeof...(key_expr_parts) == 0 ?
          darma_runtime::types::key_t(
            darma_runtime::types::key_t::request_backend_assigned_key_tag{}
          ) : darma_runtime::make_key(
            std::forward<decltype(key_expr_parts)>(key_expr_parts)...
          );
        auto var_h = std::make_shared<detail::UnmanagedHandle<T>>(key);
        auto ready_promise = std::promise<T*>();
        auto ready_future = ready_promise.get_future();
        abstract::backend::get_backend_runtime()->register_unmanaged_pointer_handle(
          var_h,
          unmanaged_data_ptr,
          [ready_promise=std::move(ready_promise),unmanaged_data_ptr](auto&&, void* data) {
            assert((intptr_t)data == (intptr_t)unmanaged_data_ptr);
            ready_promise.set_value((T*)data);
          }
        );
#if _darma_has_feature(anti_flows)
        auto use_holder = std::make_shared<UseHolder>(
          HandleUse(
            var_h,
            frontend::Permissions::Modify,
            frontend::Permissions::None,
            initial_flow(),
            null_flow(),
            insignificant_flow(),
            initial_anti_flow()
          ), /* register in ctor = */ true, /* will be dep = */ false
        );
#else
        auto use_holder = std::make_shared<UseHolder>(
          HandleUse(
            var_h,
            frontend::Permissions::Modify,
            frontend::Permissions::None,
            initial_flow(),
            //FlowRelationship::Initial, nullptr,
            null_flow()
            //FlowRelationship::Null, nullptr, false
          ), /* register in ctor = */ true, /* will be dep = */ false
        );
#endif // _darma_has_feature(anti_flows)

        auto rv_handle = AccessHandle<T,
          typename make_access_handle_traits<T, TraitsFlags...>::template from_traits<
            make_access_handle_traits_t<T,
            static_scheduling_permissions<AccessHandlePermissions::Modify>,
              copy_assignable_handle<false>
            >
          >::type
        >(
          var_h, std::move(use_holder)
        );

        return std::make_pair(rv_handle, std::move(ready_future));

      }

    );

};

} // end namespace experimental
} // end namespace darma_runtime

#endif // darma_has_feature(unmanaged_data)

#endif //DARMAFRONTEND_ACQUIRE_ACCESS_H
