/*
//@HEADER
// ************************************************************************
//
//                      compressed_pair.h
//                         DARMA
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

#ifndef DARMA_UTILITY_COMPRESSED_PAIR_H
#define DARMA_UTILITY_COMPRESSED_PAIR_H

#include <tinympl/select.hpp>
#include <tinympl/logical_and.hpp>
#include <tinympl/logical_not.hpp>
#include <tinympl/variadic/at.hpp>
#include <tinympl/detection.hpp>

#include <darma/utility/compatibility.h>

#include <type_traits>
#include <utility>
#include <tuple>
#include <cstdlib>

namespace darma_runtime {

namespace utility {

template <typename T1, typename T2>
class compressed_pair;

namespace _impl {

template <typename T1, typename T2>
class compressed_pair_typedefs {
  private:

    using first_const_reference = std::add_lvalue_reference_t<
      std::add_const_t<T1>
    >;
    using second_const_reference = std::add_lvalue_reference_t<
      std::add_const_t<T2>
    >;

  protected:

};

template <typename T1, bool IsEmpty>
class compressed_pair_part_impl;

template <typename T1>
class compressed_pair_part_impl<T1, /* IsEmpty = */ false> {
  public:

    using first_type = T1;

  protected:

    T1 t1_;

    template <typename... Args,
      typename=std::enable_if_t<
        // exclude copy and move ctors, etc.
        not std::is_same<compressed_pair_part_impl,
          std::remove_cv_t<std::remove_reference_t<
            tinympl::variadic::at_or_t<tinympl::nonesuch, 0, Args...>
          >>
        >::value
      >
    >
    compressed_pair_part_impl(Args&&... args)
      : t1_(std::forward<Args>(args)...)
    { }

  private:

    using reference = std::add_lvalue_reference_t<T1>;
    using const_reference = std::add_lvalue_reference_t<std::add_const_t<T1>>;

  protected:

    reference _get() { return t1_; }
    const_reference _get() const { return t1_; }

};

template <typename T1>
class compressed_pair_part_impl<T1, /* IsEmpty = */ true>
  : protected std::remove_cv_t<T1>
{
  protected:

    template <typename... Args,
      typename=std::enable_if_t<
        // exclude copy and move ctors, etc.
        not std::is_same<compressed_pair_part_impl,
          std::remove_cv_t<std::remove_reference_t<
            tinympl::variadic::at_or_t<tinympl::nonesuch, 0, Args...>
          >>
        >::value
      >
    >
    compressed_pair_part_impl(Args&&... args)
      : T1(std::forward<Args>(args)...)
    { }

  private:

    using reference = std::add_lvalue_reference_t<T1>;
    using const_reference = std::add_lvalue_reference_t<std::add_const_t<T1>>;

  protected:

    reference _get() { return *this; }
    const_reference _get() const { return *this; }

};

template <typename T>
class compressed_pair_first_part_impl
  : protected compressed_pair_part_impl<T, std::is_empty<T>::value>
{
  private:

    using base_t = compressed_pair_part_impl<T, std::is_empty<T>::value>;

  public:

    using first_type = T;

  protected:

    using first_reference = std::add_lvalue_reference_t<first_type>;
    using first_const_reference = std::add_lvalue_reference_t<
      std::add_const_t<first_type>
    >;

  public:

    first_reference first() {
      return this->base_t::_get();
    }
    first_const_reference first() const {
      return this->base_t::_get();
    }


  protected:

    using base_t::base_t;
};

template <typename T>
class compressed_pair_second_part_impl
  : protected compressed_pair_part_impl<T, std::is_empty<T>::value>
{
  private:

    using base_t = compressed_pair_part_impl<T, std::is_empty<T>::value>;

  public:

    using second_type = T;

  protected:

    using second_reference = std::add_lvalue_reference_t<second_type>;
    using second_const_reference = std::add_lvalue_reference_t<
      std::add_const_t<second_type>
    >;

  public:

    second_reference second() {
        return this->base_t::_get();
    }
    second_const_reference second() const {
        return this->base_t::_get();
    }

  protected:

    using base_t::base_t;
};


