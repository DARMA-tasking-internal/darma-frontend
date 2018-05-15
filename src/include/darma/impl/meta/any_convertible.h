/*
//@HEADER
// ************************************************************************
//
//                      any_convertible.h
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMA_IMPL_META_ANY_CONVERTIBLE_H
#define DARMA_IMPL_META_ANY_CONVERTIBLE_H

#include <tinympl/bool.hpp>

#include "is_callable.h"

namespace darma {

namespace meta {

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="any_arg variants">

// Much credit is owed to help from:
//   http://stackoverflow.com/questions/36581303/counting-arguments-of-an-arbitrary-callable-with-the-c-detection-idiom
// for this solution

// Works for anything.  Used for counting
struct any_arg {
  template <typename T>
  operator T();
  template <typename T>
  operator T&() const;
  template <typename T>
  operator T&&() const;
};


// Note that by value arguments (e.g., j in void foo(int j);) can be deduced
// from this in clang, but cannot be deduced from this in gcc, so be super
// careful with this (this is a known bug in gcc)
struct any_const_reference {
  template <typename T>
  operator const T&() const;
};

// If unary metafunction is always_true, works for everything (but we separate
// it from any_arg above for simplicity and compilation time efficiency)
template < template <class...> class UnaryMetafunction >
struct any_arg_conditional {
  template <typename T,
    typename = std::enable_if_t<UnaryMetafunction<T>::value>
  >
  operator T();

  template <typename T,
    typename = std::enable_if_t<UnaryMetafunction<T&>::value>
  >
  operator T&() const;

  template <typename T,
    typename = std::enable_if_t<UnaryMetafunction<T&&>::value>
  >
  operator T&&() const;
};

// Compiler bug workaround:
namespace _impl {

struct maybe_ambiguous_for_rvalue {
  template <typename T>
  operator T();
  template <typename T>
  operator T&&();
};

void _darma__unimplemented_test(int&&);

using rvalue_ref_operator_needs_const_t = tinympl::bool_<not
  is_callable_with_args<
    decltype(_darma__unimplemented_test),
    maybe_ambiguous_for_rvalue
  >::value
>;


} // end namespace _impl


struct any_nonconst_rvalue_reference {
  template <typename T>
  operator T();

  template <typename T,
    typename=std::enable_if_t<
      not std::is_const<std::remove_reference_t<T>>::value
        and _impl::rvalue_ref_operator_needs_const_t::value
    >
  >
  operator const T&&();

  template <typename T,
    typename=std::enable_if_t<
      not std::is_const<std::remove_reference_t<T>>::value
      and not _impl::rvalue_ref_operator_needs_const_t::value
    >
  >
  operator T&&();
};

// Should *ONLY* be ambiguous for value parameters
struct ambiguous_if_by_value {
  template <typename T>
  operator T();
  template <typename T>
  operator T&();
  // similar to in any_nonconst_rvalue_reference, we need to enable this for
  // gcc, since it prefers operator T&&() for a formal parameter of the form T&&
  // or const T&& (and thus T and T& is ambiguous for rvalue references as well
  // as values), but in clang operator T() and operator T&&() are somehow
  // equally valid for conversion to arguments of that form
  template <typename T,
    typename=std::enable_if_t<
      // It turns out that the condition we used for the
      // any_nonconst_rvalue_reference disambiguation of the same nature also
      // works here
      not _impl::rvalue_ref_operator_needs_const_t::value
      // an always true condition to make this SFINAE resolve at substitution
      // time and not at class definition time
      and not std::is_void<T>::value
    >
  >
  operator T&&();
};

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

} // end namespace meta

} // end namespace darma

#endif //DARMA_IMPL_META_ANY_CONVERTIBLE_H
