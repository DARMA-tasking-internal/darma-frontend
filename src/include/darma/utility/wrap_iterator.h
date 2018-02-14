/*
//@HEADER
// ************************************************************************
//
//                      wrap_iterator.h
//                         DARMA
//              Copyright (C) 2018 Sandia Corporation
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

#ifndef DARMAFRONTEND_UTILITY_WRAP_ITERATOR_H
#define DARMAFRONTEND_UTILITY_WRAP_ITERATOR_H

#include <iterator>
#include <utility>

namespace darma_runtime {
namespace utility {

/** @brief Formally wraps an Integer into a RandomAccessIterator based on the
 *         concept requirements in the standard
 */
template <typename Integer>
class integer_wrap_iterator {
  private:

    Integer value_;

  public:

    //--------------------------------------------------------------------------
    // <editor-fold desc="Iterator"> {{{2
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Iterator requirements:
    //   - required by InputIterator
    //   + requires CopyConstructible
    //   + requires CopyAssignable
    //   + requires Destructible
    //   + requires Swappable

    using value_type = std::remove_cv_t<Integer>;
    using difference_type = std::make_signed_t<decltype(std::declval<value_type const&>() - std::declval<value_type const&>())>;
    using pointer = Integer const*;
    using reference = Integer const&;
    using iterator_category = std::random_access_iterator_tag;

    constexpr
    integer_wrap_iterator& operator++() {
      ++value_;
      return *this;
    }

    constexpr
    reference operator*() {
      return value_;
    }

    // </editor-fold> }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="InputIterator"> {{{2
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // InputIterator requirements:
    //   - required by ForwardIterator
    //   + requires EqualityComparable
    //   + requires Iterator
    //   * (operator*() and operator++() requirements implemented under Iterator)

    constexpr auto
    operator!=(integer_wrap_iterator const& other) const noexcept {
      return value_ != other.value_;
    }

    constexpr pointer operator->() const noexcept { return &value_; }

    constexpr integer_wrap_iterator operator++(int) noexcept {
      auto rv = integer_wrap_iterator(value_);
      value_++;
      return rv;
    }

    // </editor-fold> }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="ForwardIterator"> {{{2
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // ForwardIterator requirements:
    //   - required by BidirectionalIterator
    //   + requires DefaultConstructible
    //   + requires InputIterator
    //   * (operator*() and operator++(int) requirements implemented under InputIterator)
    // </editor-fold> }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="BidirectionalIterator"> {{{2
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // InputIterator requirements:
    //   - required by RandomAccessIterator
    //   + requires ForwardIterator

    constexpr integer_wrap_iterator& operator--() noexcept {
      --value_;
      return *this;
    }

    constexpr integer_wrap_iterator& operator--(int) noexcept {
      auto rv = integer_wrap_iterator(value_);
      value_--;
      return rv;
    }

    // </editor-fold> }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="RandomAccessIterator"> {{{2
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // InputIterator requirements:
    //   + requires BidiretionalIterator

    constexpr integer_wrap_iterator&
    operator +=(difference_type n) & noexcept {
      value_ += n;
      return *this;
    }

    constexpr integer_wrap_iterator
    operator +(difference_type n) const noexcept {
      return integer_wrap_iterator(value_ + n);
    }

    constexpr integer_wrap_iterator&
    operator -=(difference_type n) & noexcept {
      value_ -= n;
      return *this;
    }

    constexpr integer_wrap_iterator
    operator -(difference_type n) const noexcept {
      return integer_wrap_iterator(value_ - n);
    }

    constexpr reference
    operator[](difference_type n) const noexcept {
      return value_ + n;
    }

    constexpr auto
    operator <(integer_wrap_iterator const& other) const noexcept {
      return value_ < other.value_;
    }

    constexpr auto
    operator >(integer_wrap_iterator const& other) const noexcept {
      return value_ > other.value_;
    }

    constexpr auto
    operator <=(integer_wrap_iterator const& other) const noexcept {
      return value_ <= other.value_;
    }

    constexpr auto
    operator >=(integer_wrap_iterator const& other) const noexcept {
      return value_ >= other.value_;
    }

    // </editor-fold> }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="EqualityComparable"> {{{2
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // EqualityComparable requirements:
    //   - required by InputIterator

    constexpr auto
    operator==(integer_wrap_iterator const& other) const noexcept {
      return value_ == other.value_;
    }

    // </editor-fold> }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="CopyConstructible"> {{{2
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // CopyConstructible and CopyAssignable requirements:
    //   + requires MoveConstructible and MoveAssignable
    //   - required by Iterator

    constexpr
    integer_wrap_iterator(integer_wrap_iterator const& other) noexcept
      : value_(other.value_)
    { }

    constexpr
    integer_wrap_iterator&
    operator=(integer_wrap_iterator const& other) noexcept {
      value_ = other.value_;
      return *this;
    }

    // </editor-fold> }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="MoveConstructible"> {{{2
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // MoveConstructible and MoveAssignable requirements:
    //   - required by CopyConstructible and CopyAssignable
    //   * Also satisfies Swappable via std::swap on MoveAssiganble (required by Iterator)

    constexpr
    integer_wrap_iterator(integer_wrap_iterator&& other) noexcept
      : value_(std::move(other.value_))
    { }

    constexpr
    integer_wrap_iterator&
    operator=(integer_wrap_iterator&& other) noexcept {
      value_ = std::move(other.value_);
      return *this;
    }

    // </editor-fold> }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="DefaultConstructible"> {{{2
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // DefaultConstructible requirements:
    //   - required by ForwardIterator

    constexpr
    integer_wrap_iterator() noexcept
      : value_{}
    { }

    // </editor-fold> }}}2
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // <editor-fold desc="Destructible"> {{{2
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Destructible requirements:
    //   - required by Iterator

    ~integer_wrap_iterator() noexcept = default;

    // </editor-fold> }}}2
    //--------------------------------------------------------------------------

};

// One additional requirement of RandomAccessIterator:
template <typename Integer>
integer_wrap_iterator<Integer>
operator+(
  typename integer_wrap_iterator<Integer>::difference_type n,
  integer_wrap_iterator<Integer> const& it
) {
  return it + n;
}

} // end namespace utility
} // end namespace darma_runtime

#endif //DARMAFRONTEND_UTILITY_WRAP_ITERATOR_H
