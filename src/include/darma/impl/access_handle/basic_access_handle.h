/*
//@HEADER
// ************************************************************************
//
//                      basic_access_handle.h
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

#ifndef DARMAFRONTEND_BASIC_ACCESS_HANDLE_H
#define DARMAFRONTEND_BASIC_ACCESS_HANDLE_H

#include <darma/impl/access_handle_base.h>
#include <darma/impl/use/use_holder.h>

namespace darma_runtime {
namespace detail {

template <typename ParentAHC, typename UseHolderPtr>
class IndexedAccessHandle;

// forward declaration
template <typename Op>
struct all_reduce_impl;

struct unfetched_access_handle_tag { };

template <typename AccessHandleT>
struct _publish_impl;

template <typename T, typename... TraitsFlags>
struct _initial_access_key_helper;

template <typename>
struct _read_access_helper;

class BasicAccessHandle : public AccessHandleBase {

  //============================================================================
  // <editor-fold desc="type aliases"> {{{1

  protected:

    using use_holder_t = detail::UseHolder<HandleUse>;

  // </editor-fold> end type aliases }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="constructors"> {{{1

  protected:

    BasicAccessHandle()
      : AccessHandleBase()
    { }

    BasicAccessHandle(BasicAccessHandle&& other) noexcept
      : AccessHandleBase(std::move(other))
    { }
    
    BasicAccessHandle(
      std::shared_ptr<VariableHandleBase> const& var_handle,
      std::shared_ptr<use_holder_t> const& use_holder_ptr
    ) : AccessHandleBase(var_handle, use_holder_ptr)
    { }

  // </editor-fold> end constructors }}}1
  //============================================================================


  //==============================================================================
  // <editor-fold desc="assignment operator implementations"> {{{1

    void _do_assignment(BasicAccessHandle const& other) {
        this->AccessHandleBase::_do_assignment(other);
    }

    void _do_assignment(BasicAccessHandle&& other) noexcept {
      this->AccessHandleBase::_do_assignment(std::move(other));
    }


  // </editor-fold> end assignment operator implementations }}}1
  //==============================================================================


  protected:

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

    // TODO cull these!

    friend class detail::create_work_attorneys::for_AccessHandle;
    friend class detail::access_attorneys::for_AccessHandle;

    template <typename, typename, size_t, typename>
    friend
    struct detail::_task_collection_impl::_get_task_stored_arg_helper;

    template <typename, typename, typename, typename>
    friend
    struct detail::_task_collection_impl::_get_storage_arg_helper;

    friend class detail::TaskBase;

    template <typename, typename>
    friend
    class AccessHandle;

    template <typename, typename, typename>
    friend
    class AccessHandleCollection;

    template <typename, typename>
    friend struct AccessHandleCollectionCaptureDescription;

    template <typename, typename>
    friend struct AccessHandleCaptureDescription;

    template <typename, typename, typename...>
    friend
    class detail::TaskCollectionImpl;

    template <typename Op>
    friend
    struct detail::all_reduce_impl;

    template <typename AccessHandleT>
    friend
    struct detail::_publish_impl;

    template <typename, typename...>
    friend struct detail::_commutative_access_impl;
    template <typename, typename...>
    friend struct detail::_noncommutative_access_impl;

    template <typename, typename>
    friend
    class detail::IndexedAccessHandle;

    // Analogs with different privileges are friends too
    friend struct detail::analogous_access_handle_attorneys::AccessHandleAccess;

    // Allow implicit conversion to value in the invocation of the task
    friend struct meta::splat_tuple_access<detail::AccessHandleBase>;

    template <typename, typename...>
    friend struct detail::_initial_access_key_helper;

    template <typename>
    friend struct detail::_read_access_helper;

    template <typename>
    friend class detail::CopyCapturedObject;

    template <typename>
    friend class BasicAccessHandleCollection;

  // </editor-fold> end friends }}}1
  //============================================================================

};

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_BASIC_ACCESS_HANDLE_H
