/*
//@HEADER
// ************************************************************************
//
//                      access_handle_collection_base.h
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

#ifndef DARMAFRONTEND_ACCESS_HANDLE_COLLECTION_BASE_H
#define DARMAFRONTEND_ACCESS_HANDLE_COLLECTION_BASE_H

#include <darma/impl/task_collection/task_collection_fwd.h>

#include <darma/impl/access_handle_base.h>

namespace darma_runtime {

namespace serialization {

// Forward declaration
template <typename T, typename IndexRangeT, typename Traits>
struct Serializer<darma_runtime::AccessHandleCollection<T, IndexRangeT, Traits>>;

} // end namespace serialization

namespace detail {

template <typename... AccessorDetails>
struct AccessHandleCollectionAccess;

template <typename, typename>
struct AccessHandleCollectionCaptureDescription;

template <typename IndexRangeT>
class BasicAccessHandleCollection
  : public AccessHandleBase
{
  public:

    using index_range_type = IndexRangeT;

  protected:

    using use_t = BasicCollectionManagingUse<IndexRangeT>;
    using use_holder_t = UseHolder<use_t>;
    using index_range_traits_t = indexing::index_range_traits<index_range_type>;
    using mapping_to_dense_t = typename index_range_traits_t::mapping_to_dense_type;
    using mapping_to_dense_traits_t = indexing::mapping_traits<mapping_to_dense_t>;
    using element_use_holder_t = BasicAccessHandle::use_holder_t;
    using element_use_holder_ptr = std::shared_ptr<element_use_holder_t>;

  protected:

    BasicAccessHandleCollection()
      : AccessHandleBase()
    { }

    BasicAccessHandleCollection(BasicAccessHandleCollection&& other) noexcept
    : AccessHandleBase(std::move(other))
    { }

    BasicAccessHandleCollection(
      std::shared_ptr<VariableHandleBase> const& var_handle,
      std::shared_ptr<use_holder_t> const& use_holder_ptr
    ) : AccessHandleBase(var_handle, use_holder_ptr)
    { }

    inline bool
    has_use_holder() const {
        return current_use_base_ != nullptr;
    }

    inline use_holder_t*
    get_current_use() const {
        assert(has_use_holder() || !"get_current_use() called when use holder was null");
        return utility::safe_static_cast<use_holder_t*>(current_use_base_.get());
    }

    inline void
    set_current_use(std::shared_ptr<use_holder_t> const& new_use_ptr) const {
        current_use_base_ = new_use_ptr;
    }

  //============================================================================
  // <editor-fold desc="friends"> {{{1

  private:

    // TODO cull these!

    template <typename, typename>
    friend struct detail::AccessHandleCollectionCaptureDescription;

    template <typename, typename, typename>
    friend
    class AccessHandleCollection;

    template <typename... AccessorDetails>
    friend
    struct detail::AccessHandleCollectionAccess;

    template <typename, typename, typename, typename>
    friend
    struct detail::_task_collection_impl::_get_storage_arg_helper;

    template <typename, typename, size_t, typename>
    friend
    struct detail::_task_collection_impl::_get_task_stored_arg_helper;

    template <typename, typename, typename>
    friend
    struct detail::_task_collection_impl::_get_call_arg_helper;

    template <typename, typename>
    friend
    struct detail::MappedHandleCollection;

    template <typename, typename>
    friend
    struct detail::IndexedAccessHandle;

    template <typename, typename, typename...>
    friend
    struct detail::TaskCollectionImpl;

//    template <typename T, typename Range, typename Traits>
//    friend
//    struct serialization::Serializer<AccessHandleCollection<T, Range, Traits>>;


    template <typename, typename...>
    friend struct detail::_commutative_access_impl;

    template <typename, typename...>
    friend struct detail::_noncommutative_collection_access_impl;

  // </editor-fold> end friends }}}1
  //============================================================================

};


} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_ACCESS_HANDLE_COLLECTION_BASE_H
