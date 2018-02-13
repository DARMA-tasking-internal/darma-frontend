/*
//@HEADER
// ************************************************************************
//
//                      registration.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMAFRONTEND_REGISTRATION_H
#define DARMAFRONTEND_REGISTRATION_H

#include <vector>
#include <functional>

namespace darma_runtime {
namespace registration {

template <typename ReturnType>
using callable_registry = std::vector<
  std::function<ReturnType(void*)>
>;

using callable_registry_index = std::size_t;

template <typename ReturnType=void>
callable_registry&
get_callable_registry() {
  static callable_registry registry_instance = { };
  return registry_instance;
}

template <typename Callable, typename ReturnType, typename... Args>
struct StatelessCallableRegistrar {
  callable_registry_index index;
  template <size_t... ArgIdxs>
  explicit
  StatelessCallableRegistrar(std::integer_sequence<size_t, ArgIdxs>) {
    callable_registry<ReturnType>& reg = get_callable_registry<ReturnType>();
    index = reg.size();
    reg.emplace_back([](void* args_as_bits) -> ReturnType {
      return Callable{}(
        std::get<ArgIdxs>(
          static_cast<std::tuple<Args...>*>(args_as_bits)
        )...
      );
    });
  }
};

template <typename ReturnType, typename Callable, typename... Args>
struct StatelessCallableRegistrarWrapper {
  static StatelessCallableRegistrar<ReturnType, Callable, Args...> registrar;
};

template <typename ReturnType, typename Callable, typename... Args>
StatelessCallableRegistrar<ReturnType, Callable, Args...>
StatelessCallableRegistrarWrapper<ReturnType, Callable, Args...>::registrar
  = { std::index_sequence_for<Args...>{} };


namespace detail {

template <typename Callable, typename ReturnType, typename... Args>
auto _get_stateless_callable_idx_helper(
  ReturnType (Callable::* call_op)(Args...) const&
) {
  return StatelessCallableRegistrarWrapper<Callable, ReturnType, Args...>::registrar.index;
};

} // end namespace detail

template <typename Callable>
callable_registry_index
get_registration_index_for_stateless_callable(Callable&& callable) {
  return detail::_get_stateless_callable_idx_helper(
    &(std::decay<Callable>::operator())
  );

}


} // end namespace registration
} // end namespace darma_runtime

#endif //DARMAFRONTEND_REGISTRATION_H
