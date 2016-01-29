/*
//@HEADER
// ************************************************************************
//
//                          move_if.h
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

#ifndef META_MOVE_IF_H_
#define META_MOVE_IF_H_

#include <utility> // std::move

namespace dharma_runtime { namespace meta {

////////////////////////////////////////////////////////////////////////////////

/* several move_if_* functors for correct transfer of move semantics     {{{1 */ #if 1 // begin fold

template <typename T>
struct move_if_not_lvalue_reference
{
  T&& operator()(T&& val) const noexcept {
    return std::move(val);
  }
};

template <typename T>
struct move_if_not_lvalue_reference<T&>
{
  T& operator()(T& val) const noexcept {
    return val;
  }
};

template <bool condition, typename T>
struct move_if
{
  T&& operator()(T&& val) const noexcept {
    return std::move(val);
  }
};

template <typename T>
struct move_if<false, T>
{
  T& operator()(T& val) const noexcept {
    return val;
  }
};

template <bool by_value_condition, bool move_condition, typename T>
struct by_value_if_or_move_if
{ };

template <bool move_condition, typename T>
struct by_value_if_or_move_if<true, move_condition, T>
{
  T operator()(T&& val) const noexcept {
    return val;
  }
};

template <typename T>
struct by_value_if_or_move_if<false, true, T>
{
  T&& operator()(T&& val) const noexcept {
    return std::move(val);
  }
};

template <typename T>
struct by_value_if_or_move_if<false, false, T>
{
  T& operator()(T& val) const noexcept {
    return val;
  }
};


/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////

}} // end namespace dharma_mockup::detail

#endif /* META_MOVE_IF_H_ */
