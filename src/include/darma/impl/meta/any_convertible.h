/*
//@HEADER
// ************************************************************************
//
//                      any_convertible.h
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

#ifndef DARMA_IMPL_META_ANY_CONVERTIBLE_H
#define DARMA_IMPL_META_ANY_CONVERTIBLE_H

namespace darma_runtime {

namespace meta {

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="any_arg variants">

// Much credit is owed to help from:
//   http://stackoverflow.com/questions/36581303/counting-arguments-of-an-arbitrary-callable-with-the-c-detection-idiom
// for this solution

struct any_arg {
  template <typename T>
  operator T();
  template <typename T>
  operator T&() const;
  template <typename T>
  operator T&&() const;
};

struct ambiguous_if_by_value {
  template <typename T>
  operator T();
  template <typename T>
  operator T&();
};

// Note that by value arguments (e.g., j in void foo(int j);) can be deduced from this
// in clang, but cannot be deduced from this in gcc, so be super careful with this
// (this is a known bug in gcc)
struct any_const_reference {
  template <typename T>
  operator const T&() const;
};

// Be careful!  This doesn't work with things like is_const or is_reference
//template <
//  template <class...> class UnaryMetafunction
//>
//struct any_arg_conditional {
//  // Note that this first one is a non-const operator!  (This is the key to
//  // the whole thing working)
//  template <typename T,
//    typename = std::enable_if_t<UnaryMetafunction<T>::value>
//  >
//  operator T();
//
//  template <typename T,
//    typename = std::enable_if_t<UnaryMetafunction<const T>::value>
//  >
//  operator const T() const;
//
//  template <typename T,
//    typename = std::enable_if_t<UnaryMetafunction<T&>::value>
//  >
//  operator T&() volatile;
//
//  template <typename T,
//    typename = std::enable_if_t<UnaryMetafunction<const T&>::value>
//  >
//  operator const T&() const volatile;
//
//  template <typename T,
//    // for now, leave off the rvalue reference for consistency between gcc and clang
//    typename = std::enable_if_t<UnaryMetafunction<T>::value>
//  >
//  operator T&&() const volatile;
//};

template <
  template <class...> class UnaryMetafunction
>
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

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

} // end namespace meta

} // end namespace darma_runtime

#endif //DARMA_IMPL_META_ANY_CONVERTIBLE_H
