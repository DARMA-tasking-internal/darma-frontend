/*
//@HEADER
// ************************************************************************
//
//                      indexed_access_handle.h
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

#ifndef DARMA_IMPL_TASK_COLLECTION_INDEXED_ACCESS_HANDLE_H
#define DARMA_IMPL_TASK_COLLECTION_INDEXED_ACCESS_HANDLE_H

#include <cstdlib> // std::size_t
#include <limits> // std::numeric_limits
#include <utility> // std::move

#include <tinympl/logical_and.hpp> // and_

#include <darma_types.h>

#include <darma/interface/app/keyword_arguments/version.h>

#include <darma/impl/task_collection/task_collection_fwd.h>

#include <darma/utility/static_assertions.h> // _darma__static_failure
#include <darma/utility/darma_assert.h>
#include <darma/impl/handle_fwd.h> // AccessHandlePermissions
#include <darma/keyword_arguments/parse.h> // AccessHandlePermissions

#include <darma/impl/task_collection/errors.h>
#include <darma/impl/access_handle/access_handle_traits.h>

#include <darma/interface/app/access_handle.h>

namespace darma_runtime {
namespace detail {

//==============================================================================
// <editor-fold desc="IndexedAccessHandle"> {{{1

// Proxy type returned by the AccessHandleCollection index operation
template <typename ParentAHC, typename UseHolderPtr>
class IndexedAccessHandle {
  private:

    using parent_traits_t = typename ParentAHC::traits;
    using value_type = typename ParentAHC::value_type;

    //------------------------------------------------------------------------------
    // <editor-fold desc="private members"> {{{2

    using use_holder_ptr = UseHolderPtr;
    use_holder_ptr use_holder_;
    ParentAHC const& parent_;

    bool has_local_access_; // actually redundant, since use_holder_ == null tells us this...
    std::size_t backend_index_;

    // </editor-fold> end private members }}}2
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    // <editor-fold desc="private ctors"> {{{2

    IndexedAccessHandle(
      ParentAHC const& parent,
      use_holder_ptr const& use_holder_ptr,
      bool has_local_access,
      std::size_t backend_index = std::numeric_limits<std::size_t>::max()
    ) : parent_(parent),
        use_holder_(use_holder_ptr),
        has_local_access_(has_local_access),
        backend_index_(backend_index)
    { }

    IndexedAccessHandle(IndexedAccessHandle&& other)
      : parent_(other.parent_),
        use_holder_(other.use_holder_),
        has_local_access_(std::move(other.has_local_access_)),
        backend_index_(std::move(other.backend_index_))
    { /* should never execute because of move elision RVO */ }                  // LCOV_EXCL_LINE

    // </editor-fold> end private ctors }}}2
    //------------------------------------------------------------------------------

  public:

    // Can only be called as part of `ahc[...].local_access()`; rvalue reference
    // specifier enforces this
    auto
    local_access() const && {
      DARMA_ASSERT_MESSAGE(has_local_access_,
        "Attempted to obtain local access to index of AccessHandleCollection"
          " that is not local"
      );
      // TODO mangled name shortening optimization by making these traits a subclass of access_handle_traits

      auto handle = use_holder_->use()->handle_;

      auto rv = AccessHandle<value_type,
        make_access_handle_traits_t<value_type,
          copy_assignability<false>,
          access_handle_trait_tags::permissions_traits<
            typename parent_traits_t::permissions_traits
          >,
          access_handle_trait_tags::allocation_traits<
            typename parent_traits_t::allocation_traits
          >
          // TODO set publishable to KnownTrue
        >
      >(
        std::static_pointer_cast<detail::VariableHandle<value_type>>(handle),
        std::move(use_holder_)
      );

      rv.other_private_members_.set_can_be_published_dynamic(true);

      #if _darma_has_feature(task_collection_token)
      rv.other_private_members_.task_collection_token() = parent_.task_collection_token_;
      #endif // _darma_has_feature(task_collection_token)

      return std::move(rv);
    }

    // Can only be called as part of `ahc[...].read_access(...)`; rvalue
    // reference specifier enforces this
    template <typename... Args>
    auto
    read_access(Args&&... args) && {
      DARMA_ASSERT_MESSAGE(not has_local_access_,
        "Attempted to fetch an AccessHandle corresponding to an index of an"
          " AccessHandleCollection that is local to the fetching context"
      );

      // TODO allocator support

      using namespace darma_runtime::detail;
      using parser = detail::kwarg_parser<
        overload_description<
          _optional_keyword<converted_parameter, keyword_tags_for_publication::version>
      >
      >;
      using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

      return parser()
        .with_converters(
          [](auto&&... parts) {
            return darma_runtime::make_key(std::forward<decltype(parts)>(parts)...);
          }
        )
        .with_default_generators(
          keyword_arguments_for_publication::version=[]{
            // Defaults to empty key, **not** backend defined!!!
            return darma_runtime::make_key();
          }
        )
        .parse_args(std::forward<Args>(args)...)
        .invoke([this](
          types::key_t&& version_key
        ) -> decltype(auto) {

          using namespace darma_runtime::abstract::frontend;
          using namespace darma_runtime::detail::flow_relationships;

          auto* backend_runtime = abstract::backend::get_backend_runtime();

          auto old_key = parent_.var_handle_base_->get_key();
          auto new_handle = std::make_shared<detail::VariableHandle<value_type>>(
            utility::safe_static_cast<detail::VariableHandle<value_type> const*>(
              parent_.var_handle_base_.get()
            )->with_different_key(
              old_key.is_backend_generated() ?
                // TODO shorten this
                detail::key_traits<types::key_t>::make_awaiting_backend_assignment_key()
                : make_key(
                    old_key, "_backend_index_", backend_index_,
                    "_read_access_version_key_", version_key
                  )
            )
          );

          assert(use_holder_ == nullptr);

          // Make the use holder
          use_holder_ = std::pointer_traits<UseHolderPtr>::element_type::create(
            new_handle,
            frontend::Permissions::Read, // Read scheduling permissions
            frontend::Permissions::None, // No immediate permissions
            /* In flow description */
            indexed_fetching_flow(
              &parent_.get_current_use()->use()->in_flow_, &version_key, backend_index_
            ),
            /* Out flow description */
            insignificant_flow(),
            /* Anti-In flow description */
            insignificant_flow(),
            /* Anti-Out flow description */
            indexed_fetching_anti_flow(
              &parent_.get_current_use()->use()->anti_out_flow_, &version_key, backend_index_
            )
          );

          return AccessHandle<value_type,
            make_access_handle_traits_t<value_type,
              copy_assignability<true>, // statically read-only, so it doesn't
                                        // matter if it gets copied (right?)
                                        // TODO think through whether this should be the default...
              static_scheduling_permissions<AccessHandlePermissions::Read>,
              required_scheduling_permissions<AccessHandlePermissions::Read>,
              access_handle_trait_tags::allocation_traits<
                typename parent_traits_t::allocation_traits
              >
            >
          >(
            new_handle,
            std::move(use_holder_)
          );

        });

    }

