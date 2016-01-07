/*
//@HEADER
// ************************************************************************
//
//                          indices_where.h
//                         dharma_mockup
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

#ifndef META_INDICES_WHERE_H_
#define META_INDICES_WHERE_H_

#include <tuple>
#include <type_traits>

// TODO remove this when metaprogramming.h is fully split up
#include "../meta/metaprogramming.h" // join_metalists, size_t_metavector

namespace dharma_mockup { namespace detail {

////////////////////////////////////////////////////////////////////////////////
/* indices_where                                                         {{{1 */ #if 1 // begin fold

namespace impl {

template <
  bool check_for_tuple,
  size_t Spot, size_t N,
  template <typename T> class metafunction,
  typename AccumResult,
  typename... Args
>
struct _indices_where { };

template <
  template <typename T> class metafunction,
  typename AccumResult,
  typename Tuple
>
struct _indices_where_tuple;

template <
  template <typename T> class metafunction,
  typename AccumResult,
  typename... TupleArgs
>
struct _indices_where_tuple<
  metafunction, AccumResult,
  std::tuple<TupleArgs...>
>
{
  typedef typename _indices_where<
    false, 0, sizeof...(TupleArgs),
    metafunction, AccumResult,
    TupleArgs...
  >::type type;
};

template <
  bool check_for_tuple,
  size_t Spot, size_t N,
  template <typename T> class metafunction,
  typename AccumResult,
  typename Arg1,
  typename... Args
>
struct _indices_where<
  check_for_tuple, Spot, N, metafunction,
  AccumResult, Arg1, Args...
> {
  typedef typename std::conditional<
      check_for_tuple and is_tuple<Arg1>::value,
      _indices_where_tuple<metafunction, AccumResult, Arg1>,
      _indices_where<false, Spot+1, N, metafunction,
        typename std::conditional<
          metafunction<Arg1>::value,
          typename join_metalists<AccumResult, size_t_metavector<Spot>>::type,
          AccumResult
        >::type,
        Args...
      >
  >::type::type type;
};

template <
  bool check_for_tuple,
  size_t N,
  template <typename T> class metafunction,
  typename AccumResult
>
struct _indices_where<
  check_for_tuple, N, N, metafunction,
  AccumResult
> {
  typedef AccumResult type;
};


} // end namespace impl


template<
  template <typename T> class metafunction,
  typename... Args
>
struct indices_where {
  typedef typename impl::_indices_where<
      sizeof...(Args) == 1, 0, sizeof...(Args),
      metafunction, size_t_metavector<>,
      Args...
  >::type type;
};

/*                                                                            */ #endif // end fold
////////////////////////////////////////////////////////////////////////////////

}} // end namespace dharma_mockup::detail


#endif /* META_INDICES_WHERE_H_ */
