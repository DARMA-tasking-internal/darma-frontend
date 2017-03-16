/*
//@HEADER
// ************************************************************************
//
//                      managing_ptr.h
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

#ifndef DARMA_IMPL_UTIL_MANAGING_PTR_H
#define DARMA_IMPL_UTIL_MANAGING_PTR_H

#include <type_traits>
#include <darma/impl/util/not_a_type.h>

namespace darma_runtime {
namespace detail {

// TODO unit tests for managing_ptr

// A wrapper around a smart pointer that sets the value of some dumb
// pointer to a base class of the smart pointer's underlying type
// (This is not thread safe, since the two pointers are set in succession)
template <typename UnderlyingSmartPtr, typename BasePtr>
class managing_ptr {
  public:

    using smart_ptr_t = UnderlyingSmartPtr;
    using managed_ptr_t = BasePtr;

  private:

    smart_ptr_t smart_ptr_ = nullptr;
    managed_ptr_t& managed_ptr_;

    template <typename SmartPtrT, typename... Args>
    using _smart_ptr_is_constructible_archetype = decltype(
      SmartPtrT(std::declval<Args>()...)
    );

    template <typename... Args>
    using smart_ptr_is_constructible_with = tinympl::bool_<
      meta::is_detected<
        _smart_ptr_is_constructible_archetype, smart_ptr_t, Args...
      >::value
    >;

    template <typename SmartPtrT, typename Rhs>
    using _smart_ptr_is_assignable_archetype = decltype(
      std::declval<SmartPtrT&>() = std::declval<Rhs>()
    );

    template <typename Rhs>
    using smart_ptr_is_assignable_from = tinympl::bool_<
      meta::is_detected<
        _smart_ptr_is_assignable_archetype, smart_ptr_t, Rhs
      >::value
    >;

    using arrow_operator_return_t = decltype(
      std::declval<smart_ptr_t&>().operator->()
    );

    using const_arrow_operator_return_t = decltype(
      std::declval<smart_ptr_t const&>().operator->()
    );

    using dereference_operator_return_t = decltype(
      std::declval<smart_ptr_t&>().operator*()
    );

    using const_dereference_operator_return_t = decltype(
      std::declval<smart_ptr_t const&>().operator*()
    );

    template <typename SmartPtrT, typename ManagedPtrT>
    using _managed_is_directly_assignable_archetype = decltype(
      std::declval<ManagedPtrT&>() = std::declval<SmartPtrT>()
    );

    using managed_is_directly_assignable = tinympl::bool_<
      meta::is_detected<
        _managed_is_directly_assignable_archetype, smart_ptr_t, managed_ptr_t
      >::value
    >;

    template <typename _Ignored_SFINAE=void>
    void
    _assign_managed(
      std::enable_if_t<
        std::is_void<_Ignored_SFINAE>::value
          and managed_is_directly_assignable::value,
        _not_a_type_numbered<1>
      > = {}
    ) {
      managed_ptr_ = smart_ptr_;
    }

    template <typename _Ignored_SFINAE=void>
    void
    _assign_managed(
      std::enable_if_t<
        std::is_void<_Ignored_SFINAE>::value
          and not managed_is_directly_assignable::value,
        _not_a_type_numbered<2>
      > = {}
    ) {
      managed_ptr_ = smart_ptr_.get();
    }

    template <typename _Ignored_SFINAE=void>
    void
    _assign_managed(
      nullptr_t,
      std::enable_if_t<
        std::is_void<_Ignored_SFINAE>::value
          and managed_is_directly_assignable::value,
        _not_a_type_numbered<1>
    > = {}
    ) {
      managed_ptr_.reset(nullptr);
    }

    template <typename _Ignored_SFINAE=void>
    void
    _assign_managed(
      nullptr_t,
      std::enable_if_t<
        std::is_void<_Ignored_SFINAE>::value
          and not managed_is_directly_assignable::value,
        _not_a_type_numbered<2>
    > = {}
    ) {
      managed_ptr_ = nullptr;
    }

  public:

    managing_ptr() = delete;

    managing_ptr(managed_ptr_t& managed_ptr)
      : managed_ptr_(managed_ptr)
    {
      managed_ptr_ = nullptr;
    }

    template <
      typename Arg1,
      typename... Args
    >
    managing_ptr(
      std::enable_if_t<
        smart_ptr_is_constructible_with<Arg1, Args...>::value,
        managed_ptr_t
      >& managed_ptr,
      Arg1&& arg1, Args&&... args
    ) : smart_ptr_(
          std::forward<Arg1>(arg1),
          std::forward<Args>(args)...
        ),
        managed_ptr_(managed_ptr)
    {
      _assign_managed();
    }

    managing_ptr(
      managed_ptr_t& managed_ptr,
      managing_ptr const& other
    ) : smart_ptr_(other.smart_ptr_),
        managed_ptr_(managed_ptr)
    {
      _assign_managed();
    }

    managing_ptr(
      managed_ptr_t& managed_ptr,
      managing_ptr&& other
    ) : smart_ptr_(other.smart_ptr_),
        managed_ptr_(managed_ptr)
    {
      _assign_managed();
    }

    managing_ptr& operator=(std::nullptr_t) {
      smart_ptr_ = nullptr;
      managed_ptr_ = nullptr;
      return *this;
    }

    managing_ptr& operator=(
      managing_ptr const& other
    ) {
      smart_ptr_ = other.smart_ptr_;
      _assign_managed();
      return *this;
    }

    managing_ptr& operator=(
      managing_ptr&& other
    ) {
      smart_ptr_ = std::move(other.smart_ptr_);
      _assign_managed();
      return *this;
    }

    template <typename Rhs,
      typename=std::enable_if_t<
        not std::is_base_of<std::decay_t<Rhs>, managing_ptr>::value
        and smart_ptr_is_assignable_from<Rhs>::value
      >
    >
    managing_ptr& operator=(
      Rhs&& other
    ) {
      smart_ptr_ = std::forward<Rhs>(other);
      _assign_managed();
      return *this;
    }

    arrow_operator_return_t
    operator->() { return smart_ptr_.operator->(); }

    const_arrow_operator_return_t
    operator->() const { return smart_ptr_.operator->(); }

    dereference_operator_return_t
    operator*() { return smart_ptr_.operator*(); }

    const_dereference_operator_return_t
    operator*() const { return smart_ptr_.operator*(); }

    auto get() { return smart_ptr_.get(); }
    auto get() const { return smart_ptr_.get(); }

    // const only, so it can't be set and thus lose the managed ptr
    smart_ptr_t const&
    get_smart_ptr() const { return smart_ptr_; }

    smart_ptr_t&&
    release_smart_ptr() {
      _assign_managed(nullptr);
      return std::move(smart_ptr_);
    }

    bool operator==(std::nullptr_t) const {
      return smart_ptr_ == nullptr;
    }

    bool operator!=(std::nullptr_t) const {
      return smart_ptr_ != nullptr;
    }

    operator bool() const {
      return bool(smart_ptr_);
    }

    friend void swap(managing_ptr& a, managing_ptr& b) {
      std::swap(a.smart_ptr_, b.smart_ptr_);
      a._assign_managed();
      b._assign_managed();
    };

    friend void swap(managing_ptr& a, smart_ptr_t& b_smart_ptr) {
      std::swap(a.smart_ptr_, b_smart_ptr);
      a._assign_managed();
    };

    friend void swap(smart_ptr_t& a_smart_ptr, managing_ptr& b) {
      std::swap(a_smart_ptr, b.smart_ptr_);
      b._assign_managed();
    };

};

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMA_IMPL_UTIL_MANAGING_PTR_H
