/*
//@HEADER
// ************************************************************************
//
//                      darma_region.h
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

#ifndef DARMAFRONTEND_DARMA_REGION_H
#define DARMAFRONTEND_DARMA_REGION_H

#include <darma/impl/feature_testing_macros.h>
#include <darma/interface/backend/darma_region.h>

#if _darma_has_feature(darma_regions)

#include <darma_types.h>
#include <darma/interface/backend/runtime.h>

namespace darma {
namespace experimental {

namespace _impl {

template <typename AlwaysVoid=void>
auto&
_get_default_instance_token_ptr() {
  static auto _rv = std::make_unique<
    darma::types::runtime_instance_token_t
  >(
    darma::backend::initialize_runtime_instance()
  );
  return _rv;
}

inline auto&
get_default_instance_token() {
  return *_get_default_instance_token_ptr<>().get();
}


} // end namespace _impl


/**
 *
 *  Keyword arguments to darma_region()
 *
 *    - `n_threads` (alias `n_workers`):  The number of threads or workers
 *      to use when executing the region (default: all threads as set up
 *      in `darma_initialize`, or all threads minus 1 if `blocking=false`
 *    - `blocking`:  whether or not to completely execute all of the tasks
 *      spawned by the region before returning.  If `false`, a `std::future<RT>`
 *      will be returned, where `RT` is the return type of the `Callable`
 *      (potentially unwrapped as `T` if an `AccessHandle<T>` is returned).
 *      Note that `blocking=true` will also return a `std::future<RT>` that will
 *      always be ready upon return and can be safely ignored.  To avoid generating
 *      a `std::future<RT>` althogether (and instead get back a `RT` directly),
 *      use `darma_region_blocking()`.  Default value is `false`
 *    - `launch_from_parallel`:  If `true`, DARMA will expect `n_threads`
 *      identical calls of `darma_region()`, with exactly one having `true`
 *      as the value for `is_parallel_launch_master`.  This is used to transition
 *      from a multi-threaded context to a multi-threaded use of DARMA without
 *      launching new threads.  DARMA will use the threads calling in to
 *      this function as workers, and the `Callable` argument will be run on
 *      the thread calling with `is_parallel_launch_master=true` (or on any one of
 *      the threads if that keyword is not given).  A `std::shared_future<RT>` will
 *      be returned on each thread.
 *    - `is_parallel_launch_master`:  Indicates which thread will run the `callable`
 *      when running in `launch_from_parallel` mode.  Requires `launch_from_parallel=true`.
 *
 *  @TODO finish this
 *
 * @tparam Callable
 * @param callable
 * @return
 */
template <typename Callable>
auto
darma_region(Callable&& callable) {
  auto done_promise = std::make_shared<std::promise<void>>();
  auto done_future = done_promise->get_future();
  backend::register_runtime_instance_quiescence_callback(
    _impl::get_default_instance_token(),
    std::function<void()>([done_promise=std::move(done_promise)]() mutable {
      done_promise->set_value();
    })
  );
  backend::with_active_runtime_instance(
    _impl::get_default_instance_token(),
    std::forward<Callable>(callable)
  );
  return done_future;
}

inline auto
darma_initialize(int& argc, char**& argv) {
  backend::initialize_with_arguments(argc, argv);
}

inline auto
darma_finalize() {
  backend::finalize();
}

} // end namespace experimental
} // end namespace darma

#endif // _darma_has_feature(darma_regions)

#endif //DARMAFRONTEND_DARMA_REGION_H
