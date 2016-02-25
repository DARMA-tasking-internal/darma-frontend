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

#include <tinympl/join.hpp>
#include <tinympl/at.hpp>
#include <tinympl/is_instantiation_of.hpp>
#include <tinympl/identity.hpp>
#include <tinympl/delay.hpp>
#include <tinympl/copy_traits.hpp>

namespace darma_runtime { namespace meta {

////////////////////////////////////////////////////////////////////////////////

/* generic tuple_for_each                                                {{{1 */ #if 1 // begin fold

namespace _tuple_for_each_impl {

namespace m = tinympl;

template <typename ReturnType, typename GenericLambda, bool IncludeIndex, size_t Index>
struct invoker {
  template <typename T>
  inline constexpr ReturnType
  operator()(T&& val, GenericLambda&& lambda) const {
    return lambda(std::forward<T>(val));
  }
};

template <typename ReturnType, typename GenericLambda, size_t Index>
struct invoker<ReturnType, GenericLambda, true, Index> {
  template <typename T>
  inline constexpr ReturnType
  operator()(T&& val, GenericLambda&& lambda) const {
    return lambda(std::forward<T>(val), Index);
  }
};


template <typename T, typename Like>
struct undecay_like { typedef T type; };

template <typename T, typename Like>
struct undecay_like<T, Like&> { typedef T& type; };

template <typename T, typename Like>
struct undecay_like<T, Like&&> { typedef T&& type; };

template <
  typename Tuple, typename GenericLambda,
  typename return_type, size_t I, typename next_t,
  typename i_return_t, typename TupleArg,
  bool IncludeIndex
>
struct _helper1 {
  constexpr inline return_type
  operator()(TupleArg&& tup, GenericLambda&& lambda) const && {
    typedef typename m::copy_cv_qualifiers<TupleArg>::template apply<
        typename m::copy_reference_type<TupleArg>::template apply<
          typename std::tuple_element<I, Tuple>::type
        >::type
    >::type qualified_element_t;
    return std::tuple_cat(
      std::tuple<i_return_t>(
        invoker<i_return_t, GenericLambda, IncludeIndex, I>()(
          std::forward<
            decltype(std::get<I>(std::forward<TupleArg>(tup)))
          >(
          //std::forward<qualified_element_t>(
            std::get<I>(std::forward<TupleArg>(tup))
          ),
          std::forward<GenericLambda>(lambda)
        )
      ),
      next_t()(
        std::forward<TupleArg>(tup),
        std::forward<GenericLambda>(lambda)
      )
    );
  }
};

template <
  typename Tuple, typename GenericLambda,
  size_t I, typename next_t, typename i_return_t,
  typename TupleArg, bool IncludeIndex
>
struct _helper1<Tuple, GenericLambda, void, I, next_t, i_return_t, TupleArg, IncludeIndex> {
  constexpr inline void
  operator()(TupleArg&& tup, GenericLambda&& lambda) const && {
    typedef typename m::copy_cv_qualifiers<TupleArg>::template apply<
        typename m::copy_reference_type<TupleArg>::template apply<
          typename std::tuple_element<I, Tuple>::type
        >::type
    >::type qualified_element_t;
    invoker<i_return_t, GenericLambda, IncludeIndex, I>()(
      std::forward<
        decltype(std::get<I>(std::forward<TupleArg>(tup)))
      >(
        std::get<I>(std::forward<TupleArg>(tup))
      ),
      std::forward<GenericLambda>(lambda)
    );
    next_t()(
      std::forward<TupleArg>(tup),
      std::forward<GenericLambda>(lambda)
    );
  }
};

template <typename T, typename... Args>
struct call_operator_return {
  typedef decltype(std::declval<T>()(
    std::declval<Args>()...
  )) type;
};

template <
  size_t I, size_t N,
  typename Tuple,
  typename GenericLambda,
  typename TupleArg,
  bool IncludeIndex = false,
  bool PrevIsVoid = false /* ignored for I=0 */
>
struct _tuple_for_each {
  static_assert(
    m::is_instantiation_of<std::tuple, Tuple>::value,
    "tuple_for_each only works on std::tuple"
  );


