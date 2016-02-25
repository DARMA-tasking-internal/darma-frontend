/*
//@HEADER
// ************************************************************************
//
//                                  at.hpp                                 
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


#ifndef TINYMPL_VARIADIC_AT_HPP
#define TINYMPL_VARIADIC_AT_HPP

#include <cstddef>

namespace tinympl {
namespace variadic {

/**
 * \ingroup VarBasics
 * \class at
 * \brief Extract the i-th element of a variadic template
 * \param i The index to extract
 */
template<std::size_t i, class ... Args>
struct at;

template<std::size_t i, class ... Args>
using at_t = typename at<i, Args...>::type;

template<std::size_t i, class Head, class ... Tail>
struct at<i, Head, Tail...>
{
  static_assert(i < sizeof ... (Tail) + 1,"Index out of range");
  typedef typename at<i-1,Tail...>::type type;
};

template<class Head, class ... Tail> struct at<0,Head,Tail...>
{
  typedef Head type;
};

namespace types_only {

template <typename WrappedSpot, class... Args>
struct at : public tinympl::variadic::at<WrappedSpot::value, Args...>
{ };

} // end namespace types_only

} // namespace variadic
} // namespace tinympl

#endif // TINYMPL_VARIADIC_AT_HPP
