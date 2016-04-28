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

#ifndef NEW_DEPENDENCY_H_
#define NEW_DEPENDENCY_H_

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
#include <darma/interface/defaults/version.h>
#include <darma/impl/util.h>
#include <darma/impl/darma_assert.h>
#include <darma/impl/serialization/archive.h>
#include <darma/impl/serialization/nonintrusive.h>
#include <darma/impl/serialization/traits.h>
#include <darma/impl/serialization/allocation.h>

#include <darma/interface/backend/data_block.h>
#include <darma/impl/keyword_arguments/keyword_arguments.h>

// TODO move these to appropriate header files in interface/app
DeclareDarmaTypeTransparentKeyword(publication, n_readers);
DeclareDarmaTypeTransparentKeyword(publication, version);

namespace darma_runtime {

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

namespace detail {

template <typename key_type>
class KeyedObject
{
  public:

    typedef key_type key_t;

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

template <typename version_type>
class VersionedObject
{
  public:

    typedef version_type version_t;

    VersionedObject(const version_type& v) : version_(v) { }

    const version_t&
    get_version() const {
      return version_;
    }

  public:

    void push_subversion() {
      version_.push_subversion();
    }

    void pop_subversion() {
      version_.pop_subversion();
    }

    void increment_version() {
      ++version_;
    }

    void
    set_version(const version_t& v) {
      version_ = v;
    }

    version_t version_;
};


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
};

} // end namespace DependencyHandle_attorneys

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="DependencyHandleBase">

template <
  typename key_type=types::key_t,
  typename version_type=types::version_t
>
class DependencyHandleBase
  : public KeyedObject<key_type>,
    public VersionedObject<version_type>,
    public abstract::frontend::DependencyHandle<key_type, version_type>
{
  public:
    typedef KeyedObject<key_type> keyed_base_t;
    typedef typename keyed_base_t::key_t key_t;
    typedef VersionedObject<version_type> versioned_base_t;
    typedef typename versioned_base_t::version_t version_t;

    const key_type&
    get_key() const override {
      return this->KeyedObject<key_type>::get_key();
    }
    const version_type&
    get_version() const override {
      return this->VersionedObject<version_type>::get_version();
    }

    DependencyHandleBase(
      const key_t& key,
      const version_t& version
    ) : keyed_base_t(key),
        versioned_base_t(version)
    { }

    virtual ~DependencyHandleBase() noexcept { }

    bool
    is_satisfied() const override { return satisfied_; }

    bool
    is_writable() const override { return writable_; }

    void
    allow_writes() override { writable_ = true; }

    bool
    version_is_pending() const override { return version_is_pending_; }

    void
    set_version_is_pending(bool is_pending = true) {
      version_is_pending_ = true;
    }

    void
    satisfy_with_data_block(
      abstract::backend::DataBlock* const data
    ) override {
      this->satisfied_ = true;
      this->data_ = data->get_data();
      this->data_block_ = data;
    }

    abstract::backend::DataBlock*
    get_data_block() const override {
      return this->data_block_;
    }

    void set_version(const version_t& v) override {
      assert(version_is_pending_);
      version_is_pending_ = false;
      this->version_ = v;
    }

  protected:
    void* data_ = nullptr;
    abstract::backend::DataBlock* data_block_ = nullptr;
    bool satisfied_ = false;
    bool writable_ = false;
    bool version_is_pending_ = false;
};

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

template <
  typename T,
  typename key_type,
  typename version_type
>
class DependencyHandle
  : public DependencyHandleBase<key_type, version_type>,
    public abstract::frontend::SerializationManager
{
  protected:

    typedef DependencyHandleBase<key_type, version_type> base_t;
    typedef serialization::detail::serializability_traits<T> serdes_traits;

  public:

    typedef typename base_t::key_t key_t;
    typedef typename base_t::version_t version_t;

    ////////////////////////////////////////////////////////////
    // Constructors and Destructor <editor-fold desc="Constructors and Destructor">

    DependencyHandle(
      const key_t& data_key,
      const version_t& data_version
    ) : base_t(data_key, data_version),
        value_((T*&)this->base_t::data_)
    {
      backend_runtime->register_handle(this);
    }

    DependencyHandle(
      const key_t& data_key,
      const key_t& user_version_tag,
      bool write_access_allowed
    ) : base_t(data_key, version_t()),
        value_((T*&)this->base_t::data_)
    {
      assert(write_access_allowed == false);
      this->set_version_is_pending(true);
      backend_runtime->register_fetching_handle(this,
        user_version_tag
      );
    }

    virtual ~DependencyHandle() {
      backend_runtime->release_handle(this);
    }

    // end Constructors and Destructor </editor-fold>
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // get_value, emplace_value, set_value, etc. <editor-fold desc="get_value, emplace_value, set_value, etc.">

    template <typename... Args>
    void
    emplace_value(Args&&... args)
    {
      // TODO decide if this should happen here or where...
      //if(value_ == nullptr) allocate_metadata(sizeof(T));
      // for now, assume/assert it's allocated to be the right size by the backend:
      assert(value_ != nullptr);
      value_ = new (value_) T(
        std::forward<Args>(args)...
      );
    }

    template <typename U>
    void
    set_value(U&& val) {
      // The enable-if is in Access handle
      // TODO handle custom operator=() case (for instance)
      emplace_value(std::forward<U>(val));
    }

    T& get_value() {
      DARMA_ASSERT_MESSAGE(value_ != nullptr, "get_value() called on handle with null value");
      return *value_;
    }

    const T& get_value() const {
      DARMA_ASSERT_MESSAGE(value_ != nullptr, "get_value() called on handle with null value");
      return *value_;
    }

    // end get_value, emplace_value, set_value, etc. </editor-fold>
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // <editor-fold desc="abstract::frontend::DependencyHandle implmentation">

    // Most of this implementation is inherited from concrete base classes

    abstract::frontend::SerializationManager*
    get_serialization_manager() override {
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
      const void* const object_data
    ) const override {
      serialization::Serializer<T> s;
      serialization::SimplePackUnpackArchive ar;
      DependencyHandle_attorneys::ArchiveAccess::start_sizing(ar);
      s.compute_size(*(T const* const)(object_data), ar);
      return DependencyHandle_attorneys::ArchiveAccess::get_size(ar);
    }

    void
    pack_data(
      const void* const object_data,
      void* const serialization_buffer
    ) const override {
      serialization::Serializer<T> s;
      serialization::SimplePackUnpackArchive ar;
      DependencyHandle_attorneys::ArchiveAccess::set_buffer(ar, serialization_buffer);
      DependencyHandle_attorneys::ArchiveAccess::start_packing(ar);
      s.pack(*(T const* const)(object_data), ar);
    }

    void
    unpack_data(
      void* const object_dest,
      const void* const serialized_data
    ) const override {
      serialization::Serializer<T> s;
      serialization::SimplePackUnpackArchive ar;
      // Need to cast away constness of the buffer because the Archive requires
      // a non-const buffer to be able to operate in pack mode (i.e., so that
      // the user can write one function for both serialization and deserialization)
      DependencyHandle_attorneys::ArchiveAccess::set_buffer(
        ar, const_cast<void* const>(serialized_data)
      );
      DependencyHandle_attorneys::ArchiveAccess::start_unpacking(ar);
      s.unpack(object_dest, ar);
    }

    void*
    allocate_data() const override {
      serialization::SimplePackUnpackArchive ar;
      return serialization::detail::allocation_traits<T>::allocate(ar, 1);
    }

    // end SerializationManager implementation </editor-fold>
    ////////////////////////////////////////////////////////////

    bool has_subsequent_at_version_depth = false;

  private:

    T*& value_;


};

namespace create_work_attorneys {

class for_AccessHandle;

} // end namespace create_work_attorneys

namespace access_attorneys {

class for_AccessHandle;

} // end namespace access_attorneys

class AccessHandleBase {
  public:
    virtual ~AccessHandleBase() = default;

