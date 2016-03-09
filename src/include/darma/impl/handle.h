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

DeclareDarmaTypeTransparentKeyword(publication, n_readers);
// TODO make this a key expression instead of a std::string
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
    get_data_block() const override {
      return this->data_block_;
    }


  private:

    T*& value_;

    // TODO more general serialization
    // for now...
    trivial_serialization_manager<T> ser_manager_;

};

namespace create_work_attorneys {

class for_AccessHandle;

} // end namespace create_work_attorneys

} // end namespace darma_runtime::detail

template <
  typename T = void,
  typename key_type = types::key_t,
  typename version_type = types::version_t,
  template <typename...> class smart_ptr_template = types::shared_ptr_template
>
class AccessHandle;

template <
  typename T,
  typename key_type,
  typename version_type,
  template <typename...> class smart_ptr_template
>
class AccessHandle
{
  protected:

    typedef detail::DependencyHandle<
        typename std::conditional<std::is_same<T, void>::value,
          detail::EmptyClass, T
        >::type, key_type, version_type> dep_handle_t;
    typedef smart_ptr_template<dep_handle_t> dep_handle_ptr;
    typedef smart_ptr_template<const dep_handle_t> dep_handle_const_ptr;
    typedef detail::TaskBase task_t;
    typedef smart_ptr_template<task_t> task_ptr;
    typedef typename detail::smart_ptr_traits<std::shared_ptr>::template maker<dep_handle_t>
      dep_handle_ptr_maker_t;

