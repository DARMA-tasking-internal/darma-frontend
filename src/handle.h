/*
//@HEADER
// ************************************************************************
//
//                          dependency.h
//                         dharma_mockup
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

#include <tinympl/variadic/find_if.hpp>
#include <tinympl/bind.hpp>
#include <tinympl/lambda.hpp>

#include "task.h"
#include "runtime.h"
#include "abstract/defaults/version.h"
#include "util.h"
#include "dharma_assert.h"

#include "abstract/backend/data_block.h"
#include "keyword_arguments/keyword_arguments.h"

DeclareDharmaKeyword(publication, n_readers, size_t);
// TODO make this a key expression instead of a std::string
DeclareDharmaKeyword(publication, version, int);

namespace dharma_runtime {

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
    // TODO
    return types::key_t();
  }

  inline types::key_t
  get_version_tag(
    Args&&... args
  ) const {
    // TODO
    return types::key_t();
  }

};

template <
  typename... Args
>
struct publish_expr_helper {
  static constexpr size_t find_spot = mv::find_if<
    m::lambda<
      is_kwarg_expression_with_tag<mp::_,
        dharma_runtime::keyword_tags_for_publication::n_readers
      >
    >::template apply,
    Args...
  >::value;

  inline size_t
  get_n_readers(
    Args&&... args
  ) const {
    // Default value
    // TODO fix this
    return 1;
  }

  //inline
  //typename std::enable_if<
  //  (find_spot < sizeof...(Args)),
  //  size_t
  //>::type
  //get_n_readers(
  //  Args&&... args
  //) const {
  //  // TODO won't work with 0 args
  //  return std::get<
  //    find_spot == sizeof...(Args) ? 0 : find_spot
  //  >(std::forward_as_tuple(args...));
  //}

  inline
  types::key_t
  get_version_tag(
    Args&&... args
  ) const {
    // TODO
    return types::key_t();
  }


  inline bool
  version_tag_is_final(
    Args&&... args
  ) const {
    // TODO
    return false;
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


typedef enum AccessPermissions {
  ReadOnly,
  OverwriteOnly,
  ReadWrite,
  Create,
  AccessPermissions_MAX=Create,
  AccessPermissions_MIN=ReadOnly
} AccessPermissions;


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
    set_version_is_pending(bool is_pending) {
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
      backend_runtime->register_fetching_handle(this,
        user_version_tag, write_access_allowed
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
      value_ = new (&value_) T(
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

    ~DependencyHandle() {
      backend_runtime->release_handle(this);
    }

    T& get_value() {
      assert(value_ != nullptr);
      return *value_;
    }

    const T& get_value() const {
      assert(value_ != nullptr);
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
    get_data_block() override {
      return this->data_block_;
    }


  private:

    T*& value_;

    // TODO more general serialization
    // for now...
    trivial_serialization_manager<T> ser_manager_;

};

} // end namespace dharma_runtime::detail

template <
  typename T = void,
  typename key_type = types::key_t,
  typename version_type = types::version_t,
  template <typename...> class smart_ptr_template = types::shared_ptr_template
>
class AccessHandle;

template <
  typename U,
  typename... KeyExprParts
>
AccessHandle<U>
read_access(
  KeyExprParts&&... parts
);

template <
  typename U,
  typename... KeyExprParts
>
AccessHandle<U>
read_write(
  KeyExprParts&&... parts
);

// TODO !!! 0.2.0 call set_version_is_pending() in the proper places
template <
  typename T,
  typename key_type,
  typename version_type,
  template <typename...> class smart_ptr_template
>
class AccessHandle
{
  protected:

    typedef detail::DependencyHandle<T, key_type, version_type> dep_handle_t;
    typedef smart_ptr_template<dep_handle_t> dep_handle_ptr;
    typedef smart_ptr_template<const dep_handle_t> dep_handle_const_ptr;
    typedef detail::TaskBase task_t;
    typedef smart_ptr_template<task_t> task_ptr;
    typedef typename detail::smart_ptr_traits<std::shared_ptr>::template maker<dep_handle_t>
      dep_handle_ptr_maker_t;

    typedef typename dep_handle_t::key_t key_t;
    typedef typename dep_handle_t::version_t version_t;

  private:
    ////////////////////////////////////////
    // private inner classes

    struct read_only_usage_holder {
      dep_handle_ptr handle_;
      read_only_usage_holder(const dep_handle_ptr& handle)
        : handle_(handle)
      { }
      ~read_only_usage_holder() {
        detail::backend_runtime->release_read_only_usage(handle_.get());
      }
    };

    typedef typename detail::smart_ptr_traits<std::shared_ptr>::template maker<read_only_usage_holder>
      read_only_usage_holder_ptr_maker_t;

  public:
    AccessHandle() : prev_copied_from(nullptr) {
      // TODO more safety checks to make sure uninitialized handles aren't getting captured
    }

    AccessHandle&
    operator=(const AccessHandle& other) {
      // for now...
      assert(other.prev_copied_from == nullptr);
      dep_handle_ = other.dep_handle_;
      permissions_ = other.permissions_;
      return *this;
    }

    AccessHandle&
    operator=(const AccessHandle& other) const {
      // for now...
      assert(other.prev_copied_from == nullptr);
      dep_handle_ = other.dep_handle_;
      permissions_ = other.permissions_;
      return *this;
    }

    AccessHandle(const AccessHandle& copied_from)
      : dep_handle_(copied_from.dep_handle_),
        permissions_(copied_from.permissions_),
        // this copy constructor may be invoked in ordinary usage or
        // may be the actual capture itself.  In the latter case, the subsequent
        // move needs access back to the outer context object, so we need to
        // save a pointer back to other.  It should be ignored otherwise, though.
        prev_copied_from(const_cast<AccessHandle* const>(&copied_from))
    {
      // get the shared_ptr from the weak_ptr stored in the runtime object
      capturing_task = dynamic_cast<detail::TaskBase*>(
        detail::backend_runtime->get_running_task()
      )->current_create_work_context;

      // Now check if we're in a capturing context:
      if(capturing_task != nullptr) {
        assert(copied_from.prev_copied_from != nullptr);
        AccessHandle& outer = *(copied_from.prev_copied_from);
        DHARMA_ASSERT_NOT_NULL(copied_from.prev_copied_from);
        // TODO also ask the task if any special permissions downgrades were given
        // TODO !!! connect outputs to corresponding parent task output
        switch(outer.get_permissions()) {
        case detail::ReadOnly: {
            // just copy the handle pointer; other remains unchanged
            dep_handle_ = outer.dep_handle_;
            permissions_ = detail::ReadOnly;
            // and add it as an input
            capturing_task->add_dependency(
              dep_handle_,
              /*needs_read_data = */ true,
              /*needs_write_data = */ false
            );
            read_only_holder_ = outer.read_only_holder_;
            break;
          } // end case detail::ReadOnly
        case detail::ReadWrite: {
            // Permissions stay the same, both for this and other
            permissions_ = outer.permissions_ = detail::ReadWrite;
            // Version for other gets incremented
            auto other_version = outer.dep_handle_->get_version();
            ++other_version;
            // this now assumes control of other's handle
            dep_handle_ = outer.dep_handle_;
            // ...and increments the subversion depth
            dep_handle_->push_subversion();
            // Add the dep_handle_ as a dependency
            capturing_task->add_dependency(
              dep_handle_,
              /*needs_read_data = */ true,
              /*needs_write_data = */ true
            );
            // Now future uses of other will have a later version.
            outer.dep_handle_ = dep_handle_ptr_maker_t()(
              outer.dep_handle_->get_key(),
              other_version
            );
            // setup the read only holder for outer, but specifically
            // do not transfer the old holder to this.  When all
            // other uses of dep_handle_ are released, the destructor
            // of the usage_holder will be called, telling the runtime
            // system that no more read-only tasks will be created using
            // this handle (in an unsatisfied state; read/write tasks
            // can create read only tasks, but since the handle is satisfied,
            // the data can just be read in place)
            outer.read_only_holder_ = read_only_usage_holder_ptr_maker_t()(outer.dep_handle_);
            // Note that other.dep_handle_ is the output handle
            break;
          } // end case detail::ReadWrite
        case detail::OverwriteOnly: {
            // Version gets incremented for other
            auto other_version = outer.dep_handle_->get_version();
            ++other_version;
            // this now assumes control of other's handle
            dep_handle_ = outer.dep_handle_;
            // ...and increments the subversion depth
            dep_handle_->push_subversion();
            // Add it as a dependency
            capturing_task->add_dependency(
              dep_handle_,
              /*needs_read_data = */ false,
              /*needs_write_data = */ true
            );
            // Now subsequent uses of other will need to refer to
            // a later version that is now ReadWrite (since
            // the overwritten data will be available to the
            // handle)
            outer.dep_handle_ = dep_handle_ptr_maker_t()(
              outer.dep_handle_->get_key(),
              other_version
            );
            outer.set_permissions(detail::ReadWrite);
            // see ReadWrite case for explanation
            outer.read_only_holder_ = read_only_usage_holder_ptr_maker_t()(outer.dep_handle_);
            // Note that other.dep_handle_ is the output handle
            break;
          } // end case detail::OverwriteOnly
        case detail::Create: {
            assert(dep_handle_->get_version() == version_t());
            // Version gets incremented for other
            auto other_version = outer.dep_handle_->get_version();
            ++other_version;
            // this now assumes control of other's handle
            dep_handle_ = outer.dep_handle_;
            // ...and increments the subversion depth
            dep_handle_->push_subversion();
            // now add it as a dependency, except it neither needs to
            // read nor overwrite
            capturing_task->add_dependency(
              dep_handle_,
              /*needs_read_data = */ false,
              /*needs_write_data = */ true
            );
            // permissions for this remain Create, however
            // future references to other will have ReadWrite,
            // since they can read the data created here
            outer.dep_handle_ = dep_handle_ptr_maker_t()(
              outer.dep_handle_->get_key(),
              other_version
            );
            outer.set_permissions(detail::ReadWrite);
            // see ReadWrite case for explanation
            outer.read_only_holder_ = read_only_usage_holder_ptr_maker_t()(outer.dep_handle_);
            // Note that other.dep_handle_ is the output handle
            break;
          } // end case detail::Create
        } // end switch
      } // end if capturing_task != nullptr

    }

    T* operator->() const {
      return &dep_handle_->get_value();
    }

    template <typename U>
    typename std::enable_if<
      std::is_convertible<U, T>::value,
      void
    >::type
    set_value(U&& val) const {
      dep_handle_->set_value(val);
    }

    template <typename U>
    void
    operator=(const U& other) const { }

    const T&
    get_value() const {
      return dep_handle_->get_value();
    }

    T&
    get_reference() const {
      return dep_handle_->get_value();
    }

    template <
      typename... PublishExprParts
    >
    void publish(
      PublishExprParts&&... parts
    ) const {
      assert(permissions_ == detail::ReadWrite || permissions_ == detail::OverwriteOnly);
      detail::publish_expr_helper<PublishExprParts...> helper;
      detail::backend_runtime->publish_handle(
        dep_handle_.get(),
        helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
        helper.get_n_readers(std::forward<PublishExprParts>(parts)...),
        helper.version_tag_is_final(std::forward<PublishExprParts>(parts)...)
      );
    }

    ~AccessHandle() {
      // TODO make sure this is correct
      if(capturing_task) {
        detail::backend_runtime->handle_done_with_version_depth(dep_handle_.get());
      }
    }

   private:

    ////////////////////////////////////////
    // private constructors

    AccessHandle(
      const key_type& key,
      const version_type& version,
      detail::AccessPermissions permissions
    ) : dep_handle_(
          dep_handle_ptr_maker_t()(key, version)
        ),
        permissions_(permissions),
        read_only_holder_(read_only_usage_holder_ptr_maker_t()(dep_handle_))
    {
      if(permissions == detail::Create || permissions == detail::OverwriteOnly) {
        // Release immediately; there's nothing to read
        read_only_holder_.reset();
      }
    }

    AccessHandle(
      const key_type& key,
      detail::AccessPermissions permissions,
      const key_type& user_version_tag
    ) : dep_handle_(
          dep_handle_ptr_maker_t()(key, user_version_tag, permissions != detail::ReadOnly)
        ),
        permissions_(permissions),
        read_only_holder_(read_only_usage_holder_ptr_maker_t()(dep_handle_))
    {
      assert(permissions != detail::Create);
      if(permissions == detail::OverwriteOnly) {
        // Release immediately; there's nothing to read
        read_only_holder_.reset();
      }
    }

    //template <typename Deleter>
    //AccessHandle(
    //  const key_type& key,
    //  const version_type& version,
    //  detail::AccessPermissions permissions,
    //  Deleter deleter
    //) : dep_handle_(
    //      dep_handle_ptr_maker_t()(key, version),
    //      deleter
    //    ),
    //    permissions_(permissions)
    //{ }


    ////////////////////////////////////////
    // private methods

    detail::AccessPermissions
    get_permissions() const {
      return permissions_;
    }

    void
    set_permissions(detail::AccessPermissions p) const {
      permissions_ = p;
    }

    ////////////////////////////////////////
    // private members

    mutable dep_handle_ptr dep_handle_;
    mutable detail::AccessPermissions permissions_;

    types::shared_ptr_template<read_only_usage_holder> read_only_holder_;
    task_t* capturing_task = nullptr;
    AccessHandle* const prev_copied_from = nullptr;

   public:

    ////////////////////////////////////////
    // Friend functions

    //template <
    //  typename... KeyExprParts
    //>
    //friend AccessHandle
    //initial_access(
    //  KeyExprParts&&... parts
    //);

    friend void
    _initial_access_impl(
      const key_t& key, const version_t& version, AccessHandle& rv
    ) {
      rv = AccessHandle(key, version, detail::Create);
    }



    friend void
    _read_access_impl(
      const key_type& key, const key_type& user_version_tag, AccessHandle& rv
    ) {
      rv = AccessHandle(key, detail::ReadOnly, user_version_tag);
    }

    friend void
    _read_write_impl(
      const key_type& key, const key_type& user_version_tag, AccessHandle& rv
    ) {
      rv = AccessHandle(key, detail::ReadWrite, user_version_tag);
    }

};

