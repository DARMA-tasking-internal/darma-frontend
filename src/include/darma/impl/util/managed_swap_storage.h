/*
//@HEADER
// ************************************************************************
//
//                      managed_swap_storage.h
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

#ifndef DARMAFRONTEND_MANAGED_SWAP_STORAGE_H
#define DARMAFRONTEND_MANAGED_SWAP_STORAGE_H

#include <type_traits>
#include <cassert>

namespace darma_runtime {
namespace detail {

namespace util {

struct in_place_tag_t { };
constexpr in_place_tag_t in_place_tag { };

namespace _impl {

/**
 *  @internal
 *  @brief This is basically __optional_storage from the standard library
 *
 *  Large portions essentially copied from the standard library.
 */
template <typename T>
struct _inline_storage
{
    // Exactly the same as the trivially destructible case, but with a destructor

    using value_type = T;

    union {
      char null_state;
      value_type value;
    };

    bool is_active = false;

    constexpr _inline_storage() noexcept
      : null_state('\0')
    { }

    template <
      typename _sfinae_only=void,
      typename=std::enable_if_t<
        std::is_void<_sfinae_only>::value
          and std::is_copy_constructible<value_type>::value
      >
    >
    _inline_storage(_inline_storage const& other)
      : is_active(other.is_active)
    {
      if(is_active) {
        new (std::addressof(value)) value_type(other.value);
      }
    }

    template <
      typename _sfinae_only=void,
      typename=std::enable_if_t<
        std::is_void<_sfinae_only>::value
          and std::is_move_constructible<value_type>::value
      >
    >
    _inline_storage(_inline_storage&& other)
      : is_active(other.is_active)
    {
      if(is_active) {
        new (std::addressof(value)) value_type(std::move(other.value));
      }
    }

    template <
      typename _sfinae_only=void,
      typename=std::enable_if_t<
        std::is_void<_sfinae_only>::value
          and std::is_copy_constructible<value_type>::value
      >
    >
    _inline_storage(value_type const& other)
      : value(other),
        is_active(true)
    { }

    template <
      typename _sfinae_only=void,
      typename=std::enable_if_t<
        std::is_void<_sfinae_only>::value
          and std::is_move_constructible<value_type>::value
      >
    >
    _inline_storage(value_type&& other)
      : value(std::move(other)),
        is_active(true)
    { }

    template <
      typename... CtorArgs
    >
    _inline_storage(in_place_tag_t, CtorArgs&&... args)
      : value(std::forward<CtorArgs>(args)...),
        is_active(true)
    { }


    ~_inline_storage() {
      if(is_active) {
        value.~value_type();
      }
    }
};

} // end namespace _impl


/**
 *  @internal
 *  @brief Basically a pair of std::optionals with some bells and whistles
 *
 *
 *  Used to store `Use` objects in a `UseHolder` without any pointers to
 *  heap-allocated memory.  Since the specification requires that the address
 *  of a `Use` remain unchanged throughout its lifetime, and since a
 *  `UseHolder` often needs to register one `Use` before releasing another,
 *  we need inline storage for two `Use` instances (with only one active
 *  for the vast majority of the time).
 *
 *  @tparam ConcreteT Type of the value to be stored
 *  @tparam BaseT Type of the pointer to the base, a reference to which be
 *          maintained to point to the active value
 */
template <typename ConcreteT, typename BaseT>
struct managed_swap_storage {

  public:

    using value_type = ConcreteT;
    using base_pointer = BaseT*;

  private:

    using inline_storage_t = _impl::_inline_storage<value_type>;

    inline_storage_t first_;
    inline_storage_t second_;
    bool first_is_active_;
    base_pointer& managed_;

    /// Active is maintained to avoid branches on get_value(), which would be
    /// an unacceptable overhead.
    value_type* active_ = nullptr;

  public:

    template <typename... CtorArgs>
    managed_swap_storage(
      base_pointer& managed,
      in_place_tag_t,
      CtorArgs&&... args
    ) : first_(
          in_place_tag,
          std::forward<CtorArgs>(args)...
        ),
        managed_(managed),
        first_is_active_(true)
    {
      managed_ = active_ = std::addressof(first_.value);
    }


  //============================================================================
  // <editor-fold desc="swap() and variants"> {{{1

