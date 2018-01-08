/*
//@HEADER
// ************************************************************************
//
//                      const.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_CONST_H
#define DARMAFRONTEND_CONST_H

#include <darma/serialization/nonintrusive.h>
#include <darma/serialization/serialization_traits.h>

namespace darma_runtime {
namespace serialization {

template <typename T, typename Archive>
struct is_sizable_with_archive<T const, Archive>
  : is_sizable_with_archive<T, Archive>
{ };

template <typename T, typename Archive>
struct is_packable_with_archive<T const, Archive>
  : is_packable_with_archive<T, Archive>
{ };

template <typename T, typename Archive>
struct is_unpackable_with_archive<T const, Archive>
  : is_unpackable_with_archive<T, Archive>
{ };

template <typename T>
struct Serializer<T const>
  : Serializer<T>
{
  using Serializer<T>::compute_size;
  using Serializer<T>::pack;
  using Serializer<T>::unpack;
};

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_CONST_H
