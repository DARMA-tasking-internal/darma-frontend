
/*
//@HEADER
// ************************************************************************
//
//                                vector.hpp                               
//                         darma_mockup
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


#ifndef TINYMPL_VECTOR_HPP
#define TINYMPL_VECTOR_HPP

#include "variadic/at.hpp"
#include "variadic/erase.hpp"
#include "erase.hpp"
#include "insert.hpp"

namespace tinympl {

/**
 * \defgroup Containers Containers
 * Full and half compile time containers of types and values.
 * @{
 */

/**
 * \class vector
 * \brief A compile time vector of types
 * Vector is the simplest tinympl sequence type. It provides standard modifiers and random access
 * to its elements.
 */
template<class ... Args>
struct vector
{
  enum
  {
    size = sizeof ... (Args) //!< The size of the vector
  };

  enum
  {
    empty = (size == 0) //!< Determine whether the vector is empty
  };

  //! Access the i-th element
  template<std::size_t i>
  struct at
  {
    static_assert(i < size,"Index i is out of range");
    typedef typename variadic::at<i,Args...>::type type;
  };

  template<std::size_t i>
  using at_t = typename at<i>::type;

  //! Access the i-th element, but safe protected
  template<typename Default, std::size_t i>
  struct at_or
  {
    typedef typename variadic::at_or<Default, i, Args...>::type type;
  };

  template<typename Default, std::size_t i>
  using at_or_t = typename at_or<Default, i>::type;

  //! Return a new vector constructed by inserting `T` on the back of the current vector
  template<class T>
  struct push_back
  {
    typedef vector<Args...,T> type;
  };

  //! Return a new vector constructed by inserting `T` on the front of the current vector
  template<class T>
  struct push_front
  {
    typedef vector<T,Args...> type;
  };

  //! Return a new vector constructed by removing the last element of the current vector
  struct pop_back
  {
    typedef typename variadic::erase<size-1,size,tinympl::vector,Args...>::type type;
  };

  //! Return a new vector constructed by removing the first element of the current vector
  struct pop_front
  {
    typedef typename variadic::erase<0,1,tinympl::vector,Args...>::type type;
  };

  //! Return a new vector constructed by erasing the elements in the range [first,last) of the current vector
  template<std::size_t first,std::size_t last>
  struct erase : tinympl::erase<first,last,vector<Args...>,tinympl::vector> {};

  //! Return a new vector constructed by inserting the elements `Ts...` in the current vector starting at the index `i`
  template<std::size_t i,class ... Ts>
  struct insert : tinympl::insert<i,
    sequence<Ts...>,
    vector<Args...>,
    tinympl::vector
  > {};

  //! Return the first element of the vector
  struct front
  {
    typedef typename variadic::at<0,Args...>::type type;
  };

  //! Return the last element of the vector
  struct back
  {
    typedef typename variadic::at<size-1,Args...>::type type;
  };
};

/** @} */

/**
 * \ingroup SeqCustom
 * \brief Customization point to allow `vector` to work as a tinympl sequence
 */
template<class... Args> struct as_sequence<vector<Args...> >
{
  typedef sequence<Args...> type;
  template<class... Ts> using rebind = vector<Ts...>;
};

}

#endif // TINYMPL_VECTOR_HPP