    template <
      typename Callback, typename... CtorArgs
    >
    void
    swap_with_callback_before_destruction(
      Callback&& called_between_ctor_and_destruction,
      in_place_tag_t,
      CtorArgs&&... args
    ) {
      assert(active_ || !"Can't call swap on a null managed_swap_storage!");
      if(first_is_active_) {
        assert(first_.is_active);
        assert(not second_.is_active);
        managed_ = active_ = new (std::addressof(second_.value)) value_type(
          std::forward<CtorArgs>(args)...
        );
        second_.is_active = true;
        first_is_active_ = false;
        std::forward<Callback>(called_between_ctor_and_destruction)(
          second_.value,
          first_.value
        );
        first_.value.~value_type();
        first_.is_active = false;
      }
      else {
        assert(second_.is_active);
        assert(not first_.is_active);
        managed_ = active_ = new (std::addressof(first_.value)) value_type(
          std::forward<CtorArgs>(args)...
        );
        first_.is_active = true;
        first_is_active_ = true;
        std::forward<Callback>(called_between_ctor_and_destruction)(
          first_.value,
          second_.value
        );
        second_.value.~value_type();
        second_.is_active = false;
      }
    };

    template <
      typename ConcreteConvertibleT,
      typename Callback,
      // Distinguish from null version of variadic overload above
      typename=std::enable_if_t<
        not std::is_same<std::decay_t<Callback>, in_place_tag_t>::value
      >
    >
    void
    swap_with_callback_before_destruction(
      ConcreteConvertibleT&& new_value,
      Callback&& called_between_ctor_and_destruction
    ) {
      swap_with_callback_before_destruction(
        std::forward<Callback>(called_between_ctor_and_destruction),
        in_place_tag,
        std::forward<ConcreteConvertibleT>(new_value)
      );
    };


    template <
      typename... CtorArgs
    >
    void
    swap(
      CtorArgs&&... args
    ) {
      swap_with_callback_before_destruction(
        []{},
        in_place_tag,
        std::forward<CtorArgs>(args)...
      );
    };

  // </editor-fold> end swap() and variants }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="value accessors"> {{{1

    inline constexpr value_type& value() & {
      assert(active_ || !"Can't call value() on null managed_swap_storage!");
      return *active_;
    }
    inline constexpr value_type const & value() const & {
      assert(active_ || !"Can't call value() on null managed_swap_storage!");
      return *active_;
    }
    inline constexpr value_type&& value() && {
      assert(active_ || !"Can't call value() on null managed_swap_storage!");
      return *active_;
    }
    inline constexpr value_type const && value() const && {
      assert(active_ || !"Can't call value() on null managed_swap_storage!");
      return *active_;
    }

    inline constexpr value_type const* get() const { return active_; }
    inline constexpr value_type* get() { return active_; }
    inline constexpr value_type const* operator->() const { return active_; }
    inline constexpr value_type* operator->() { return active_; }

    inline constexpr value_type& operator*() & {
      assert(active_ || !"Can't dereference null managed_swap_storage!");
      return *active_;
    }
    inline constexpr value_type const & operator*() const & {
      assert(active_ || !"Can't dereference null managed_swap_storage!");
      return *active_;
    }
    inline constexpr value_type&& operator*() && {
      assert(active_ || !"Can't dereference null managed_swap_storage!");
      return *active_;
    }
    inline constexpr value_type const && operator*() const && {
      assert(active_ || !"Can't dereference null managed_swap_storage!");
      return *active_;
    }

    inline constexpr explicit operator bool() const noexcept {
      return active_ != nullptr;
    }
    inline constexpr bool has_value() const noexcept {
      return active_ != nullptr;
    }

  // </editor-fold> end value accessors }}}1
  //============================================================================


    inline void reset() {
      assert(active_ || !"Can't reset null managed_swap_storage!");
      assert(first_is_active_ || second_.is_active || !"Sanity check failed");
      assert(!first_is_active_ || first_.is_active || !"Sanity check failed");
      active_->~value_type();
      if(first_is_active_) {
        assert(first_.is_active || !"Sanity check failed");
        first_.is_active = false;
      }
      else {
        assert(second_.is_active || !"Sanity check failed");
        second_.is_active = false;
      }
      managed_ = active_ = nullptr;
    }

    template <typename... CtorArgs>
    inline void emplace(
      CtorArgs&&... args
    ) {
      if(active_ == nullptr) {
        managed_ = active_ = new (std::addressof(first_.value)) value_type(
          std::forward<CtorArgs>(args)...
        );
        first_is_active_ = true;
      }
      else if(first_is_active_) {
        first_.value.~value_type();
        managed_ = active_ = new (std::addressof(first_.value)) value_type(
          std::forward<CtorArgs>(args)...
        );
      }
      else {
        assert(!first_is_active_ || !"Sanity check failed");
        second_.value.~value_type();
        managed_ = active_ = new (std::addressof(second_.value)) value_type(
          std::forward<CtorArgs>(args)...
        );
      }
    }


};


} // end namespace util
} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_MANAGED_SWAP_STORAGE_H
