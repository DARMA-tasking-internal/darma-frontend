/*
//@HEADER
// ************************************************************************
//
//                          serialization.h
//                         darma_new
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

#ifndef SRC_SERIALIZATION_H_
#define SRC_SERIALIZATION_H_

#include <cstring>

// TODO RETHINK THIS!!! I don't like how it handles, e.g., (non-default) constructors

#include "meta/member_detector.h"

#include <tinympl/variadic/any_of.hpp>
#include <tinympl/logical_not.hpp>
#include <tinympl/logical_or.hpp>
#include <tinympl/logical_and.hpp>


namespace darma_runtime {

class Archive;

template <typename T, typename Enable=void>
struct nonintrusive_serialization_interface { };

template <typename T>
struct ensure_implements_zero_copy {
  virtual size_t num_zero_copy_slots(const T&) =0;
  virtual size_t zero_copy_slot_size(const T&, size_t) =0;
  virtual void*& get_zero_copy_slot(T&, size_t) =0;
  virtual ~ensure_implements_zero_copy() =default;
};

namespace detail {

namespace m = tinympl;
namespace mv = tinympl::variadic;

DARMA_META_MAKE_MEMBER_DETECTORS(reconstruct);
DARMA_META_MAKE_MEMBER_DETECTORS(num_zero_copy_slots);
DARMA_META_MAKE_MEMBER_DETECTORS(get_zero_copy_slot);
DARMA_META_MAKE_MEMBER_DETECTORS(zero_copy_slot_size);
DARMA_META_MAKE_MEMBER_DETECTORS(serialize_metadata);
DARMA_META_MAKE_MEMBER_DETECTORS(serialize_data);

template <typename T, typename EnableIntrusive=void>
struct invoke_num_zero_copy_slots {
  inline size_t operator()(const T& datum) const {
    return nonintrusive_serialization_interface<T>().num_zero_copy_slots(datum);
  }
};

template <typename T>
struct invoke_num_zero_copy_slots<T,
  typename std::enable_if<has_method_named_num_zero_copy_slots_with_signature<T, void*&(size_t)>::value>::type
> {
  inline size_t operator()(const T& datum) const {
    return datum.num_zero_copy_slots();
  }
};

template <typename T, typename EnableIntrusive=void>
struct invoke_get_zero_copy_slot {
  inline void*& operator()(T& datum, size_t slot) const {
    return nonintrusive_serialization_interface<T>().get_zero_copy_slot(datum, slot);
  }
};

template <typename T>
struct invoke_get_zero_copy_slot<T,
  typename std::enable_if<has_method_named_get_zero_copy_slot_with_signature<T, void*&(size_t)>::value>::type
> {
  inline void*& operator()(T& datum, size_t slot) const {
    return datum.get_zero_copy_slot(slot);
  }
};

template <typename T, typename EnableIntrusive=void>
struct invoke_zero_copy_slot_size {
  inline void*& operator()(const T& datum, size_t slot) const {
    return nonintrusive_serialization_interface<T>().zero_copy_slot_size(datum, slot);
  }
};
template <typename T>
struct invoke_zero_copy_slot_size<T,
  typename std::enable_if<has_method_named_zero_copy_slot_size_with_signature<T, size_t(size_t)>::value>::type
> {
  inline size_t operator()(const T& datum, size_t slot) const {
    return datum.zero_copy_slot_size(slot);
  }
};

template <typename T, typename EnableIntrusive=void>
struct invoke_serialize_metadata {
  inline void operator()(T& datum, Archive& ar) const {
    nonintrusive_serialization_interface<T>().serialize_metadata(datum, ar);
  }
};

template <typename T>
struct invoke_serialize_metadata<T,
  typename std::enable_if<has_method_named_serialize_metadata_with_signature<T, void(Archive&)>::value>::type
> {
  inline void operator()(T& datum, Archive& ar) const {
    datum.serialize_metadata(ar);
  }
};

template <typename T, typename EnableIntrusive=void>
struct invoke_serialize_data {
  inline void*& operator()(T& datum, Archive& ar) const {
    return nonintrusive_serialization_interface<T>().serialize_data(datum, ar);
  }
};
template <typename T>
struct invoke_serialize_data<T,
  typename std::enable_if<has_method_named_serialize_metadata_with_signature<T, void(Archive&)>::value>::type
> {
  inline size_t operator()(T& datum, Archive& ar) const {
    return datum.serialize_data(ar);
  }
};

template <typename in_T, typename Enable=void>
struct check_has_zero_copy_slots {
  private:
    typedef typename std::decay<in_T>::type T;
  public:
    typedef typename m::and_<
      m::or_<
        has_method_named_get_zero_copy_slot_with_signature<
          T, void*&(size_t)
        >,
        has_method_named_get_zero_copy_slot_with_signature<
          nonintrusive_serialization_interface<T>, void*&(T&, size_t)
        >
      >,
      m::or_<
        has_method_named_zero_copy_slot_size_with_signature<
          T, size_t(size_t)
        >,
        has_method_named_zero_copy_slot_size_with_signature<
          nonintrusive_serialization_interface<T>, size_t(const T&, size_t)
        >
      >,
      m::or_<
        has_method_named_num_zero_copy_slots_with_signature<
          T, size_t()
        >,
        has_method_named_num_zero_copy_slots_with_signature<
          nonintrusive_serialization_interface<T>, size_t(const T&)
        >
      >
    >::type type;
};

template <typename in_T, typename Enable=void>
struct check_has_serialize_metadata {
  private:
    typedef typename std::decay<in_T>::type T;
  public:
    typedef typename m::or_<
      has_method_named_serialize_metadata_with_signature<
        T, void(Archive&)
      >,
      has_method_named_serialize_metadata_with_signature<
        nonintrusive_serialization_interface<T>, void(Archive&)
      >
    >::type type;
};

template <typename in_T, typename Enable=void>
struct check_has_serialize_data {
  private:
    typedef typename std::decay<in_T>::type T;
  public:
    typedef typename m::or_<
      has_method_named_serialize_data_with_signature<
        T, void(Archive&)
      >,
      has_method_named_serialize_data_with_signature<
        nonintrusive_serialization_interface<T>, void(Archive&)
      >
    >::type type;
};

template <typename T>
struct is_trivially_serializable
  : m::not_<m::or_<
      check_has_serialize_data<T>,
      check_has_serialize_metadata<T>,
      check_has_zero_copy_slots<T>
    >>::type
{ };

typedef enum ArchiveMode {
  SizingMetadata,
  PackingMetadata,
  UnpackingMetadata,
  SizingData,
  PackingData,
  UnpackingData,
  CountingZeroCopySlots,
  CollectingZeroCopyPointers
} ArchiveMode;

namespace _impl {

template <
  typename T,
  bool has_serialize_metadata,
  bool has_serialize_data,
  bool has_zero_copy_slots,
  ArchiveMode mode
>
struct serializer;

template <typename T, bool has_serialize_data, bool has_zero_copy_slots>
struct serializer<T,
  /*has_serialize_metadata=*/ false,
  has_serialize_data, has_zero_copy_slots,
  SizingMetadata
> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    spot += sizeof(T);
  }
};

template <typename T, bool has_serialize_data, bool has_zero_copy_slots>
struct serializer<T,
  /*has_serialize_metadata=*/ false,
  has_serialize_data, has_zero_copy_slots,
  PackingMetadata
> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    memcpy((char*)buffer+spot, &datum, sizeof(T));
    spot += sizeof(T);
  }
};

template <typename T, bool has_serialize_data, bool has_zero_copy_slots>
struct serializer<T,
  /*has_serialize_metadata=*/ false,
  has_serialize_data, has_zero_copy_slots,
  UnpackingMetadata
> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    memcpy(&datum, (const char*)buffer+spot, sizeof(T));
    spot += sizeof(T);
  }
};

template <typename T, bool has_serialize_data, bool has_zero_copy_slots>
struct serializer<T,
  /*has_serialize_metadata=*/ true,
  has_serialize_data, has_zero_copy_slots,
  SizingMetadata
> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    invoke_serialize_metadata<T>(datum, ar);
  }
};

template <typename T, bool has_serialize_data, bool has_zero_copy_slots>
struct serializer<T,
  /*has_serialize_metadata=*/ true,
  has_serialize_data, has_zero_copy_slots,
  PackingMetadata
> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    invoke_serialize_metadata<T>(datum, ar);
  }
};