template <typename T1, typename T2>
class compressed_pair_impl
  : public compressed_pair_first_part_impl<T1>,
    public compressed_pair_second_part_impl<T2>
{
  protected:

    using _first_base_t = compressed_pair_first_part_impl<T1>;
    using _second_base_t = compressed_pair_second_part_impl<T2>;

    template <typename U1, typename U2>
    using enable_conversion_ctor_if_t = std::enable_if_t<
      std::is_convertible<U1, T1>::value
        and std::is_convertible<U2, T2>::value
        and not (
          std::is_same<U1, typename _first_base_t::first_const_reference>::value
            and std::is_same<U2, typename _second_base_t::second_const_reference>::value
        )
    >;

    template <typename U1, typename U2>
    using enable_pair_conversion_ctor_if_t = std::enable_if_t<
      std::is_convertible<U1, T1>::value
        and std::is_convertible<U2, T2>::value
    >;

    template <typename U1, typename U2>
    using enable_copy_conversion_assignment_op_if_t = std::enable_if_t<
      std::is_assignable<T1&, U1 const&>::value
        and std::is_assignable<T2&, U2 const&>::value
        and not (
          std::is_same<T1, U1>::value
            and std::is_same<T2, U2>::value
        )
    >;

    template <typename U1, typename U2>
    using enable_move_conversion_assignment_op_if_t = std::enable_if_t<
      std::is_assignable<T1&, U1&&>::value
        and std::is_assignable<T2&, U2&&>::value
        and not (
          std::is_same<T1, U1>::value
            and std::is_same<T2, U2>::value
        )
    >;

    template <class... Args1, class... Args2, std::size_t... Idxs1, std::size_t... Idxs2>
    compressed_pair_impl(
      std::piecewise_construct_t,
      std::tuple<Args1...>& first_args,
      std::tuple<Args2...>& second_args,
      std::integer_sequence<std::size_t, Idxs1...>,
      std::integer_sequence<std::size_t, Idxs2...>
    ) : _first_base_t(std::get<Idxs1>(first_args)...),
        _second_base_t(std::get<Idxs2>(second_args)...)
    { }

  public:

    inline constexpr
    compressed_pair_impl()
      : _first_base_t(), _second_base_t()
    { }

    inline DARMA_CONSTEXPR_14
    compressed_pair_impl(
      typename _first_base_t::first_const_reference v1,
      typename _second_base_t::second_const_reference v2
    )
      : _first_base_t(v1), _second_base_t(v2)
    { }

    template <typename U1, typename U2,
      typename=enable_conversion_ctor_if_t<U1, U2>
    >
    inline DARMA_CONSTEXPR_14
    compressed_pair_impl(U1&& u1, U2&& u2)
      : _first_base_t(std::forward<U1>(u1)), _second_base_t(std::forward<U2>(u2))
    { }

    template <typename U1, typename U2,
      typename=enable_pair_conversion_ctor_if_t<U1, U2>
    >
    inline DARMA_CONSTEXPR_14
    compressed_pair_impl(compressed_pair_impl<U1, U2> const& p)
      : _first_base_t(p.first()), _second_base_t(p.second())
    { }

    template <typename U1, typename U2,
      typename=enable_pair_conversion_ctor_if_t<U1, U2>
    >
    inline DARMA_CONSTEXPR_14
    compressed_pair_impl(compressed_pair_impl<U1, U2>&& p)
      : _first_base_t(std::move<U1>(p.first())), _second_base_t(std::move<U2>(p.second()))
    { }

    template <class... Args1, class... Args2>
    inline
    compressed_pair_impl(
      std::piecewise_construct_t _pc,
      std::tuple<Args1...> first_args,
      std::tuple<Args2...> second_args
    ) : compressed_pair_impl(
          _pc, first_args, second_args,
          std::index_sequence_for<Args1...>(),
          std::index_sequence_for<Args2...>()
        )
    { }

};


} // end namespace _impl


template <typename T1, typename T2>
class compressed_pair
  : private _impl::compressed_pair_impl<T1, T2>
{
  private:

    using base_t = _impl::compressed_pair_impl<T1, T2>;

  public:

    using base_t::base_t;
    using base_t::first;
    using base_t::second;


    inline DARMA_CONSTEXPR_14 compressed_pair() = default;
    inline DARMA_CONSTEXPR_14 compressed_pair(compressed_pair const&) = default;
    inline DARMA_CONSTEXPR_14 compressed_pair(compressed_pair&&) = default;

    inline DARMA_CONSTEXPR_14
    compressed_pair& operator=(compressed_pair const&) = default;
    inline DARMA_CONSTEXPR_14
    compressed_pair& operator=(compressed_pair&&) = default;

    template <typename U1, typename U2,
      typename=typename base_t
        ::template enable_copy_conversion_assignment_op_if_t<U1, U2>
    >
    inline DARMA_CONSTEXPR_14
    compressed_pair& operator=(
      compressed_pair<U1, U2> const& other
    ) {
      first() = other.first();
      second() = other.second();
      return *this;
    };

    template <typename U1, typename U2,
      typename=typename base_t
        ::template enable_move_conversion_assignment_op_if_t<U1, U2>
    >
    inline DARMA_CONSTEXPR_14
    compressed_pair& operator=(
      compressed_pair<U1, U2>&& other
    ) {
      first() = std::move(other.first());
      second() = std::move(other.second());
      return *this;
    };

};

} // end namespace utility

} // end namespace darma_runtime


#endif //DARMA_UTILITY_COMPRESSED_PAIR_H
