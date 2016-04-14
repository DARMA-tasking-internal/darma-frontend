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

#include <darma/impl/meta/splat_tuple.h>

#include <darma/impl/task.h>
#include <darma/impl/keyword_arguments/check_allowed_kwargs.h>
#include <darma/impl/util.h>

namespace darma_runtime {

template <
  typename T,
  typename key_type,
  typename version_type,
  typename traits
>
class AccessHandle : public detail::AccessHandleBase
{
  public:

    typedef T value_type;

  protected:


    typedef detail::DependencyHandle<T, key_type, version_type> dep_handle_t;
    typedef types::shared_ptr_template<dep_handle_t> dep_handle_ptr;
    typedef types::shared_ptr_template<const dep_handle_t> dep_handle_const_ptr;
    typedef detail::TaskBase task_t;
    typedef types::shared_ptr_template<task_t> task_ptr;
    typedef typename detail::smart_ptr_traits<std::shared_ptr>::template maker<dep_handle_t>
      dep_handle_ptr_maker_t;

    typedef typename dep_handle_t::key_t key_t;
    typedef typename dep_handle_t::version_t version_t;

  public:

    static constexpr auto is_compile_time_modifiable = traits::is_compile_time_modifiable;
    static constexpr auto is_compile_time_readable = traits::is_compile_time_readable;

    using CompileTimeReadAccessAnalog = AccessHandle<T, key_t, version_t,
      typename traits::template with_compile_time_modifiable<false>::type
    >;
    using CompileTimeModifiableAnalog = AccessHandle<T, key_t, version_t,
      typename traits::template with_compile_time_modifiable<true>::type
    >;


  private:

