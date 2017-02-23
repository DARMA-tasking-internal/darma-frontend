/*
//@HEADER
// ************************************************************************
//
//                      registry.h
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

#ifndef DARMA_IMPL_RUNNABLE_REGISTRY_H
#define DARMA_IMPL_RUNNABLE_REGISTRY_H

#include <darma/impl/feature_testing_macros.h>

#include "runnable_fwd.h"

namespace darma_runtime {
namespace detail {

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="Runnable registry and helpers">

#if _darma_has_feature(task_migration)

typedef std::vector<std::function<std::unique_ptr<RunnableBase>(void*)>> runnable_registry_t;

// TODO make sure this pattern works on all compilers at all optimization levels
template <typename = void>
runnable_registry_t&
get_runnable_registry()  {
  static runnable_registry_t reg;
  return reg;
}

namespace _impl {

template <typename Runnable>
struct RunnableRegistrar {
  size_t index;
  RunnableRegistrar() {
    runnable_registry_t &reg = get_runnable_registry<>();
    index = reg.size();
    reg.emplace_back([](void* archive_as_void) -> std::unique_ptr<RunnableBase> {
      using ArchiveT = serialization::SimplePackUnpackArchive;

      return Runnable::template construct_from_archive<
        serialization::SimplePackUnpackArchive
      >(*static_cast<serialization::SimplePackUnpackArchive*>(archive_as_void));
    });
  }
};

template <typename Runnable>
struct RunnableRegistrarWrapper {
  static RunnableRegistrar<Runnable> registrar;
};

template <typename Runnable>
RunnableRegistrar<Runnable> RunnableRegistrarWrapper<Runnable>::registrar = { };

} // end namespace _impl

template <typename Runnable>
size_t
register_runnable() {
  return _impl::RunnableRegistrarWrapper<Runnable>::registrar.index;
}

#endif // _darma_has_feature(task_migration)

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////


} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_RUNNABLE_REGISTRY_H
