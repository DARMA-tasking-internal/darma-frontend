/*
//@HEADER
// ************************************************************************
//
//                          tuple_for_each.h
//                         darma_mockup
//              Copyright (C) 2015 Sandia Corporation
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

#ifndef META_TUPLE_FOR_EACH_H_
#define META_TUPLE_FOR_EACH_H_

#include <tuple>
#include <utility>
#include <type_traits>

#include <darma/impl/compatibility.h>

#include <tinympl/join.hpp>
#include <tinympl/at.hpp>
#include <tinympl/is_instantiation_of.hpp>
#include <tinympl/identity.hpp>
#include <tinympl/delay.hpp>
#include <tinympl/repeat.hpp>
#include <tinympl/pop_back.hpp>
#include <tinympl/pop_front.hpp>
#include <tinympl/copy_traits.hpp>
#include <tinympl/always_true.hpp>
#include <tinympl/find_if.hpp>
#include <tinympl/min_element.hpp>
#include <tinympl/int.hpp>
#include <tinympl/zip.hpp>
#include <tinympl/tuple_as_sequence.hpp>
#include <tinympl/vector.hpp>
#include <tinympl/variadic/back.hpp>
#include <tinympl/splat.hpp>

#include "tuple_zip.h"
#include "splat_tuple.h"
#include "tuple_pop_back.h"

namespace darma_runtime {
namespace meta {

namespace _tuple_for_each_impl {

using std::conditional_t;
using std::tuple_element_t;
using std::is_same;
using std::integral_constant;
using std::tuple_size;
using std::remove_reference_t;
using std::forward;
using std::forward_as_tuple;

template <
  template <typename...> class Metafunction,
  size_t I, size_t N, bool CallWithIndex
>
struct _impl {

  using next_t = _impl<Metafunction, I+1, N, CallWithIndex>;

  //----------------------------------------
  // <editor-fold desc="_do_call_if_with_index">
  // does call differently depending on whether index is required for call

  template <typename T, typename Callable>
  inline decltype(auto)
  _do_arg_call_if_with_index(std::false_type, T&& arg, Callable&& f) const {
    return splat_tuple(
      forward<T>(arg),
      forward<Callable>(f)
    );
  }

  template <size_t... Is, typename T>
  inline constexpr auto
  _index_extended_tuple(std::index_sequence<Is...>, T&& arg) const {
    return std::forward_as_tuple(
      std::get<Is>(arg)..., I
    );
  };

  template <typename T, typename Callable>
  inline decltype(auto)
  _do_arg_call_if_with_index(std::true_type, T&& arg, Callable&& f) const {
    return splat_tuple(
      _index_extended_tuple(
        std::make_index_sequence<std::tuple_size<std::decay_t<T>>::value>(),
        std::forward<T>(arg)
      ),
      std::forward<Callable>(f)
    );
  }

  template <typename T, typename Callable>
  inline decltype(auto)
  _do_arg_call(T&& arg, Callable&& f) const {
    return _do_arg_call_if_with_index(integral_constant<bool, CallWithIndex>(),
      forward<T>(arg), forward<Callable>(f)
    );
  };

  // </editor-fold>
  //----------------------------------------

  //----------------------------------------
  // <editor-fold desc="_call_impl_if_return_void">
  // Tag dispatch based on whether call returns void, dispatches to _do_call_if_with_index and
  // invokes specialization for I+1

  // Callable returns void, so return void and don't forward any Args to next
  template <typename Tuple, typename Callable, typename... Args>
  inline void _call_impl_if_return_void(std::true_type,
    Tuple&& tup, Callable&& f, Args&&... args
  ) const {
    static_assert(sizeof...(args) == 0,
      "tuple_for_each variant invoked mixed void-returning and non-void-returning callable"
    );
    _do_arg_call(
      std::get<I>(std::forward<Tuple>(tup)), std::forward<Callable>(f)
    );
    next_t()(std::forward<Tuple>(tup), std::forward<Callable>(f));
  };

  // Callable does not return void, so we need to forward the return as part of args...
  template <typename Tuple, typename Callable, typename... Args>
  inline auto _call_impl_if_return_void(std::false_type,
    Tuple&& tup, Callable&& f, Args&&... args
  ) const {
    return next_t()(
      std::forward<Tuple>(tup), std::forward<Callable>(f), std::forward<Args>(args)...,
      _do_arg_call(
        std::get<I>(std::forward<Tuple>(tup)), std::forward<Callable>(f)
      )
    );
  };

  // </editor-fold>
  //----------------------------------------

  //----------------------------------------
  // <editor-fold desc="_call_impl_if_mfn">
  // Tag dispatch based on metafunction evaluation, invokes return void dispatch if true

  // Metafunction is true, so forward the Ith argument to the callable
  template <typename Tuple, typename Callable, typename... Args>
  inline auto _call_impl_if_mfn(std::true_type,
    Tuple&& tup, Callable&& f, Args&&... args
  ) const {
    return _call_impl_if_return_void(
      std::integral_constant<bool,
        std::is_same<
          decltype(this->_do_arg_call(std::get<I>(forward<Tuple>(tup)),
            forward<Callable>(f)
          )), void
        >::value
      >(),
      forward<Tuple>(tup), forward<Callable>(f),
      forward<Args>(args)...
    );
  }

  // Metafunction is false, so don't forward the Ith argument
  template <typename Tuple, typename Callable, typename... Args>
  inline auto _call_impl_if_mfn(std::false_type,
    Tuple&& tup, Callable&& f, Args&&... args
  ) const {
    return next_t()(
      forward<Tuple>(tup), forward<Callable>(f),
      forward<Args>(args)...
    );
  }

  // </editor-fold>
  //----------------------------------------

  // Forward to tag dispatch based on metafunction evaluation
  template <typename Tuple, typename Callable, typename... Args>
  inline auto operator()(Tuple&& tup, Callable&& f, Args&&... args) const {
    return _call_impl_if_mfn(
      std::integral_constant<bool, tinympl::splat_to_t<
        remove_reference_t<tuple_element_t<I, remove_reference_t<Tuple>>>,
        Metafunction
      >::value>(),
      forward<Tuple>(tup), forward<Callable>(f), forward<Args>(args)...
    );
  }

};

template <template <typename...> class Metafunction, size_t N, bool CallWithIndex>
struct _impl<Metafunction, N, N, CallWithIndex> {

  template <typename Tuple, typename Callable>
  inline void _call_impl_if_return_void(std::true_type, Tuple&&, Callable&&) const {
    /* do nothing */
  }

  template <typename Tuple, typename Callable, typename... Args>
  inline auto _call_impl_if_return_void(std::false_type,
    Tuple&&, Callable&&, Args&&... args
  ) const {
    return std::forward_as_tuple(forward<Args>(args)...);
  };

  template <typename Tuple, typename Callable, typename... Args>
  inline auto operator()(Tuple&& tup, Callable&& f, Args&&... args) const {
    return _call_impl_if_return_void(
      integral_constant<bool,
        sizeof...(args) == 0
          // if the tuple is zero-sized, return std::tuple<> rather than void
          and tuple_size<remove_reference_t<Tuple>>::value != 0
      >(),
      forward<Tuple>(tup), forward<Callable>(f), forward<Args>(args)...
    );
  }
};