  public:
    AccessHandle()
      : dep_handle_(nullptr),
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
        std::forward<AccessHandle>(other)
      );
      return *this;
    }

    AccessHandle const&
    operator=(AccessHandle&& other) const noexcept {
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

    /**
      Add an allreduce contribution from this data block into another data block
      @param output     The handle for the data that will hold the result of the
                        allreduce
      @param nChunks    The number of chunks that will contribute to the all-reduce
    */
    void
    allreduce(AccessHandle const& output, int nChunks){
      //TODO - this should make a lambda task for the allreduce to properly capture permissions
    }
    
    /**
      Add an allreduce contribution from this data block. Do the allreduce in place.
      @param nChunks    The number of chunks that will contribute to the all-reduce
    */
    void
    allreduce(int nChunks){
      //TODO - this should make a lambda task for the allreduce to properly capture permissions
    }

    AccessHandle(AccessHandle const& copied_from) noexcept {
      // get the shared_ptr from the weak_ptr stored in the runtime object
      detail::TaskBase* running_task = detail::safe_static_cast<detail::TaskBase* const>(
        detail::backend_runtime->get_running_task()
      );
      capturing_task = running_task->current_create_work_context;

      // Now check if we're in a capturing context:
      if(capturing_task != nullptr) {
        capturing_task->do_capture<AccessHandle>(*this, copied_from);
      } // end if capturing_task != nullptr
      else {
        // regular copy
        dep_handle_ = copied_from.dep_handle_;
        read_only_holder_ = copied_from.read_only_holder_;
        state_ = copied_from.state_;
      }
    }

    template <
      typename _Ignored = void,
      typename = std::enable_if_t<not is_compile_time_modifiable and std::is_same<_Ignored, void>::value>
    >
    AccessHandle(
      CompileTimeModifiableAnalog const& copied_from
    ) noexcept {
      // get the shared_ptr from the weak_ptr stored in the runtime object
      detail::TaskBase* running_task = detail::safe_static_cast<detail::TaskBase* const>(
        detail::backend_runtime->get_running_task()
      );
      capturing_task = running_task->current_create_work_context;

      // Now check if we're in a capturing context:
      if(capturing_task != nullptr) {
        copied_from.captured_as_ |= CapturedAsInfo::ReadOnly;
        capturing_task->do_capture<AccessHandle>(*this, copied_from);
      } // end if capturing_task != nullptr
      else {
        // regular copy
        dep_handle_ = copied_from.dep_handle_;
        read_only_holder_ = copied_from.read_only_holder_;
        state_ = copied_from.state_;
      }
    }

    AccessHandle(AccessHandle&&) noexcept = default;

    // These two allow trasparent casting away of compile-time modifiability

    //template <
    //  typename _Ignored = void,
    //  typename = std::enable_if_t<is_compile_time_modifiable and std::is_same<_Ignored, void>::value>
    //>
    //operator AccessHandle<T, key_t, version_t,
    //  typename traits::template with_compile_time_modifiable<false>::type
    //>() {
    //  return *(reinterpret_cast<
    //      AccessHandle<T, key_t, version_t, typename traits::template with_compile_time_modifiable<false>::type>*
    //    >(this)
    //  );
    //};

    //template <
    //  typename _Ignored = void,
    //  typename = std::enable_if_t<is_compile_time_modifiable and std::is_same<_Ignored, void>::value>
    //>
    //operator const AccessHandle<T, key_t, version_t,
    //  typename traits::template with_compile_time_modifiable<false>::type
    //>() const
    //{
    //  return *(reinterpret_cast<
    //      AccessHandle<T, key_t, version_t, typename traits::template with_compile_time_modifiable<false>::type>*
    //    >(this)
    //  );
    //};

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

    template <
      typename _Ignored = void
    >
    std::enable_if_t<
      is_compile_time_modifiable
      and std::is_same<_Ignored, void>::value
    >
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
      typename = std::enable_if_t<
        std::is_convertible<U, T>::value
        and is_compile_time_modifiable
      >
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

    template <typename _Ignored = void, typename... Args>
    std::enable_if_t<
      not std::is_same<T, void>::value
        and is_compile_time_modifiable
        and std::is_same<_Ignored, void>::value
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
        and is_compile_time_readable
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
        and is_compile_time_modifiable
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
      typename _Ignored = void,
      typename... PublishExprParts
    >
    std::enable_if_t<
      is_compile_time_readable
      and std::is_same<_Ignored, void>::value
    >
    publish(
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
          read_only_holder_ = nullptr;
          dep_handle_ = dep_handle_ptr_maker_t()(dep_handle_->get_key(), next_version);
          read_only_holder_ = read_only_usage_holder_ptr_maker_t()(dep_handle_.get());
          // Continuing state is MR
          state_ = Modify_Read;
          // Now publish this.  The backend should have satisfied it at this point
          detail::backend_runtime->publish_handle(
            dep_handle_.get(),
            helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
            helper.get_n_readers(std::forward<PublishExprParts>(parts)...)
          );
          break;
        }
      };
    }

    ~AccessHandle() { }

   private:

    ////////////////////////////////////////
    // private conversion operators

    template <
      typename = std::enable_if<
        not std::is_same<T, void>::value
          and is_compile_time_readable
      >
    >
    operator T const&() const {
      return get_value();
    }

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
        read_only_holder_(read_only_usage_holder_ptr_maker_t()(dep_handle_.get()))
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
        read_only_holder_(read_only_usage_holder_ptr_maker_t()(dep_handle_.get()))
    {
      assert(state_ == Read_None);
      dep_handle_->has_subsequent_at_version_depth = true;
    }

    ////////////////////////////////////////
    // private members

    mutable dep_handle_ptr dep_handle_ = { nullptr };
    mutable state_t state_ = None_None;
    mutable types::shared_ptr_template<read_only_usage_holder> read_only_holder_;
    mutable unsigned captured_as_ = CapturedAsInfo::Normal;

    task_t* capturing_task = nullptr;

   public:

    ////////////////////////////////////////
    // Attorneys for create_work and *_access functions
    friend class detail::create_work_attorneys::for_AccessHandle;
    friend class detail::access_attorneys::for_AccessHandle;

    ////////////////////////////////////////
    // TaskBase is also a friend
    friend class detail::TaskBase;

    ////////////////////////////////////////
    // Analogs with different privileges are friends too
    friend CompileTimeReadAccessAnalog;
    friend CompileTimeModifiableAnalog;

    // Allow implicit conversion to value in the invocation of the task
    friend struct meta::splat_tuple_access<detail::AccessHandleBase>;

#ifdef DARMA_TEST_FRONTEND_VALIDATION
    friend class ::TestAccessHandle;
    FRIEND_TEST(::TestAccessHandle, set_value);
    FRIEND_TEST(::TestAccessHandle, get_reference);
    FRIEND_TEST(::TestAccessHandle, publish_MN);
    FRIEND_TEST(::TestAccessHandle, publish_MM);
#endif

};

template <typename T,
  typename key_t = types::key_t,
  typename version_t = types::version_t
>
using ReadAccessHandle = AccessHandle<T, key_t, version_t, detail::access_handle_traits<false, true>>;

} // end namespace darma_runtime

#endif /* SRC_INCLUDE_DARMA_INTERFACE_APP_ACCESS_HANDLE_H_ */
