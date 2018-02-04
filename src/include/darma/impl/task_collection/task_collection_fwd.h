/*
//@HEADER
// ************************************************************************
//
//                      task_collection_fwd.h
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

#ifndef DARMA_IMPL_TASK_COLLECTION_TASK_COLLECTION_FWD_H
#define DARMA_IMPL_TASK_COLLECTION_TASK_COLLECTION_FWD_H

#include <cstdio> // size_t

#include <darma/utility/optional_boolean.h>

#include <darma/impl/task_collection/access_handle_collection_traits.h>

namespace darma_runtime {
namespace detail {

typedef enum HandleCollectiveLabel
{
  Reduce = 0
} handle_collective_label_t;

template <
  typename AccessHandleCollectionT,
  typename ReduceOp, handle_collective_label_t
>
struct _collective_awaiting_assignment;


namespace _task_collection_impl {

// Argument to TaskCollectionImpl storage helper
template <
  typename GivenArg,
  typename ParamTraits,
  typename CollectionIndexRangeT,
  typename Enable=void
>
struct _get_storage_arg_helper;

template <
  typename Functor,
  typename CollectionArg, size_t Position,
  typename Enable=void
>
struct _get_task_stored_arg_helper;

template <typename StoredArg, typename ParamTraits, typename Enable=void>
struct _get_call_arg_helper; // TODO better error message for default case

} // end namespace _task_collection_impl

template <
  typename Functor,
  typename IndexRangeT,
  typename... Args
>
struct TaskCollectionImpl;

template <typename IndexRangeT>
class BasicAccessHandleCollection;

} // end namespace detail

template <typename T, typename IndexRangeT,
  typename Traits=detail::access_handle_collection_traits<T, IndexRangeT>
>
class AccessHandleCollection;

} // end namespace darma_runtime

#endif //DARMA_IMPL_TASK_COLLECTION_TASK_COLLECTION_FWD_H
