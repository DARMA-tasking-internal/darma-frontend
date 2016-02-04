/*
//@HEADER
// ************************************************************************
//
//                          dependency.h
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

#ifndef SRC_ABSTRACT_FRONTEND_DEPENDENCY_HANDLE_H_
#define SRC_ABSTRACT_FRONTEND_DEPENDENCY_HANDLE_H_


namespace dharma_runtime {

namespace abstract {

// Forward declaration of DataBlock
namespace backend {
  class DataBlock;
} // end namespace backend

namespace frontend {

template <
  typename Key,
  typename Version
>
class DependencyHandle {
  public:

    virtual const Key&
    get_key() const =0;

    virtual const Version&
    get_version() const =0;

    virtual void
    set_version(const Version& v) =0;

    //// TODO we need move this, since some tasks will read only and some tasks will read/write the same handle
    //virtual bool
    //needs_read_data() const =0;

    //// TODO we need move this, since some tasks will read only and some tasks will read/write the same handle
    //virtual bool
    //needs_overwrite_data() const =0;

    SerializationManager*
    get_serialization_manager() =0;

    virtual void
    satisfy_with_data_block(
      abstract::backend::DataBlock* const data
    ) =0;

    virtual abstract::backend::DataBlock*
    get_data_block() =0;

    virtual bool
    is_satisfied() const =0;

    virtual ~DependencyHandle() noexcept { };
};


} // end namespace frontend

} // end namespace abstract

} // end namespace dharma_runtime

#endif /* SRC_ABSTRACT_FRONTEND_DEPENDENCY_HANDLE_H_ */