// Always return an empty tuple in the 0 args case
template <template <typename...> class Metafunction, bool CallWithIndex>
struct _impl<Metafunction, 0, 0, CallWithIndex> {
  template <typename Tuple, typename Callable>
  inline auto operator()(Tuple&& tup, Callable&& f) const {
    return std::forward_as_tuple();
  }
};

template <
  template <typename...> class Metafunction, bool WithIndex,
  typename Callable, typename ArgsTup, size_t... Is
>
auto _zipped_impl(
  std::index_sequence<Is...>,
  Callable&& callable,
  ArgsTup&& args_tup
) {
  return _impl<Metafunction, 0,
    std::tuple_size<std::decay_t<
      std::tuple_element_t<0, std::decay_t<ArgsTup>>
    >>::value,
    WithIndex
  >()(
    tuple_zip(std::get<Is>(args_tup)...), std::forward<Callable>(callable)
  );
};

} // end namespace _tuple_for_each_impl

template <
  template <typename...> class Metafunction, bool WithIndex,
  typename... Args
>
auto tuple_for_each_zipped_general(
  Args&&... args
) {
  return _tuple_for_each_impl::_zipped_impl< Metafunction, WithIndex >(
    std::make_index_sequence<sizeof...(Args) - 1>(),
    std::get<sizeof...(Args)-1>(std::forward_as_tuple(std::forward<Args>(args)...)),
    std::forward_as_tuple(std::forward<Args>(args)...)
  );
};

template <typename... Args>
auto tuple_for_each_zipped(
  Args&&... args
) {
  return tuple_for_each_zipped_general<
    tinympl::always_true, false
  >(std::forward<Args>(args)...);
}

template <typename... Args>
auto tuple_for_each_zipped_with_index(
  Args&&... args
) {
  return tuple_for_each_zipped_general<
    tinympl::always_true, true
  >(std::forward<Args>(args)...);
}

// Note: calling with an empty tuple returns an empty tuple, not void
template <
  template <typename...> class UnaryMetafunction, bool WithIndex = false,
  typename Tuple, typename Callable
>
auto tuple_for_each_filtered_type(
  Tuple&& tup,
  Callable&& callable
) {
  return tuple_for_each_zipped_general<UnaryMetafunction, WithIndex>(
    std::forward<Tuple>(tup), std::forward<Callable>(callable)
  );
}

template <typename Tuple, typename Callable>
auto tuple_for_each(
  Tuple&& tup,
  Callable&& callable
) {
  return tuple_for_each_filtered_type<tinympl::always_true>(
    std::forward<Tuple>(tup), std::forward<Callable>(callable)
  );
};


template <
  template <typename...> class UnaryMetafunction,
  typename Tuple, typename Callable
>
auto
tuple_for_each_filtered_type_with_index(
  Tuple&& tup,
  Callable&& callable
) {
  return tuple_for_each_filtered_type<tinympl::always_true, true>(
    std::forward<Tuple>(tup), std::forward<Callable>(callable)
  );
};

template < typename Tuple, typename Callable >
auto
tuple_for_each_with_index(
  Tuple&& tup, Callable&& callable
) {
  return tuple_for_each_filtered_type_with_index<tinympl::always_true>(
    std::forward<Tuple>(tup), std::forward<Callable>(callable)
  );
};

} // end namespace meta
} // end namespace darma_runtime

#endif /* META_TUPLE_FOR_EACH_H_ */
