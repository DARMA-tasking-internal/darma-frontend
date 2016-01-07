/*
//@HEADER
// ************************************************************************
//
//                          filter_types.h
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

#ifndef META_FILTER_TYPES_H_
#define META_FILTER_TYPES_H_

#include <tuple>

namespace dharma_mockup {
namespace detail {

////////////////////////////////////////////////////////////////////////////////

/* filter_types                                                 {{{1 */ #if 1 // begin fold

namespace impl {

template<
  bool check_for_tuple,
  template <typename T> class metafunction,
  typename AccumResult,
  typename... Args
>
struct filter_types_helper { };

template<
  template <typename T> class metafunction, typename Arg1
>
struct filter_types_tuple_helper { };

template<
  template <typename T> class metafunction,
  template <typename...> class tuple_template,
  typename... Args
>
struct filter_types_tuple_helper<
  metafunction, tuple_template<Args...>
> {
  typedef typename filter_types_helper<
      false, metafunction, std::tuple<>, Args...
  >::type type;
};

template<
  bool check_for_tuple,
  template <typename T> class metafunction,
  typename AccumResult,
  typename Arg1,
  typename... Args
>
struct filter_types_helper<
  check_for_tuple, metafunction,
  AccumResult, Arg1, Args...
> {
  typedef typename std::conditional<
    check_for_tuple and is_tuple<Arg1>::value,
    filter_types_tuple_helper<metafunction, Arg1>,
    filter_types_helper<
      false, metafunction,
      typename std::conditional<
        metafunction<Arg1>::type::value,
        typename tuple_append<AccumResult, Arg1>::type,
        AccumResult
      >::type, Args...
    >
  >::type::type type;
};

template<
  bool check_for_tuple,
  template <typename T> class metafunction,
  typename AccumResult
>
struct filter_types_helper<
  check_for_tuple, metafunction,
  AccumResult
> {
  typedef AccumResult type;
};

} // end namespace impl


template<
  template <typename T> class metafunction,
  typename... Args
>
struct filter_types {
  typedef typename impl::filter_types_helper<
      sizeof...(Args) == 1,
      metafunction, std::tuple<>,
      Args...
  >::type type;
};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////

}} // end namespace dharma_mockup::detail

#endif /* META_FILTER_TYPES_H_ */
