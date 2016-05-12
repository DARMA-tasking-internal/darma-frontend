/*
//@HEADER
// ************************************************************************
//
//                         archive.h
//                          DARMA
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

#ifndef DARMA_IMPL_SERIALIZATION_ARCHIVE_H_
#define DARMA_IMPL_SERIALIZATION_ARCHIVE_H_

#include <darma/impl/serialization/nonintrusive.h>
#include <darma/impl/serialization/serialization_fwd.h>
#include <darma/impl/serialization/traits.h>

#include <tinympl/is_instantiation_of.hpp>

#include "range.h"
#include "traits.h"

namespace darma_runtime {

namespace serialization {

////////////////////////////////////////////////////////////////////////////////

// A simple frontend object for interacting with user-defined serializations
// TODO Archives that check for pointer loops and stuff

namespace detail {

template <typename ArchiveT>
class ArchiveOperatorsMixin {
  public:
    template <typename T>
    inline ArchiveT&
    operator<<(T &&val) {
      static_cast<ArchiveT *>(this)->pack_item(std::forward<T>(val));
      return *static_cast<ArchiveT *>(this);
    }

    template <typename T>
    inline ArchiveT&
    operator>>(T &&val) {
      static_cast<ArchiveT *>(this)->unpack_item(std::forward<T>(val));
      return *static_cast<ArchiveT *>(this);
    }

    template <typename T>
    inline ArchiveT&
    operator%(T &&val) {
      static_cast<ArchiveT *>(this)->incorporate_size(std::forward<T>(val));
      return *static_cast<ArchiveT *>(this);
    }

    template <typename T>
    inline ArchiveT&
    operator|(T&& val) {
      static_cast<ArchiveT *>(this)->serialize_item(std::forward<T>(val));
      return *static_cast<ArchiveT *>(this);
    }
};

template <typename ArchiveT, typename MoreGeneralMixin>
class ArchiveRangesMixin : public MoreGeneralMixin {
  private:
    template <typename T>
    using enable_condition = tinympl::is_instantiation_of<SerDesRange, T>;

  public:
    template <typename T, typename ReturnValue = void>
    using enabled_version = enable_if_t<enable_condition<T>::value, ReturnValue>;
    template <typename T, typename ReturnValue = void>
    using disabled_version = enable_if_t<not enable_condition<T>::value, ReturnValue>;

    // operator>> does the unpacking
    template <typename T>
    inline enabled_version<T, ArchiveT&>
    operator>>(T &&val) {
      typedef typename allocation_traits<T>::template allocator_traits<ArchiveT>::size_type size_type;
      typedef typename T::iterator_traits::value_type value_type;
      typedef allocation_traits<value_type> value_allocation_traits;
      ArchiveT* this_archive = static_cast<ArchiveT *>(this);
      assert(this_archive->is_unpacking());
      // initialize to prevent e.g. spurious valgrind errors and perhaps compiler warnings
      size_type size = 0;
      this_archive->unpack_item(size);
      val.begin() = value_allocation_traits::allocate(*this_archive, size);
      val.end() = val.begin() + size;
      for(auto&& item : val) {
        this_archive->unpack_item(item);
      }
      return *this_archive;
    }
    template <typename T>
    inline disabled_version<T, ArchiveT&>
    operator>>(T &&val) { return this->MoreGeneralMixin::template operator>>(std::forward<T>(val)); }

    // operator<< does the packing
    template <typename T>
    inline enabled_version<T, ArchiveT&>
    operator<<(T &&val) {
      typedef typename allocation_traits<T>::template allocator_traits<ArchiveT>::size_type size_type;
      typedef typename T::iterator_traits::value_type value_type;
      typedef typename allocation_traits<value_type>::template allocator_traits<ArchiveT> value_allocator_traits;
      size_type size = val.end() - val.begin();
      ArchiveT* this_archive = static_cast<ArchiveT *>(this);
      this_archive->pack_item(size);
      for(auto&& item : val) {
        this_archive->pack_item(item);
      }
      return *static_cast<ArchiveT *>(this);
    }
    template <typename T>
    inline disabled_version<T, ArchiveT&>
    operator<<(T &&val) { return this->MoreGeneralMixin::template operator<<(std::forward<T>(val)); }

    // operator% does sizing (rarely used)
    template <typename T>
    inline enabled_version<T, ArchiveT&>
    operator%(T &&val) {
      typedef typename allocation_traits<T>::template allocator_traits<ArchiveT>::size_type size_type;
      typedef typename T::iterator_traits::value_type value_type;
      typedef typename allocation_traits<value_type>::template allocator_traits<ArchiveT> value_allocator_traits;
      size_type size = val.end() - val.begin();
      ArchiveT* this_archive = static_cast<ArchiveT *>(this);
      this_archive->incorporate_size(size);
      for(auto&& item : val) {
        this_archive->incorporate_size(item);
      }
      return *static_cast<ArchiveT *>(this);
    }
    template <typename T>
    inline disabled_version<T, ArchiveT&>
    operator%(T &&val) { return this->MoreGeneralMixin::template operator%(std::forward<T>(val)); }

    template <typename T>
    inline enabled_version<T, ArchiveT&>
    operator|(T&& val) {
      typedef typename allocation_traits<T>::template allocator_traits<ArchiveT>::size_type size_type;
      typedef typename T::iterator_traits::value_type value_type;
      typedef typename allocation_traits<value_type>::template allocator_traits<ArchiveT> value_allocator_traits;
      ArchiveT* this_archive = static_cast<ArchiveT *>(this);
      if(this_archive->is_unpacking()) {
        this->operator>>(std::forward<T>(val));
      }
      else {
        size_type size = val.end() - val.begin();
        this_archive->serialize_item(size);
        for(auto&& item : val) {
          this_archive->serialize_item(item);
        }
      }
      return *static_cast<ArchiveT *>(this);
    }
    template <typename T>
    inline disabled_version<T, ArchiveT&>
    operator|(T &&val) { return this->MoreGeneralMixin::template operator|(std::forward<T>(val)); }
};

} // end namespace detail


////////////////////////////////////////////////////////////////////////////////

class SimplePackUnpackArchive
  : public detail::ArchiveRangesMixin<SimplePackUnpackArchive,
      detail::ArchiveOperatorsMixin<SimplePackUnpackArchive>
    >
{
  private:
    // readability alias
    using byte = char;

    byte* start = nullptr;
    byte* spot = nullptr;

    detail::SerializerMode mode = detail::SerializerMode::None;

    template <typename T>
    using is_serializable_with_this = typename
      detail::serializability_traits<T>::template is_serializable_with_archive<SimplePackUnpackArchive>;
    template <typename T>
    using traits = typename detail::serializability_traits<T>;

    template <typename T>
    using has_only_serialize = std::integral_constant<bool,
      traits<T>::template has_nonintrusive_serialize<SimplePackUnpackArchive>::value
      and not (
        traits<T>::template has_nonintrusive_compute_size<SimplePackUnpackArchive>::value
        or traits<T>::template has_nonintrusive_pack<SimplePackUnpackArchive>::value
        or traits<T>::template has_nonintrusive_unpack<SimplePackUnpackArchive>::value
      )
    >;

  public:

    bool is_sizing() const { return mode == detail::SerializerMode::Sizing; }
    bool is_packing() const { return mode == detail::SerializerMode::Packing; }
    bool is_unpacking() const { return mode == detail::SerializerMode::Unpacking; }

    template <typename T>
    std::enable_if_t<is_serializable_with_this<T>::value>
    incorporate_size(T const& val) {
      traits<T>::compute_size(val, *this);
    }

    template <typename T>
    std::enable_if_t<is_serializable_with_this<T>::value>
    pack_item(T const& val) {
      traits<T>::template pack(val, *this);
    }

    template <typename T>
    std::enable_if_t<is_serializable_with_this<T>::value>
    unpack_item(T& val) {
      traits<T>::unpack(const_cast<void*>(static_cast<const void*>(&val)), *this);
    }

    template <typename T>
    std::enable_if_t<
      is_serializable_with_this<T>::value and not has_only_serialize<T>::value
    >
    serialize_item(T& val) {
      if(is_sizing()) incorporate_size(val);
      else if(is_packing()) pack_item(val);
      else {
        assert(is_unpacking());
        unpack_item(val);
      }
    }

    template <typename T>
    std::enable_if_t<
      is_serializable_with_this<T>::value and has_only_serialize<T>::value
    >
    serialize_item(T& val) {
      typename traits<T>::serializer ser;
      if(is_unpacking()) new (&val) T;
      ser.serialize(val, *this);
    }

  private:

    friend class Serializer_attorneys::ArchiveAccess;
    friend class darma_runtime::detail::DependencyHandle_attorneys::ArchiveAccess;

};

////////////////////////////////////////////////////////////////////////////////

} // end namespace serialization

} // end namespace darma_runtime

#endif /* DARMA_IMPL_SERIALIZATION_ARCHIVE_H_ */
