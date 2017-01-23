/*
//@HEADER
// ************************************************************************
//
//                      task_collection_task.h
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

#ifndef DARMA_IMPL_TASK_COLLECTION_TASK_COLLECTION_TASK_H
#define DARMA_IMPL_TASK_COLLECTION_TASK_COLLECTION_TASK_H

#include <type_traits>

#include <darma/impl/task.h>

#include "impl/tc_storage_to_task_storage.h"
#include "impl/task_storage_to_param.h"

namespace darma_runtime {

template <typename Index>
struct ConcurrentContext {
  private:

    Index index_;
    size_t backend_index_;
    size_t backend_size_;


  public:

    ConcurrentContext(
      Index const& index,
      size_t backend_index,
      size_t backend_size
    ) : index_(index), backend_index_(backend_index), backend_size_(backend_size) { }

    operator Index() { return index_; }

    Index const& index() const { return index_; }

    template <typename ReduceOp=detail::op_not_given, typename... Args>
    void allreduce(Args&&... args) {
      darma_runtime::allreduce<ReduceOp>(
        std::forward<Args>(args)...,
        darma_runtime::keyword_arguments_for_collectives::piece=backend_index_,
        darma_runtime::keyword_arguments_for_collectives::n_pieces=backend_size_
      );
    }

};

namespace detail {

//==============================================================================
// <editor-fold desc="TaskCollectionTaskImpl">

template <
  typename Functor, typename Mapping,
  typename... StoredArgs
>
struct TaskCollectionTaskImpl
  : abstract::frontend::TaskCollectionTask<TaskBase>
{
  using base_t = abstract::frontend::TaskCollectionTask<TaskBase>;

  using args_tuple_t = std::tuple<StoredArgs...>;

  size_t backend_index_;
  size_t backend_size_;
  // This is the mapping from frontend index to backend index for the collection itself
  Mapping mapping_;
  args_tuple_t args_;

  template <size_t... Spots>
  auto
  _get_call_args_impl(
    std::index_sequence<Spots...>
  ) {
    return std::forward_as_tuple(
      _task_collection_impl::_get_call_arg_helper<
        typename std::tuple_element<Spots, args_tuple_t>::type,
        // Offset by 1 to allow for the index parameter to be given
        typename meta::callable_traits<Functor>::template param_n_traits<Spots+1>
      >{}(
        std::get<Spots>(std::move(args_))
      )...
    );
  }

  auto
  _get_first_argument() {
    return ConcurrentContext<
        decltype(mapping_.map_backward(backend_index_))
    >(
      mapping_.map_backward(backend_index_),
      backend_index_, backend_size_
    );
  }

  template <typename TaskCollectionT, typename CollectionStoredArgs, size_t... Spots>
  TaskCollectionTaskImpl(
    std::size_t backend_index, Mapping const& mapping,
    TaskCollectionT& parent,
    std::index_sequence<Spots...>,
    CollectionStoredArgs&& args_stored
  ) : backend_index_(backend_index),
      backend_size_(parent.size()),
      mapping_(mapping),
      args_(
        _task_collection_impl::_get_task_stored_arg_helper<
          Functor,
          std::remove_reference_t<decltype(std::get<Spots>(args_stored))>,
          Spots
        >{}(
          parent, backend_index,
          std::get<Spots>(std::forward<CollectionStoredArgs>(args_stored)),
          *this
        )...
      )
  { }

  void run() override {
    meta::splat_tuple(
      _get_call_args_impl(std::index_sequence_for<StoredArgs...>{}),
      [&](auto&&... args) mutable {
        Functor()(
          this->_get_first_argument(),
          std::forward<decltype(args)>(args)...
        );
      }
    );
  }


  bool is_migratable() const override {
    return false;
  }

  virtual ~TaskCollectionTaskImpl() override { }

};

// </editor-fold> end TaskCollectionTaskImpl
//==============================================================================


} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_TASK_COLLECTION_TASK_H
