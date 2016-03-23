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

#include <darma/impl/task.h>
#include <darma/impl/runtime.h>
#include <darma/interface/defaults/version.h>
#include <darma/impl/util.h>
#include <darma/impl/darma_assert.h>

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

struct EmptyClass { };

template <
  typename T,
  typename key_type,
  typename version_type
>
class DependencyHandle
  : public DependencyHandleBase<key_type, version_type>
{
  protected:

    typedef DependencyHandleBase<key_type, version_type> base_t;

  public:

    typedef typename base_t::key_t key_t;
    typedef typename base_t::version_t version_t;

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

    void
    set_value(const T& val)
    {
      // Invoke the copy constructor
      emplace_value(val);
    }

    void
    set_value(T&& val)
    {
      // Invoke the move constructor, if available
      emplace_value(std::forward<T>(val));
    }

    template <typename U>
    std::enable_if_t<
      std::is_convertible<U, T>::value
    >
    set_value(U&& val) {
      // TODO handle custom operator=() case (for instance)
      emplace_value(std::forward<U>(val));
    }

    virtual ~DependencyHandle() {
      if(not has_subsequent_at_version_depth) backend_runtime->handle_done_with_version_depth(this);
      backend_runtime->release_handle(this);
    }

    T& get_value() {
      DARMA_ASSERT_NOT_NULL(value_, "get_value() called on handle with null value");
      return *value_;
    }

    const T& get_value() const {
      DARMA_ASSERT_NOT_NULL(value_, "get_value() called on handle with null value");
      return *value_;
    }

  private:

    template <typename U>
    struct trivial_serialization_manager
      : public abstract::frontend::SerializationManager
    {
      size_t get_metadata_size(
        const void* const deserialized_data
      ) const override {
        return sizeof(U);
      }
    };

  public:

    abstract::frontend::SerializationManager*
    get_serialization_manager() override {
      return &ser_manager_;
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


    bool has_subsequent_at_version_depth = false;

  private:

    T*& value_;

    // TODO more general serialization
    // for now...
    trivial_serialization_manager<T> ser_manager_;

};

namespace create_work_attorneys {

class for_AccessHandle;

} // end namespace create_work_attorneys

namespace access_attorneys {

class for_AccessHandle;

} // end namespace access_attorneys

} // end namespace darma_runtime::detail

template <
  typename T = void,
  typename key_type = types::key_t,
  typename version_type = types::version_t,
  template <typename...> class smart_ptr_template = types::shared_ptr_template
>
class AccessHandle;

}





#endif /* NEW_DEPENDENCY_H_ */
