/*
//@HEADER
// ************************************************************************
//
//                          metaprogramming.h
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

#ifndef META_METAPROGRAMMING_H_
#define META_METAPROGRAMMING_H_

#include <type_traits>
#include <tuple>
#include <functional>
#include <limits>

#include "tinympl/identity.hpp"

#define DHARMA_DETAIL_make_get_member_type_metafunction(memtype) \
  template <typename T> \
  struct get_##memtype##_metafunction { \
    typedef typename T::memtype type; \
  };

#include "../meta/move_if.h"

// TODO dharma_tuple type so user tuple types don't get interpreted as bundles in e.g. transform or fold
// TODO replace std::tuple with generic tuple type derived from extract_template in places where it makes sense

namespace dharma_mockup { namespace detail {

// void_t for detection idiom; should be wrapped in #if's for C++17 compatibility
// Also doesn't actually work with pure C++11 I don't think
template <class...>
using void_t = void;

// Used in metafunctions to mark ends of things and stuff
struct sentinal_type_ {
  constexpr sentinal_type_() { }
};

namespace impl {

template <typename T,
  int Spot, int ToGet, int Size, T... Vals
>
struct constant_metalist_get_helper;

template <typename T,
  int Spot, int ToGet, int Size, T Val, T... Vals
>
struct constant_metalist_get_helper<
  T, Spot, ToGet, Size, Val, Vals...
>
{
  using sfinae_helper = typename std::enable_if<Spot != ToGet>::type;
  static constexpr T value = constant_metalist_get_helper<
      T, Spot+1, ToGet, Size, Vals...
  >::value;
};

template <typename T,
  int Spot, int Size, T Val, T... Vals
>
struct constant_metalist_get_helper<T, Spot, Spot, Size, Val, Vals...>
{
  static constexpr T value = Val;
};

} // end namespace impl

template <typename T, T... vals>
struct constant_metalist
{
  template <int N>
  static constexpr T get() {
    return impl::constant_metalist_get_helper<
        T, 0, N, sizeof...(vals), vals...
    >::value;
  }

  static constexpr size_t size = sizeof...(vals);
};

template <int... vals>
using int_metavector = constant_metalist<int, vals...>;
template <int... vals>
using size_t_metavector = constant_metalist<size_t, vals...>;

template <typename... lists>
struct join_metalists;

template <typename list1, typename list2, typename list3, typename... lists>
struct join_metalists<
  list1, list2, list3, lists...
>{
  typedef typename join_metalists<
      typename join_metalists<list1, list2>::type, list3, lists...
  >::type type;
};

template <
  typename T,
  T... vals1,
  T... vals2
>
struct join_metalists<
  constant_metalist<T, vals1...>,
  constant_metalist<T, vals2...>
>
{
  typedef constant_metalist<T, vals1..., vals2...> type;
};

template <typename list1>
struct join_metalists<list1> { typedef list1 type; };

template <typename T>
struct join_metalist_tuple;

template <
  typename... metalists
>
struct join_metalist_tuple<
  std::tuple<metalists...>
> {
  typedef typename join_metalists<metalists...>::type type;
};

static constexpr size_t
first_index_not_found = std::numeric_limits<size_t>::max();

namespace impl {

template <
  size_t Spot, size_t Size,
  typename T, T Val,
  typename mlist
>
struct _first_index_of
{
  typedef typename std::conditional<
      mlist::template get<Spot>() == Val,
      std::integral_constant<size_t, Spot>,
      typename std::conditional<
        Spot <= Size-1,
        _first_index_of<Spot+1, Size, T, Val, mlist>,
        std::integral_constant<size_t, first_index_not_found>
      >::type
  >::type __conditional_checker;
  static constexpr size_t value = __conditional_checker::value;
};


} // end namespace impl


template <
  typename T, T Val,
  typename mlist
>
struct first_index_of
{
  static constexpr size_t value =
      impl::_first_index_of<
        0, mlist::size,
        T, Val, mlist
      >::value;
};


namespace impl {

template <
  size_t remaining,
  typename T, T val,
  typename AccumVal
>
struct _repeated_metalist
{
  typedef typename _repeated_metalist<
    remaining-1, T, val,
    typename join_metalists<AccumVal, constant_metalist<T, val>>::type
  >::type type;
};

template <
  typename T, T val,
  typename AccumVal
>
struct _repeated_metalist<
  0, T, val, AccumVal
>
{
  typedef AccumVal type;
};

} // end namespace impl

template <
  typename T, T val,
  size_t repeats
>
struct repeated_metalist
{
  typedef typename impl::_repeated_metalist<
      repeats, T, val, constant_metalist<T>
  >::type type;

};


////////////////////////////////////////////////////////////////////////////////
/* tuple_append                                                          {{{1 */ #if 1 // begin fold

