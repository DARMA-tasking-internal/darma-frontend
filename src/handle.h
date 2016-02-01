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
#include "runtime.h"
#include "version.h"

#include "abstract/backend/data_block.h"
#include "keyword_arguments/keyword_arguments.h"

DeclareDharmaKeyword(publication, n_readers, size_t);

namespace dharma_runtime {

namespace detail {

namespace m = tinympl;
namespace mv = tinympl::variadic;
namespace mp = tinympl::placeholders;

template <
  typename... Args
>
struct access_expr_helper {

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

  inline
  typename std::enable_if<
    find_spot == sizeof...(Args),
    size_t
  >::type
  get_n_readers(
    Args&&... args
  ) const {
    // Default value
    return 1;
  }

  inline
  typename std::enable_if<
    (find_spot < sizeof...(Args)),
    size_t
  >::type
  get_n_readers(
    Args&&... args
  ) const {
    // TODO won't work with 0 args
    return std::get<
      find_spot == sizeof...(Args) ? 0 : find_spot
    >(std::forward_as_tuple(args...));
  }

};


} // end namespace detail

namespace detail {


template <typename key_type>
class KeyedObject
{
  public:

    typedef key_type key_t;

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

    const version_t&
    get_version() const {
      return version_;
    }

  protected:

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
  typename key_type=default_key_t,
  typename version_type=default_version_t
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

    using KeyedObject<key_type>::get_key;
    using VersionedObject<version_type>::get_version;

    DependencyHandleBase(
      const key_t& key,
      const version_t& version,
      AccessPermissions permissions = ReadOnly
    ) : keyed_base_t(key),
        versioned_base_t(version),
        permissions_(permissions)
    { }



    void
    allocate_metadata(size_t size) {
      // Only allowed if data is unallocated
      assert(data_ == nullptr);
      assert(satisfied_);
      data_block_->allocate_data(size);
      data_ = data_block_->get_data();
    }

    virtual void delete_value() = 0;

    virtual ~DependencyHandleBase() noexcept { }

    bool satisfied() { return true; }

  protected:
    void* data_ = nullptr;
    abstract::backend::DataBlock* data_block_ = nullptr;
    bool satisfied_ = false;
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

    DependencyHandle()
      : DependencyHandleBase(
          key_t(),
          version_t()
        ),
        value_((T*)this->data_)
    { }

    DependencyHandle(
      const key_t& data_key
    ) : DependencyHandleBase(
          data_key,
          version_t()
        ),
        value_((T*)this->data_)
    { }

    DependencyHandle(
      const key_t& data_key,
      const version_t& data_version
    ) : DependencyHandleBase(
          data_key,
          data_version
        )
    { }

    template <typename... Args>
    void
    emplace_value(Args&&... args)
    {
      if(value_ == nullptr) allocate_metadata(sizeof(T));
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

    ~DependencyHandle() noexcept = default;

    T& get_value() {
      assert(value_ != nullptr);
      return *value_;
    }

    const T& get_value() const {
      assert(value_ != nullptr);
      return *value_;
    }

  private:

    template <typename U, typename Enable=void>
    struct serialization_manager_setter {
      inline void
      operator()(
        abstract::backend::DataBlock* const dblk
      ) const {


      }

    };

  public:

    void
    satisfy_with_data(
      abstract::backend::DataBlock* const data
    ) override {
      this->satisfied_ = true;

      this->data_ = data->get_data();
    }

  private:

    T*& value_;

};

} // end namespace dharma_runtime::detail

template <
  typename T = void,
  typename key_type = types::key_t,
  typename version_type = types::version_t,
  template <typename...> class smart_ptr_template = types::shared_ptr_template
>
class AccessHandle
{
  protected:

    typedef detail::DependencyHandle<T, key_type, version_type> dep_handle_t;
    typedef smart_ptr_template<dep_handle_t> dep_handle_ptr;
    typedef smart_ptr_template<const dep_handle_t> dep_handle_const_ptr;
    typedef detail::TaskBase<key_type, version_type> task_t;
    typedef smart_ptr_template<task_t> task_ptr;
    typedef typename detail::smart_ptr_traits<smart_ptr_template>::maker<dep_handle_t>
      dep_handle_ptr_maker_t;

