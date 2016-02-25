/*
//@HEADER
// ************************************************************************
//
//                          containment_manager.h
//                         darma_new
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

#ifndef SRC_ABSTRACT_FRONTEND_CONTAINMENT_MANAGER_H_
#define SRC_ABSTRACT_FRONTEND_CONTAINMENT_MANAGER_H_

#include "dependency_handle.h"

namespace darma_runtime {

namespace abstract {

namespace frontend {

/** @todo document this for the 0.2.1 spec
 *
 */
template <
  typename Key,
  typename Version
>
class ContainmentManager
{
  public:

    typedef DependencyHandle<Key, Version> handle_t;

    virtual bool
    is_offset_size_relationship() const =0;

    virtual bool
    is_strided() const =0;

    virtual size_t
    get_size() const =0;

    virtual ptrdiff_t
    get_offset() const =0;

    virtual ptrdiff_t
    get_stride() const =0;

    virtual void
    slice_in(
      const handle_t* const inner_handle,
      handle_t* const outer_handle
    ) const {
      // TODO explain why default implementation does nothing
    }

    virtual void
    slice_out(
      handle_t* const inner_handle,
      const handle_t* const outer_handle
    ) const {
      // TODO explain why default implementation does nothing
    }

    // Virtual destructor, since we have virtual methods

    virtual ~ContainmentManager() noexcept { }

};

} // end namespace frontend

} // end namespace abstract

} // end namespace darma_runtime


#endif /* SRC_ABSTRACT_FRONTEND_CONTAINMENT_MANAGER_H_ */
