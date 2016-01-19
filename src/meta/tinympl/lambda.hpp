/*
//@HEADER
// ************************************************************************
//
//                           lambda.hpp
//                         dharma_mockup
//              Copyright (C) 2015 Sandia Corporation
// This file was adapted from its original form in the tinympl library.
// The original file bore the following copyright:
//   Copyright (C) 2013, Ennio Barbaro.
// See LEGAL.md for more information.
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


#ifndef TINYMPL_LAMBDA_HPP
#define TINYMPL_LAMBDA_HPP

#include "bind.hpp"
#include "contains_placeholder.hpp"
#include <type_traits>
#include "identity.hpp"
#include "is_suspension_of.hpp"

namespace tinympl {

/**
 * \ingroup BindAndLambda
 * @{
 */



template<class Expr>
struct lambda
{
  template<class... Ts>
  struct eval
  {
    template<class T, class Enable=void>
    struct pick {
        typedef T type;
    };

    template<class T>
    struct pick<T,
      typename std::enable_if< (is_placeholder<T>::type::value > 0)>::type
    > {
        typedef variadic::at_t<is_placeholder<T>::value-1, Ts...> type;
    };

    template<class T>
    struct pick<T,
    typename std::enable_if< is_bind_expression<T>::type::value>::type
    > {
        typedef typename T::template eval<Ts...>::type type;
    };

    typedef typename pick<Expr>::type type;
  };

  template<class... Ts> using eval_t = typename eval<Ts...>::type;
  template<class... Ts> using apply = eval<Ts...>;
};

template<
  template<class...> class F,
  class... Args
>
struct lambda<F<Args...> >
{
  template<class ... Ts>
  struct eval
  {
    template <class T>
    using forward_t = typename std::conditional<
      contains_placeholder<T>::value,
      typename lambda<T>::template eval<Ts...>,
      identity<T>
    >::type::type;

    typedef typename std::conditional<
      is_suspension_of<F, F<forward_t<Args>...>>::value,
      F< forward_t<Args>... >,
      identity<F<forward_t<Args>...>>
    >::type::type type;
  };

  template<class ... Ts> using eval_t = typename eval<Ts...>::type;
  template<class ... Ts> using apply = eval<Ts...>;
};

// Shouldn't be needed anymore
//template<class T>
//struct protect {
//  typedef T type;
//};
//template <
//  template <class ...> class F,
//  class ... Args
//>
//struct lambda< protect<F<Args...> > >
//{
//	template<class ... Ts>
//	struct eval
//	{
//		template<class T> using forward_t = typename T::template eval<Ts...>::type;
//
//		typedef F< forward_t< lambda<protect<Args> > > ... > type;
//	};
//
//	template<class ... Ts> using eval_t = typename eval<Ts...>::type;
//};

template<class Expr> struct is_bind_expression<lambda<Expr> > : std::true_type {};

/** @} */

}

#endif // TINYMPL_LAMBDA_HPP
