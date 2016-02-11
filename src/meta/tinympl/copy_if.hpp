
/*
//@HEADER
// ************************************************************************
//
//                               copy_if.hpp                               
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


#ifndef TINYMPL_COPY_IF_HPP
#define TINYMPL_COPY_IF_HPP

#include "variadic/copy_if.hpp"
#include "as_sequence.hpp"
#include "sequence.hpp"

namespace tinympl {

/**
 * \ingroup SeqModAlgs
 * \class copy_if
 * \brief Copy the elements of a given input sequence which satisfy a given
predicate - the ordering is preserved.
 * \param SequenceIn The input sequence
 * \param F The test predicate - F<T>::type::value shall be convertible to bool
 * \param Out The output sequence type - defaults to the same sequence kind of
the input sequence
 * \return `copy_if<...>::type` is a type templated from `Out` which is
constructed with the elements of SequenceIn which satisfy the predicate `F`.
 * \sa variadic::copy_if
 */
template<class SequenceIn, template<class ... T> class F, template<class ...>
class Out = as_sequence<SequenceIn>::template rebind>
struct copy_if : copy_if<as_sequence_t<SequenceIn>, F, Out> {};

template<template<class ... T> class F, template<class ...> class Out, class ...
Args>
struct copy_if<sequence<Args...>, F, Out>  : variadic::copy_if<F, Out, Args...>
{};

} // namespace tinympl

#endif // TINYMPL_COPY_IF_HPP