template<int N, typename Tuple, typename... Args>
struct tuple_append_helper
{
  typedef typename tuple_append_helper<
      N-1, Tuple, typename std::tuple_element<N, Tuple>::type, Args...
  >::type type;
};

template<typename Tuple, typename... Args>
struct tuple_append_helper<-1, Tuple, Args...>
{
  typedef typename std::tuple<Args...> type;
};

template<typename Tuple, typename T>
struct tuple_append
{
  typedef typename tuple_append_helper<
      int(std::tuple_size<Tuple>::value) - 1,
      Tuple, T
  >::type type;
};

/*                                                                       }}}1 */ #endif // end fold
////////////////////////////////////////////////////////////////////////////////

template <typename... Tuples>
struct tuple_cat_type {
  typedef decltype(std::tuple_cat(std::declval<Tuples>()...)) type;
};

template <typename T>
struct extract_template;

template <
  template <typename...> class wrapper,
  typename... Args
>
struct extract_template<
  wrapper<Args...>
>
{
  template <typename... NewArgs>
  struct apply
  {
    typedef wrapper<NewArgs...> type;
  };

};

////////////////////////////////////////////////////////////////////////////////
/* identity_metafunction                                                 {{{1 */ #if 1 // begin fold

template <typename T>
using identity_metafunction = tinympl::identity<T>;

/*                                                                            */ #endif // end fold
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/* matches_pattern                                                       {{{1 */ #if 1 // begin fold

template <
  template <typename...> class pattern,
  typename T
>
struct matches_pattern
  : public std::false_type
{ };

template <
  template <typename...> class pattern,
  typename... pattern_args
>
struct matches_pattern<pattern, pattern<pattern_args...>>
  : public std::true_type
{ };

template <typename T>
using is_tuple = matches_pattern<std::tuple, T>;

template <template <typename...> class pattern>
struct make_matches_pattern_metafunction
{
  template <typename T>
  struct apply
    : public matches_pattern<pattern, T>
  { };
};

/*                                                                            */ #endif // end fold
////////////////////////////////////////////////////////////////////////////////

}} // end namespace dharma_mockup::detail

#include "../meta/filter_types.h"

#include "../meta/indices_where.h"

namespace dharma_mockup { namespace detail {


////////////////////////////////////////////////////////////////////////////////
/* transform                                                             {{{1 */ #if 1 // begin fold

namespace impl {

template<
  bool check_for_tuple,
  template <typename...> class metafunction,
  typename AccumResult,
  typename... Args
>
struct transform_helper { };

template<
  template <typename...> class metafunction, typename Arg1
>
struct transform_tuple_helper { };

template<
  template <typename...> class metafunction,
  template <typename...> class tuple_template,
  typename... Args
>
struct transform_tuple_helper<
  metafunction, tuple_template<Args...>
> {
  typedef typename transform_helper<
      false, metafunction, std::tuple<>, Args...
  >::type type;
};

template<
  bool check_for_tuple,
  template <typename...> class metafunction,
  typename AccumResult,
  typename Arg1,
  typename... Args
>
struct transform_helper<
  check_for_tuple, metafunction,
  AccumResult, Arg1, Args...
> {
  typedef typename std::conditional<
    check_for_tuple and is_tuple<Arg1>::value,
    identity_metafunction<void>, /* __insert_type is ignored in this case */
    metafunction<Arg1>
  >::type::type __insert_type;

  typedef typename std::conditional<
    check_for_tuple and is_tuple<Arg1>::value,
    transform_tuple_helper<metafunction, Arg1>,
    transform_helper<
      false, metafunction,
      typename tuple_append<AccumResult,
        __insert_type
      >::type, Args...
    >
  >::type::type type;
};

template<
  bool check_for_tuple,
  template <typename...> class metafunction,
  typename AccumResult
>
struct transform_helper<
  check_for_tuple, metafunction,
  AccumResult
> {
  typedef AccumResult type;
};

} // end namespace impl

template<
  template <typename...> class metafunction,
  typename... Args
>
struct transform {
  typedef typename impl::transform_helper<
      sizeof...(Args) == 1,
      metafunction, std::tuple<>,
      Args...
  >::type type;
};

/*                                                                            */ #endif // end fold
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/* apply_metafunction_to_types_matching_template                         {{{1 */ #if 1 // begin fold

DHARMA_DETAIL_make_get_member_type_metafunction(value_t)
DHARMA_DETAIL_make_get_member_type_metafunction(tag)
DHARMA_DETAIL_make_get_member_type_metafunction(type)

template<
  template <typename...> class with_template,
  template <typename...> class metafunction,
  typename... Args
>
struct apply_metafunction_to_types_matching_template
{
  typedef typename transform<
    metafunction,
    typename filter_types<
      make_matches_pattern_metafunction<with_template>::template apply,
      Args...
    >::type
  >::type type;
};

/*                                                                       1}}} */ #endif // end fold
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/* splat_tuples                                                          {{{1 */ #if 1 // begin fold


namespace impl {

template <
  int Spot, int Size,
  typename ResultType,
  typename FunctionType,
  typename JoinedTuple,
  typename... Splatted
>
struct _splat_tuples
{
  typedef _splat_tuples<
    Spot+1, Size,
    ResultType, FunctionType,
    JoinedTuple, Splatted...,
    typename std::tuple_element<Spot, JoinedTuple>::type
  > next_helper_t;


