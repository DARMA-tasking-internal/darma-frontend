/*
//@HEADER
// ************************************************************************
//
//                          dependency.h
//                         darma_mockup
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

#ifndef DARMA_IMPL_HANDLE_H
#define DARMA_IMPL_HANDLE_H

#include <atomic>
#include <cassert>
#include <tuple>

#include <tinympl/variadic/find_if.hpp>
#include <tinympl/bind.hpp>
#include <tinympl/lambda.hpp>
#include <tinympl/filter.hpp>
#include <tinympl/logical_not.hpp>
#include <tinympl/copy_traits.hpp>

#include <darma/impl/task_fwd.h>
#include <darma/impl/runtime.h>
#include <darma/impl/util.h>
#include <darma/impl/darma_assert.h>
#include <darma/impl/serialization/archive.h>
#include <darma/impl/serialization/nonintrusive.h>
#include <darma/impl/serialization/traits.h>
#include <darma/impl/serialization/allocation.h>

#include <darma/impl/keyword_arguments/keyword_arguments.h>
#include <darma/interface/app/keyword_arguments/n_readers.h>
#include <darma/interface/app/keyword_arguments/version.h>


namespace darma_runtime {

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="expression helpers">


namespace detail {

namespace m = tinympl;
namespace mv = tinympl::variadic;
namespace mp = tinympl::placeholders;

template <
  typename... Args
>
struct access_expr_helper {
  inline types::key_t
  get_key(
    Args&&... args
  ) const {
    return make_key_from_tuple(
      get_positional_arg_tuple(std::forward<Args>(args)...)
    );
  }

  inline types::key_t
  get_version_tag(
    Args&&... args
  ) const {
    return get_typeless_kwarg_with_converter_and_default<
      darma_runtime::keyword_tags_for_publication::version
    >([](auto&&... key_parts){
      return make_key(std::forward<decltype(key_parts)>(key_parts)...);
    }, types::key_t(), std::forward<Args>(args)...);
  }

};

template <
  typename... Args
>
struct publish_expr_helper {

  inline constexpr size_t
  get_n_readers(
    Args&&... args
  ) const {
    return get_typeless_kwarg_with_default_as<
      keyword_tags_for_publication::n_readers,
      size_t
    >(1, std::forward<Args>(args)...);

  }

  inline
  types::key_t
  get_version_tag(
    Args&&... args
  ) const {
    return get_typeless_kwarg_with_converter_and_default<
      keyword_tags_for_publication::version
    >(
      [](auto&&... key_parts){
        return make_key(std::forward<decltype(key_parts)>(key_parts)...);
      },
      types::key_t(),
      std::forward<Args>(args)...
    );
  }


  inline bool
  version_tag_is_final(
    Args&&... args
  ) const {
    return get_version_tag(std::forward<Args>(args)...) == types::key_t();
  }
};


} // end namespace detail

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="KeyedObject">

namespace detail {

template <typename key_type>
class KeyedObject
{
  public:

    typedef key_type key_t;

    KeyedObject() : key_() { }

    KeyedObject(const key_type& key) : key_(key) { }

    const key_t&
    get_key() const {
      return key_;
    }

    void
    set_key(const key_t& key) {
      key_ = key;
    }

  protected:
    key_t key_;
};

} // end namespace detail

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="DependencyHandle attorneys">

namespace detail {

// TODO rename this, and move some of this functionality
namespace DependencyHandle_attorneys {

struct ArchiveAccess {
  template <typename ArchiveT>
  static void set_buffer(ArchiveT& ar, void* const buffer) {
    // Assert that we're not overwriting a buffer, at least until
    // a use case for that sort of thing comes up
    assert(ar.start == nullptr);
    ar.start = ar.spot = (char* const)buffer;
  }
  template <typename ArchiveT>
  static void* get_spot(ArchiveT& ar) {
    return ar.spot;
  }
  template <typename ArchiveT>
  static size_t get_size(ArchiveT& ar) {
    assert(ar.is_sizing());
    return ar.spot - ar.start;
  }
  template <typename ArchiveT>
  static inline void
  start_sizing(ArchiveT& ar) {
    assert(not ar.is_sizing()); // for now, to avoid accidental resets
    ar.start = nullptr;
    ar.spot = nullptr;
    ar.mode = serialization::detail::SerializerMode::Sizing;
  }

  template <typename ArchiveT>
  static inline void
  start_packing(ArchiveT& ar) {
    ar.mode = serialization::detail::SerializerMode::Packing;
    ar.spot = ar.start;
  }

  template <typename ArchiveT>
  static inline void
  start_unpacking(ArchiveT& ar) {
    ar.mode = serialization::detail::SerializerMode::Unpacking;
    ar.spot = ar.start;
  }

  template <typename ArchiveT>
  static inline void
  start_unpacking_with_buffer(ArchiveT& ar, void* buffer) {
    ar.mode = serialization::detail::SerializerMode::Unpacking;
    ar.spot = ar.start = (char* const)buffer;
  }
};

} // end namespace DependencyHandle_attorneys

} // end namespace detail

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="DependencyHandleBase">

namespace detail {

// Tag type for handle migration unpack
struct handle_migration_unpack_t { };
static constexpr handle_migration_unpack_t handle_migration_unpack = {};

class VariableHandleBase
  : public KeyedObject<types::key_t>,
    public abstract::frontend::Handle {
  public:
    typedef types::key_t key_t;
    typedef KeyedObject<key_t> keyed_base_t;

    const key_t &
    get_key() const override {
      return this->KeyedObject<key_t>::get_key();
    }

    explicit VariableHandleBase(
      const key_t &key
    ) : keyed_base_t(key) { }

    VariableHandleBase() : keyed_base_t(key_t()) { }

    virtual ~VariableHandleBase() noexcept { }

};

} // end namespace detail

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="VariableHandle">

namespace detail {

template <typename T>
class VariableHandle
  : public VariableHandleBase,
    public abstract::frontend::SerializationManager {
  protected:

    typedef VariableHandleBase base_t;
    typedef serialization::detail::serializability_traits<T> serdes_traits;

  public:

    typedef typename base_t::key_t key_t;

    ////////////////////////////////////////////////////////////
    // Constructors and Destructor <editor-fold desc="Constructors and Destructor">

    VariableHandle() : base_t() { }

    VariableHandle(
      const key_t &data_key
    ) : base_t(data_key) { }

    virtual ~VariableHandle() noexcept { }

    // end Constructors and Destructor </editor-fold>
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // <editor-fold desc="abstract::frontend::DependencyHandle implmentation">

    abstract::frontend::SerializationManager const*
    get_serialization_manager() const override {
      return this;
    }

    // </editor-fold>
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // SerializationManager implementation <editor-fold desc="SerializationManager implementation">

    STATIC_ASSERT_SERIALIZABLE_WITH_ARCHIVE(T, serialization::SimplePackUnpackArchive,
      "Handles to non-serializable types not yet supported"
    );

    size_t
    get_metadata_size() const override {
      return sizeof(T);
    }

    size_t
    get_packed_data_size(
      const void *const object_data,
      abstract::backend::SerializationPolicy* ser_pol
    ) const override {
      serialization::Serializer<T> s;
      serialization::SimplePackUnpackArchive ar;
      DependencyHandle_attorneys::ArchiveAccess::start_sizing(ar);
      s.compute_size(*(T const *const) (object_data), ar);
      return DependencyHandle_attorneys::ArchiveAccess::get_size(ar);
    }

    void
    pack_data(
      const void *const object_data,
      void *const serialization_buffer,
      abstract::backend::SerializationPolicy* ser_pol
    ) const override {
      serialization::Serializer<T> s;
      serialization::SimplePackUnpackArchive ar;
      DependencyHandle_attorneys::ArchiveAccess::set_buffer(ar, serialization_buffer);
      DependencyHandle_attorneys::ArchiveAccess::start_packing(ar);
      s.pack(*(T const *const) (object_data), ar);
    }

    void
    unpack_data(
      void *const object_dest,
      const void *const serialized_data,
      abstract::backend::SerializationPolicy* ser_pol,
      abstract::backend::AllocationPolicy* alloc_pol
    ) const override {
      serialization::Serializer<T> s;
      serialization::SimplePackUnpackArchive ar;
      // Need to cast away constness of the buffer because the Archive requires
      // a non-const buffer to be able to operate in pack mode (i.e., so that
      // the user can write one function for both serialization and deserialization)
      DependencyHandle_attorneys::ArchiveAccess::set_buffer(
        ar, const_cast<void *const>(serialized_data)
      );
      DependencyHandle_attorneys::ArchiveAccess::start_unpacking(ar);
      s.unpack(object_dest, ar);
    }

    //bool
    //is_directly_serializable() const override {
    //  return serdes_traits::is_directly_serializable;
    //}


    void
    default_construct(void* allocated) const override {
      // Will fail if T is not default constructible...
      new (allocated) T();
    }

    void
    destroy(void* constructed_object) const override {
      // TODO allocator awareness?
      ((T*)constructed_object)->~T();
    }

    // end SerializationManager implementation </editor-fold>
    ////////////////////////////////////////////////////////////

};

} // end namespace detail

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="assorted attorneys">

namespace detail {

namespace create_work_attorneys {

class for_AccessHandle;

} // end namespace create_work_attorneys

namespace access_attorneys {

class for_AccessHandle;

} // end namespace access_attorneys

namespace analogous_access_handle_attorneys {

struct AccessHandleAccess {
  template <typename AccessHandleT>
  static auto&
  captured_as(AccessHandleT& ah) {
    return ah.captured_as_;
  }
  template <typename AccessHandleT>
  static auto&
  dep_handle(AccessHandleT& ah) {
    return ah.dep_handle_;
  }
  template <typename AccessHandleT>
  static auto&
  read_only_holder(AccessHandleT& ah) {
    return ah.read_only_holder_;
  }
  template <typename AccessHandleT>
  static auto&
  state(AccessHandleT& ah) {
    return ah.state_;
  }
};

} // end namespace analogous_access_handle_attorneys

} // end namespace detail

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="AccessHandleBase">

namespace detail {

class AccessHandleBase {
  public:

    typedef enum CaptureOp {
      ro_capture,
      mod_capture
    } capture_op_t;

    // TODO figure out if this as efficient as a bitfield (it's definitely more readable)
    typedef enum CapturedAsInfo {
      Normal = 0,
      Ignored = 1,
      ReadOnly = 2,
      // Future use:
      ScheduleOnly = 4,
      Leaf = 8,
      Uncaptured = 16
    } captured_as_info_t;

    typedef typename abstract::frontend::Handle handle_t;
};

} // end namespace detail

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="access_handle_traits and helpers">

namespace detail {

typedef enum AccessHandlePermissions {
  NotGiven=-1,
  None=0, Read=1, Modify=2
} access_handle_permissions_t;

// (Not really true, needs more explanation): Min permissions refers to as a parameter, max permissions refers to as a call argument or lvalue
// (or as a parameter for determining whether a capture is read-only).  All are only the known compile-time
// bounds; if no restrictions are given at compile time, all will be AccessHandlePermissions::NotGiven
// TODO more full documentation
template <
  access_handle_permissions_t MinSchedulePermissions = NotGiven,
  access_handle_permissions_t MinImmediatePermissions = NotGiven,
  access_handle_permissions_t MaxSchedulePermissions = NotGiven,
  access_handle_permissions_t MaxImmediatePermissions = NotGiven
>
struct access_handle_traits {

  static constexpr auto min_schedule_permissions = MinSchedulePermissions;
  static constexpr auto min_schedule_permissions_given = MinSchedulePermissions != NotGiven;
  static constexpr auto min_immediate_permissions = MinImmediatePermissions;
  static constexpr auto min_immediate_permissions_given = MinImmediatePermissions != NotGiven;
  static constexpr auto max_schedule_permissions = MaxSchedulePermissions;
  static constexpr auto max_schedule_permissions_given = MaxSchedulePermissions != NotGiven;
  static constexpr auto max_immediate_permissions = MaxImmediatePermissions;
  static constexpr auto max_immediate_permissions_given = MaxImmediatePermissions != NotGiven;

  template <access_handle_permissions_t new_min_schedule_permissions>
  struct with_min_schedule_permissions {
    typedef access_handle_traits<
      new_min_schedule_permissions,
      MinImmediatePermissions,
      MaxSchedulePermissions,
      MaxImmediatePermissions
    > type;
  };

  template <access_handle_permissions_t new_max_schedule_permissions>
  struct with_max_schedule_permissions {
    typedef access_handle_traits<
      MinSchedulePermissions,
      MinImmediatePermissions,
      new_max_schedule_permissions,
      MaxImmediatePermissions
    > type;
  };

  template <access_handle_permissions_t new_min_immediate_permissions>
  struct with_min_immediate_permissions {
    typedef access_handle_traits<
      MinSchedulePermissions,
      new_min_immediate_permissions,
      MaxSchedulePermissions,
      MaxImmediatePermissions
    > type;
  };

  template <access_handle_permissions_t new_max_immediate_permissions>
  struct with_max_immediate_permissions {
    typedef access_handle_traits<
      MinSchedulePermissions,
      MinImmediatePermissions,
      MaxSchedulePermissions,
      new_max_immediate_permissions
    > type;
  };

};


//------------------------------------------------------------
// <editor-fold desc="make_access_traits and associated 'keyword' template arguments">

template <access_handle_permissions_t permissions>
struct min_schedule_permissions {
  static constexpr auto value = permissions;
};

template <access_handle_permissions_t permissions>
struct max_schedule_permissions {
  static constexpr auto value = permissions;
};

template <access_handle_permissions_t permissions>
struct min_immediate_permissions {
  static constexpr auto value = permissions;
};

template <access_handle_permissions_t permissions>
struct max_immediate_permissions {
  static constexpr auto value = permissions;
};

namespace _impl {

template <typename traits, typename... modifiers>
struct _make_access_handle_traits;

template <typename traits, access_handle_permissions_t permissions, typename... modifiers>
struct _make_access_handle_traits<traits, min_schedule_permissions<permissions>, modifiers...> {
  using type = typename _make_access_handle_traits<
    typename traits::template with_min_schedule_permissions<permissions>::type,
    modifiers...
  >::type;
};

template <typename traits, access_handle_permissions_t permissions, typename... modifiers>
struct _make_access_handle_traits<traits, max_schedule_permissions<permissions>, modifiers...> {
  using type = typename _make_access_handle_traits<
    typename traits::template with_max_schedule_permissions<permissions>::type,
    modifiers...
  >::type;
};

template <typename traits, access_handle_permissions_t permissions, typename... modifiers>
struct _make_access_handle_traits<traits, min_immediate_permissions<permissions>, modifiers...> {
  using type = typename _make_access_handle_traits<
    typename traits::template with_min_immediate_permissions<permissions>::type,
    modifiers...
  >::type;
};

template <typename traits, access_handle_permissions_t permissions, typename... modifiers>
struct _make_access_handle_traits<traits, max_immediate_permissions<permissions>, modifiers...> {
  using type = typename _make_access_handle_traits<
    typename traits::template with_max_immediate_permissions<permissions>::type,
    modifiers...
  >::type;
};

template <typename traits>
struct _make_access_handle_traits<traits> {
  using type = traits;
};

} // end namespace _impl

template <typename... modifiers>
struct make_access_handle_traits {
  using type = typename
    _impl::_make_access_handle_traits<access_handle_traits<>, modifiers...>::type;
};

template <typename... modifiers>
using make_access_handle_traits_t = typename make_access_handle_traits<modifiers...>::type;

// </editor-fold>
//------------------------------------------------------------

} // end namespace detail

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

// Forward declaration of AccessHandle
template <
  typename T = void,
  typename traits = detail::access_handle_traits<>
>
class AccessHandle;

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="is_access_handle">

namespace detail {

template <typename T, typename Enable=void>
struct is_access_handle
  : std::false_type { };

// TODO decide if this is a good or bad idea
template <typename T>
struct is_access_handle<T,
  std::enable_if_t<not std::is_same<
    T, std::remove_cv_t<std::remove_reference_t<T>>
  >::value>
> : is_access_handle<std::remove_cv_t<std::remove_reference_t<T>>> { };

template <typename... Args>
struct is_access_handle<AccessHandle<Args...>, void>
  : std::true_type { };

namespace _impl {

template <typename T>
using _value_type_archetype = typename T::value_type;

} // end namespace _impl

template <typename T, typename Otherwise=meta::nonesuch>
using value_type_if_access_handle = std::conditional<
  is_access_handle<T>::value,
  meta::detected_t<_impl::_value_type_archetype, std::decay_t<T>>,
  Otherwise
>;

template <typename T, typename Otherwise=meta::nonesuch>
using value_type_if_access_handle_t = typename value_type_if_access_handle<T, Otherwise>::type;

} // end namespace detail

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////


} // end namespace darma_runtime


#endif /* DARMA_IMPL_HANDLE_H_ */
