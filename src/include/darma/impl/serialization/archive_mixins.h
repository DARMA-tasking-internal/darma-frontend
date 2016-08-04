/*
//@HEADER
// ************************************************************************
//
//                      archive_mixins.h
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

#ifndef DARMA_IMPL_SERIALIZATION_ARCHIVE_MIXINS_H
#define DARMA_IMPL_SERIALIZATION_ARCHIVE_MIXINS_H

namespace darma_runtime {

namespace serialization {

namespace detail {

template <typename ArchiveT>
class ArchiveOperatorsMixin {
  public:
    template <typename T>
    inline ArchiveT&
    operator<<(T&& val) {
      static_cast<ArchiveT *>(this)->pack_item(val);
      return *static_cast<ArchiveT *>(this);
    }

    template <typename T>
    inline ArchiveT&
    operator>>(T&& val) {
      static_cast<ArchiveT*>(this)->unpack_item(val);
      return *static_cast<ArchiveT*>(this);
    }

    template <typename T>
    inline ArchiveT&
    operator%(T&& val) {
      static_cast<ArchiveT*>(this)->incorporate_size(val);
      return *static_cast<ArchiveT*>(this);
    }

    template <typename T>
    inline ArchiveT&
    operator|(T&& val) {
      static_cast<ArchiveT *>(this)->serialize_item(val);
      return *static_cast<ArchiveT *>(this);
    }
};

template <typename ArchiveT, typename MoreGeneralMixin>
class ArchiveRangesMixin : public MoreGeneralMixin {
  private:
    template <typename T>
    using enable_condition = tinympl::is_instantiation_of<SerDesRange, T>;

    template <typename T, typename ReturnValue = void>
    using enabled_version =
      std::enable_if_t<enable_condition<T>::value, ReturnValue>;

    template <typename T, typename ReturnValue = void>
    using disabled_version =
      std::enable_if_t<not enable_condition<T>::value, ReturnValue>;

    template <typename T>
    inline void
    _unpack_direct_if_possible(
      T const& range, ArchiveT& ar, size_t size, std::true_type
    ) {
      // Assert that we have a contiguous iterator
      assert(
        static_cast<typename std::decay_t<T>::value_type const*>(
          std::addressof(*(range.end()))
        ) - static_cast<typename std::decay_t<T>::value_type const*>(
          std::addressof(*(range.begin()))
        ) == std::distance(range.begin(), range.end())
      );
      ar.template unpack_direct<typename std::decay_t<T>::value_type>(
        range.begin(), size
      );
    }

    template <typename T>
    inline void
    _unpack_direct_if_possible(
      T const& range, ArchiveT& ar, size_t size, std::false_type
    ) {
      for(auto&& item : range) {
        ar.unpack_item(item);
      }
    }

    template <typename T>
    inline void
    _pack_direct_if_possible(
      T const& range, ArchiveT& ar, std::true_type
    ) {
      // Assert that we have a contiguous iterator
      assert(
        static_cast<typename std::decay_t<T>::value_type const*>(std::addressof(*(range.end())))
          - static_cast<typename std::decay_t<T>::value_type const*>(std::addressof(*(range.begin())))
        == std::distance(range.begin(), range.end())
      );
      ar.pack_direct(range.begin(), range.end());
    }

    template <typename T>
    inline void
    _pack_direct_if_possible(
      T const& range, ArchiveT& ar, std::false_type
    ) {
      for(auto&& item : range) {
        ar.pack_item(item);
      }
    }

  public:

    //--------------------------------------------------------------------------

    // operator>> does the unpacking
    template <typename T>
    inline enabled_version<T, ArchiveT&>
    operator>>(T&& val) {
      using value_type = typename std::decay_t<T>::value_type;
      using value_allocation_traits =
        typename allocation_traits<value_type>::template allocator_traits<ArchiveT>;

      ArchiveT* this_archive = static_cast<ArchiveT*>(this);
      assert(this_archive->is_unpacking());

      // initialize to prevent e.g. spurious valgrind errors and perhaps compiler warnings
      size_t size = 0;
      this_archive->unpack_item(size);

      val.begin() = val.get_allocator().allocate(size);
      val.end() = val.begin() + size;

      _unpack_direct_if_possible(std::forward<T>(val), *this_archive, size,
        std::integral_constant<bool, std::decay_t<T>::is_contiguous
          and std::decay_t<T>::value_is_directly_serializable
        >()
      );

      return *this_archive;
    }

    template <typename T>
    inline disabled_version<T, ArchiveT&>
    operator>>(T&& val) { return this->MoreGeneralMixin::template operator>>(std::forward<T>(val)); }

    //--------------------------------------------------------------------------

    // operator<< does the packing
    template <typename T>
    inline enabled_version<T, ArchiveT&>
    operator<<(T&& val) {
      using value_type = typename std::decay_t<T>::value_type;

      size_t size = std::distance(val.begin(), val.end());
      ArchiveT* this_archive = static_cast<ArchiveT*>(this);
      this_archive->pack_item(size);

      _pack_direct_if_possible(std::forward<T>(val), *this_archive,
        std::integral_constant<bool, std::decay_t<T>::is_contiguous
          and std::decay_t<T>::value_is_directly_serializable
        >()
      );

      return *static_cast<ArchiveT *>(this);
    }

    template <typename T>
    inline disabled_version<T, ArchiveT&>
    operator<<(T&& val) { return this->MoreGeneralMixin::template operator<<(std::forward<T>(val)); }

    //--------------------------------------------------------------------------

    // operator% does sizing (rarely used)
    template <typename T>
    inline enabled_version<T, ArchiveT&>
    operator%(T&& val) {
      size_t size = std::distance(val.begin(), val.end());
      ArchiveT* this_archive = static_cast<ArchiveT*>(this);
      this_archive->incorporate_size(size);

      // This is really a "constexpr if"
      if(std::decay_t<T>::value_is_directly_serializable and std::decay_t<T>::is_contiguous) {
        this_archive->add_to_size_direct(val.begin(), size);
      }
      else {
        for(auto&& item : val) {
          this_archive->incorporate_size(item);
        }
      }
      return *this_archive;
    }

    template <typename T>
    inline disabled_version<T, ArchiveT&>
    operator%(T&& val) { return this->MoreGeneralMixin::template operator%(std::forward<T>(val)); }

    //--------------------------------------------------------------------------

    template <typename T>
    inline enabled_version<T, ArchiveT&>
    operator|(T&& val) {
      ArchiveT* this_archive = static_cast<ArchiveT*>(this);
      if(this_archive->is_unpacking()) {
        this->operator>>(std::forward<T>(val));
      }
      else if(this_archive->is_packing()){
        this->operator<<(std::forward<T>(val));
      }
      else { // sizing
        this->operator%(std::forward<T>(val));
      }
      return *static_cast<ArchiveT*>(this);
    }

    template <typename T>
    inline disabled_version<T, ArchiveT&>
    operator|(T&& val) { return this->MoreGeneralMixin::template operator|(std::forward<T>(val)); }
};

} // end namespace detail

} // end namespace serialization

} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_ARCHIVE_MIXINS_H
