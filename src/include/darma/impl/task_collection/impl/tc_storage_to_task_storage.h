/*
//@HEADER
// ************************************************************************
//
//                      tc_storage_to_task_storage.h
//                         DARMA
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

#ifndef DARMA_IMPL_TASK_COLLECTION_IMPL_TC_STORAGE_TO_TASK_STORAGE_H
#define DARMA_IMPL_TASK_COLLECTION_IMPL_TC_STORAGE_TO_TASK_STORAGE_H

#include <type_traits>

#include <darma/interface/app/access_handle.h>

#include <darma/impl/handle.h>
#include <darma/impl/capture.h>
#include <darma/impl/task_collection/handle_collection.h>
#include <darma/impl/index_range/index_range_traits.h>

namespace darma_runtime {
namespace detail {
namespace _task_collection_impl {

// TODO switch this over to ParamTraits argument

// Default "regular argument" case

//==============================================================================
// <editor-fold desc="Generic case"> {{{1

template <
  typename Functor, typename CollectionArg, size_t Position,
  typename Enable /* =void */
>
struct _get_task_stored_arg_helper {
  // TODO decide if this should be allowed to be a reference to the parent (probably not...)
  using type = std::decay_t<CollectionArg>;
  template <typename TaskCollectionInstanceT, typename TaskInstance>
  type
  operator()(TaskCollectionInstanceT& collection, size_t backend_index, CollectionArg const& arg,
    TaskInstance& task
  ) const {
    return arg;
  }
};

// </editor-fold> end Generic case }}}1
//==============================================================================

//==============================================================================
// <editor-fold desc="AccessHandle case"> {{{1

// AccessHandle unique modify case
template <typename Functor, typename CollectionArg, size_t Position>
struct _get_task_stored_arg_helper<
  Functor, CollectionArg, Position,
  std::enable_if_t<decayed_is_access_handle<CollectionArg>::value
    and is_access_handle_captured_as_unique_modify<std::decay_t<CollectionArg>>::value
  >
> {
  using type = typename CollectionArg::template with_traits<
    typename CollectionArg::traits::template with_owning_index_type<
      std::size_t
    >::type
  >;
  using return_type = type; // readability

  template <typename TaskCollectionInstanceT, typename TaskInstance>
  type
  operator()(TaskCollectionInstanceT& instance, size_t backend_index, CollectionArg const& arg,
    TaskInstance& task
  ) const {
    if(backend_index == arg.owning_backend_index_) {
      assert(arg.current_use_);
      // "Transfer" the use...
      return_type rv = return_type(
        arg.var_handle_,
        arg.current_use_
      );
      arg.current_use_ = nullptr;
      task.add_dependency(*(&(rv.current_use_->use)));
      return rv;
    }
    else {
      // We still need to create a new use holder (but not a use!) for the task itself, and don't register it!!!!
      auto new_use_holder = std::make_shared<UseHolder>(
        HandleUse(
          arg.var_handle_,
          nullptr, nullptr,
          HandleUse::Read, HandleUse::Read
        )
      );
      return_type rv(
        detail::unfetched_access_handle_tag{},
        arg.var_handle_,
        new_use_holder
      );
      rv.owning_index_ = arg.owning_backend_index_;
      return rv;

    }
  }

};

//------------------------------------------------------------------------------

// AccessHandle shared read case
template <typename Functor, typename CollectionArg, size_t Position>
struct _get_task_stored_arg_helper<
  Functor, CollectionArg, Position,
  std::enable_if_t<decayed_is_access_handle<CollectionArg>::value
    and is_access_handle_captured_as_shared_read<std::decay_t<CollectionArg>>::value
  >
