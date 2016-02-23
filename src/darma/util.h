/*
//@HEADER
// ************************************************************************
//
//                          util.h
//                         darma_mockup
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

#ifndef NEW_UTIL_H_
#define NEW_UTIL_H_

#include "meta/metaprogramming.h"
#include <memory> // std::shared_ptr
#include <tuple> // std::tuple
#include <functional>  // std::hash

namespace darma_runtime {

namespace detail {

struct variadic_constructor_arg_t { };
constexpr const variadic_constructor_arg_t variadic_constructor_arg = { };

/** @brief A traits class for smart pointers that allows us to provide
 *  specializations with hooks to helpers (e.g., make_shared<T> for
 *  shared_ptr<T>)
 */
template <
  template <class...> class smart_ptr
>
struct smart_ptr_traits { };

template <>
struct smart_ptr_traits<std::shared_ptr>
{
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

// Borrowed from boost implementation and
//   http://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
template <class T>
inline void
hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}


//template <typename... Iterators>
//struct zip_iterator
//{
//  typedef std::tuple<Iterators...> iter_tuple;
//  typedef std::tuple<typename std::iterator_traits<Iterators>::value_type...> value_type;
//  typedef std::tuple<typename std::iterator_traits<Iterators>::reference...> reference;
//
//  static reference
//  deref_impl(Iterators&&... iters) {
//    return std::make_tuple(*iters...);
//  }
//
//  reference
//  operator*() {
//    return darma_mockup::detail::splat_tuples<reference>(
//      &zip_iterator::deref_impl, std::forward<iter_tuple>(iters_)
//    );
//  }
//
//  static void
//  pre_incr_impl(Iterators&&... iters) {
//    ++iters...;
//  }
//
//  zip_iterator&
//  operator++() {
//    darma_mockup::detail::splat_tuples<void>(
//      &zip_iterator::pre_incr_impl, std::forward<iter_tuple>(iters_)
//    );
//    return *this;
//  }
//
//
//  iter_tuple iters_;
//
//};
//
//template <typename... Iterables>
//struct zip_container
//{
//  typedef
//
//
//
//};

} // end namespace detail

} // end namespace darma_runtime

// Add a hash of std::pair and std::tuple
namespace std {

template <typename U, typename V>
struct hash<std::pair<U,V>> {
  inline size_t
  operator()(const std::pair<U, V>& val) const {
    size_t rv = std::hash<U>()(val.first);
    darma_runtime::detail::hash_combine(rv, val.second);
    return rv;
  }
};

namespace _tup_hash_impl {
  template <size_t Spot, size_t Size, typename... Ts>
  struct _tup_hash_impl {
    inline size_t
    operator()(const std::tuple<Ts...>& tup) const {
      size_t rv = _tup_hash_impl<Spot+1, Size, Ts...>()(tup);
      hash_combine(rv, std::get<Spot>(tup));
      return rv;
    }
  };
  template <size_t Size, typename... Ts>
  struct _tup_hash_impl<Size, Size, Ts...> {
    inline size_t
    operator()(const std::tuple<Ts...>& tup) const {
      return 0;
    }
  };
}

template <typename... Ts>
struct hash<std::tuple<Ts...>> {
  inline size_t
  operator()(const std::tuple<Ts...>& tup) const {
    return _tup_hash_impl::_tup_hash_impl<0, sizeof...(Ts), Ts...>()(tup);
  }
};

} // end namespace std



#endif /* NEW_UTIL_H_ */