  ResultType
  operator()(
    FunctionType&& function,
    JoinedTuple&& tup,
    Splatted&&... splatted
  ) const
  {
    return next_helper_t()(
        std::forward<FunctionType>(function),
        std::forward<JoinedTuple>(tup),
        std::forward<Splatted>(splatted)...,
        std::forward<
          typename std::tuple_element<Spot, JoinedTuple>::type
        >(std::get<Spot>(std::forward<JoinedTuple>(tup)))
    );

  }

};

template <
  int Spot,
  typename ResultType,
  typename FunctionType,
  typename JoinedTuple,
  typename... Splatted
>
struct _splat_tuples<
  Spot, Spot, ResultType,
  FunctionType, JoinedTuple,
  Splatted...
>
{
  ResultType
  operator()(
    FunctionType&& function,
    JoinedTuple&& tup,
    Splatted&&... splatted
  ) const
  {
    return meta::move_if_not_lvalue_reference<FunctionType&&>()(function)(
        std::forward<Splatted>(splatted)...
    );
  }

};


} // end namespace impl


template <
  typename ResultType,
  typename FunctionType,
  typename... TuplesToSplat
>
ResultType
splat_tuples(
    FunctionType&& function,
    TuplesToSplat&&... tuples
)
{
  typedef decltype(
      std::tuple_cat(std::forward<TuplesToSplat>(tuples)...)
  ) joined_tuple_t;

  return impl::_splat_tuples<
      0, std::tuple_size<joined_tuple_t>::value,
      ResultType, FunctionType, joined_tuple_t
  >()(
      std::forward<FunctionType>(function),
      std::tuple_cat(
          std::forward<TuplesToSplat>(tuples)...
      )
  );
}


/*                                                                            */ #endif // end fold
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/* splat_tuples_to_positions                                             {{{1 */ #if 1 // begin fold


namespace impl {

template <
  int Spot, int Size,
  typename PositionsList,
  typename ResultType, typename FunctionType,
  typename JoinedTuple,
  typename... Splatted
>
struct _splat_tuples_positions
{
  static constexpr size_t index_of_spot_arg
    = first_index_of<
        size_t, Spot, PositionsList
    >::value;
  static_assert(index_of_spot_arg != size_t(-1), "huh?");
  typedef typename std::tuple_element<
      index_of_spot_arg, JoinedTuple
  >::type spot_arg_t;

  typedef _splat_tuples_positions<
      Spot+1, Size, PositionsList,
      ResultType, FunctionType,
      JoinedTuple,
      Splatted..., spot_arg_t
  > next_helper_t;