> {
  using type = CollectionArg;
  using return_type = type;

  template <typename TaskCollectionInstanceT, typename TaskInstance>
  return_type
  operator()(TaskCollectionInstanceT& instance, size_t backend_index, CollectionArg const& arg,
    TaskInstance& task
  ) const {
    // We still need to create a new use for the task itself...
    auto new_use_holder = std::make_shared<UseHolder>(
      HandleUse(
        arg.var_handle_,
        arg.current_use_->use.in_flow_,
        arg.current_use_->use.out_flow_,
        arg.current_use_->use.scheduling_permissions_, // better be something like Read or less
        arg.current_use_->use.immediate_permissions_  // better be something like Read or less
      )
    );
    new_use_holder->do_register();
    task.add_dependency(*(&(new_use_holder->use)));
    return return_type(
      arg.var_handle_,
      new_use_holder
    );
  }

};

template <typename Functor, typename CollectionArg, size_t Position>
struct _get_task_stored_arg_helper<
  Functor, CollectionArg, Position,
  std::enable_if_t<decayed_is_access_handle<CollectionArg>::value
    and not is_access_handle_captured_as_shared_read<std::decay_t<CollectionArg>>::value
    and not is_access_handle_captured_as_unique_modify<std::decay_t<CollectionArg>>::value
  >
> {
  // TODO reasonable error message

};

// </editor-fold> end AccessHandle case }}}1
//==============================================================================

// Mapped HandleCollection case
template <typename Functor, typename CollectionArg, size_t Position>
struct _get_task_stored_arg_helper<
  Functor, CollectionArg, Position,
  std::enable_if_t<
    tinympl::is_instantiation_of<
      MappedHandleCollection,
      std::decay_t<CollectionArg>
    >::value
  >
>
{
  using type = typename CollectionArg::access_handle_collection_t;
  using return_type = type; // readability

  template <typename TaskCollectionInstanceT, typename TaskInstance>
  return_type
  operator()(TaskCollectionInstanceT& instance, size_t backend_index, CollectionArg const& arg,
    TaskInstance& task
  ) const {
    // make a copy of the handle collection for the task instance
    auto rv = arg.collection;
    rv.mapped_backend_index_ = backend_index;

    rv._setup_local_uses(task);
    return rv;
  }

//  template <typename TaskInstanceT>
//  return_type
//  operator()(TaskInstanceT& instance, CollectionArg const& arg) const {
//    auto* backend_runtime = abstract::backend::get_backend_runtime();
//
//    using handle_range_traits_t = indexing::index_range_traits<
//      CollectionArg::access_handle_collection_t::index_range_type
//    >;
//    using handle_mapping_traits_t = indexing::mapping_traits<
//      typename handle_range_traits_t::mapping_to_dense_type
//    >;
//
//    auto& handle_range = arg.collection.current_use_->use.index_range;
//    auto& task_range =
//    auto handle_mapping_to_dense = handle_mapping_traits_t::mapping_to_dense(
//      handle_range
//    );
//
//    // mapping in the collection stored arg already incorporates frontend->backend
//    // transformation of the collection index, so we can use it here directly
//    // with the backend index.  However, we still need to convert the use collection
//    // fronend index to a backend index
//    auto backend_collection_index = mapping_traits_t::map_forward(
//      collection_mapping_to_dense,
//      arg.mapping.map_backward(instance.backend_index_),
//      collection_range
//    );
//
//
//    auto local_in_flow = detail::make_flow_ptr(
//      backend_runtime->make_indexed_local_flow(
//        arg.collection.current_use_.in_flow_,
//        backend_collection_index
//      )
//    );
//
//    auto local_out_flow = detail::make_flow_ptr(
//      backend_runtime->make_indexed_local_flow(
//        arg.collection.current_use_.out_flow_,
//        backend_collection_index
//      )
//    );
//
//    auto new_use_holder = std::make_shared<UseHolder>(
//      HandleUse(
//        arg.collection.var_handle_,
//        local_in_flow, local_out_flow,
//        arg.collection.current_use_->use.scheduling_permissions_,
//        arg.collection.current_use_->use.immediate_permissions_
//      )
//    );
//    new_use_holder->do_register();
//
//    auto rv = return_type(new_use_holder);
//  }

};

// </editor-fold> end _get_task_stored_arg_helper for creating TaskCollectionTasks
//==============================================================================

} // end namespace _task_collection_impl
} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_TASK_COLLECTION_IMPL_TC_STORAGE_TO_TASK_STORAGE_H