template <typename T, bool has_serialize_data, bool has_zero_copy_slots>
struct serializer<T,
  /*has_serialize_metadata=*/ true,
  has_serialize_data, has_zero_copy_slots,
  UnpackingMetadata
> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    invoke_serialize_metadata<T>(datum, ar);
  }
};

// *Data version

template <typename T, ArchiveMode mode, bool has_zero_copy_slots>
struct handle_zero_copy;

template <typename T, ArchiveMode mode>
struct handle_zero_copy<T, mode, false> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    /* do nothing */
  }
  inline size_t get_slot_size(const T& datum, size_t slot) const {
    return 0;
  }
  inline void*& get_slot(T& datum, size_t slot) const {
    static void* unused = nullptr;
    return unused;
  }
};

template <typename T, ArchiveMode mode>
struct handle_zero_copy<T, mode, true> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    size_t n_slots = invoke_num_zero_copy_slots<T>()(datum);
    if(mode == CountingZeroCopySlots) {
      spot = n_slots;
      return;
    }
    else {
      for(size_t i = 0; i < n_slots; ++i) {
        size_t slot_size = invoke_zero_copy_slot_size<T>()(datum, i);
        if(mode != SizingData) {
          void*& slot_spot = invoke_get_zero_copy_slot<T>()(datum, i);
          if(mode == PackingData) {
            memcpy(buffer, slot_spot, slot_size);
          }
          else if(mode == UnpackingData) {
            memcpy(slot_spot, buffer, slot_size);
          }
        }
        spot += slot_size;
      }
    }
  }
  inline size_t get_slot_size(const T& datum, size_t slot) const {
    return invoke_zero_copy_slot_size<T>()(datum, slot);
  }
  inline void*& get_slot(T& datum, size_t slot) const {
    return invoke_get_zero_copy_slot<T>()(datum, slot);
  }
};

template <typename T, bool has_serialize_metadata, bool has_zero_copy_slots>
struct serializer<T,
  has_serialize_metadata,
  /*has_serialize_data=*/ false,
  has_zero_copy_slots,
  SizingData
> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    handle_zero_copy<T, SizingData, has_zero_copy_slots>()(datum, buffer, spot);
  }
};

template <typename T, bool has_serialize_metadata, bool has_zero_copy_slots>
struct serializer<T,
  has_serialize_metadata,
  /*has_serialize_data=*/ false,
  has_zero_copy_slots,
  PackingData
> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    /* do nothing */
    handle_zero_copy<T, PackingData, has_zero_copy_slots>()(datum, buffer, spot);
  }
};

template <typename T, bool has_serialize_metadata, bool has_zero_copy_slots>
struct serializer<T,
  has_serialize_metadata,
  /*has_serialize_data=*/ false,
  has_zero_copy_slots,
  UnpackingData
> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    handle_zero_copy<T, UnpackingData, has_zero_copy_slots>()(datum, buffer, spot);
  }
};

template <typename T, bool has_serialize_metadata, bool has_zero_copy_slots>
struct serializer<T,
  has_serialize_metadata,
  /*has_serialize_data=*/ true,
  has_zero_copy_slots,
  SizingData
> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    invoke_serialize_data<T>(datum, ar);
    handle_zero_copy<T, SizingData, has_zero_copy_slots>()(datum, buffer, spot);
  }
};

template <typename T, bool has_serialize_metadata, bool has_zero_copy_slots>
struct serializer<T,
  has_serialize_metadata,
  /*has_serialize_data=*/ true,
  has_zero_copy_slots,
  PackingData
> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    invoke_serialize_data<T>(datum, ar);
    handle_zero_copy<T, PackingData, has_zero_copy_slots>()(datum, buffer, spot);
  }
};

