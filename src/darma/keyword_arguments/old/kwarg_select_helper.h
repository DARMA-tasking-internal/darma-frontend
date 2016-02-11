/*
//@HEADER
// ************************************************************************
//
//                          kwarg_select_helper.h
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

#ifndef KEYWORD_ARGUMENTS_KWARG_SELECT_HELPER_H_
#define KEYWORD_ARGUMENTS_KWARG_SELECT_HELPER_H_


namespace darma_runtime { namespace detail {

//////////////////////////////////////////////////////////////////////////////////
//
///* kwarg_select_helper                                                   {{{1 */ #if 1 // begin fold
//
//static constexpr size_t
//argument_position_keyword_only = std::numeric_limits<size_t>::max() - 1;
//
///* Empty (unspecialized) cases for helpers        {{{2 */ #if 2 // begin fold
//
//template<
//  size_t PositionalOffset, size_t PositionsChecked, bool FirstKeywordFound,
//  bool ArgumentAlreadyFound, typename Tag, typename RVType,
//  typename... Args
//>
//struct kwsel_handle_kwarg;
//
//template<
//  size_t PositionalOffset, size_t PositionsChecked, bool FirstKeywordFound,
//  bool ArgumentAlreadyFound, typename Tag, typename RVType,
//  typename... Args
//>
//struct kwsel_detect_kw_or_pos;
//
//template<
//  size_t PositionalOffset, size_t PositionsChecked, bool FirstKeywordFound,
//  bool ArgumentAlreadyFound, typename Tag, typename RVType,
//  typename Arg1, typename... Args
//>
//struct kwsel_handle_positional;
//
///*******************************************************/ #endif //2}}}
//
///* The outer wrapper called to get value for Tag  {{{2 */ #if 2 // begin fold
//
//template<typename Tag, typename RVType, size_t PositionalOffset, typename... Args>
//struct kwarg_select_helper {
//
//  typedef kwsel_detect_kw_or_pos<
//      PositionalOffset, 0, false, false, Tag, RVType, Args...
//  > selected_helper_t;
//
//  static constexpr bool selected_arg_is_lvalue =
//      selected_helper_t::selected_arg_is_lvalue;
//
//  typedef typename selected_helper_t::actual_rv_t actual_rv_t;
//
//  inline actual_rv_t
//  operator()(Args&&... args) const {
//    return meta::move_if_not_lvalue_reference<actual_rv_t&&>()(
//      kwsel_detect_kw_or_pos<
//        PositionalOffset, 0, false, false, Tag, RVType, Args...
//      >()(std::forward<Args>(args)...)
//    );
//  }
//
//};
//
///*******************************************************/ #endif //2}}}
//
///* kwsel_detect_kw_or_pos                         {{{2 */ #if 2 // begin fold
//
//template<
//  size_t PositionalOffset, size_t PositionsChecked, bool FirstKeywordFound,
//  bool ArgumentAlreadyFound, typename Tag, typename RVType,
//  typename Arg1, typename... Args
//>
//struct kwsel_detect_kw_or_pos<
//  PositionalOffset, PositionsChecked, FirstKeywordFound,
//  ArgumentAlreadyFound, Tag, RVType,
//  Arg1, Args...
//>
//{
//  typedef typename std::conditional<
//    is_kwarg_expression<Arg1>::value,
//    kwsel_handle_kwarg<
//      PositionalOffset, PositionsChecked, FirstKeywordFound,
//      ArgumentAlreadyFound, Tag, RVType, Arg1, Args...
//    >,
//    kwsel_handle_positional<
//      PositionalOffset, PositionsChecked, FirstKeywordFound,
//      ArgumentAlreadyFound, Tag, RVType, Arg1, Args...
//    >
//  >::type selected_helper_t;
//
//  static constexpr bool selected_arg_is_lvalue =
//      selected_helper_t::selected_arg_is_lvalue;
//
//  typedef typename selected_helper_t::no_duplicates_found_t no_duplicates_found_t;
//
//  typedef typename selected_helper_t::actual_rv_t actual_rv_t;
//
//  inline actual_rv_t
//  operator()(
//    Arg1&& a1,
//    Args&&... args
//  ) const
//  {
//    return meta::move_if_not_lvalue_reference<actual_rv_t&&>()(
//      selected_helper_t()(
//        std::forward<Arg1>(a1),
//        std::forward<Args>(args)...
//      )
//    );
//  }
//};
//
//////////////////////////////////////////
//// The fail/base case
//
//// Makes compilation output a little more readable
//template<typename Tag, bool __see_backtrace_for_missing_keyword_name_and_function__>
//struct missing_required_keyword_argument {
//  static_assert(__see_backtrace_for_missing_keyword_name_and_function__,
//    "missing required keyword argument"
//  );
//};
//
//// The no default case.  The argument is mandatory and not found, so it's an error
//template<bool has_default, typename Tag, typename RVType>
//struct kwsel_default_value_helper
//  : missing_required_keyword_argument<Tag, false>
//{
//  // obviously there are no duplicates, since there isn't even a first one
//  typedef std::true_type no_duplicates_found_t;
//  // Nothing selected, so it's not an lvalue
//  static constexpr bool selected_arg_is_lvalue = false;
//  typedef RVType actual_rv_t;
//  // Define a call operator that does nothing so that the
//  // correct error is displayed (if this isn't here, the user might
//  // get a cryptic "doesn't define a call operator" error
//  // TODO silence potential warning on some compilers about no return using #pragmas ?
//  RVType operator()() const { /* intentionally empty */ }
//};
//
//// We do have a default, so use that
//template<typename Tag, typename RVType>
//struct kwsel_default_value_helper<true, Tag, RVType>
//{
//  typedef std::true_type no_duplicates_found_t;
//  static constexpr bool selected_arg_is_lvalue = false;
//  typedef RVType actual_rv_t;
//  RVType operator()() const {
//    return detail::tag_data<Tag>::get_default_value();
//  }
//};
//
///* if there are no Args... and we haven't returned, then
// * we've reached the end of the arguments without finding
// * the keyword argument (or finding it again, in the 'check'
// * case)
// */
//template<
//  size_t PositionalOffset, size_t PositionsChecked, bool FirstKeywordFound,
//  typename Tag, typename RVType
//>
//struct kwsel_detect_kw_or_pos<
//  PositionalOffset, PositionsChecked, FirstKeywordFound,
//  false, Tag, RVType
//>
//{
//  typedef kwsel_default_value_helper<
//      detail::tag_data<Tag>::has_default_value,
//      Tag, RVType
//  > default_val_helper_t;
//
//  // obviously there are no duplicates, since there isn't even a first one
//  typedef std::true_type no_duplicates_found_t;
//
//  static constexpr bool selected_arg_is_lvalue =
//      default_val_helper_t::selected_arg_is_lvalue;
//
//  typedef typename default_val_helper_t::actual_rv_t actual_rv_t;
//  // Define a call operator that does nothing so that the
//  // correct error is displayed (if this isn't here, the user might
//  // get a cryptic "doesn't define a call operator" error
//  RVType operator()() const {
//    return default_val_helper_t()();
//  }
//};
//
//
//template<
//  size_t PositionalOffset, size_t PositionsChecked, bool FirstKeywordFound,
//  typename Tag, typename RVType
//>
//struct kwsel_detect_kw_or_pos<
//  PositionalOffset, PositionsChecked, FirstKeywordFound,
//  true, Tag, RVType
//>
//{
//  // We got to the end without a no_duplicates_found_t being
//  // typedef'd to false_type, so we didn't find any duplicates
//  typedef std::true_type no_duplicates_found_t;
//  // Call operator should never be invoked and code calling it
//  // should never be generated, so we're safe to omit it here
//  // This is needed to generate the return types on the ArgumentAlreadyFound = true
//  // versions of the functors, even though they should never be called
//  static constexpr bool selected_arg_is_lvalue = false;
//  typedef RVType actual_rv_t;
//
//};
//
//////////////////////////////////////////
//
///*******************************************************/ #endif //2}}}
//
///* kwsel_handle_kwarg                             {{{2 */ #if 2 // begin fold
//
///* the keyword argument, tag matches case */
//template<
//  size_t PositionalOffset, size_t PositionsChecked, bool FirstKeywordFound,
//  bool ArgumentAlreadyFound, typename Tag, typename RVType, typename KWValType,
//  typename KWNameValType, bool kwarg_rhs_is_lvalue,
//  typename... Args
//>
//struct kwsel_handle_kwarg<
//  PositionalOffset, PositionsChecked, FirstKeywordFound,
//  ArgumentAlreadyFound, Tag, RVType,
//  kwarg_expression<KWValType,
//    keyword_argument_name<KWNameValType, Tag, typename tag_data<Tag>::keyword_catagory_t>,
//    kwarg_rhs_is_lvalue
//  >,
//  Args...
//>
//{
//  typedef kwarg_expression<KWValType,
//    keyword_argument_name<KWNameValType, Tag, typename tag_data<Tag>::keyword_catagory_t>,
//    kwarg_rhs_is_lvalue
//  > KWArg1;
//
//  typedef kwsel_detect_kw_or_pos<
//      PositionalOffset, PositionsChecked+1, true,
//      /* ArgumentAlreadyFound = */ true, Tag, RVType, Args...
//  > check_next_arg_t;
//
//  static constexpr bool selected_arg_is_lvalue = KWArg1::rhs_is_lvalue;
//
//  // Define no_duplicates_found_t as false_type if
//  // we already found this argument, since this is a duplicate
//  typedef typename std::conditional<
//      ArgumentAlreadyFound,
//      std::false_type,
//      typename check_next_arg_t::no_duplicates_found_t
//  >::type no_duplicates_found_t;
//
//  // We've found KWArg1 to be an expression of the argument, so assert that
//  // the value isn't also given elsewhere in the argument list
//  static_assert(
//      no_duplicates_found_t::value,
//      "value for keyword argument given more than once (note that first value may"
//      " have been deduced from a positional argument)"
//  );
//
//  typedef typename std::conditional<
//    selected_arg_is_lvalue,
//    RVType&, RVType
//  >::type actual_rv_t;
//
//  inline actual_rv_t
//  operator()(
//    KWArg1&& a1,
//    Args&&... args
//  ) const
//  {
//    return meta::move_if_not_lvalue_reference<actual_rv_t>()(
//        std::forward<actual_rv_t>(a1.value())
//    );
//  }
//};
//
///* the keyword argument, tag doesn't match case */
//template<
//  size_t PositionalOffset, size_t PositionsChecked, bool FirstKeywordFound,
//  bool ArgumentAlreadyFound, typename Tag, typename RVType,
//  typename KWValueType, typename KWArg1Name,
//  bool kwarg1_rhs_is_lvalue,
//  typename... Args
//>
//struct kwsel_handle_kwarg<
//  PositionalOffset, PositionsChecked, FirstKeywordFound,
//  ArgumentAlreadyFound, Tag, RVType,
//  kwarg_expression<KWValueType, KWArg1Name, kwarg1_rhs_is_lvalue>,
//  Args...
//>
//{
//  typedef kwarg_expression<KWValueType, KWArg1Name, kwarg1_rhs_is_lvalue
//  > KWArg1;
//
//  typedef kwsel_detect_kw_or_pos<
//      PositionalOffset, PositionsChecked+1, true,
//      ArgumentAlreadyFound, Tag, RVType, Args...
//  > check_next_arg_t;
//
//  typedef typename check_next_arg_t::no_duplicates_found_t no_duplicates_found_t;
//
//  static constexpr bool selected_arg_is_lvalue =
//      check_next_arg_t::selected_arg_is_lvalue;
//
//  typedef typename check_next_arg_t::actual_rv_t actual_rv_t;
//
//  inline actual_rv_t
//  operator()(
//    KWArg1&& a1,
//    Args&&... args
//  ) const
//  {
//    return meta::move_if_not_lvalue_reference<actual_rv_t>()(
//      check_next_arg_t()(
//        std::forward<Args>(args)...
//      )
//    );
//  }
//};
//
///*******************************************************/ #endif //2}}}
//
///* The positional argument case                   {{{2 */ #if 2 // begin fold
//
///* yet another helper, this time for selecting the positional
// * argument conditional on whether we've gone beyond the offset
// */
//template<bool enabled,
//  size_t PositionalOffset, size_t PositionsChecked,
//  bool ArgumentAlreadyFound, typename Tag, typename RVType,
//  typename PosArgType, typename... Args
//>
//struct kwarg_positional_select_helper
//{ /* intentionally empty */ };
//
//
//
///* passthrough to positional select helper */
//template<
//  size_t PositionalOffset, size_t PositionsChecked,
//  bool ArgumentAlreadyFound, typename Tag, typename RVType,
//  typename Arg1, typename... Args
//>
//struct kwsel_handle_positional<
//  PositionalOffset, PositionsChecked, false,
//  ArgumentAlreadyFound, Tag, RVType, Arg1, Args...
//>
//{
//  /* The conditional expression in the first argument of
//   * the helper typedef can be understood as follows:
//   * We don't want to select this positional argument if
//   * we're below the offset (i.e. it's already been used
//   * for another argument) or if we've already found this
//   * argument as an earlier positional argument (in which
//   * case we're just checking for duplicates, and a positional
//   * argument can never be a duplicate of an earlier positional
//   * argument)
//   */
//  typedef kwarg_positional_select_helper<
//    PositionalOffset <= PositionsChecked and not ArgumentAlreadyFound,
//    PositionalOffset, PositionsChecked,
//    ArgumentAlreadyFound, Tag, RVType, Arg1, Args...
//  > positional_helper_t;
//
//  static constexpr bool selected_arg_is_lvalue =
//      positional_helper_t::selected_arg_is_lvalue;
//
//  typedef typename positional_helper_t::no_duplicates_found_t no_duplicates_found_t;
//
//  typedef typename positional_helper_t::actual_rv_t actual_rv_t;
//
//  inline actual_rv_t
//  operator()(
//    Arg1&& a1,
//    Args&&... args
//  ) const
//  {
//    return meta::move_if_not_lvalue_reference<actual_rv_t>()(
//      std::forward<actual_rv_t>(
//        positional_helper_t()(
//          std::forward<Arg1>(a1),
//          std::forward<Args>(args)...
//        )
//      )
//    );
//  }
//
//};
//
//
//// the enabled case: offset <= positions checked
//// Note that ArgumentAlreadyFound must always
//// be false here because of the condition imposed in
//// the passthrough wrapper's typedef above
//template<
//  size_t PositionalOffset, size_t PositionsChecked,
//  typename Tag, typename RVType,
//  typename PosArgType, typename... Args
//>
//struct kwarg_positional_select_helper<true,
//  PositionalOffset, PositionsChecked,
//  /* ArgumentAlreadyFound = */ false, Tag, RVType,
//  PosArgType, Args...
//>
//{
//  typedef kwsel_detect_kw_or_pos<
//    PositionalOffset, PositionsChecked+1, false,
//    /* ArgumentAlreadyFound = */ true, Tag, RVType, Args...
//  > check_next_arg_t;
//
//  // Even though this is
//  typedef typename check_next_arg_t::no_duplicates_found_t no_duplicates_found_t;
//
//  // We've found PosArgType to be an expression of the argument, so assert that we
//  //   haven't already found it and that it isn't in any of the later arguments either
//  static_assert(no_duplicates_found_t::value,
//      "value for keyword argument given more than once (first value "
//      " was deduced from a positional argument)"
//  );
//
//  static constexpr bool selected_arg_is_lvalue =
//      std::is_lvalue_reference<PosArgType&&>::value;
//
//  static constexpr bool arg_type_needs_conversion =
//      not std::is_base_of<
//        typename std::decay<RVType>::type,
//        typename std::decay<PosArgType>::type
//      >::value;
//
//  typedef typename std::conditional<
//    selected_arg_is_lvalue and not arg_type_needs_conversion,
//    RVType&, typename std::conditional<arg_type_needs_conversion,
//        RVType, RVType&&
//    >::type
//  >::type actual_rv_t;
//
//
//  inline actual_rv_t
//  operator()(
//    PosArgType&& p1,
//    Args&&... args
//  ) const noexcept
//  {
//    return actual_rv_t(meta::by_value_if_or_move_if<
//        arg_type_needs_conversion, not selected_arg_is_lvalue, PosArgType
//    >()(
//        std::forward<PosArgType>(p1)
//    ));
//  }
//
//};
//
//
///* the disabled case: offset > positions checked (i.e., positional argument already used),
// *   just move on to the next argument/kwarg
// */
//template<
//  size_t PositionalOffset, size_t PositionsChecked,
//  bool ArgumentAlreadyFound, typename Tag, typename RVType,
//  typename PosArgType, typename... Args
//>
//struct kwarg_positional_select_helper<false,
//  PositionalOffset, PositionsChecked,
//  ArgumentAlreadyFound, Tag, RVType,
//  PosArgType, Args...
//>
//{
//  typedef kwsel_detect_kw_or_pos<
//    PositionalOffset, PositionsChecked+1, false,
//    ArgumentAlreadyFound, Tag, RVType, Args...
//  > check_next_arg_t;
//
//  typedef typename check_next_arg_t::no_duplicates_found_t no_duplicates_found_t;
//
//  static constexpr bool selected_arg_is_lvalue =
//      check_next_arg_t::selected_arg_is_lvalue;
//
//  typedef typename check_next_arg_t::actual_rv_t actual_rv_t;
//
//  inline actual_rv_t
//  operator()(
//    PosArgType&& p1,
//    Args&&... args
//  ) const
//  {
//    return meta::move_if_not_lvalue_reference<actual_rv_t>()(
//      check_next_arg_t()(std::forward<Args>(args)...)
//    );
//  }
//};
//
//
//////////////////////////////////////////
//// The fail case, for when a positional
//// argument is given after the first
//// keyword argument
//
//// Makes compilation output a little more readable
//template<typename Tag, bool __see_below_for_details__>
//struct positional_argument_given_after_first_keyword_argument {
//  static_assert(__see_below_for_details__,
//    "positional argument given after first keyword argument"
//  );
//};
//
//// positional argument, but after first keyword argument
//// note that we still want to fail here even if the argument
//// has already been found, since this will cause a failure later anyway
//template<
//  size_t PositionalOffset, size_t PositionsChecked,
//  bool ArgumentAlreadyFound, typename Tag, typename RVType,
//  typename Arg1, typename... Args
//>
//struct kwsel_handle_positional<
//  PositionalOffset, PositionsChecked, true,
//  ArgumentAlreadyFound, Tag, RVType, Arg1, Args...
//> : positional_argument_given_after_first_keyword_argument<Tag, false>
//{
//  // Typedef this as true_type so that the correct error gets thrown
//  // This doesn't mean that there are actually no duplicates; the
//  // duplicates error(s) will show up after the out-of-order positional
//  // issue is resolved
//  typedef std::true_type no_duplicates_found_t;
//  // Just define this here so that we get the right type of error
//  static constexpr bool selected_arg_is_lvalue = false;
//  // Likewise, define a call operator that does nothing so that the
//  // correct error is displayed (if this isn't here, the user might
//  // get a cryptic "doesn't define a call operator" error
//  // TODO silence potential warning on some compilers about no return using #pragmas ?
//  RVType operator()(Arg1&&, Args&&...) const { /* intentionally empty */ }
//};
//
//////////////////////////////////////////
//
///*******************************************************/ #endif //2}}}
//
///* collect_positional_arguments                   {{{2 */ #if 2 // begin fold
//
//namespace _impl {
//
//template <
//  bool current_is_kwarg /* = false */,
//  size_t Spot, size_t Size,
//  typename AllArgsTuple
//>
//struct _collect_positional
//{
//  typedef _collect_positional<
//    std::conditional<
//      Spot <= Size-2,
//      tinympl::delay<
//        is_kwarg_expression,
//        std::tuple_element<Spot+1, AllArgsTuple>
//      >,
//      // if there's nothing left, just go to the base case
//      std::true_type
//    >::type::type::value,
//    Spot+1, Size,
//    AllArgsTuple
//  > next_helper_t;
//
//  typedef typename tinympl::join<
//    std::tuple<
//      typename std::tuple_element<Spot, AllArgsTuple>::type
//    >,
//    typename next_helper_t::return_t
//  >::type return_t;
//
//  inline return_t
//  operator()(AllArgsTuple&& all_args) const
//  {
//    return std::tuple_cat(
//      std::forward_as_tuple(
//        std::get<Spot>(
//          std::forward<AllArgsTuple>(all_args)
//        )
//      ),
//      next_helper_t()(
//        std::forward<AllArgsTuple>(all_args)
//      )
//    );
//  }
//};
//
//// Base case
//template <
//  size_t Spot, size_t Size,
//  typename AllArgsTuple
//>
//struct _collect_positional<
//  /* bool current_is_kwarg = */ true,
//  Spot, Size, AllArgsTuple
//>
//{
//  typedef std::tuple<> return_t;
//
//  inline return_t
//  operator()(AllArgsTuple&& all_args) const
//  {
//    return std::tuple<>();
//  }
//};
//
//} // end namespace impl
//
//template <typename... Args>
//struct collect_positional_arguments
//{
//  typedef _impl::_collect_positional<
//    std::conditional<
//      sizeof...(Args) >= 1,
//      tinympl::delay<
//        is_kwarg_expression,
//        std::tuple_element<0,
//          std::tuple<Args...>
//        >
//      >,
//      // if there's nothing left, just go to the base case
//      std::true_type
//    >::type::type::value,
//    0, sizeof...(Args),
//    std::tuple<Args...>
//  > helper_t;
//};
//
///*******************************************************/ #endif //2}}}
//
//
///*                                                                       }}}1 */ #endif // end fold
//
//////////////////////////////////////////////////////////////////////////////////

}} // end namespace darma_mockup::detail


#endif /* KEYWORD_ARGUMENTS_KWARG_SELECT_HELPER_H_ */
