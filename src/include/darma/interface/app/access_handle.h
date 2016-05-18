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
#include <darma/impl/serialization/traits.h>

namespace darma_runtime {

template <
  typename T,
  typename key_type,
  typename version_type,
  typename Traits
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

    using traits = Traits;

    template <typename NewTraits>
    using with_traits = AccessHandle<T, key_t, version_t, NewTraits>;

    using CompileTimeReadAccessAnalog = with_traits<
      typename traits
        ::template with_max_schedule_permissions<detail::AccessHandlePermissions::Read>::type
        ::template with_max_immediate_permissions<detail::AccessHandlePermissions::Read>::type
    >;


    static constexpr bool is_compile_time_immediate_modifiable =
      (traits::max_immediate_permissions == detail::AccessHandlePermissions::Modify
        or not traits::max_immediate_permissions_given);
    static constexpr bool is_compile_time_schedule_modifiable =
      (traits::max_schedule_permissions == detail::AccessHandlePermissions::Modify
        or not traits::max_schedule_permissions_given);
    static constexpr bool is_compile_time_immediate_readable =
      (traits::max_immediate_permissions == detail::AccessHandlePermissions::Read
        or traits::max_immediate_permissions == detail::AccessHandlePermissions::Modify
        or not traits::max_immediate_permissions_given);
    static constexpr bool is_compile_time_schedule_readable =
      (traits::max_schedule_permissions == detail::AccessHandlePermissions::Read
        or traits::max_schedule_permissions == detail::AccessHandlePermissions::Modify
        or not traits::max_schedule_permissions_given);

    static constexpr bool is_leaf = (
      traits::max_schedule_permissions == detail::AccessHandlePermissions::None
    );

    // Assert that max_schedule_permissions, if given, is >= min_schedule_permissions, if also given
    static_assert(
      not traits::max_schedule_permissions_given or not traits::min_schedule_permissions_given
      or (int)traits::max_schedule_permissions >= (int)traits::min_schedule_permissions,
      "Tried to create handle with max_schedule_permissions < min_schedule_permissions"
    );
    // Assert that max_immediate_permissions, if given, is >= min_immediate_permissions, if also given
    static_assert(
      not traits::max_immediate_permissions_given or not traits::min_immediate_permissions_given
        or (int)traits::max_immediate_permissions >= (int)traits::min_immediate_permissions,
      "Tried to create handle with max_immediate_permissions < min_immediate_permissions"
    );


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

    ////////////////////////////////////////
    // Analogous type conversion constructor

    template <
      typename AccessHandleT,
      typename = std::enable_if_t<
        // Check if it's an AccessHandle type that's not this type
        detail::is_access_handle<AccessHandleT>::value
          and not std::is_same<AccessHandleT, AccessHandle>::value
        // Check if the conversion is allowed based on min permissions and max permissions
        and (
          not traits::max_immediate_permissions_given
            or not AccessHandleT::traits::min_immediate_permissions_given
            or traits::max_immediate_permissions >= AccessHandleT::traits::min_immediate_permissions
        )
        // same thing for schedule case
        and (
          not traits::max_schedule_permissions_given
            or not AccessHandleT::traits::min_schedule_permissions_given
            or traits::max_schedule_permissions >= AccessHandleT::traits::min_schedule_permissions
        )
      >
    >
    AccessHandle(
      AccessHandleT const& copied_from
    ) noexcept {
      using detail::analogous_access_handle_attorneys::AccessHandleAccess;
      // get the shared_ptr from the weak_ptr stored in the runtime object
      detail::TaskBase* running_task = detail::safe_static_cast<detail::TaskBase* const>(
        detail::backend_runtime->get_running_task()
      );
      capturing_task = running_task->current_create_work_context;

      // Now check if we're in a capturing context:
      if(capturing_task != nullptr) {
        if(
          // If this type is a compile-time read-only handle, mark it as such here
          traits::max_immediate_permissions == detail::AccessHandlePermissions::Read
          // If the other type is compile-time read-only and we don't know, mark it as a read
          or (AccessHandleT::traits::max_immediate_permissions == detail::AccessHandlePermissions::Read
            and not traits::max_immediate_permissions_given
          )
        ) {
          AccessHandleAccess::captured_as(copied_from) |= CapturedAsInfo::ReadOnly;
        }
        // TODO mark leaf, schedule-only, etc
        // TODO require dynamic modify from RHS if this class is static modify
        capturing_task->do_capture<AccessHandle>(*this, copied_from);
      } // end if capturing_task != nullptr
      else {
        // regular copy
        dep_handle_ = AccessHandleAccess::dep_handle(copied_from);
        read_only_holder_ = AccessHandleAccess::read_only_holder(copied_from);
        state_ = AccessHandleAccess::state(copied_from);
      }
    }

