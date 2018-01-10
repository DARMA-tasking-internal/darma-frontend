/*
//@HEADER
// ************************************************************************
//
//                      pointer_reference_handler.h
//                         DARMA
//              Copyright (C) 2018 Sandia Corporation
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

#ifndef DARMAFRONTEND_SERIALIZATION_POINTER_REFERENCE_HANDLER_H
#define DARMAFRONTEND_SERIALIZATION_POINTER_REFERENCE_HANDLER_H

#include "simple_handler_fwd.h"
#include "pointer_reference_handler_fwd.h"

#include <darma/utility/compatibility.h>

#include <darma/serialization/pointer_reference_archive.h>

#include <darma/serialization/simple_archive.h>
#include <darma/serialization/simple_handler.h>

namespace darma_runtime {
namespace serialization {

template <typename FallbackHandler /*=SimpleSerializationHandler<std::allocator<char>>*/>
_darma_requires( SerializationHandler<FallbackHandler> )
struct PointerReferenceSerializationHandler
  : FallbackHandler
{

  public:

    static auto make_packing_archive(char*& ptr) {
      return PointerReferencePackingArchive<>(ptr);
    }

    static auto make_packing_archive(void*& ptr) {
      return PointerReferencePackingArchive<>(ptr);
    }

    template <typename CompatiblePackingArchive>
    _darma_requires( requires(CompatiblePackingArchive a) { a._data_spot() => char*&; } )
    static auto make_packing_archive_referencing(CompatiblePackingArchive& ar) {
      return PointerReferencePackingArchive<>(
        SimpleSerializationHandler<>::template _data_spot_reference_as<char>(ar)
      );
    }

    static auto make_unpacking_archive(char*& ptr) {
      return PointerReferenceUnpackingArchive<>(ptr);
    }

    static auto make_unpacking_archive(char const*& ptr) {
      return PointerReferenceUnpackingArchive<>(ptr);
    }

    static auto make_unpacking_archive(void*& ptr) {
      return PointerReferenceUnpackingArchive<>(ptr);
    }

    static auto make_unpacking_archive(void const*& ptr) {
      return PointerReferenceUnpackingArchive<>(ptr);
    }

    template <typename CompatibleUnpackingArchive>
    _darma_requires( requires(CompatibleUnpackingArchive a) { a._data_spot() => char const*&; } )
    static auto make_unpacking_archive_referencing(
      CompatibleUnpackingArchive& ar
    ) {
      using allocator_t = std::decay_t<decltype(ar.get_allocator())>;
      return PointerReferenceUnpackingArchive<allocator_t>(
        SimpleSerializationHandler<allocator_t>::template _const_data_spot_reference_as<char>(ar)
      );
    }

};

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_SERIALIZATION_POINTER_REFERENCE_HANDLER_H
