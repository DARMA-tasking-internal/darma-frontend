/*
//@HEADER
// ************************************************************************
//
//                          delay.hpp
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

#ifndef META_TINYMPL_DELAY_HPP_
#define META_TINYMPL_DELAY_HPP_

#include <tinympl/detection.hpp>
#include <tinympl/identity.hpp>

namespace tinympl {

/**
 * \ingroup Functional
 * \class delay
 * \brief Delay not evaluate arguments to `F` until delay<...> is evaluated
 */
template <
  template <class...> class F,
  typename... Types
>
struct delay {
  typedef typename F<
    typename Types::type...
  >::type type;
};

template <
  template <class...> class F,
  typename... Types
>
struct delay_instantiate {
  using type = F<
    typename Types::type...
  >;
};

template <
  template <class...> class Delayed
>
struct undelay {
  template <typename... Args>
  struct apply {
    typedef typename Delayed<Args...>::type::type type;
  };
};

namespace _impl {

template <typename T>
using _is_lazy_value_suspension_archetype = decltype( T::type::value );

template <typename T>
using _has_member_value_archetype = decltype( T::value );

} // end namespace _impl

template <typename T>
using is_lazy_value_suspension = is_detected<
  _impl::_is_lazy_value_suspension_archetype, T>;

template <typename T>
using is_lazy_boolean_value_suspension = is_detected_convertible<bool,
  _impl::_is_lazy_value_suspension_archetype, T>;

template <typename T>
using has_member_value = is_detected<_impl::_has_member_value_archetype, T>;

template <typename T>
using has_member_value_convertible_to_bool =
  is_detected_convertible<bool, _impl::_has_member_value_archetype, T>;

template <typename T>
struct extract_value_potentially_lazy {
  private:
    using _type_to_evaluate =
      typename std::conditional_t<
        has_member_value<T>::value,
        identity<T>, T
      >::type;

  public:

    static constexpr auto value = _type_to_evaluate::value;

    using type = std::integral_constant<decltype(value), value>;

};

template <typename T>
struct extract_bool_value_potentially_lazy {
  private:
    using _type_to_evaluate = typename std::conditional_t<
      has_member_value_convertible_to_bool<T>::value,
      identity<T>, T
    >::type;

  public:

    static constexpr bool value = _type_to_evaluate::value;

    using type = std::integral_constant<bool, value>;
};

template <typename T>
struct extract_value_potentially_lazy_arbitrary_depth
{
  private:

    template <typename U>
    struct _get_type_to_eval {
      using type = typename std::conditional_t<
        has_member_value<U>::value,
        identity<U>, _get_type_to_eval<typename U::type>
      >::type;
    };

    using _type_to_evaluate = typename _get_type_to_eval<T>::type;

  public:

    static constexpr auto value = _type_to_evaluate::value;

    using type = std::integral_constant<decltype(value), value>;

};

template <typename T>
struct extract_bool_value_potentially_lazy_arbitrary_depth
{
  private:

    template <typename U>
    struct _get_type_to_eval {
      using type = typename std::conditional_t<
        has_member_value_convertible_to_bool<U>::value,
        identity<U>, _get_type_to_eval<typename U::type>
      >::type;
    };

    using _type_to_evaluate = typename _get_type_to_eval<T>::type;

  public:

    static constexpr bool value = _type_to_evaluate::value;

    using type = std::integral_constant<bool, value>;

};


} // end namespace tinympl



#endif /* META_TINYMPL_DELAY_HPP_ */
