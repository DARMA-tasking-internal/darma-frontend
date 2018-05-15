/*
//@HEADER
// ************************************************************************
//
//                      smart_pointers.h
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
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

#ifndef DARMA_IMPL_UTIL_SMART_POINTERS_H
#define DARMA_IMPL_UTIL_SMART_POINTERS_H

#include <memory>

#include <darma_types.h>

namespace darma {

namespace detail {

/** @brief A traits class for smart pointers that allows us to provide
 *  specializations with hooks to helpers (e.g., make_shared<T> for
 *  shared_ptr<T>)
 */
template <
  typename T
>
struct smart_ptr_traits;

template <typename... InTs>
struct smart_ptr_traits<std::shared_ptr<InTs...>> {
template <typename... Ts>
struct maker {
  template <typename... Args>
  inline std::shared_ptr<Ts...>
  operator()(Args&&... args) const {
    return std::make_shared<Ts...>(
      std::forward<Args>(args)...
    );
  }
};
template <typename... Ts>
struct allocator {
  template <typename Alloc, typename... Args>
  inline std::shared_ptr<Ts...>
  operator()(Alloc&& alloc, Args&&... args) const {
    return std::allocate_shared<Ts...>(
      std::forward<Alloc>(alloc),
      std::forward<Args>(args)...
    );
  }
};
template <typename... Ts>
using weak_ptr_template = std::weak_ptr<Ts...>;
template <typename... Ts>
using shared_from_this_base = std::enable_shared_from_this<Ts...>;
};

template <typename... InTs>
struct smart_ptr_traits<std::unique_ptr<InTs...>>
{
template <typename... Ts>
struct maker {
  template <typename... Args>
  inline auto
  operator()(Args&&... args) const {
    return std::make_unique<Ts...>(
      std::forward<Args>(args)...
    );
  }
};

};

template <typename T, typename... Args>
auto
make_shared(Args&&... args) {
  using ptr_maker_t = typename smart_ptr_traits<
    types::shared_ptr_template<T>
  >::template maker<T>;
  return ptr_maker_t{}(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
auto
make_unique(Args&&... args) {
  using ptr_maker_t = typename smart_ptr_traits<
    types::unique_ptr_template<T>
  >::template maker<T>;
  return ptr_maker_t{}(std::forward<Args>(args)...);
}

} // end namespace detail

} // end namespace darma

#endif //DARMA_IMPL_UTIL_SMART_POINTERS_H