template <
  typename T,
  typename... KeyExprParts
>
AccessHandle<T>
initial_access(
  KeyExprParts&&... parts
) {
  types::key_t key = detail::access_expr_helper<KeyExprParts...>().get_key(
    std::forward<KeyExprParts>(parts)...
  );
  types::version_t version = { };
  AccessHandle<T> rv;
  _initial_access_impl(key, version, rv);
  return rv;
}

template <
  typename U,
  typename... KeyExprParts
>
AccessHandle<U>
read_access(
  KeyExprParts&&... parts
) {
  typedef detail::access_expr_helper<KeyExprParts...> helper_t;
  helper_t helper;
  types::key_t key = helper.get_key(std::forward<KeyExprParts>(parts)...);
  types::key_t user_version_tag = helper.get_version_tag(std::forward<KeyExprParts>(parts)...);
  AccessHandle<U> rv;
  _read_access_impl(key, user_version_tag, rv);
  return rv;
}

template <
  typename U,
  typename... KeyExprParts
>
AccessHandle<U>
read_write(
  KeyExprParts&&... parts
) {
  typedef detail::access_expr_helper<KeyExprParts...> helper_t;
  helper_t helper;
  types::key_t key = helper.get_key(std::forward<KeyExprParts>(parts)...);
  types::key_t user_version_tag = helper.get_version_tag(std::forward<KeyExprParts>(parts)...);
  AccessHandle<U> rv;
  _read_write_impl(key, user_version_tag, rv);
  return rv;
}



} // end namespace dharma



#endif /* NEW_DEPENDENCY_H_ */