  ResultType
  operator()(
      FunctionType&& function,
      JoinedTuple&& tup,
      Splatted&&... splatted
  ) const
  {
    return next_helper_t()(
        std::forward<FunctionType>(function),
        std::forward<JoinedTuple>(tup),
        std::forward<Splatted>(splatted)...,
        std::forward<spot_arg_t>(
            std::get<index_of_spot_arg>(
                std::forward<JoinedTuple>(tup)
            )
        )
    );
  }

};

template <
  int Size,
  typename PositionsList,
  typename ResultType, typename FunctionType,
  typename JoinedTuple,
  typename... Splatted
>
struct _splat_tuples_positions<
  Size, Size, PositionsList,
  ResultType, FunctionType,
  JoinedTuple, Splatted...
>
{
  using sfinae_check = typename std::enable_if<
      not matches_pattern<std::pair, FunctionType>::value
  >::type;

  ResultType
  operator()(
      FunctionType&& function,
      JoinedTuple&& tup,
      Splatted&&... splatted
  ) const
  {
    return function(std::forward<Splatted>(splatted)...);
  }

};

template <
  int Size,
  typename PositionsList,
  typename ResultType,
  typename InstanceType,
  typename MemberFunctionPointer,
  typename JoinedTuple,
  typename... Splatted
>
struct _splat_tuples_positions<
  Size, Size, PositionsList,
  ResultType, std::pair<InstanceType, MemberFunctionPointer>,
  JoinedTuple, Splatted...
>
{
  ResultType
  operator()(
      std::pair<InstanceType, MemberFunctionPointer> function_pair,
      JoinedTuple&& tup,
      Splatted&&... splatted
  ) const
  {
    return std::mem_fn(function_pair.second)(
        function_pair.first, std::forward<Splatted>(splatted)...
    );
  }
};

} // end namespace impl

template <
  typename ResultType,
  /* TODO this should probably be an analog of mpl::vector */
  typename PositionListsTuple,
  typename FunctionType,
  typename... TuplesToSplat
>
ResultType
splat_tuples_to_positions(
    FunctionType&& function,
    TuplesToSplat&&... tuples
)
{
  typedef decltype(
      std::tuple_cat(std::forward<TuplesToSplat>(tuples)...)
  ) joined_tuple_t;


  return impl::_splat_tuples_positions<
      0, std::tuple_size<joined_tuple_t>::value,
      typename join_metalist_tuple<PositionListsTuple>::type,
      ResultType, FunctionType, joined_tuple_t
  >()(
      std::forward<FunctionType>(function),
      std::tuple_cat(
          std::forward<TuplesToSplat>(tuples)...
      )
  );
}



/*                                                                            */ #endif // end fold
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/* iter_tuple_casting_element_pointers                                   {{{1 */ #if 1 // begin fold

template <typename T>
struct pointer_to_value
{
  T* operator()(T& val) const { return &val; }
};

template <typename T>
struct pointer_to_value<T*>
{
  T*& operator()(T*& val) const { return val; }
};

// Four versions with pointers/non-pointers in various places, so avoid
// some copy-pasting by standardizing the template params
template <typename TArg,  typename TypeToCast, typename Functor>
struct do_functor_with_pointer_cast_helper_impl
{
  static_assert(std::is_base_of<TypeToCast,
      typename std::remove_pointer<typename std::decay<TArg>::type>::type>::value,
      "iter_tuple_casting_element_pointers() must cast to a base (for now)");
  void operator()(TArg& val, Functor&& f) const {
    f((TypeToCast*)pointer_to_value<TArg>()(val));
  }
};

template <typename T, typename TypeToCast, typename Functor>
struct do_functor_with_pointer_cast_helper
 : public do_functor_with_pointer_cast_helper_impl<T, TypeToCast, Functor>
{ };

template <typename T, typename TypeToCast, typename Functor>
struct do_functor_with_pointer_cast_helper<T, TypeToCast*, Functor>
  : public do_functor_with_pointer_cast_helper_impl<T, TypeToCast, Functor>
{ };

template <typename T, typename TypeToCast, typename Functor>
struct do_functor_with_pointer_cast_helper<T*, TypeToCast*, Functor>
  : public do_functor_with_pointer_cast_helper_impl<T*, TypeToCast, Functor>
{ };

template <typename T, typename TypeToCast, typename Functor>
struct do_functor_with_pointer_cast_helper<T*, TypeToCast, Functor>
  : public do_functor_with_pointer_cast_helper_impl<T*, TypeToCast, Functor>
{ };

template <int Spot, int Size, typename TupleType, typename TypeToCast, typename Functor>
struct iter_tuple_casting_helper
{
  void operator()(TupleType& tup, Functor&& f) const {
    do_functor_with_pointer_cast_helper<
      typename std::tuple_element<Spot, TupleType>::type, TypeToCast, Functor
    >()(
        std::get<Spot>(tup), std::forward<Functor>(f)
    );
    iter_tuple_casting_helper<
      Spot+1, Size, TupleType, TypeToCast, Functor
    >()(tup, std::forward<Functor>(f));
  }
};

// base case
template <int Size, typename TupleType, typename TypeToCast, typename Functor>
struct iter_tuple_casting_helper<Size, Size, TupleType, TypeToCast, Functor>
{
  void operator()(TupleType& tup, Functor&& f) const { /* intentionally empty */ }
};


// note: return of functor is ignored
template <typename TypeToCast, typename TupleType, typename Functor>
void iter_tuple_casting_element_pointers(TupleType& tup, Functor&& f)
{
  iter_tuple_casting_helper<
    0, std::tuple_size<TupleType>::value, TupleType, TypeToCast, Functor
  >()(tup, std::forward<Functor>(f));
}

/*                                                                            */ #endif // end fold
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
/* fold_metafunction                                                     {{{1 */ #if 1 // begin fold

namespace impl {

template <
  int Spot, int Size,
  template <typename, typename> class metafunction,
  typename AccumVal,
  typename Tuple
>
struct _fold_metafunction
{
  typedef typename _fold_metafunction<
      Spot+1, Size, metafunction,
      typename metafunction<
          AccumVal,
          typename std::tuple_element<Spot, Tuple>::type
      >::type,
      Tuple
  >::type type;
};

template <
  int Size,
  template <typename, typename> class metafunction,
  typename AccumVal,
  typename Tuple
>
struct _fold_metafunction<
  Size, Size, metafunction,
  AccumVal, Tuple
> {
  typedef AccumVal type;
};

} // end namespace impl

// Applies metafunction<InitialValue, T1> to the first type in Tuple,
// then applies metafunction<Result, T2> to the second type in Tuple,
// where Result is metafunction<InitialValue, T1>::type,
// and so on.
template <
  template <typename, typename> class metafunction,
  typename Tuple, typename InitialValue
>
struct fold_metafunction {
  typedef typename impl::_fold_metafunction<
      0, std::tuple_size<Tuple>::value,
      metafunction, InitialValue, Tuple
  >::type type;

};

/*                                                                            */ #endif // end fold
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/* tuple_flatten                                                         {{{1 */ #if 1 // begin fold

//template <typename Tuple>
//struct tuple_flatten
//{
//  typedef Tuple type;
//
//};

/*                                                                            */ #endif // end fold
////////////////////////////////////////////////////////////////////////////////

}} // end namespace dharma_mockup::detail

