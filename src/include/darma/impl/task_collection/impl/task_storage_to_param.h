/*
//@HEADER
// ************************************************************************
//
//                      task_storage_to_param.h
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

#ifndef DARMA_IMPL_TASK_COLLECTION_IMPL_TASK_STORAGE_TO_PARAM_H
#define DARMA_IMPL_TASK_COLLECTION_IMPL_TASK_STORAGE_TO_PARAM_H

#include <type_traits>

#include <darma/interface/app/access_handle.h>

#include <darma/impl/handle.h>
#include <darma/impl/capture.h>
#include <darma/impl/task_collection/access_handle_collection.h>

namespace darma_runtime {
namespace detail {
namespace _task_collection_impl {

//==============================================================================
// <editor-fold desc="_get_call_arg_helper">

// TODO parameter is a AccessHandleCollection case
// TODO clean this up and generalize it?


// Normal arg, param by value
template <typename StoredArg, typename ParamTraits>
struct _get_call_arg_helper<StoredArg, ParamTraits,
  std::enable_if_t<
    // Arg is not AccessHandle
    (not is_access_handle<StoredArg>::value
      and not is_access_handle_collection<StoredArg>::value)
    // Param is not AccessHandle, is by value
    and (not ParamTraits::template matches<decayed_is_access_handle>::value)
    and ParamTraits::is_by_value
  >
>
{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    // Move into the parameter
    return std::move(arg);
  }
};

// Normal arg, param is const ref value
template <typename StoredArg, typename ParamTraits>
struct _get_call_arg_helper<StoredArg, ParamTraits,
  std::enable_if_t<
    // Arg is not AccessHandle
    (not is_access_handle<StoredArg>::value and
      not is_access_handle_collection<StoredArg>::value)
    // Param is not AccessHandle, but is const ref
    and (not ParamTraits::template matches<decayed_is_access_handle>::value)
    and ParamTraits::is_const_lvalue_reference
  >
>
{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    // Return a reference to the argument in the tuple
    // TODO make sure this works
    return arg;
  }
};

// AccessHandle arg, Param is by value or const lvalue reference
// TODO this needs to check for a shared read stored arg type
template <typename StoredArg, typename ParamTraits>
struct _get_call_arg_helper<StoredArg, ParamTraits,
  std::enable_if_t<
    // StoredArg is an AccessHandle
    is_access_handle<StoredArg>::value
    // Param is by value or a const lvalue reference
    and (
      not ParamTraits::template matches<decayed_is_access_handle>::value
      and (
        ParamTraits::is_by_value
          or ParamTraits::is_const_lvalue_reference
      )
    )
>

>
{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    return arg.get_value();
  }
};

// AccessHandle arg, Param is non-const lvalue reference
// TODO this should be prohibited
//template <typename StoredArg, typename ParamTraits>
//struct _get_call_arg_helper<StoredArg, ParamTraits,
//  std::enable_if_t<
//    // StoredArg is an AccessHandle
//    is_access_handle<StoredArg>::value
//    // Param is non-const lvalue reference
//    and ParamTraits::is_nonconst_lvalue_reference
//  >
//>
//{
//  decltype(auto)
//  operator()(StoredArg&& arg) const {
//    return arg.get_reference();
//  }
//};

// AccessHandle arg, Param is AccessHandle (by value)
template <typename StoredArg, typename ParamTraits>
struct _get_call_arg_helper<StoredArg, ParamTraits,
  std::enable_if_t<
    // StoredArg is an AccessHandle
    is_access_handle<StoredArg>::value
    // Param is AccessHandle by value (so we can move into it)
    and (
      ParamTraits::template matches<decayed_is_access_handle>::value
        and ParamTraits::is_by_value
    )
  >
>
{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    return std::move(arg);
  }
};

// AccessHandle arg, Param is AccessHandle (by reference; can't move into it)
template <typename StoredArg, typename ParamTraits>
struct _get_call_arg_helper<StoredArg, ParamTraits,
  std::enable_if_t<
    // StoredArg is an AccessHandle
    is_access_handle<StoredArg>::value
    // Param is AccessHandle is a (potentially const) lvalue reference  (so we can't move into it)
    and (
      ParamTraits::template matches<decayed_is_access_handle>::value
        and (
          ParamTraits::is_nonconst_lvalue_reference
            or ParamTraits::is_const_lvalue_reference
        )
    )
  > // end enable_if_t
>
{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    return arg;
  }
};

// AccessHandleCollection arg, Param is AccessHandleCollection (by value)
template <typename StoredArg, typename ParamTraits>
struct _get_call_arg_helper<StoredArg, ParamTraits,
  std::enable_if_t<
    // StoredArg is an AccessHandleCollection
    is_access_handle_collection<StoredArg>::value
    // Param is AccessHandleCollection
    and ParamTraits::template matches<decayed_is_access_handle_collection>::value
    and ParamTraits::is_by_value
  > // end enable_if_t
>
{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    arg.dynamic_is_outer = false;
    return std::move(arg);
  }
};

// AccessHandleCollection arg, Param is AccessHandleCollection (by reference; can't move)
template <typename StoredArg, typename ParamTraits>
struct _get_call_arg_helper<StoredArg, ParamTraits,
  std::enable_if_t<
    // StoredArg is an AccessHandleCollection
    is_access_handle_collection<StoredArg>::value
      // Param is AccessHandleCollection
      and ParamTraits::template matches<decayed_is_access_handle_collection>::value
      and (
        ParamTraits::is_nonconst_lvalue_reference
          or ParamTraits::is_const_lvalue_reference
      )
  > // end enable_if_t
>
{
  decltype(auto)
  operator()(StoredArg&& arg) const {
    arg.dynamic_is_outer = false;
    return arg;
  }
};

// </editor-fold> end _get_call_arg_helper
//==============================================================================

} // end namespace _task_collection_impl
} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_TASK_COLLECTION_IMPL_TASK_STORAGE_TO_PARAM_H
