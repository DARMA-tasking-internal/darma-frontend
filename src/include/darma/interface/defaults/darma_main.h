/*
//@HEADER
// ************************************************************************
//
//                          darma_main.h
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

#ifndef FRONTEND_INCLUDE_DARMA_DARMA_MAIN_H_
#define FRONTEND_INCLUDE_DARMA_DARMA_MAIN_H_

#include <functional>

#include <darma/impl/compatibility.h>

namespace darma_runtime {
namespace detail {

template <typename _ignored = void>
std::function<int(int, char**)>*
_darma__generate_main_function_ptr() {
  static std::unique_ptr<std::function<int(int, char**)>> _rv =
      std::make_unique<std::function<int(int, char**)>>(nullptr);
  return _rv.get();
}

//static std::function<int(int, char**)>* user_main_function_ptr = _darma__generate_main_function_ptr<void>();

template <typename T>
DARMA_CONSTEXPR_14 int
register_user_main(T main_fxn) {
  *(_darma__generate_main_function_ptr<>()) = main_fxn;
  return 42;
}

} // end namespace detail
} // end namespace darma_runtime

#define darma_main(...) \
  _darma__ignore_this = 42; \
  int _darma__user_main(__VA_ARGS__); \
  int _darma__ignore_this_too = \
    ::darma_runtime::detail::register_user_main((int(*)(__VA_ARGS__))_darma__user_main); \
  int _darma__user_main(__VA_ARGS__)


//  int(*)(__VA_ARGS__), (int(*)(__VA_ARGS__))_darma__user_main> _darma__user_main_metaptr_t;

#endif /* FRONTEND_INCLUDE_DARMA_DARMA_MAIN_H_ */