namespace dharma_mockup {

/**
 * For our purposes, a type T is contiguous if a pointer to
 * a char[size] can be validly reinterpret-cast into a pointer
 * to an instance of the type.  For instance, at least the following
 * snippet must be valid (though this isn't sufficient since it
 * also assumes that any pointer members remain valid across
 * shared memory boundaries):
 *   size_t size = ... // get size somehow
 *   T val_a = ... // initialize val_a
 *   char* data = reinterpret_cast<char*>(&val_a);
 *   char* other_data = new char[size];
 *   memcpy(other_data, data, size);
 *   T& val_b = *reinterpret_cast<T*>(other_data);
 *   ... // use val_b just like you would use val_a
 *
 * Defaults to false, specialize to have a type treated as a contiguous blob
 */
// in other words, a memcpy is a deep copy
template <typename T>
struct is_contiguous
{
  typedef typename std::remove_reference<T>::type U;
  typedef typename std::remove_all_extents<U>::type V;
  typedef typename std::conditional<
        std::is_same<T, V>::value,
        std::integral_constant<
          bool,
          // logic goes here
          std::is_fundamental<T>::value
        >,
        // else, recurse
        is_contiguous<V>
  >::type type;
  static constexpr bool value = type::value;
};

// TODO move this to some meta-test suite
static_assert(is_contiguous<double[13][278]>::value,
    "is_contiguous doesn't work as expected");

namespace detail {

template <typename T>
struct dereference_value
{
  auto operator()(T&& val) -> decltype(*val)
  {
    return *val;
  }
};

template <
  template <typename...> class metafunction,
  typename... unevaluated
>
struct meta_apply_to_unevaluated
{
  typedef typename metafunction<
    typename identity_metafunction<unevaluated>::type...
  >::type type;
};

template <
  template <typename...> class wrapper,
  typename... unevaluated
>
struct meta_evaluate_and_wrap
{
  typedef wrapper<
    typename unevaluated::type...
  > type;
};

}} // end namespace dharma_mockup::detail


#include "../meta/tuple_for_each.h"

#endif /* META_METAPROGRAMMING_H_ */
