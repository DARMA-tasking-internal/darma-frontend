/*
//@HEADER
// ************************************************************************
//
//                      vector_view.h
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


#ifndef DARMA_INTERFACE_APP_VECTOR_VIEW_H
#define DARMA_INTERFACE_APP_VECTOR_VIEW_H

#include <vector>
#include <type_traits>

#include <darma/impl/darma_assert.h>
#include <darma/impl/serialization/nonintrusive.h>

namespace darma {

namespace detail {

template <typename T>
class basic_vector_view {

  private:

    // i.e., vector of T with default allocator
    using simple_vector_t = std::vector<
      std::remove_const_t<T>, std::allocator<std::remove_const_t<T>>
    >;


  public:

    using reference = T&;
    using const_reference = std::add_const_t<T>&;
    using iterator = T*;
    using const_iterator = std::add_const_t<T>*;
    using size_type = typename simple_vector_t::size_type;

  private:
    T* data_;
    size_type size_;

  public:

    template <typename Allocator>
    basic_vector_view(
      std::vector<T, Allocator>& v,
      size_type offset,
      size_type size
    ) : data_(v.data() + offset), size_(size)
    { }

    template <typename Allocator>
    basic_vector_view(
      std::vector<T, Allocator>& v,
      size_type size
    ) : basic_vector_view(v, 0, size)
    { }

    template <
      typename Allocator,
      typename=std::enable_if_t<
        std::is_const<T>::value
        and not std::is_void<Allocator>::value
      >
    >
    basic_vector_view(
      std::vector<std::remove_const_t<T>, Allocator> const& v,
      size_type offset,
      size_type size
    ) : data_(v.data() + offset), size_(size)
    { }

    template <
      typename Allocator,
      typename=std::enable_if_t<
        std::is_const<T>::value
        and not std::is_void<Allocator>::value
      >
    >
    basic_vector_view(
      std::vector<std::remove_const_t<T>, Allocator> const& v,
      size_type size
    ) : basic_vector_view(v, 0, size)
    { }

    inline size_type
    size() const noexcept { return size_; }

    inline bool
    empty() const noexcept { return size_ == 0; }

    inline reference
    at(size_type pos) {
      DARMA_ASSERT_RELATED_VERBOSE(pos, <, size_); //, "Index out of range");
      return *data_[pos];
    }

    inline const_reference
    at(size_type pos) const {
      DARMA_ASSERT_RELATED_VERBOSE(pos, <, size_); //, "Index out of range");
      return *data_[pos];
    }

    inline iterator
    begin() noexcept { return data_;  }

    inline const_iterator
    begin() const noexcept { return data_; }

    inline const_iterator
    cbegin() const noexcept { return data_; }

    inline iterator
    end() { return data_ + size_; }

    inline const_iterator
    end() const { return data_ + size_; }

    inline const_iterator
    cend() const { return data_ + size_; }

    inline reference
    operator[](size_type pos) { return at(pos); }

    inline reference
    operator[](size_type pos) const { return at(pos); }

    T* data() { return data_; }
    T const* data() const { return data_; }

};

} // end namespace detail

template <typename T>
class offset_vector_view
  : public detail::basic_vector_view<T>
{
  private:

    using base_t = detail::basic_vector_view<T>;

  public:

    using reference = typename base_t::reference;
    using const_reference = typename base_t::const_reference;
    using iterator = typename base_t::iterator;
    using const_iterator = typename base_t::const_iterator;
    using size_type = typename base_t::size_type;

  private:

    size_type offset_;

  public:

    template <typename Allocator>
    offset_vector_view(std::vector<T, Allocator>& v)
      : base_t(v)
    { }


    template <typename Allocator>
    offset_vector_view(
      std::vector<T, Allocator>& v,
      size_type offset,
      size_type size
    ) : base_t(v, offset, size), offset_(offset)
    { }

    template <typename Allocator>
    offset_vector_view(
      std::vector<T, Allocator>& v,
      size_type size
    ) : offset_vector_view(v, 0, size)
    { }

    inline reference
    at(size_type pos) {
      DARMA_ASSERT_RELATED_VERBOSE(pos, >=, offset_); //, "Offset index out of range");
      return base_t::at(pos - offset_);
    }

    inline const_reference
    at(size_type pos) const {
      DARMA_ASSERT_RELATED_VERBOSE(pos, >=, offset_); //, "Offset index out of range");
      return base_t::at(pos - offset_);
    }

    inline reference
    operator[](size_type pos) { return at(pos); }

    inline reference
    operator[](size_type pos) const { return at(pos); }
};

template <typename T>
class vector_view
  : public detail::basic_vector_view<T>
{
  private:

    using base_t = detail::basic_vector_view<T>;

  public:

    using base_t::base_t;

    using reference = typename base_t::reference;
    using const_reference = typename base_t::const_reference;
    using iterator = typename base_t::iterator;
    using const_iterator = typename base_t::const_iterator;
    using size_type = typename base_t::size_type;
};

} // end namespace darma

namespace darma_runtime {

namespace serialization {

template <typename T>
struct Serializer<::darma::vector_view<T>> {
  private:

    // TODO: Packability is what's important, not serializability...
    template <typename Archive>
    using enable_if_serializable_with = std::enable_if_t<
      detail::serializability_traits<T>
        ::template is_serializable_with_archive<Archive>::value
    >;

    using view_t = ::darma::vector_view<T>;

  public:

    template <typename Archive>
    enable_if_serializable_with<Archive>
    compute_size(view_t const& v, Archive& ar) const {
      ar % serialization::range(v.begin(), v.end());
    }

    template <typename Archive>
    enable_if_serializable_with<Archive>
    pack(view_t const& v, Archive& ar) const {
      ar << serialization::range(v.begin(), v.end());
    }

    // Can't be unpacked
};

} // end namespace serialization


} // end namespace darma_runtime

#endif //DARMA_INTERFACE_APP_VECTOR_VIEW_H
