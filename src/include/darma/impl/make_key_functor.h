/*
//@HEADER
// ************************************************************************
//
//                      make_key_functor.h
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

#ifndef DARMA_IMPL_MAKE_KEY_FUNCTOR_H_
#define DARMA_IMPL_MAKE_KEY_FUNCTOR_H_

#include <cstdlib>     // size_t
#include <type_traits> // decay_t

#include <tinympl/select.hpp>
#include <darma/impl/access_handle_base.h>
#include <darma/impl/handle_attorneys.h>
#include <darma/interface/app/access_handle.h>

namespace darma_runtime {
namespace detail {

struct _make_key_functor {

  private:
  
    struct variadic_is_ah_tag {};
    struct variadic_no_ah_tag {};

  private:

    // Process all arguments but access handles
    template <typename T>
    inline decltype(auto) 
    _get_arg_impl(
      variadic_no_ah_tag,
      T&& arg
    ) const {
      return std::forward<T>(arg);
    }

    // Process access handles
    template <typename T>
    inline decltype(auto) 
    _get_arg_impl(
      variadic_is_ah_tag,
      T&& ah
    ) const {
      return ah.get_value(); 
    }
  
  private:

    // Dispatch tag evaluation
    template <typename T> using variadic_tag = tinympl::select_first_t<
      detail::decayed_is_access_handle<T>,
      /* => */ variadic_is_ah_tag,
      std::true_type,
      /* => */ variadic_no_ah_tag
    >;

    template <typename... Args, 
      size_t... Idxs
    >
    inline auto
    _impl(
      std::tuple<Args...>&& args_tuple,
      std::integer_sequence<size_t, Idxs...>
    ) const {
      return make_key(_get_arg_impl(variadic_tag<decltype(std::get<Idxs>(args_tuple))>{}, std::get<Idxs>(args_tuple))...);
    } 

  public:

    _make_key_functor() = default;

    template <typename... Args>
    auto 
    operator()(
      Args&&... args
    ) const && {
      return _impl(std::forward_as_tuple(std::forward<Args>(args)...),
                   std::index_sequence_for<Args...>{});
    }

};

}  // End namespace detail
}  // End namespace darma_runtime

#endif
