/*
//@HEADER
// ************************************************************************
//
//                          handle_attorneys.h
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

#ifndef SRC_INCLUDE_DARMA_IMPL_HANDLE_ATTORNEYS_H_
#define SRC_INCLUDE_DARMA_IMPL_HANDLE_ATTORNEYS_H_

#include <darma/impl/handle.h>
#include <darma/interface/app/access_handle.h>


namespace darma_runtime {

namespace detail {

namespace access_attorneys {

struct for_AccessHandle {
  // call the private constructors
  template <typename T, typename Key, typename Version>
  static AccessHandle<T>
  construct_initial_access(Key const& key, Version const& version) {
    return { key, version, AccessHandle<T>::State::Modify_None };
  }
  template <typename T, typename Key>
  static AccessHandle<T>
  construct_read_access(Key const& key, Key const& user_version_tag) {
    return { key, AccessHandle<T>::State::Read_None, user_version_tag };
  }

};

}

namespace create_work_attorneys {

struct for_AccessHandle {
  template <typename AccessHandleType>
  static inline
  typename tinympl::copy_cv_qualifiers<AccessHandleType>::template apply<
    typename AccessHandleType::dep_handle_t
  >::type* const
  get_dep_handle(
    AccessHandleType const& ah
  ) {
    return ah.dep_handle_.get();
  }

  template <typename AccessHandleType>
  static inline
  typename tinympl::copy_cv_qualifiers<AccessHandleType>::template apply<
    typename AccessHandleType::dep_handle_ptr
  >::type const
  get_dep_handle_ptr(
    AccessHandleType& ah
  ) {
    return ah.dep_handle_;
  }

  template <typename AccessHandleType>
  static inline
  typename tinympl::copy_volatileness<AccessHandleType>::template apply<
    typename AccessHandleType::version_t
  >::type const&
  get_version(
    AccessHandleType& ah
  ) {
    return ah.dep_handle_->get_version();
  }
};

} // end namespace create_work_attorneys

} // end namespace detail

} // end namespace darma




#endif /* SRC_INCLUDE_DARMA_IMPL_HANDLE_ATTORNEYS_H_ */