#if _darma_has_feature(commutative_access_handles)
    template <typename _Ignored_SFINAE=void,
      typename=std::enable_if_t<
        std::is_void<_Ignored_SFINAE>::value
          and ParentAHC::traits::permissions_traits::static_scheduling_permissions
            == AccessHandlePermissions::Commutative
      >
    >
    auto
    commutative_access() && {
      using namespace darma_runtime::detail::flow_relationships;
      // TODO make_indexed_?_flow
      use_holder_ = std::make_shared<UseHolder>(
        HandleUse(
          parent_.var_handle_.get_smart_ptr(),
          frontend::Permissions::Commutative,
          frontend::Permissions::None,
          indexed_flow(&parent_.current_use_->use->in_flow_, backend_index_),
          //FlowRelationship::Indexed, &parent_.current_use_->use->in_flow_,
          indexed_flow(&parent_.current_use_->use->out_flow_, backend_index_)
          //FlowRelationship::Indexed, &parent_.current_use_->use->out_flow_, false,
#if _darma_has_feature(anti_flows)
          , indexed_anti_flow(&parent_.current_use_->use->anti_in_flow_, backend_index_),
          insignificant_flow()
#endif // _darma_has_feature(anti_flows)
        )
      );

      return AccessHandle<value_type,
        make_access_handle_traits_t<value_type,
          copy_assignability<false>,
          static_scheduling_permissions<AccessHandlePermissions::Commutative>,
          required_scheduling_permissions<AccessHandlePermissions::Commutative>,
          access_handle_trait_tags::allocation_traits<
            typename parent_traits_t::allocation_traits
          >
        >
      >(
        parent_.var_handle_.get_smart_ptr(),
        std::move(use_holder_)
      );
    };
#endif // _darma_has_feature(commutative_access_handles)

    //------------------------------------------------------------------------------
    // <editor-fold desc="friends"> {{{2

    template <typename, typename, typename>
    friend
    class darma_runtime::AccessHandleCollection;

    // </editor-fold> end friends }}}2
    //------------------------------------------------------------------------------
};


// </editor-fold> end IndexedAccessHandle }}}1
//==============================================================================

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_TASK_COLLECTION_INDEXED_ACCESS_HANDLE_H
