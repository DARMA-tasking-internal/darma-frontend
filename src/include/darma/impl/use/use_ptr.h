/*
//@HEADER
// ************************************************************************
//
//                      use_ptr.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_USE_PTR_H
#define DARMAFRONTEND_USE_PTR_H

#include <darma/impl/handle_use_base.h>

namespace darma_runtime {
namespace detail {

class UsePtrBase {
  protected:


  public:



};

template <typename UnderlyingUse>
class UsePtr : public UsePtrBase {

  private:

    // For use in emulating private constructors that have to go through
    // a shared_ptr interface.
    static constexpr struct private_ctor_tag_t { } private_ctor_tag;

  protected:

    using underlying_ptr_t = managing_ptr<
      std::unique_ptr<UnderlyingUse>, HandleUseBase*
    >;

    underlying_ptr_t use_;

    template <typename... UseCtorArgs>
    UsePtr(
      private_ctor_tag_t,
      UseCtorArgs&&... args
    ) : use_()


  public:


    template <typename... UseCtorArgs>
    std::shared_ptr<UsePtr>
    create(UseCtorArgs&&... args) {

    }


    // UsePtr objects can't be moved or copied; they can only be constructed
    // in place as part of a smart pointer
    UsePtr(UsePtr&&) = delete;
    UsePtr(UsePtr const&) = delete;


    UnderlyingUse* use() { return use_.get(); }
    UnderlyingUse const* use() const { return use_.get(); }

};

// Needs to be defined since make_shared<> binds a reference to it.
template <typename UnderlyingUse>
typename UsePtr<UnderlyingUse>::private_ctor_tag_t
UsePtr<UnderlyingUse>::private_ctor_tag = { };

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_USE_PTR_H