    typedef typename dep_handle_t::key_t key_t;
    typedef typename dep_handle_t::version_t version_t;

  public:

    AccessHandle(const AccessHandle& other) = delete;

    // TODO this needs to be a move constructor (probably?)
    AccessHandle(
      AccessHandle& other
    ) : dep_handle_(other.dep_handle_)
    {
      // get the shared_ptr from the weak_ptr stored in the runtime object
      capturing_task = static_cast<detail::TaskBase* const>(
        detail::backend_runtime->get_running_task()
      )->current_create_work_context;
      if(capturing_task != nullptr) {
        // TODO also ask the task if any special permissions downgrades were given
        // TODO !!! connect outputs to corresponding parent task output
        switch(other.dep_handle_->get_permissions()) {
        case detail::ReadOnly: {
            // just copy the handle pointer; other remains unchanged
            dep_handle_ = other.dep_handle_;
            permissions_ = detail::ReadOnly;
            // and add it as an input
            capturing_task->add_input(dep_handle_);
            break;
          } // end case detail::ReadOnly
        case detail::ReadWrite: {
            // Permissions stay the same, both for this and other
            permissions_ = other.permissions_ = detail::ReadWrite;
            // Version for other gets incremented
            auto other_version = other.dep_handle_->get_version();
            ++other_version;
            // this now assumes control of other's handle
            dep_handle_ = other.dep_handle_;
            // Add the dep_handle_ as an input dependency
            // (i.e., data needs to be there for task to start)
            capturing_task->add_input(dep_handle_);
            // ...and increments the subversion depth
            dep_handle_->push_subversion();
            // Now future uses of other will have a later version.
            other.dep_handle_ = dep_handle_ptr_maker_t()(
              other.dep_handle_->key(),
              other_version
            );
            // Add other.dep_handle_ as an output handle,
            // (i.e., the data written inside the task must
            // be in other.dep_handle_ when the task reports
            // completion)
            capturing_task->add_output(other.dep_handle_);
            break;
          } // end case detail::ReadWrite
        case detail::OverwriteOnly: {
            // Version gets incremented for other
            auto other_version = other.dep_handle_->get_version();
            ++other_version;
            // this now assumes control of other's handle
            dep_handle_ = other.dep_handle_;
            // ...and increments the subversion depth
            dep_handle_->push_subversion();
            // Now subsequent uses of other will need to refer to
            // a later version that is now ReadWrite (since
            // the overwritten data will be available to the
            // handle)
            other.dep_handle_ = dep_handle_ptr_maker_t()(
              other.dep_handle_->key(),
              other_version,
              detail::ReadWrite
            );
            // Add other.dep_handle_ as an output handle,
            // (i.e., the data written inside the task must
            // be in other.dep_handle_ when the task reports
            // completion)
            capturing_task->add_output(other.dep_handle_);
            break;
          } // end case detail::OverwriteOnly
        case detail::Create: {
            // Version gets incremented for other
            auto other_version = other.dep_handle_->get_version();
            ++other_version;
            // this now assumes control of other's handle
            dep_handle_ = other.dep_handle_;
            // ...and increments the subversion depth
            dep_handle_->push_subversion();
            // permissions for this remain Create, however
            // future references to other will have ReadWrite,
            // since they can read the data created here
            other.dep_handle_ = dep_handle_ptr_maker_t()(
              other.dep_handle_->key(),
              other_version,
              detail::ReadWrite
            );
            // Add other.dep_handle_ as an output handle,
            // (i.e., the data written inside the task must
            // be in other.dep_handle_ when the task reports
            // completion)
            capturing_task->add_output(other.dep_handle_);
            break;
          } // end case detail::Create
        } // end switch
      } // end if capturing_task != nullptr

    }

    T* operator->() {
      return &dep_handle_->get_value();
    }

    const T* operator->() const {
      return &dep_handle_->get_value();
    }

