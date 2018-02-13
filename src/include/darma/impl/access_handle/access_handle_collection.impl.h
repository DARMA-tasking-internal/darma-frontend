/*
//@HEADER
// ************************************************************************
//
//                      access_handle_collection.impl.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_ACCESS_HANDLE_COLLECTION_IMPL_H
#define DARMAFRONTEND_ACCESS_HANDLE_COLLECTION_IMPL_H

#include <darma/impl/task_collection/access_handle_collection.h>

#include <darma/impl/collective/allreduce.h>

namespace darma_runtime {

#if _darma_has_feature(handle_collection_based_collectives)
template <typename T, typename IndexRange, typename Traits>
template <typename ReduceOp, typename... Args>
auto
AccessHandleCollection<T, IndexRange, Traits>::reduce(Args&& ... args) const {
  using namespace darma_runtime::detail;
  using parser = detail::kwarg_parser<
    overload_description<
      _keyword< /* required for now */
        deduced_parameter, darma_runtime::keyword_tags_for_collectives::output
      >,
      _optional_keyword<
        converted_parameter, darma_runtime::keyword_tags_for_collectives::tag
      >
    >
  >;
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

  // TODO somehow check if we're inside a task collection, and fail unless we're not

  return parser()
    .with_converters(
      [](auto&&... parts) {
        return darma_runtime::make_key(std::forward<decltype(parts)>(parts)...);
      }
    )
    .with_default_generators(
      darma_runtime::keyword_arguments_for_collectives::tag=[]{
        return darma_runtime::make_key();
      }
    )
    .parse_args(std::forward<Args>(args)...)
    .invoke([this](
      auto& output_handle,
      types::key_t const& tag
    ) -> decltype(auto) {

      auto cap_result_holder = detail::make_captured_use_holder(
        output_handle.var_handle_base_,
        /* requested_scheduling_permissions= */
        darma_runtime::frontend::Permissions::None,
        /* requested_immediate_permissions= */
        darma_runtime::frontend::Permissions::Modify,
        output_handle.get_current_use()
      );

      // The cloning ctor of BasicCollectionManagingUse takes care of
      // transferring over the use collection
      auto cap_collection_holder = detail::make_captured_use_holder(
        this->var_handle_base_,
        /* requested_scheduling_permissions= */
        frontend::Permissions::None,
        /* requested_immediate_permissions= */
        frontend::Permissions::Read,
        this->get_current_use()
      );

      // piece and n_pieces are ignored now
      auto coll_dets = detail::_get_collective_details_t<
        ReduceOp,
        AccessHandleCollection,
        decltype(output_handle)
      >(0, 0);

      abstract::backend::get_backend_runtime()->reduce_collection_use(
        cap_collection_holder->relinquish_into_destructible_use(),
        cap_result_holder->relinquish_into_destructible_use(),
        &coll_dets,
        tag
      );

    });
}
#endif //_darma_has_feature(handle_collection_based_collectives)

//==============================================================================

template <typename T, typename IndexRange, typename Traits>
template <typename, typename /* SFINAE */>
auto AccessHandleCollection<T, IndexRange, Traits>::operator[](
  typename base_t::index_range_traits_t::index_type const& idx
) const {
  using return_type = detail::IndexedAccessHandle<
    AccessHandleCollection,
    typename base_t::element_use_holder_ptr
  >;

  auto use_iter = local_use_holders_.find(idx);
  if (use_iter == local_use_holders_.end()) {

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // make an unfetched handle

    auto const& idx_range = get_index_range();
    auto map_dense = base_t::index_range_traits_t::mapping_to_dense(
      idx_range
    );
    auto backend_index = base_t::mapping_to_dense_traits_t::map_forward(
      map_dense, idx, idx_range
    );

    return return_type(
      /* parent = */ *this,
      /* use_holder_ptr = */ nullptr,
      /* has_local_access = */ false,
      /* backend_index = */ backend_index
    );
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  } else {

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Local access case
    return return_type(
      /* parent = */ *this,
      /* use_holder_ptr = */ use_iter->second,
      /* has local access = */ true
      /* backend_index is unnecessary, since we're not fetching */
    );
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  }
}

//==============================================================================

template <typename T, typename IndexRange, typename Traits>
void
AccessHandleCollection<T, IndexRange, Traits>::_setup_local_uses(
  detail::TaskBase& task
) const {

  auto* use_holder = this->get_current_use();
  assert(use_holder != nullptr);
  auto* my_use = use_holder->use();
  assert(my_use != nullptr);

  assert(my_use->get_managed_collection()->is_mapped()
    || !"_setup_local_uses() called on AccessHandleCollection with unmapped Use"
  );

  auto local_idxs = my_use->get_managed_collection()->local_indices_for(mapped_backend_index_);
  auto const& idx_range = get_index_range();

  auto map_dense = base_t::index_range_traits_t::mapping_to_dense(idx_range);

  for (auto&& idx : local_idxs) {

    auto fe_idx = base_t::mapping_to_dense_traits_t::map_backward(
      map_dense, idx, idx_range
    );

    using namespace darma_runtime::detail::flow_relationships;

    auto old_key = this->var_handle_base_->get_key();
    auto new_handle = std::make_shared<detail::VariableHandle<T>>(
      utility::safe_static_pointer_cast<detail::VariableHandle<T>>(
        this->var_handle_base_
      )->with_different_key(
        old_key.is_backend_generated() ?
          detail::key_traits<types::key_t>::make_awaiting_backend_assignment_key()
          // TODO shorten this?!?
          : make_key(old_key, "_index_", idx, "_local_access_")
      )
    );

    if(
      my_use->immediate_permissions_ == frontend::Permissions::Modify
        or my_use->scheduling_permissions_ == frontend::Permissions::Modify
      ) {

      auto idx_use_holder = base_t::element_use_holder_t::create(
        new_handle,
        my_use->scheduling_permissions_,
        my_use->immediate_permissions_,
        // In relationship
        indexed_local_flow(&my_use->in_flow_, idx),
        // Out relationship
        indexed_local_flow(&my_use->out_flow_, idx),
        /* anti-in flow */
        indexed_local_anti_flow(&my_use->anti_in_flow_, idx),
        //* anti-out flow */
        indexed_local_anti_flow(&my_use->anti_out_flow_, idx),
        my_use->coherence_mode_
      );
      // TODO change this to call_add_dependency??!?
      task.add_dependency(*idx_use_holder->use_base);
      local_use_holders_.insert(std::make_pair(fe_idx, idx_use_holder));

    }
    else {

      auto idx_use_holder = base_t::element_use_holder_t::create(
        new_handle,
        my_use->scheduling_permissions_,
        my_use->immediate_permissions_,
        // In relationship and source
        indexed_local_flow(&my_use->in_flow_, idx),
        // out relationship
        insignificant_flow(),
        // anti-in relationship
        insignificant_flow(),
        // anti-out
        indexed_local_anti_flow(&my_use->anti_out_flow_, idx),
        my_use->coherence_mode_
      );
      // TODO change this to call_add_dependency??!?
      task.add_dependency(*idx_use_holder->use_base);
      local_use_holders_.insert(std::make_pair(fe_idx, idx_use_holder));

    }

  } // end loop over local idxs

}

//==============================================================================

} // end namespace darma_runtime

#endif //DARMAFRONTEND_ACCESS_HANDLE_COLLECTION_IMPL_H