    // end analogous type conversion constructor
    ////////////////////////////////////////


    AccessHandle(AccessHandle&&) noexcept = default;

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
      std::is_same<_Ignored, void>::value
    >
    release() const {
      DARMA_ASSERT_MESSAGE(
        state_ != None_None,
        "release() called on handle that was already released"
      );
      read_only_holder_ = nullptr;
      dep_handle_ = nullptr;
      state_ = None_None;
    }

    template <
      typename U,
      typename = std::enable_if_t<
        std::is_convertible<U, T>::value
        and is_compile_time_immediate_modifiable
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
        and is_compile_time_immediate_modifiable
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
        and is_compile_time_immediate_readable
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
        and is_compile_time_immediate_modifiable
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
      is_compile_time_schedule_readable
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

    //template <
    //  typename = std::enable_if<
    //    not std::is_same<T, void>::value
    //      and is_compile_time_readable
    //  >
    //>
    //operator T const&() const {
    //  return get_value();
    //}

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
    }


    template <typename ArchiveT>
    AccessHandle(
      serialization::unpack_constructor_tag_t,
      ArchiveT& ar
    )
    {
      dep_handle_ = dep_handle_ptr_maker_t()(detail::handle_migration_unpack, ar);
      ar >> state_;
      bool holds_read_only = false;
      ar >> holds_read_only;
      if(holds_read_only) {
        read_only_holder_ = read_only_usage_holder_ptr_maker_t()(dep_handle_.get());
      }
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
    //friend CompileTimeReadAccessAnalog;
    //friend CompileTimeModifiableAnalog;
    friend struct detail::analogous_access_handle_attorneys::AccessHandleAccess;

    // Allow implicit conversion to value in the invocation of the task
    friend struct meta::splat_tuple_access<detail::AccessHandleBase>;

    friend struct darma_runtime::serialization::Serializer<AccessHandle>;

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
using ReadAccessHandle = AccessHandle<
  T, key_t, version_t,
  typename detail::make_access_handle_traits<
    detail::max_schedule_permissions<detail::AccessHandlePermissions::Read>,
    detail::max_immediate_permissions<detail::AccessHandlePermissions::Read>
  >::type
>;

namespace serialization {

template <typename... Args>
struct Serializer<AccessHandle<Args...>> {
  private:
    using AccessHandleT = AccessHandle<Args...>;
    using dep_handle_ptr = typename AccessHandleT::dep_handle_ptr;
    using dep_handle_ptr_maker_t = typename AccessHandleT::dep_handle_ptr_maker_t;

  public:
    template <typename ArchiveT>
    void compute_size(AccessHandleT const& val, ArchiveT& ar) const {
      auto const& dep_handle = val.dep_handle_;
      ar % dep_handle.get_key();
      ar % dep_handle.get_version();
      ar % dep_handle.version_is_pending();
      ar % val.state_;
      // Omit captured_as_; it should always be normal here
      assert(val.captured_as_ == AccessHandleT::CapturedAsInfo::Normal);
      // for whether or not the read-only holder is active
      ar % bool();
      // capturing_task will be replaced by task serialization, so we don't need to pack it here
    }

    template <typename ArchiveT>
    void pack(AccessHandleT const& val, ArchiveT& ar) const {
      auto const& dep_handle = val.dep_handle_;
      ar << dep_handle.get_key();
      ar << dep_handle.get_version();
      ar << dep_handle.version_is_pending();
      ar << val.state_;
      // Omit captured_as_; it should always be normal here
      assert(val.captured_as_ == AccessHandleT::CapturedAsInfo::Normal);
      // only need to store whether or not the read-only holder is active
      ar << bool(val.read_only_holder_);
      // capturing_task will be replaced by task serialization, so we don't need to pack it here
    }

    template <typename ArchiveT>
    void unpack(void* allocated, ArchiveT& ar) const {
      // Call an unpacking constructor
      new (allocated) AccessHandleT(serialization::unpack_constructor_tag, ar);
    }
};

} // end namespace serialization

} // end namespace darma_runtime

#endif /* SRC_INCLUDE_DARMA_INTERFACE_APP_ACCESS_HANDLE_H_ */