    template <
      typename... PublishExprParts
    >
    void publish(
      PublishExprParts&&... parts
    ) {
      assert(permissions_ == detail::ReadWrite || permissions_ == detail::OverwriteOnly);
      detail::publish_expr_helper<PublishExprParts...> helper;
      auto& rts = detail::thread_runtime;
      rts.backend_runtime->publish_handle(
        dep_handle_.get(),
        helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
        helper.get_n_readers(std::forward<PublishExprParts>(parts)...)
      );
    }


   private:

    AccessHandle(
      const key_type& key,
      const version_type& version,
      detail::AccessPermissions permissions
    ) : dep_handle_(
          dep_handle_ptr_maker_t()(key, version)
        ),
        permissions_(permissions)
    { }

    template <typename Deleter>
    AccessHandle(
      const key_type& key,
      const version_type& version,
      detail::AccessPermissions permissions,
      Deleter deleter
    ) : dep_handle_(
          dep_handle_ptr_maker_t()(key, version),
          deleter
        ),
        permissions_(permissions)
    { }


    detail::AccessPermissions
    get_permissions() const {
      return permissions_;
    }

    void
    set_permissions(detail::AccessPermissions p) {
      permissions_ = p;
    }

    task_t* capturing_task = nullptr;
    mutable dep_handle_ptr dep_handle_;
    mutable detail::AccessPermissions permissions_;

    template <
      typename T,
      typename... KeyExprParts
    >
    friend AccessHandle<T>
    initial_access(
      KeyExprParts&&... parts
    ) {
      // TODO this may not be correct
      key_type key = make_key_from_tuple(
        detail::access_expr_helper<KeyExprParts...>().get_key_tuple(
          std::forward<KeyExprParts>(parts...)
        )
      );
      //version_type version;
      //auto rv = AccessHandle<T>(key, version, detail::Create);
      //auto& rtc = detail::thread_runtime;
      //rtc.backend_runtime->register_handle(rv.dep_handle_);
      //return rv;
    }

    template <
      typename T,
      typename... KeyExprParts
    >
    friend AccessHandle<T>
    read_access(
      KeyExprParts&&... parts
    ) {
      // TODO this may not be correct
      // typedef detail::access_expr_helper<KeyExprParts...> helper_t;
      // helper_t helper;
      // auto& rtc = detail::thread_runtime;
      // key_type key = make_key_from_tuple(
      //   helper.get_key_tuple(std::forward<KeyExprParts>(parts)...)
      // );
      // version_type version = rtc.backend_runtime->resolve_version_tag(
      //   key, helper.get_version_tag(std::forward<KeyExprParts>(parts)...)
      // );
      // auto rv = AccessHandle<T>(key, version, detail::ReadOnly,
      //   [](const dep_handle_t* const to_release) {
      //     auto& rtc = detail::thread_runtime;
      //     rtc.backend_runtime->release_fetcher(to_release);
      //   }
      // );
      // rtc.backend_runtime->register_fetcher(rv.dep_handle_.get());
    }

    template <
      typename T,
      typename... KeyExprParts
    >
    friend AccessHandle<T>
    read_write(
      KeyExprParts&&... parts
    ) {
      // TODO write this
      //typedef detail::access_expr_helper<KeyExprParts...> helper_t;
      //helper_t helper;
      //auto& rtc = detail::thread_runtime;
      //key_type key = make_key_from_tuple(
      //  helper.get_key_tuple(std::forward<KeyExprParts>(parts)...)
      //);
      //version_type version = rtc.backend_runtime->resolve_version_tag(
      //  key, helper.get_version_tag(std::forward<KeyExprParts>(parts)...)
      //);
      //auto rv = AccessHandle<T>(key, version, detail::ReadWrite,
      //  [](const dep_handle_t* const to_release) {
      //    auto& rtc = detail::thread_runtime;
      //    rtc.backend_runtime->release_fetcher(to_release);
      //    delete to_release;
      //  }
      //);
      //rtc.backend_runtime->register_fetcher(rv.dep_handle_.get());
    }

};




} // end namespace dharma



#endif /* NEW_DEPENDENCY_H_ */
