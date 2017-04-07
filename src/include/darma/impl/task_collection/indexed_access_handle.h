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

#include <darma/impl/util/static_assertions.h> // _darma__static_failure
#include <darma/impl/darma_assert.h>
#include <darma/impl/handle_fwd.h> // AccessHandlePermissions
#include <darma/impl/keyword_arguments/parse.h> // AccessHandlePermissions

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

    using parent_traits_t = typename ParentAHC::traits_t;
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

    // We have to make the move ctor templated and non-default to make the
    // error-displaying copy ctor show up (rather than a copy ctor implicitly
    // deleted message).  Also, this is private because
    template <typename _Ignored_SFINAE=void>
    IndexedAccessHandle(IndexedAccessHandle&& other)
      : parent_(other.parent_),
        use_holder_(other.use_holder_),
        has_local_access_(std::move(other.has_local_access_)),
        backend_index_(std::move(other.backend_index_))
    { /* should never execute because of move elision RVO */ }                  // LCOV_EXCL_LINE

    // </editor-fold> end private ctors }}}2
    //------------------------------------------------------------------------------

  public:

    //------------------------------------------------------------------------------
    // <editor-fold desc="error-generating copy ctor"> {{{2

    // Basically, if anything would cause the generation of a copy ctor, don't
    // let it, and instead instantiate a darma static failure (because and_
    // will try to access ::type of _darma__static_failure, which will generate
    // the error)
    // TODO make sure this actually generates the right error; SFINAE might stop it from doing so
//    template <
//      typename _for_SFINAE_only=void,
//      typename=tinympl::and_<
//        std::is_void<_for_SFINAE_only>,
//        _darma__static_failure<
//#ifdef DARMA_PRETTY_PRINT_COMPILE_TIME_ERRORS
//          typename _darma__errors::_____cannot_capture_return_of_AccessHandleCollection_subscript_directly
//            ::template __for_access_handle_collection_wrapping_type<AHValueType>
//            ::__must_call__local_access__or__read_access__method_first
//#else
//          __________use_dash_D_DARMA_PRETTY_PRINT_COMPILE_TIME_ERRORS__to_see_a_better_error_message
//#endif
//        >
//      >
//    >
//    IndexedAccessHandle(IndexedAccessHandle const&) { /* unreachable */ }       // LCOV_EXCL_LINE

    // </editor-fold> end error-generating copy ctor }}}2
    //------------------------------------------------------------------------------


    // Can only be called as part of `ahc[...].local_access()`; rvalue reference
    // specifier enforces this
    auto
    local_access() const && {
      DARMA_ASSERT_MESSAGE(has_local_access_,
        "Attempted to obtain local access to index of AccessHandleCollection"
          " that is not local"
      );
      // TODO mangled name shortening optimization by making these traits a subclass of access_handle_traits
      return AccessHandle<value_type,
        make_access_handle_traits_t<value_type,
          copy_assignability<false>,
          access_handle_trait_tags::permissions_traits<
            typename parent_traits_t::permissions_traits
          >,
          access_handle_trait_tags::allocation_traits<
            typename parent_traits_t::allocation_traits
          >
        >
      >(
        parent_.var_handle_.get_smart_ptr(),
        std::move(use_holder_)
      );
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

          auto* backend_runtime = abstract::backend::get_backend_runtime();

          assert(use_holder_ == nullptr);

          // Make the use holder
          use_holder_ = std::make_shared<typename std::pointer_traits<UseHolderPtr>::element_type>(
            HandleUse(
              parent_.var_handle_.get_smart_ptr(),
              HandleUse::Read, // Read scheduling permissions
              HandleUse::None, // No immediate permissions
              /* In flow description */
              FlowRelationship::IndexedFetching, &parent_.current_use_->use->in_flow_,
              /* Out flow description */
              FlowRelationship::Same, nullptr, true,
              /* In flow version key and index */
              &version_key, backend_index_
              /* Out flow version key and index not needed */
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
            parent_.var_handle_.get_smart_ptr(),
            std::move(use_holder_)
          );

        });

    }


    template <typename _Ignored_SFINAE=void,
      typename=std::enable_if_t<
        std::is_void<_Ignored_SFINAE>::value
          and ParentAHC::traits_t::permissions_traits::static_scheduling_permissions
            == AccessHandlePermissions::Commutative
      >
    >
    auto
    commutative_access() && {
      using namespace darma_runtime::abstract::frontend;
      // TODO make_indexed_?_flow
      use_holder_ = std::make_shared<UseHolder>(
        HandleUse(
          parent_.var_handle_.get_smart_ptr(),
          HandleUse::Commutative,
          HandleUse::None,
          FlowRelationship::Indexed, &parent_.current_use_->use->in_flow_,
          FlowRelationship::Indexed, &parent_.current_use_->use->out_flow_, false,
          nullptr, backend_index_,
          nullptr, backend_index_
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