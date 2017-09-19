/*
//@HEADER
// ************************************************************************
//
//                      darma_region.h
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

#ifndef DARMAFRONTEND_DARMA_REGION_H
#define DARMAFRONTEND_DARMA_REGION_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(darma_regions)

#include <darma_types.h>
#include <darma/interface/backend/runtime.h>

namespace darma_runtime {
namespace experimental {

namespace _impl {

template <typename AlwaysVoid=void>
auto&
_get_default_instance_token_ptr() {
  static auto _rv = std::make_unique<
    darma_runtime::types::runtime_instance_token_t
  >(
    darma_runtime::backend::initialize_runtime_instance()
  );
  return _rv;
}

auto&
get_default_instance_token() {
  return *_get_default_instance_token_ptr<>().get();
}


} // end namespace _impl

template <typename Callable>
auto
darma_region(Callable&& callable) {
  auto done_promise = std::promise<void>();
  auto done_future = done_promise.get_future();
  backend::register_runtime_instance_quiescence_callback(
    _impl::get_default_instance_token(),
    std::function<void()>([done_promise=std::move(done_promise)]{
      done_promise.set_value();
    })
  );
  backend::with_active_runtime_instance(
    _impl::get_default_instance_token(),
    std::function<void()>(std::forward<Callable>(callable)())
  );
  return done_future;
}

} // end namespace experimental
} // end namespace darma_runtime

#endif // _darma_has_feature(darma_regions)

#endif //DARMAFRONTEND_DARMA_REGION_H