    typedef typename dep_handle_t::key_t key_t;
    typedef typename dep_handle_t::version_t version_t;

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
      state_ = other.state_;
      read_only_holder_ = other.read_only_holder_;
      return *this;
    }

    AccessHandle&
    operator=(const AccessHandle& other) const {
      // for now...
      assert(other.prev_copied_from == nullptr);
      dep_handle_ = other.dep_handle_;
      state_ = other.state_;
      read_only_holder_ = other.read_only_holder_;
      return *this;
    }

    AccessHandle&
    operator=(std::nullptr_t) const {
      if(state_ == Modify_Modify || state_ == Modify_None || state_ == Modify_Read) {
        detail::backend_runtime->handle_done_with_version_depth(dep_handle_.get());
      }
      dep_handle_ = 0;
      state_ = None_None;
      read_only_holder_ = 0;
      return *this;
    }

    AccessHandle(const AccessHandle& copied_from)
      : dep_handle_(copied_from.dep_handle_),
        // this copy constructor may be invoked in ordinary usage or
        // may be the actual capture itself.  In the latter case, the subsequent
        // move needs access back to the outer context object, so we need to
        // save a pointer back to other.  It should be ignored otherwise, though.
        // note that the below const_cast is needed to convert from AccessHandle const* to AccessHandle* const
        prev_copied_from(const_cast<AccessHandle* const>(&copied_from)),
        read_only_holder_(copied_from.read_only_holder_)
    {
      // get the shared_ptr from the weak_ptr stored in the runtime object
      detail::TaskBase* running_task = dynamic_cast<detail::TaskBase* const>(
        detail::backend_runtime->get_running_task()
      );
      capturing_task = running_task->current_create_work_context;

      // Now check if we're in a capturing context:
      if(capturing_task != nullptr) {

        DARMA_ASSERT_NOT_NULL(copied_from.prev_copied_from);
        AccessHandle& outer = *(copied_from.prev_copied_from);

        {
          // Clean out the middle thing
          AccessHandle* copied_from_ptr = const_cast<AccessHandle*>(&copied_from);
          copied_from_ptr->dep_handle_.reset();
          copied_from_ptr->read_only_holder_.reset();
        }

        bool ignored = running_task->ignored_handles.find(dep_handle_.get())
            != running_task->ignored_handles.end();

        if(not ignored) {

          ////////////////////////////////////////////////////////////////////////////////

          // Determine the capture type
          // TODO check that any explicit permissions are obeyed

          capture_op_t capture_type;

          // first check for any explicit permissions
          auto found = running_task->read_only_handles.find(outer.dep_handle_.get());
          if(found != running_task->read_only_handles.end()) {
            capture_type = ro_capture;
            running_task->read_only_handles.erase(found);
          }
          else {
            // Deduce capture type from state
            switch(outer.state_) {
              case Read_None:
              case Read_Read: {
                capture_type = ro_capture;
                break;
              }
              case Modify_None:
              case Modify_Read:
              case Modify_Modify: {
                capture_type = mod_capture;
                break;
              }
              case None_None: {
                DARMA_ASSERT_MESSAGE(false, "Handle used after release");
                break;
              }
            };
          }

          ////////////////////////////////////////////////////////////////////////////////

          switch(capture_type) {
            case ro_capture: {
              switch(outer.state_) {
                case None_None: {
                  DARMA_ASSERT_MESSAGE(false, "Handle used after release");
                  break;
                }
                case Read_None:
                case Read_Read:
                case Modify_None:
                case Modify_Read: {
                  dep_handle_ = outer.dep_handle_;
                  state_ = Read_Read;
                  read_only_holder_ = outer.read_only_holder_;

                  // Outer dep handle, state, read_only_holder stays the same

                  break;
                }
                case Modify_Modify: {
                  version_t next_version = outer.dep_handle_->get_version();
                  ++next_version;
                  dep_handle_ = dep_handle_ptr_maker_t()(
                    outer.dep_handle_->get_key(),
                    next_version
                  );
                  read_only_holder_ = read_only_usage_holder_ptr_maker_t()(dep_handle_);
                  state_ = Read_Read;

                  outer.dep_handle_ = dep_handle_;
                  outer.read_only_holder_ = read_only_holder_;
                  outer.state_ = Modify_Read;

                  break;
                }
              }; // end switch outer.state_
              capturing_task->add_dependency(
                dep_handle_,
                /*needs_read_data = */ true,
                /*needs_write_data = */ false
              );
              break;
            }
            case mod_capture: {
              switch(outer.state_) {
                case None_None: {
                  DARMA_ASSERT_MESSAGE(false, "Handle used after release");
                  break;
                }
                case Read_None:
                case Read_Read: {
                  // TODO error here
                  assert(false);
                  break; // unreachable
                }
                case Modify_None:
                case Modify_Read: {
                  version_t outer_version = outer.dep_handle_->get_version();
                  ++outer_version;
                  dep_handle_ = outer.dep_handle_;
                  read_only_holder_ = 0;
                  state_ = Modify_Modify;

                  outer.dep_handle_ = dep_handle_ptr_maker_t()(
                    dep_handle_->get_key(),
                    outer_version
                  );
                  outer.read_only_holder_ = read_only_usage_holder_ptr_maker_t()(outer.dep_handle_);
                  outer.state_ = Modify_None;

                  dep_handle_->push_subversion();

                  break;
                }
                case Modify_Modify: {
                  version_t outer_version = outer.dep_handle_->get_version();
                  ++outer_version;
                  version_t captured_version = outer.dep_handle_->get_version();
                  captured_version.push_subversion();
                  ++captured_version;

                  // avoid releasing the old until these two are made
                  auto tmp = outer.dep_handle_;
                  auto tmp_ro = outer.read_only_holder_;

                  dep_handle_ = dep_handle_ptr_maker_t()(
                    dep_handle_->get_key(), captured_version
                  );
                  // No read only uses of this new handle
                  read_only_holder_ = read_only_usage_holder_ptr_maker_t()(dep_handle_);;
                  read_only_holder_.reset();
                  state_ = Modify_Modify;

                  outer.dep_handle_ = dep_handle_ptr_maker_t()(
                    dep_handle_->get_key(), outer_version
                  );
                  outer.read_only_holder_ = read_only_usage_holder_ptr_maker_t()(outer.dep_handle_);
                  outer.state_ = Modify_None;

                  break;
                }
              } // end switch outer.state
              capturing_task->add_dependency(
                dep_handle_,
                /*needs_read_data = */ dep_handle_->get_version() != version_t(),
                /*needs_write_data = */ true
              );
              break;
            } // end mod_capture case
          } // end switch(capture_type)
        } else {
          // ignored
          capturing_task = nullptr;
          dep_handle_.reset();
          read_only_holder_.reset();
          state_ = None_None;
        }

        // This doesn't really matter until we have modifiable fetching versions, but still...
        if(dep_handle_->version_is_pending()) {
          outer.dep_handle_->set_version_is_pending(true);
        }

      } // end if capturing_task != nullptr
    }

    T* operator->() const {
      return &dep_handle_->get_value();
    }

    template <
      typename U,
      typename = std::enable_if<std::is_convertible<U, T>::value>
    >
    void
    set_value(U&& val) const {
      dep_handle_->set_value(val);
    }

    template <typename... Args>
    void
    emplace_value(Args&&... args) const {
      dep_handle_->emplace_value(std::forward<Args>(args)...);
    }

    template <
      typename = std::enable_if<
        not std::is_same<T, void>::value
      >
    >
    const T&
    get_value() const {
      return dep_handle_->get_value();
    }

    const key_t&
    get_key() const {
      return dep_handle_->get_key();
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
      detail::publish_expr_helper<PublishExprParts...> helper;
      switch(state_) {
        case None_None: {
          DARMA_ASSERT_MESSAGE(false, "Handle used after release");
          break;
        }
        case Read_None:
        case Read_Read:
        case Modify_None:
        case Modify_Read: {
          detail::backend_runtime->publish_handle(
            dep_handle_.get(),
            helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
            helper.get_n_readers(std::forward<PublishExprParts>(parts)...)
          );
          // State unchanged...
          break;
        }
        case Modify_Modify: {
          // Create a new handle with the next version
          auto next_version = dep_handle_->get_version();
          ++next_version;
          dep_handle_ = dep_handle_ptr_maker_t()(dep_handle_->get_key(), next_version);
          detail::backend_runtime->publish_handle(
            dep_handle_.get(),
            helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
            helper.get_n_readers(std::forward<PublishExprParts>(parts)...)
          );
          read_only_holder_ = read_only_usage_holder_ptr_maker_t()(dep_handle_);
          // Continuing state is MR
          state_ = Modify_Read;
          assert(false); // not implemented
          break;
        }
      };
    }

    ~AccessHandle() {
      if(capturing_task) {
        if(state_ == Modify_Modify || state_ == Modify_None || state_ == Modify_Read) {
          detail::backend_runtime->handle_done_with_version_depth(dep_handle_.get());
        }
      }
    }

   private:

    ////////////////////////////////////////
    // private constructors

    AccessHandle(
      const key_type& key,
      const version_type& version,
      state_t initial_state
    ) : dep_handle_(
          dep_handle_ptr_maker_t()(key, version)
        ),
        state_(initial_state),
        read_only_holder_(read_only_usage_holder_ptr_maker_t()(dep_handle_))
    { }

    AccessHandle(
      const key_type& key,
      state_t initial_state,
      const key_type& user_version_tag
    ) : dep_handle_(
          dep_handle_ptr_maker_t()(key, user_version_tag, false)
        ),
        state_(initial_state),
        read_only_holder_(read_only_usage_holder_ptr_maker_t()(dep_handle_))
    {
      assert(state_ == Read_None);
    }

    ////////////////////////////////////////
    // private members

    mutable dep_handle_ptr dep_handle_;
    mutable state_t state_;

    mutable types::shared_ptr_template<read_only_usage_holder> read_only_holder_;
    task_t* capturing_task = nullptr;
    AccessHandle* const prev_copied_from = nullptr;

   public:

    ////////////////////////////////////////
    // Friend functions

    friend void
    _initial_access_impl(
      const key_t& key, const version_t& version, AccessHandle& rv
    ) {
      assert(version == version_t());
      rv = AccessHandle(key, version, Modify_None);
    }

    friend void
    _read_access_impl(
      const key_type& key, const key_type& user_version_tag, AccessHandle& rv
    ) {
      rv = AccessHandle(key, Read_None, user_version_tag);
    }

    friend void
    _read_write_impl(
      const key_type& key, const key_type& user_version_tag, AccessHandle& rv
    ) {
      assert(false);
      //rv = AccessHandle(key, detail::ReadWrite, user_version_tag);
    }

    ////////////////////////////////////////
    // Attorney for create_work
    friend class detail::create_work_attorneys::for_AccessHandle;

};

namespace detail {

namespace create_work_attorneys {

struct for_AccessHandle {
  template <typename AccessHandleType>
  static inline
  typename tinympl::copy_cv_qualifiers<AccessHandleType>::template apply<
    typename AccessHandleType::dep_handle_t
  >::type* const
  get_dep_handle(
    AccessHandleType& ah
  ) {
    return ah.dep_handle_.get();
  }

  template <typename AccessHandleType>
  static inline
  typename tinympl::copy_cv_qualifiers<AccessHandleType>::template apply<
    typename AccessHandleType::dep_handle_ptr
  >::type const
  get_dep_handle_ptr(
    AccessHandleType& ah
  ) {
    return ah.dep_handle_;
  }

  template <typename AccessHandleType>
  static inline
  typename tinympl::copy_volatileness<AccessHandleType>::template apply<
    typename AccessHandleType::version_t
  >::type const&
  get_version(
    AccessHandleType& ah
  ) {
    return ah.dep_handle_->get_version();
  }
};

} // end namespace create_work_attorneys

} // end namespace detail

template <
  typename T=void,
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
  typename U=void,
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
  typename U=void,
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



} // end namespace darma



#endif /* NEW_DEPENDENCY_H_ */
