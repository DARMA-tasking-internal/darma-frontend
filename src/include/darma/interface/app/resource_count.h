/*
//@HEADER
// ************************************************************************
//
//                      resource_count.h
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

#ifndef DARMA_INTERFACE_APP_RESOURCE_COUNT_H
#define DARMA_INTERFACE_APP_RESOURCE_COUNT_H

#include <darma/impl/meta/tagged_constant.h>
#include <darma/impl/keyword_arguments/parse.h>

namespace darma_runtime {

// Constants...
namespace constants_for_resource_count {

namespace _catagories {

DARMA_CREATE_TAGGED_CONSTANT_CATAGORY(resource_count_catagory);
DARMA_CREATE_TAGGED_CONSTANT_CATAGORY(execution_resource_depth);

} // end namespace detail

DARMA_CREATE_TAGGED_CONSTANT(Execution, _catagories::resource_count_catagory);

DARMA_CREATE_TAGGED_CONSTANT(Process, _catagories::execution_resource_depth);
static constexpr auto Processes = Process;
DARMA_CREATE_TAGGED_CONSTANT(Socket, _catagories::execution_resource_depth);
static constexpr auto Sockets = Socket;
DARMA_CREATE_TAGGED_CONSTANT(Core, _catagories::execution_resource_depth);
static constexpr auto Cores = Core;
DARMA_CREATE_TAGGED_CONSTANT(HardwareThread, _catagories::execution_resource_depth);
static constexpr auto HardwareThreads = HardwareThread;

} // end namespace constants_for_resource_count

} // end namespace darma_runtime

DeclareDarmaTypeTransparentKeyword(resource_count, depth);
namespace darma_runtime {
namespace keyword_arguments {
static constexpr auto depth = ::darma_runtime::keyword_arguments_for_resource_count::depth;
} // end namespace keyword_arguments
} // end namespace darma_runtime

DeclareDarmaTypeTransparentKeyword(resource_count, per);
namespace darma_runtime {
namespace keyword_arguments {
static constexpr auto per = ::darma_runtime::keyword_arguments_for_resource_count::per;
} // end namespace keyword_arguments
} // end namespace darma_runtime

namespace darma_runtime {

namespace detail {

inline auto _exec_resource_count(size_t depth) {
  return abstract::backend::get_backend_runtime()->get_execution_resource_count(depth);
}

inline auto _get_depth(constants_for_resource_count::detail::Process_tag_t) { return 0; }
inline auto _get_depth(constants_for_resource_count::detail::Socket_tag_t) { return 1; }
inline auto _get_depth(constants_for_resource_count::detail::Core_tag_t) { return 2; }
inline auto _get_depth(constants_for_resource_count::detail::HardwareThread_tag_t) { return 3; }
inline auto _get_depth(int value) { return value; }
template <typename KWarg>
inline auto _get_depth(KWarg&& kwarg) { return _get_depth(std::forward<KWarg>(kwarg).value()); }

template <typename DepthKWarg, typename=std::enable_if_t<
  is_kwarg_expression_with_tag<std::decay_t<DepthKWarg>, keyword_tags_for_resource_count::depth>::value
>>
auto resource_count_impl(
  constants_for_resource_count::detail::Execution_tag_t,
  DepthKWarg&& depth
) {
  return _exec_resource_count(_get_depth(std::forward<DepthKWarg>(depth).value()));
}

template <typename DepthKWarg, typename PerKWarg, typename=std::enable_if_t<
  (
    is_kwarg_expression_with_tag<std::decay_t<DepthKWarg>, keyword_tags_for_resource_count::depth>::value
    or std::is_convertible<DepthKWarg, constants_for_resource_count::_catagories::execution_resource_depth>::value
  )
  and is_kwarg_expression_with_tag<std::decay_t<PerKWarg>, keyword_tags_for_resource_count::per>::value
>>
auto resource_count_impl(
  constants_for_resource_count::detail::Execution_tag_t,
  DepthKWarg&& depth, PerKWarg&& per_kw
) {
  auto const depth_idx = _get_depth(std::forward<DepthKWarg>(depth));
  auto const per_depth = _get_depth(std::forward<PerKWarg>(per_kw));
  DARMA_ASSERT_MESSAGE(depth_idx >= per_depth,
    "Can't compute nonsensical count of exec resources per other exec resources"
    " of lesser depth (e.g., Sockets per Core)"
  );
  return _exec_resource_count(depth_idx) / _exec_resource_count(per_depth);
}

template <typename DepthKWarg, typename PerKWarg, typename=std::enable_if_t<
  is_kwarg_expression_with_tag<std::decay_t<DepthKWarg>, keyword_tags_for_resource_count::depth>::value
  and is_kwarg_expression_with_tag<std::decay_t<PerKWarg>, keyword_tags_for_resource_count::per>::value
>>
auto resource_count_impl(
  constants_for_resource_count::detail::Execution_tag_t,
  PerKWarg&& per_kw, DepthKWarg&& depth
) {
  return resource_count_impl(constants_for_resource_count::Execution,
    std::forward<PerKWarg>(per_kw), std::forward<DepthKWarg>(depth)
  );
}

template <typename DepthArg, typename PerKWarg, typename=std::enable_if_t<
  std::is_convertible<DepthArg, constants_for_resource_count::_catagories::execution_resource_depth>::value
  and is_kwarg_expression_with_tag<std::decay_t<PerKWarg>, keyword_tags_for_resource_count::per>::value
>>
auto resource_count_impl(
  DepthArg&& depth, PerKWarg&& per_kw
) {
  return resource_count_impl(constants_for_resource_count::Execution,
    keyword_arguments_for_resource_count::depth=std::forward<DepthArg>(depth),
    std::forward<PerKWarg>(per_kw)
  );
};

//template <typename DepthKWarg, typename PerKWarg, typename=std::enable_if_t<
//  is_kwarg_expression_with_tag<std::decay_t<DepthKWarg>, keyword_tags_for_resource_count::depth>::value
//    and is_kwarg_expression_with_tag<std::decay_t<PerKWarg>, keyword_tags_for_resource_count::per>::value
//>>
//auto resource_count_impl(
//  DepthKWarg&& depth, PerKWarg&& per_kw
//) {
//  return resource_count_impl(constants_for_resource_count::Execution,
//    std::forward<DepthKWarg>(depth), std::forward<PerKWarg>(per_kw)
//  );
//}

//template <typename DepthKWarg, typename PerKWarg, typename=std::enable_if_t<
//  is_kwarg_expression_with_tag<std::decay_t<DepthKWarg>, keyword_tags_for_resource_count::depth>::value
//    and is_kwarg_expression_with_tag<std::decay_t<PerKWarg>, keyword_tags_for_resource_count::per>::value
//>>
//auto resource_count_impl(
//  PerKWarg&& per_kw, DepthKWarg&& depth
//) {
//  return resource_count_impl(constants_for_resource_count::Execution,
//    std::forward<DepthKWarg>(depth), std::forward<PerKWarg>(per_kw)
//  );
//}

template <typename DepthT, typename=std::enable_if_t<
  std::is_convertible<DepthT, constants_for_resource_count::_catagories::execution_resource_depth>::value
>>
auto resource_count_impl(
  constants_for_resource_count::detail::Execution_tag_t,
  DepthT depth // not a forwarding reference on purpose, in hopes of avoiding binding a reference to constexpr
) {
  return _exec_resource_count(_get_depth(depth));
}

template <typename DepthT, typename=std::enable_if_t<
  std::is_convertible<DepthT, constants_for_resource_count::_catagories::execution_resource_depth>::value
>>
auto resource_count_impl(
  DepthT depth
) {
  return _exec_resource_count(_get_depth(depth));
};

} // end namespace detail

template <typename... Args>
auto resource_count(Args&&... args) {
  using _check_kwargs_asserting_t = typename detail::only_allowed_kwargs_given<
    darma_runtime::keyword_tags_for_resource_count::depth,
    darma_runtime::keyword_tags_for_resource_count::per
  >::template static_assert_correct<Args...>::type;
  return detail::resource_count_impl(std::forward<Args>(args)...);
}


} // end namespace darma_runtime

#endif //DARMA_INTERFACE_APP_RESOURCE_COUNT_H
