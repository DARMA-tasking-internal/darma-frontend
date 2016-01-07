/*
//@HEADER
// ************************************************************************
//
//                          tuple_for_each.h
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

#ifndef META_TUPLE_FOR_EACH_H_
#define META_TUPLE_FOR_EACH_H_

#include <tuple>
#include <utility>

namespace dharma_mockup { namespace meta {

////////////////////////////////////////////////////////////////////////////////

/* tuple_for_each                                                        {{{1 */ #if 1 // begin fold


template <
  template <class, class...> class FunctorTemplate,
  typename TupleType
>
struct tuple_for_each
{ /* intentionally empty */ };


namespace _impl {

template<
  int Spot, int Size,
  template <class> class FunctorTemplate,
  template <class...> class TupleTemplate,
  typename... Types
>
struct _tuple_for_each
{
  typedef TupleTemplate<Types...> input_tuple_t;
  typedef _tuple_for_each<
      Spot+1, Size, FunctorTemplate, TupleTemplate, Types...
  > next_helper_t;
  typedef decltype(
    FunctorTemplate<
      typename std::tuple_element<Spot, input_tuple_t>::type
    >()(
      std::declval<
        typename std::tuple_element<Spot, input_tuple_t>::type
      >()
    )
  ) this_part_result_t;
  typedef decltype(
    std::tuple_cat(
      std::declval<TupleTemplate<this_part_result_t>>(),
      std::declval<typename next_helper_t::partial_result_t>()
    )
  ) partial_result_t;

  partial_result_t
  operator()(input_tuple_t&& tup) const
  {
    return std::tuple_cat(
        TupleTemplate<this_part_result_t>(
          FunctorTemplate<typename std::tuple_element<Spot, input_tuple_t>::type>()(
              std::get<Spot>(std::forward<input_tuple_t>(tup))
          )
        ),
        next_helper_t()(std::forward<input_tuple_t>(tup))
    );
  }

};

template<
  int Size,
  template <class> class FunctorTemplate,
  template <class...> class TupleTemplate,
  typename... Types
>
struct _tuple_for_each<
  Size, Size, FunctorTemplate, TupleTemplate, Types...
>
{
  typedef TupleTemplate<Types...> input_tuple_t;
  typedef TupleTemplate<> partial_result_t;
  partial_result_t
  operator()(input_tuple_t&& tup) const
  {
    return TupleTemplate<>();
  }

};

} // end namespace _impl

template <
  template <class> class FunctorTemplate,
  template <class...> class TupleTemplate,
  typename... Types
>
struct tuple_for_each<
  FunctorTemplate, TupleTemplate<Types...>
>
{
  typedef TupleTemplate<Types...> input_tuple_t;
  typedef _impl::_tuple_for_each<
      0, sizeof...(Types),
      FunctorTemplate, TupleTemplate,
      Types...
  > helper_t;
  typedef typename helper_t::partial_result_t result_tuple_t;

  result_tuple_t
  operator()(input_tuple_t&& tup) const
  {
    return helper_t()(
        std::forward<input_tuple_t>(tup)
    );
  }
};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* tuple_for_each_templated                                              {{{1 */ #if 1 // begin fold


template <
  typename Functor,
  typename TupleType
>
struct tuple_for_each_templated
{ /* intentionally empty */ };


namespace _impl {

template<
  int Spot, int Size,
  typename Functor,
  template <class...> class TupleTemplate,
  typename... Types
>
struct _tuple_for_each_templated
{
  typedef TupleTemplate<Types...> input_tuple_t;
  typedef _tuple_for_each_templated<
      Spot+1, Size, Functor, TupleTemplate, Types...
  > next_helper_t;

  void
  operator()(Functor& functor, input_tuple_t& tup) const
  {
    functor.template operator()<
      typename std::tuple_element<Spot, input_tuple_t>::type,
      Spot
    >(
        std::get<Spot>(tup)
    );
    next_helper_t()(
        functor,
        tup
    );
  }

};

template<
  int Size,
  typename Functor,
  template <class...> class TupleTemplate,
  typename... Types
>
struct _tuple_for_each_templated<
  Size, Size, Functor, TupleTemplate, Types...
>
{
  typedef TupleTemplate<Types...> input_tuple_t;
  void
  operator()(Functor& functor, input_tuple_t& tup) const
  { }

};

} // end namespace _impl

template <
  typename StatefulFunctor,
  template <class...> class TupleTemplate,
  typename... Types
>
struct tuple_for_each_templated<
  StatefulFunctor, TupleTemplate<Types...>
>
{
  typedef TupleTemplate<Types...> input_tuple_t;
  typedef _impl::_tuple_for_each_templated<
      0, sizeof...(Types),
      StatefulFunctor, TupleTemplate,
      Types...
  > helper_t;

  void
  operator()(StatefulFunctor& functor, input_tuple_t& tup) const
  {
    helper_t()(
        functor,
        tup
    );
  }
};



/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////

}} // end namespace dharma_mockup/detail


#endif /* META_TUPLE_FOR_EACH_H_ */
