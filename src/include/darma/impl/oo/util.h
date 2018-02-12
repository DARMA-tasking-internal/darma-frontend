/*
//@HEADER
// ************************************************************************
//
//                      util.h
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
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

#ifndef DARMA_IMPL_OO_UTIL_H
#define DARMA_IMPL_OO_UTIL_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(oo_interface)

#include <darma/impl/meta/detection.h>
#include <darma/impl/serialization/serialization_fwd.h> // unpack_constructor_tag_t

namespace darma_runtime {
namespace oo {
namespace detail {

struct empty_base { };

struct oo_sentinel_value_t { };
static constexpr oo_sentinel_value_t oo_sentinel_value = { };

struct never_instantiate_this_t { };
static constexpr never_instantiate_this_t never_instantiate_this = { };

template <typename T>
using _is_chained_base_class_archetype = typename T::_darma__is_chained_base_class;
template <typename T>
using is_chained_base_class = meta::is_detected_exact<std::true_type,
  _is_chained_base_class_archetype, T
>;

// Go backwards through the sequence to get the members in the right order (at least
// for the way most compilers lay out memory)
template <typename Seq, std::size_t I, std::size_t N>
struct _chain_base_classes_impl
  : tinympl::at_t<I-1, Seq>::template link_in_chain<
    _chain_base_classes_impl<Seq, I-1, N>
  >
{

  using _darma__is_chained_base_class = std::true_type;

  using base_t = typename tinympl::at_t<I-1, Seq>::template link_in_chain<
    _chain_base_classes_impl<Seq, I-1, N>
  >;

  // Explicitly default the copy, move, and default constructors
  constexpr inline _chain_base_classes_impl() = default;
  constexpr inline _chain_base_classes_impl(_chain_base_classes_impl const& val) = default;
  constexpr inline _chain_base_classes_impl(_chain_base_classes_impl&& val) = default;
  constexpr inline _chain_base_classes_impl& operator=(_chain_base_classes_impl&&) = default;

  // Forward to base class if it's not a copy or move constructor
  template <typename T,
    typename = std::enable_if_t<
      not std::is_same<std::decay_t<T>, _chain_base_classes_impl>::value
        and is_chained_base_class<std::decay_t<T>>::value
    >
  >
  constexpr inline explicit
  _chain_base_classes_impl(T&& val)
    : base_t(std::forward<T>(val))
  { }

  // Forward the unpacking constructor
  template <typename ArchiveT>
  constexpr inline explicit
  _chain_base_classes_impl(
    serialization::unpack_constructor_tag_t,
    ArchiveT& ar
  ) : base_t(serialization::unpack_constructor_tag, ar)
  { }

};

template <typename Seq, std::size_t N>
struct _chain_base_classes_impl<Seq, 0, N>
{
  using _darma__is_chained_base_class = std::true_type;

  // Explicitly default the copy, move, and default constructors
  constexpr inline _chain_base_classes_impl() = default;
  constexpr inline _chain_base_classes_impl(_chain_base_classes_impl&&) = default;
  constexpr inline _chain_base_classes_impl(_chain_base_classes_impl const&) = default;
  constexpr inline _chain_base_classes_impl& operator=(_chain_base_classes_impl&&) = default;

  // forward other constructors taking types that have chained bases to the default
  // constructor, since there are no more members to transfer
  template <typename T,
    typename = std::enable_if_t<
      not std::is_same<std::decay_t<T>, _chain_base_classes_impl>::value
      and is_chained_base_class<std::decay_t<T>>::value
    >
  >
  constexpr inline explicit
  _chain_base_classes_impl(T&&) : _chain_base_classes_impl() { }

  // allow (and ignore) the unpacking constructor
  template <typename ArchiveT>
  constexpr inline explicit
  _chain_base_classes_impl(
    serialization::unpack_constructor_tag_t,
    ArchiveT& ar
  ) : _chain_base_classes_impl() { }

  template <typename ArchiveT>
  constexpr inline void
  _darma__pack(ArchiveT& ar) const { }

  template <typename ArchiveT>
  constexpr inline void
  _darma__compute_size(ArchiveT& ar) const { }
};

template <typename Seq>
struct chain_base_classes {
  using type = _chain_base_classes_impl<Seq,
    tinympl::size<Seq>::value, tinympl::size<Seq>::value
  >;
};


namespace _impl {

template <typename T>
using _is_oo_name_tag_archetype = typename T::_darma__is_oo_name_tag;

} // end namespace _impl

template <typename T>
using is_oo_name_tag = tinympl::detected_or_t<std::false_type,
  _impl::_is_oo_name_tag_archetype, T
>;

} // end namespace detail
} // end namespace oo
} // end namespace darma_runtime

#endif // _darma_has_feature(oo_interface)

#endif //DARMA_IMPL_OO_UTIL_H
