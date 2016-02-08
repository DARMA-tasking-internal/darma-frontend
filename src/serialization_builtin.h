/*
//@HEADER
// ************************************************************************
//
//                          serialization_builtin.h
//                         dharma_new
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

#ifndef SRC_SERIALIZATION_BUILTIN_H_
#define SRC_SERIALIZATION_BUILTIN_H_

#include <vector>
#include <map>

#include "serialization.h"

//namespace dharma_runtime {
//
//// Examples:
//
//template <
//  typename T,
//  typename std::enable_if<
//    detail::is_trivially_serializable<T>::value
//  >::type
//>
//struct nonintrusive_serialization_interface<std::vector<T>>
//  : ensure_implements_zero_copy<std::vector<T>>
//{
//  size_t num_zero_copy_slots(const std::vector<T>&) const {
//    return 1;
//  }
//  size_t zero_copy_slot_size(const std::vector<T>& v, size_t slot) {
//    assert(slot == 0);
//    return v.capacity() * sizeof(T);
//  }
//  void*& get_zero_copy_slot(std::vector<T>& v, size_t slot) {
//    return v.data();
//  }
//};
//
//// TODO FIX THIS!!!!
//template <typename T, typename U>
//struct nonintrusive_serialization_interface<std::map<T, U>>
//{
//  void serialize_metadata(std::map<T, U>& m, Archive& ar) const {
//    if(ar.is_unpacking()) {
//      m = new (&m) std::map<T, U>();
//      size_t n_pairs = 0;
//      ar & n_pairs;
//      for(int i = 0; i < n_pairs; ++i) {
//        T t; ar & t;
//        U u; ar & u;
//        m.emplace(t, u);
//      }
//    }
//    else { // ar.is_sizing() || ar.is_packing()
//      ar & m.size();
//      for(auto&& pair : m) {
//        ar & pair.first();
//        ar & pair.second();
//      }
//    }
//  }
//  void serialize_data(std::map<T, U>& m, Archive& ar) const {
//    if(ar.is_unpacking()) {
//      m = new (&m) std::map<T, U>();
//      size_t n_pairs = 0;
//      ar & n_pairs;
//      for(int i = 0; i < n_pairs; ++i) {
//        T t; ar & t;
//        U u; ar & u;
//        m.emplace(t, u);
//      }
//    }
//    else { // ar.is_sizing() || ar.is_packing()
//      ar & m.size();
//      for(auto&& pair : m) {
//        ar & pair.first();
//        ar & pair.second();
//      }
//    }
//  }
//
//};
//
//} // end namespace dharma_runtime




#endif /* SRC_SERIALIZATION_BUILTIN_H_ */
