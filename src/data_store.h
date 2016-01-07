/*
//@HEADER
// ************************************************************************
//
//                          data_store.h
//                         dharma_mockup
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

#ifndef NEW_DATA_STORE_H_
#define NEW_DATA_STORE_H_

#include "dependency.h"
#include "key.h"
#include "version.h"
#include <unordered_map>
#include <mutex>

//namespace dharma_runtime {
//
//namespace detail {
//
//class DataStore {
//
//    DataBlockBase*
//    get_data_block(
//      const Key& key,
//      const Version& version
//    ) {
//      std::lock_guard<std::mutex>(data_mutex_);
//      return data_[{key, version}];
//    }
//
//    DataBlockBase*&
//    get_data_block_lockfree(
//      const Key& key,
//      const Version& version
//    ) {
//      return data_[{key, version}];
//    }
//
//    DataBlockMetaData*
//    get_metadata(
//      const Key& key,
//      const Version& version
//    ) {
//      std::lock_guard<std::mutex>(data_mutex_);
//      return data_[{key, version}];
//    }
//
//    DataBlockMetaData*
//    get_data_block_lockfree(
//      const Key& key,
//      const Version& version
//    ) {
//      return data_[{key, version}];
//    }
//
//  private:
//
//    typedef std::unordered_map<std::pair<Key, Version>, DataBlockBase*> data_map_t;
//    typedef std::unordered_map<std::pair<Key, Version>, DataBlockMetaData*> metadata_map_t;
//
//    std::mutex data_mutex_;
//
//    data_map_t data_;
//
//
//
//};
//
//} // end namespace detail
//
//} // end namespace dharma_runtime



#endif /* NEW_DATA_STORE_H_ */
