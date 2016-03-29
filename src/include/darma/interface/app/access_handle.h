/*
//@HEADER
// ************************************************************************
//
//                          access_handle.h
//                         dharma_new
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

#ifndef SRC_INCLUDE_DARMA_INTERFACE_APP_ACCESS_HANDLE_H_
#define SRC_INCLUDE_DARMA_INTERFACE_APP_ACCESS_HANDLE_H_

#ifdef DARMA_TEST_FRONTEND_VALIDATION
#  include <gtest/gtest_prod.h>
#endif

#include <darma/impl/handle.h>
#include <darma/impl/task.h>
#include <darma/impl/keyword_arguments/check_allowed_kwargs.h>
#include <darma/impl/util.h>

namespace darma_runtime {

template <
  typename T,
  typename key_type = types::key_t,
  typename version_type = types::version_t,
  template <typename...> class smart_ptr_template = types::shared_ptr_template
  //typename key_type,
  //typename version_type,
  //template <typename...> class smart_ptr_template
>
class AccessHandle : public detail::AccessHandleBase
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
    AccessHandle()
      : prev_copied_from(nullptr),
        dep_handle_(nullptr),
        read_only_holder_(nullptr),
        state_(None_None)
    { }

    AccessHandle&
    operator=(AccessHandle& other) = delete;

    const AccessHandle&
    operator=(AccessHandle& other) const = delete;

    AccessHandle&
    operator=(AccessHandle const& other) = delete;

    const AccessHandle&
    operator=(AccessHandle const& other) const = delete;

    AccessHandle const&
    operator=(AccessHandle&& other) noexcept {
      // Forward to const move assignment operator
      return const_cast<AccessHandle const*>(this)->operator=(
        std::move(other)
      );
      return *this;
    }

    AccessHandle const&
    operator=(AccessHandle&& other) const noexcept {
      assert(other.prev_copied_from == nullptr);
      read_only_holder_ = std::move(other.read_only_holder_);
      dep_handle_ = std::move(other.dep_handle_);
      state_ = std::move(other.state_);
      return *this;
    }

    AccessHandle const&
    operator=(std::nullptr_t) const {
      release();
      return *this;
    }

    AccessHandle(AccessHandle const& copied_from) noexcept
      : dep_handle_(copied_from.dep_handle_),
        // this copy constructor may be invoked in ordinary usage or
        // may be the actual capture itself.  In the latter case, the subsequent
        // move needs access back to the outer context object, so we need to
        // save a pointer back to other.  It should be ignored otherwise, though.
        // note that the below const_cast is needed to convert from AccessHandle const* to AccessHandle* const
        prev_copied_from(const_cast<AccessHandle* const>(&copied_from)),
        read_only_holder_(std::move(copied_from.read_only_holder_)),
        state_(copied_from.state_)
    {
      // get the shared_ptr from the weak_ptr stored in the runtime object
      detail::TaskBase* running_task = detail::safe_static_cast<detail::TaskBase* const>(
        detail::backend_runtime->get_running_task()
      );
      capturing_task = running_task->current_create_work_context;

      // Now check if we're in a capturing context:
      if(capturing_task != nullptr) {
        capturing_task->do_capture<AccessHandle>(copied_from, *this,
          const_cast<AccessHandle&>(copied_from)
        );
      } // end if capturing_task != nullptr
    }

    T* operator->() const {
      DARMA_ASSERT_MESSAGE(
        state_ != None_None,
        "handle dereferenced after release"
      );
      DARMA_ASSERT_MESSAGE(
        state_ == Read_Read || state_ == Modify_Read || state_ == Modify_Modify,
        "handle dereferenced in state without immediate access to data, with key: {" << dep_handle_->get_key() << "}"
      );
      return &dep_handle_->get_value();
    }

    T& operator*() const {
      DARMA_ASSERT_MESSAGE(
        state_ != None_None,
        "handle dereferenced after release"
      );
      DARMA_ASSERT_MESSAGE(
        state_ == Read_Read || state_ == Modify_Read || state_ == Modify_Modify,
        "handle dereferenced in state without immediate access to data, with key: {" << dep_handle_->get_key() << "}"
      );
      return dep_handle_->get_value();
    }

    void
    release() const {
      // TODO warning for multiple release (or should this be an error?)
      DARMA_ASSERT_MESSAGE(
        state_ == Modify_Modify || state_ == Modify_Read || state_ == Modify_None,
        "release() called on handle without Modify-schedule privileges"
      );
      read_only_holder_ = nullptr;
      dep_handle_ = nullptr;
      state_ = None_None;
    }

    template <
      typename U,
      typename = std::enable_if<std::is_convertible<U, T>::value>
    >
    void
    set_value(U&& val) const {
      DARMA_ASSERT_MESSAGE(
        state_ != None_None,
        "set_value() called on handle after release"
      );
      DARMA_ASSERT_MESSAGE(
        state_ == Modify_Modify,
        "set_value() called on handle not in immediately modifiable state, with key: {" << dep_handle_->get_key() << "}"
      );
      dep_handle_->set_value(val);
    }

    template <typename... Args>
    std::enable_if_t<
      not std::is_same<T, void>::value
    >
    emplace_value(Args&&... args) const {
      DARMA_ASSERT_MESSAGE(
        state_ != None_None,
        "emplace_value() called on handle after release"
      );
      DARMA_ASSERT_MESSAGE(
        state_ == Modify_Modify,
        "emplace_value() called on handle not in immediately modifiable state, with key: {" << dep_handle_->get_key() << "}"
      );
      dep_handle_->emplace_value(std::forward<Args>(args)...);
    }

    template <
      typename = std::enable_if<
        not std::is_same<T, void>::value
      >
    >
    const T&
    get_value() const {
      DARMA_ASSERT_MESSAGE(
        state_ != None_None,
        "get_value() called on handle after release"
      );
      DARMA_ASSERT_MESSAGE(
        state_ == Read_Read || state_ == Modify_Read || state_ == Modify_Modify,
        "get_value() called on handle not in immediately readable state, with key: {" << dep_handle_->get_key() << "}"
      );
      return dep_handle_->get_value();
    }

    const key_t&
    get_key() const {
      DARMA_ASSERT_MESSAGE(
        state_ != None_None,
        "get_key() called on handle after release"
      );
      return dep_handle_->get_key();
    }

    template <
      typename = std::enable_if<
        not std::is_same<T, void>::value
      >
    >
    T&
    get_reference() const {
      DARMA_ASSERT_MESSAGE(
        state_ != None_None,
        "get_reference() called on handle after release"
      );
      DARMA_ASSERT_MESSAGE(
        state_ == Modify_Modify,
        "get_reference() called on handle not in immediately modifiable state, with key: {" << dep_handle_->get_key() << "}"
      );
      return dep_handle_->get_value();
    }

    template <
      typename... PublishExprParts
    >
    void publish(
      PublishExprParts&&... parts
    ) const {
      static_assert(detail::only_allowed_kwargs_given<
          keyword_tags_for_publication::version,
          keyword_tags_for_publication::n_readers
        >::template apply<PublishExprParts...>::type::value,
        "Unknown keyword argument given to AccessHandle<>::publish()"
      );
      detail::publish_expr_helper<PublishExprParts...> helper;
      switch(state_) {
        case None_None: {
          DARMA_ASSERT_MESSAGE(false, "publish() called on handle after release");
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
          dep_handle_->has_subsequent_at_version_depth = true;
          dep_handle_ = dep_handle_ptr_maker_t()(dep_handle_->get_key(), next_version);
          detail::backend_runtime->publish_handle(
            dep_handle_.get(),
            helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
            helper.get_n_readers(std::forward<PublishExprParts>(parts)...)
          );
          read_only_holder_ = read_only_usage_holder_ptr_maker_t()(dep_handle_);
          // Continuing state is MR
          state_ = Modify_Read;
          break;
        }
      };
    }

    ~AccessHandle() { }

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
    {
      // Release read_only_holder_ immediately if this is initial access
      if(state_ == Modify_None and version == version_type()) {
        read_only_holder_.reset();
      }
    }

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
      dep_handle_->has_subsequent_at_version_depth = true;
    }

    ////////////////////////////////////////
    // private members

    mutable dep_handle_ptr dep_handle_ = { nullptr };
    mutable state_t state_ = None_None;

    mutable types::shared_ptr_template<read_only_usage_holder> read_only_holder_;
    task_t* capturing_task = nullptr;
    AccessHandle* prev_copied_from = nullptr;

   public:

    ////////////////////////////////////////
    // Attorneys for create_work and *_access functions
    friend class detail::create_work_attorneys::for_AccessHandle;
    friend class detail::access_attorneys::for_AccessHandle;

    ////////////////////////////////////////
    // TaskBase is also a friend
    friend class detail::TaskBase;


#ifdef DARMA_TEST_FRONTEND_VALIDATION
    friend class ::TestAccessHandle;
    FRIEND_TEST(::TestAccessHandle, set_value);
    FRIEND_TEST(::TestAccessHandle, get_reference);
    FRIEND_TEST(::TestAccessHandle, publish_MN);
    FRIEND_TEST(::TestAccessHandle, publish_MM);
#endif

};

} // end namespace darma_runtime

#endif /* SRC_INCLUDE_DARMA_INTERFACE_APP_ACCESS_HANDLE_H_ */
