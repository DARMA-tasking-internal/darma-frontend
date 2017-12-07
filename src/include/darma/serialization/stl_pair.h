/*
//@HEADER
// ************************************************************************
//
//                      stl_pair.h
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

#ifndef DARMA_IMPL_SERIALIZATION_STL_PAIR_H
#define DARMA_IMPL_SERIALIZATION_STL_PAIR_H

#include <type_traits>
#include <iterator>

#include "nonintrusive.h"

namespace darma_runtime {
namespace serialization {

////////////////////////////////////////////////////////////////////////////////

template <typename T1, typename T2>
struct Serializer<std::pair<T1, T2>> {

  private:

    typedef detail::serializability_traits<T1> T1_serdes_traits;
    typedef typename T1_serdes_traits::serializer Ser1;
    typedef detail::serializability_traits<T2> T2_serdes_traits;
    typedef typename T2_serdes_traits::serializer Ser2;

    template <typename ArchiveT>
    using serializable_into = std::integral_constant<bool,
      T1_serdes_traits::template is_serializable_with_archive<ArchiveT>::value
        and T2_serdes_traits::template is_serializable_with_archive<ArchiveT>::value
    >;

    typedef std::pair<T1, T2> PairT;


  public:

    using directly_serializable = std::integral_constant<bool,
      T1_serdes_traits::is_directly_serializable
        and T2_serdes_traits::is_directly_serializable
    >;

    ////////////////////////////////////////////////////////////
    // <editor-fold desc="compute_size()">

    template <typename ArchiveT>
    std::enable_if_t<serializable_into<ArchiveT>::value>
    compute_size(PairT const& c, ArchiveT& ar) const {
      assert(ar.is_sizing());
      ar.incorporate_size(c.first);
      ar.incorporate_size(c.second);
    }

    // </editor-fold>
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // <editor-fold desc="pack()">

    template <typename ArchiveT>
    std::enable_if_t<serializable_into<ArchiveT>::value>
    pack(PairT const& c, ArchiveT& ar) const {
      assert(ar.is_packing());
      ar.pack_item(c.first);
      ar.pack_item(c.second);
    }

    // </editor-fold>
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // <editor-fold desc="unpack()">

    template <typename ArchiveT>
    std::enable_if_t<serializable_into<ArchiveT>::value>
    unpack(void* allocated, ArchiveT& ar) const {
      assert(ar.is_unpacking());

      // TODO make sure that it's okay to assume that the std::pair constructors
      //      don't do anything but call the constituent constructors. (should
      //      be fine in any implementation I can imagine, but I don't know
      //      if this is standards compliant)

      detail::serializability_traits<T1>::unpack(
        (void*)(&((*(PairT*)allocated).first)), ar
      );

      detail::serializability_traits<T2>::unpack(
        (void*)(&((*(PairT*)allocated).second)), ar
      );

    }

    // </editor-fold>
    ////////////////////////////////////////////////////////////

};

} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_STL_PAIR_H
