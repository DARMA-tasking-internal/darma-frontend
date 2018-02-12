/*
//@HEADER
// ************************************************************************
//
//                      constructor.h
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMA_CONSTRUCTOR_H
#define DARMA_CONSTRUCTOR_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(oo_interface)

namespace darma_runtime {

namespace oo {

namespace detail {

template <typename OfClass, typename... Args>
struct darma_constructor_helper {

  using base_class = OfClass;
};

} // end namespace detail

template <typename OfClass, typename... Args>
struct darma_constructors
  : detail::darma_constructor_helper<OfClass, Args...>::base_class
{
  // this type can never be constructed, only cast to
  darma_constructors() = delete;
  darma_constructors(darma_constructors const&) = delete;
  darma_constructors(darma_constructors&&) = delete;
  ~darma_constructors() = delete;
};

} // end namespace oo

} // end namespace darma_runtime

#endif // _darma_has_feature(oo_interface)

#endif //DARMA_CONSTRUCTOR_H
