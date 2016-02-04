/*
//@HEADER
// ************************************************************************
//
//                          task.h
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

#ifndef SRC_ABSTRACT_FRONTEND_TASK_H_
#define SRC_ABSTRACT_FRONTEND_TASK_H_


#include "dependency_handle.h"

namespace dharma_runtime {

namespace abstract {

namespace frontend {



template <
  typename Key, typename Version,
  template <typename...> class Iterable,
  template <typename...> class smart_ptr_template
>
class Task {
  public:

    typedef abstract::frontend::DependencyHandle<Key, Version> handle_t;
    typedef smart_ptr_template<handle_t> handle_ptr;

    // includes antidependencies
    virtual
    const Iterable<handle_ptr>&
    get_dependencies() const =0;

    virtual bool
    needs_read_data(
      const handle_t* const handle
    ) const =0;

    // NOTE: Documentation here needs to explain that you can always
    // distinguish a write that creates an antidependency from one that doesn't (i.e.,
    // initial access by seeing if the version == Version(), i.e., the default
    // constructor of Version.
    virtual bool
    needs_write_data(
      const handle_t* const handle
    ) const =0;

    virtual const Key&
    get_name() const =0;

    virtual void
    set_name(const Key& name_key) =0;

    // TODO more migration callbacks and such, such as...
    virtual bool
    is_migratable() const =0;


    virtual void
    run() const =0;

    virtual ~Task() { }
};


} // end namespace frontend

} // end namespace abstract

} // end namespace dharma_runtime



#endif /* SRC_ABSTRACT_FRONTEND_TASK_H_ */