template <typename T, bool has_serialize_metadata, bool has_zero_copy_slots>
struct serializer<T,
  has_serialize_metadata,
  /*has_serialize_data=*/ true,
  has_zero_copy_slots,
  UnpackingData
> {
  inline void
  operator()(T& datum, void* buffer, size_t& spot, Archive& ar) const {
    invoke_serialize_data<T>(datum, ar);
    handle_zero_copy<T, UnpackingData, has_zero_copy_slots>()(datum, buffer, spot);
  }
};


} // end namespace impl

template <typename T, ArchiveMode mode>
struct serializer_selector {
  typedef _impl::serializer<T,
    check_has_serialize_metadata<T>::value,
    check_has_serialize_data<T>::value,
    check_has_zero_copy_slots<T>::value,
    mode
  > type;
};

struct ArchiveAccess {
 static void*& get_buffer(Archive& ar);
 static size_t& get_spot(Archive& ar);
 static ArchiveMode& get_mode(Archive& ar);
};

} // end namespace detail


class Archive {
  public:

    template <typename T>
    void
    operator&(T& datum) {
      switch(mode_) {
#define ___DARMA_tmp_ser_case(mode) \
        case mode: typename detail::serializer_selector<T, mode>::type()(datum, buffer_, spot_); break;
        ___DARMA_tmp_ser_case(detail::SizingMetadata);
        ___DARMA_tmp_ser_case(detail::PackingMetadata);
        ___DARMA_tmp_ser_case(detail::UnpackingMetadata);
        ___DARMA_tmp_ser_case(detail::SizingData);
        ___DARMA_tmp_ser_case(detail::PackingData);
        ___DARMA_tmp_ser_case(detail::UnpackingData);
        case detail::CollectingZeroCopyPointers:
          // TODO
          break;
        case detail::CountingZeroCopySlots:
          // TODO
          break;
#undef ___DARMA_tmp_ser_case
      }
    }

    bool is_packing() const {
      return mode_ == detail::PackingData || mode_ == detail::PackingMetadata;
    }

    bool is_unpacking() const {
      return mode_ == detail::UnpackingData || mode_ == detail::UnpackingMetadata;
    }

    bool is_sizing() const {
      return mode_ == detail::SizingData || mode_ == detail::SizingMetadata;
    }

    bool is_packing_metadata() const {
      return mode_ == detail::PackingMetadata;
    }

    bool is_unpacking_metadata() const {
      return mode_ == detail::UnpackingMetadata;
    }

    bool is_sizing_metadata() const {
      return mode_ == detail::SizingMetadata;
    }

    bool is_packing_data() const {
      return mode_ == detail::PackingData;
    }

    bool is_unpacking_data() const {
      return mode_ == detail::UnpackingData;
    }

    bool is_sizing_data() const {
      return mode_ == detail::SizingData;
    }

  private:

    friend struct detail::ArchiveAccess;

    detail::ArchiveMode mode_;

    size_t spot_ = 0;

    void* buffer_ = nullptr;

};

namespace detail {

inline void*& ArchiveAccess::get_buffer(Archive& ar) { return ar.buffer_; }
inline size_t& ArchiveAccess::get_spot(Archive& ar) { return ar.spot_; }
inline ArchiveMode& ArchiveAccess::get_mode(Archive& ar) { return ar.mode_; }

}


