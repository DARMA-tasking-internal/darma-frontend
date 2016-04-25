/*
//@HEADER
// ************************************************************************
//
//                      allocation.h
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

#ifndef DARMA_IMPL_SERIALIZATION_ALLOCATION_H
#define DARMA_IMPL_SERIALIZATION_ALLOCATION_H

#include <type_traits>

#include <darma/impl/meta/detection.h>

#include "serialization_fwd.h"
#include "traits.h"

namespace darma_runtime {
namespace serialization {
namespace detail {

using std::declval;

template <typename T, typename Enable>
class allocation_traits {
  private:

    typedef serializability_traits<T, void> serdes_traits;

    typedef typename serdes_traits::serializer serializer;

    // see serializability_traits for more
    using _T = typename serdes_traits::_T;
    using _const_T = typename serdes_traits::_const_T;
    using _clean_T = typename serdes_traits::_clean_T;

    ////////////////////////////////////////////////////////////////////////////////
    // <editor-fold desc="Allocator awareness detection">

  private:

    /* Priority order for getting allocator:
     *
     * -> intrusive static method Alloc make_allocator()
     * -> nonintrusive const method of Serializer<T> named make_allocator() returning Alloc
     * -> fall-back on a "operator new"-style allocator (often must be default constructible to unpack,
     *    unless an appropriate Serializer<T>::unpack() specialization is given)
     *
     */


    template <typename U, typename ArchiveT>
    using has_intrusive_make_allocator_with_archive_archetype = decltype(
      U::make_allocator(std::declval<ArchiveT&>())
    );

    template <typename U>
    using has_intrusive_make_allocator_archetype = decltype(
      U::make_allocator()
    );

    template <typename SerializerT, typename ArchiveT>
    using has_nonintrusive_make_allocator_with_archive_archetype = decltype(
      std::declval<SerializerT>().make_allocator(std::declval<ArchiveT&>())
    );

    template <typename SerializerT, typename ArchiveT>
    using has_nonintrusive_make_allocator_archetype = decltype(
      std::declval<SerializerT>().make_allocator()
    );
  public:
    template



    // </editor-fold>
    ////////////////////////////////////////////////////////////////////////////////


};


} // end namespace detail
} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_ALLOCATION_H
