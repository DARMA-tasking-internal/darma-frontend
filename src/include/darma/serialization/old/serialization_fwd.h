/*
//@HEADER
// ************************************************************************
//
//                      serialization_fwd.h
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

#ifndef DARMA_SERIALIZATION_FWD_H
#define DARMA_SERIALIZATION_FWD_H

namespace darma_runtime {

namespace serialization {

namespace detail {

template <typename T, typename Enable=void>
struct Serializer_enabled_if;

template <typename T, typename Enable=void>
struct Sizer;

template <typename T, typename Enable=void>
struct Packer;

template <typename T, typename Enable=void>
struct Unpacker;

} // end namespace detail

template <typename T>
struct Serializer;

struct unpack_constructor_tag_t { };
constexpr unpack_constructor_tag_t unpack_constructor_tag = { };

struct UnpackConstructorAccess;

struct SimplePackUnpackArchive;

namespace detail {

typedef enum SerializerMode {
  None,
  Sizing,
  Packing,
  Unpacking
} SerializerMode;

template <typename T, typename Enable=void>
struct serializability_traits;

template <typename T, typename Enable=void>
class allocation_traits;

} // end namespace detail

namespace Serializer_attorneys {

struct ArchiveAccess;

} // end namespace Serializer_attorneys

namespace detail {
namespace DependencyHandle_attorneys {

struct ArchiveAccess;

} // end namespace DependencyHandle_attorneys

} // end namespace serialization

} // end namespace detail


} // end namespace darma_runtime

#endif //DARMA_SERIALIZATION_FWD_H