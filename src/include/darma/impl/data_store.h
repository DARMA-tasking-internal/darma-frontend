/*
//@HEADER
// ************************************************************************
//
//                      data_store.h
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


#ifndef DARMA_IMPL_DATA_STORE_H
#define DARMA_IMPL_DATA_STORE_H

#include <memory>

#include <darma/interface/backend/data_store_handle.h>
#include <darma/interface/backend/runtime.h>

namespace darma_runtime {

struct DataStore;

namespace detail {
struct DataStoreAttorney {
  static inline std::shared_ptr<abstract::backend::DataStoreHandle>&
  get_handle(DataStore&);
};
} // end namespace detail

struct DataStore {
  private:

    std::shared_ptr<abstract::backend::DataStoreHandle> ds_handle_ = nullptr;

    explicit
    DataStore(std::shared_ptr<abstract::backend::DataStoreHandle> const& h)
      : ds_handle_(h)
    { }

    friend DataStore create_data_store();

    friend class detail::DataStoreAttorney;

  public:

    static constexpr struct default_data_store_tag_t { } default_data_store_tag { };

    explicit
    DataStore(default_data_store_tag_t) : ds_handle_(nullptr) { }

    bool is_default() const { return bool(ds_handle_); }

};


inline DataStore create_data_store() {
  return DataStore(abstract::backend::get_backend_runtime()->make_data_store());
}

namespace detail {

inline std::shared_ptr<abstract::backend::DataStoreHandle>&
DataStoreAttorney::get_handle(DataStore& ds) {
  return ds.ds_handle_;
}

} // end namespace detail



} // end namespace darma_runtime

#endif //DARMA_IMPL_DATA_STORE_H