namespace detail {

// TODO option to invoke a particular constructor or something when deserializing metadata

template <typename T>
class SerializationManager
  : public abstract::frontend::SerializationManager
{
  public:

    ////////////////////////////////////////
    // Metadata handling

    size_t
    get_metadata_size(
      const void* const deserialized_data
    ) const override {
      Archive ar;
      ArchiveAccess::get_mode(ar) = SizingMetadata;
      detail::serializer_selector<T, SizingMetadata>::type()(
        *(const T*)deserialized_data,
        ArchiveAccess::get_buffer(ar),
        ArchiveAccess::get_spot(ar),
        ar
      );
      return ArchiveAccess::get_spot(ar);
    }

    void
    serialize_metadata(
      const void* const deserialized_data,
      void* const buffer
    ) const override {
      Archive ar;
      const T* obj = (T*)deserialized_data;
      ArchiveAccess::get_mode(ar) = PackingMetadata;
      ArchiveAccess::get_buffer(ar) = buffer;
      detail::serializer_selector<T, PackingMetadata>::type()(
        *(T*)deserialized_data,
        ArchiveAccess::get_buffer(ar),
        ArchiveAccess::get_spot(ar),
        ar
      );
    }

    void
    deserialize_metadata(
      const void* const deserialized_data,
      void* const buffer
    ) const override {
      Archive ar;
      ArchiveAccess::get_mode(ar) = UnpackingMetadata;
      T* obj = (T*)deserialized_data;
      ArchiveAccess::get_buffer(ar) = buffer;
      detail::serializer_selector<T, UnpackingMetadata>::type()(
        *(T*)deserialized_data,
        ArchiveAccess::get_buffer(ar),
        ArchiveAccess::get_spot(ar),
        ar
      );
    }

    ////////////////////////////////////////
    // Serialized data handling

    bool needs_serialization() const override {
      return detail::check_has_serialize_data<T>::value;
    }

    size_t
    get_packed_data_size(
      const void* const deserialized_data
    ) const override {
      Archive ar;
      ArchiveAccess::get_mode(ar) = SizingData;
      detail::serializer_selector<T, SizingData>::type()(
        *(const T*)deserialized_data,
        ArchiveAccess::get_buffer(ar),
        ArchiveAccess::get_spot(ar),
        ar
      );
      return ArchiveAccess::get_spot(ar);
    }

    virtual void
    serialize_data(
      const void* const deserialized_data,
      void* const serialization_buffer
    ) const override
    {
      Archive ar;
      ArchiveAccess::get_mode(ar) = PackingData;
      ArchiveAccess::get_buffer(ar) = serialization_buffer;
      detail::serializer_selector<T, PackingData>::type()(
        *const_cast<T*>((const T*)deserialized_data),
        ArchiveAccess::get_buffer(ar),
        ArchiveAccess::get_spot(ar),
        ar
      );
    }

    virtual void
    deserialize_data(
      const void* const serialized_data,
      void* const deserialized_metadata_and_dest
    ) const
    {
      Archive ar;
      ArchiveAccess::get_mode(ar) = UnpackingData;
      ArchiveAccess::get_buffer(ar) = const_cast<void* const>(serialized_data);
      detail::serializer_selector<T, UnpackingData>::type()(
        *(T*)deserialized_metadata_and_dest,
        ArchiveAccess::get_buffer(ar),
        ArchiveAccess::get_spot(ar),
        ar
      );
    }

    ////////////////////////////////////////
    // Zero-copy data handling

    virtual size_t
    get_n_zero_copy_slots(
      const void* const deserialized_metadata
    ) const override {
      if(check_has_zero_copy_slots<T>::value) return 0;
      Archive ar;
      ArchiveAccess::get_mode(ar) = CountingZeroCopySlots;
      size_t n_slots = 0;
      detail::_impl::handle_zero_copy<
        T, CountingZeroCopySlots,
        check_has_zero_copy_slots<T>::value
      >()(
        *(const T*)deserialized_metadata,
        ArchiveAccess::get_buffer(ar),
        n_slots,
        ar
      );
      return n_slots;
    }

    virtual size_t
    get_zero_copy_data_size(
      const void* const deserialized_metadata,
      zero_copy_slot_t slot
    ) const override {
      return detail::_impl::handle_zero_copy<
        T, CollectingZeroCopyPointers,
        check_has_zero_copy_slots<T>::value
      >().get_slot_size(
        *(const T*)deserialized_metadata,
        slot
      );
    }

    virtual void*&
    get_zero_copy_slot(
      void* const deserialized_metadata,
      zero_copy_slot_t slot
    ) const override {
      return detail::_impl::handle_zero_copy<
        T, CollectingZeroCopyPointers,
        check_has_zero_copy_slots<T>::value
      >().get_slot(
        *(T*)deserialized_metadata,
        slot
      );
    }

};


} // end namespace detail


} // end namespace darma_runtime

#include "serialization_builtin.h"


#endif /* SRC_SERIALIZATION_H_ */
