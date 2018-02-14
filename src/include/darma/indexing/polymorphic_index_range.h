/*
//@HEADER
// ************************************************************************
//
//                      polymorphic_index_range.h
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

#ifndef DARMAFRONTEND_INDEXING_POLYMORPHIC_INDEX_RANGE_H
#define DARMAFRONTEND_INDEXING_POLYMORPHIC_INDEX_RANGE_H

#include <cstdint>
#include <darma/serialization/polymorphic/polymorphic_serialization_adapter.h>

namespace darma_runtime {
namespace indexing {

//==============================================================================
// <editor-fold desc="PolymprphicIndexRange pimpl"> {{{1

namespace detail {

struct _polymorphic_index_range_impl_base {

  virtual size_t size() const =0;

  virtual ~_polymorphic_index_range_impl_base() = default;

  virtual std::unique_ptr<_polymorphic_index_range_impl_base> clone() const =0;

};

template <typename IndexRange>
struct _polymorphic_index_range_impl
  : public serialization::PolymorphicSerializationAdapter<
      _polymorphic_index_range_impl<IndexRange>,
      _polymorphic_index_range_impl_base
    >
{
  private:

    IndexRange range_;

  public:

    _polymorphic_index_range_impl(_polymorphic_index_range_impl const&) = default;
    _polymorphic_index_range_impl(_polymorphic_index_range_impl&&) = default;
    _polymorphic_index_range_impl& operator=(_polymorphic_index_range_impl const&) = default;
    _polymorphic_index_range_impl& operator=(_polymorphic_index_range_impl&&) = default;

    _polymorphic_index_range_impl(IndexRange const& rng)
      : range_(rng)
    { }

    size_t size() const override {
      return range_.size();
    }

    std::unique_ptr<_polymorphic_index_range_impl_base> clone() const override {
      return std::make_unique<_polymorphic_index_range_impl>(*this);
    }

    ~_polymorphic_index_range_impl() override = default;
};

} // end namespace detail

// </editor-fold> end PolymprphicIndexRange pimpl }}}1
//==============================================================================

/** @brief Polymorphic wrapper for objects meeting the IndexRange concept
 *
 */
class PolymorphicIndexRange {
  private:

    std::unique_ptr<detail::_polymorphic_index_range_impl_base> impl_ = nullptr;

  public:

    PolymorphicIndexRange() = default;
    PolymorphicIndexRange(PolymorphicIndexRange&&) = default;
    PolymorphicIndexRange(PolymorphicIndexRange const& other)
      : impl_(other.impl_->clone())
    { }

    PolymorphicIndexRange& operator=(PolymorphicIndexRange&&) = default;
    PolymorphicIndexRange& operator=(PolymorphicIndexRange const& other) {
      impl_ = other.impl_->clone();
      return *this;
    }

    template <typename IndexRange>
    PolymorphicIndexRange(IndexRange const& rng)
      : impl_(std::make_unique<detail::_polymorphic_index_range_impl<IndexRange>>(rng))
    { }

    size_t size() const {
      return impl_->size();
    }

    template <typename Archive>
    void serialize(Archive& ar) {
      ar | impl_;
    }
};

} // end namespace indexing
} // end namespace darma_runtime

#endif //DARMAFRONTEND_INDEXING_POLYMORPHIC_INDEX_RANGE_H