  typedef typename std::conditional<
    IncludeIndex,
    call_operator_return<GenericLambda,
      typename std::tuple_element<I, Tuple>::type, size_t
    >,
    call_operator_return<GenericLambda,
      typename std::tuple_element<I, Tuple>::type
    >
  >::type::type i_return_t;

  typedef _tuple_for_each<I+1, N, Tuple, GenericLambda,
      TupleArg, IncludeIndex, std::is_same<i_return_t, void>::value
  > next_t;

  typedef typename std::conditional<
    std::is_same<i_return_t, void>::value,
    m::identity<void>,
    m::delay<
      m::join,
      m::delay<std::tuple, i_return_t>,
      m::identity<typename next_t::return_t>
    >
  >::type::type return_t;

  static_assert(
    not (
      std::is_same<return_t, void>::value
      xor std::is_same<typename next_t::return_t, void>::value
    ),
    "tuple_for_each can't mix void and non-void return types"
  );

  constexpr inline return_t
  operator()(TupleArg&& tup, GenericLambda&& lambda) const && {
    return _helper1<
        Tuple, GenericLambda, return_t, I,
        next_t, i_return_t, TupleArg, IncludeIndex
    >()(
      std::forward<TupleArg>(tup),
      std::forward<GenericLambda>(lambda)
    );
  }

};

template <typename return_type>
struct _helper2 {
  constexpr inline return_type operator()() const && {
    return std::tuple<>();
  }
};

template <>
struct _helper2<void> {
  inline void operator()() const && { /* do nothing */ }
};

template <
  size_t N,
  typename Tuple,
  typename GenericLambda,
  typename TupleArg,
  bool IncludeIndex,
  bool PrevIsVoid
>
struct _tuple_for_each<N, N, Tuple, GenericLambda, TupleArg, IncludeIndex, PrevIsVoid> {
  static_assert(
    m::is_instantiation_of<std::tuple, Tuple>::value,
    "tuple_for_each only works on std::tuple"
  );

  typedef typename std::conditional<
    PrevIsVoid, void, std::tuple<>
  >::type return_t;


  constexpr inline return_t
  operator()(TupleArg&& tup, GenericLambda&& lambda) const && {
    return _helper2<return_t>()();
  }

};

} // end namespace _tuple_for_each_impl

// Note: calling with an empty tuple returns an empty tuple, not void

template <
  typename Tuple,
  typename GenericLambda
>
typename _tuple_for_each_impl::_tuple_for_each<
  0, std::tuple_size<typename std::decay<Tuple>::type>::value,
  typename std::decay<Tuple>::type,
  GenericLambda, Tuple
>::return_t
tuple_for_each(
  Tuple&& tup,
  GenericLambda&& lambda
) {
  typedef typename _tuple_for_each_impl::_tuple_for_each<
    0, std::tuple_size<typename std::decay<Tuple>::type>::value,
    typename std::decay<Tuple>::type,
    GenericLambda, Tuple
  > impl_t;
  return impl_t()(
    std::forward<Tuple>(tup),
    std::forward<GenericLambda>(lambda)
  );
}

template <
  typename Tuple,
  typename GenericLambda
>
typename _tuple_for_each_impl::_tuple_for_each<
  0, std::tuple_size<typename std::decay<Tuple>::type>::value,
  typename std::decay<Tuple>::type,
  GenericLambda, Tuple, true
>::return_t
tuple_for_each_with_index(
  Tuple&& tup,
  GenericLambda&& lambda
) {
  typedef typename _tuple_for_each_impl::_tuple_for_each<
    0, std::tuple_size<typename std::decay<Tuple>::type>::value,
    typename std::decay<Tuple>::type,
    GenericLambda, Tuple, true
  > impl_t;
  return impl_t()(
    std::forward<Tuple>(tup),
    std::forward<GenericLambda>(lambda)
  );
}

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////

}} // end namespace darma_runtime::meta


#endif /* META_TUPLE_FOR_EACH_H_ */