    typedef enum State {
      None_None,
      Read_None,
      Read_Read,
      Modify_None,
      Modify_Read,
      Modify_Modify
    } state_t;

    typedef enum CaptureOp {
      ro_capture,
      mod_capture
    } capture_op_t;

    typedef enum CapturedAsInfo {
      Normal = 0,
      Ignored = 1,
      ReadOnly = 2,
      // Future use:
      ScheduleOnly = 4,
      Leaf = 8,
      Uncaptured = 16
    } captured_as_info_t;

    typedef typename abstract::backend::runtime_t::handle_t handle_t;

  protected:

    struct read_only_usage_holder {
      handle_t* const handle_;
      read_only_usage_holder(handle_t* const handle)
        : handle_(handle)
      { }
      ~read_only_usage_holder() {
        detail::backend_runtime->release_read_only_usage(handle_);
      }
    };

    typedef typename detail::smart_ptr_traits<std::shared_ptr>::template maker<read_only_usage_holder>
      read_only_usage_holder_ptr_maker_t;

};

template <
  bool is_compile_time_modifiable_ = true,
  bool is_compile_time_readable_ = true
>
struct access_handle_traits {
  static constexpr auto is_compile_time_modifiable = is_compile_time_modifiable_;
  static constexpr auto is_compile_time_readable = is_compile_time_readable_;
  template <bool new_compile_time_mod_value>
  struct with_compile_time_modifiable {
    typedef access_handle_traits<
      new_compile_time_mod_value, is_compile_time_readable
    > type;
  };
};

} // end namespace detail

template <
  typename T = void,
  typename key_type = types::key_t,
  typename version_type = types::version_t,
  typename traits = detail::access_handle_traits<>
>
class AccessHandle;

namespace detail {

template <typename T>
struct is_access_handle
  : std::false_type { };

template <typename... Args>
struct is_access_handle<AccessHandle<Args...>>
  : std::true_type { };


} // end namespace detail

} // end namespace darma_runtime





#endif /* NEW_DEPENDENCY_H_ */
