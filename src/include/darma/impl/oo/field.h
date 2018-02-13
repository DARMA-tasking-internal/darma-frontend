/*
//@HEADER
// ************************************************************************
//
//                      field.h
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

#ifndef DARMA_IMPL_OO_FIELD_H
#define DARMA_IMPL_OO_FIELD_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(oo_interface)

namespace darma_runtime {

namespace oo {

template <typename... Args>
struct private_fields {
  using args_vector_t = tinympl::vector<Args...>;
};

template <typename... Args>
struct public_fields {
  using args_vector_t = tinympl::vector<Args...>;
};

namespace detail {

template <typename T, typename Tag>
struct _field_tag_with_type {
  using value_type = T;
  using tag = Tag;
};

template <typename T, typename Tag>
struct _private_field_in_chain {
  using type = T;
  using tag = Tag;
  template <typename Base>
  using link_in_chain = typename tag::template as_private_field_in_chain<
    T, Base
  >;
};

template <typename T, typename Tag>
struct _public_field_in_chain {
  using type = T;
  using tag = Tag;
  template <typename Base>
  using link_in_chain = typename tag::template as_public_field_in_chain<
    T, Base
  >;
};

} // end namespace detail

} // end namespace oo

} // end namespace darma_runtime

#endif // _darma_has_feature(oo_interface)

#endif //DARMA_IMPL_OO_FIELD_H
