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
#include <darma/impl/use.h>
#include <darma/impl/publication_details.h>

namespace darma_runtime {


// todo move this to a more appropriate place
template <typename T>
T darma_copy(T& lvalue) {
  return lvalue;
}

template <typename T, typename Traits>
class AccessHandle : public detail::AccessHandleBase {
  public:

    typedef T value_type;

  protected:

    using variable_handle_t = detail::VariableHandle<T>;
    using variable_handle_ptr = types::shared_ptr_template<detail::VariableHandle<T>>;
    using handle_use_ptr = types::unique_ptr_template<detail::HandleUse>;

    using task_t = detail::TaskBase;
    using key_t = types::key_t;

  public:

    using traits = Traits;

    template <typename NewTraits>
    using with_traits = AccessHandle<T, NewTraits>;

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
    static constexpr bool is_compile_time_immediate_read_only =
      (is_compile_time_immediate_readable and not is_compile_time_immediate_modifiable);

    static constexpr bool is_leaf = (
      traits::max_schedule_permissions == detail::AccessHandlePermissions::None
    );

    // Assert that max_schedule_permissions, if given, is >= min_schedule_permissions, if also given
    static_assert(
      not traits::max_schedule_permissions_given or not traits::min_schedule_permissions_given
        or (int) traits::max_schedule_permissions >= (int) traits::min_schedule_permissions,
      "Tried to create handle with max_schedule_permissions < min_schedule_permissions"
    );
    // Assert that max_immediate_permissions, if given, is >= min_immediate_permissions, if also given
    static_assert(
      not traits::max_immediate_permissions_given or not traits::min_immediate_permissions_given
        or (int) traits::max_immediate_permissions >= (int) traits::min_immediate_permissions,
      "Tried to create handle with max_immediate_permissions < min_immediate_permissions"
    );


  private:

  public:
    AccessHandle() = default;

    AccessHandle &
      operator=(AccessHandle &other) = delete;

    const AccessHandle &
      operator=(AccessHandle &other) const = delete;

    AccessHandle &
      operator=(AccessHandle const &other) = delete;

    const AccessHandle &
      operator=(AccessHandle const &other) const = delete;

    AccessHandle const &
    operator=(AccessHandle &&other) noexcept {
      // Forward to const move assignment operator
      return const_cast<AccessHandle const *>(this)->operator=(
        std::forward<AccessHandle>(other)
      );
    }

    AccessHandle const &
    operator=(AccessHandle &&other) const noexcept {
      std::swap(var_handle_, other.var_handle_);
      std::swap(current_use_, other.current_use_);
      return *this;
    }

    AccessHandle const &
    operator=(std::nullptr_t) const {
      release();
      return *this;
    }

    AccessHandle(AccessHandle const &copied_from) noexcept {
      // get the shared_ptr from the weak_ptr stored in the runtime object
      detail::TaskBase *running_task = detail::safe_static_cast<detail::TaskBase *const>(
        detail::backend_runtime->get_running_task()
      );
      capturing_task = running_task->current_create_work_context;

      // Now check if we're in a capturing context:
      if (capturing_task != nullptr) {
        capturing_task->do_capture<AccessHandle>(*this, copied_from);
      } // end if capturing_task != nullptr
      else {
        // regular copy
        // TODO figure out how this works with respect to who is responsible for destruction
        assert(false);  // Should never get here???!?!?!
        //var_handle_ = copied_from.var_handle_;
        //current_use_ = copied_from.current_use_;
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
      AccessHandleT const &copied_from
    ) noexcept {
      using detail::analogous_access_handle_attorneys::AccessHandleAccess;
      // get the shared_ptr from the weak_ptr stored in the runtime object
      detail::TaskBase *running_task = detail::safe_static_cast<detail::TaskBase *const>(
        detail::backend_runtime->get_running_task()
      );
      capturing_task = running_task->current_create_work_context;

      // Now check if we're in a capturing context:
      if (capturing_task != nullptr) {
        if (
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
        assert(false);  // Should never get here???!?!?!
        //var_handle_ = copied_from.var_handle_;
        //current_use_ = copied_from.current_use_;
      }
    }

    // end analogous type conversion constructor
    ////////////////////////////////////////

    AccessHandle(AccessHandle &&) noexcept = default;

    template <typename _Ignored=void,
      typename = std::enable_if_t<
        is_compile_time_immediate_readable
          and std::is_same<_Ignored, void>::value
      >
    >
    std::conditional_t<
      is_compile_time_immediate_read_only,
      T const *,
      T*
    >
    operator->() const {
      DARMA_ASSERT_MESSAGE(
        current_use_->immediate_permissions_ != abstract::frontend::Use::Permissions::None,
        "handle dereferenced in state without immediate access to data, with key: {" << get_key() << "}"
      );
      using return_t_decay = std::conditional_t<
        is_compile_time_immediate_read_only,
        T const,
        T
      >;
      return static_cast<return_t_decay*>(current_use_->data_);
    }

    template <typename _Ignored=void,
      typename = std::enable_if_t<
        is_compile_time_immediate_readable
          and std::is_same<_Ignored, void>::value
      >
    >
    std::conditional_t<
      is_compile_time_immediate_read_only,
      T const&,
      T&
    >
    operator*() const {
      DARMA_ASSERT_MESSAGE(
        current_use_->immediate_permissions_ != abstract::frontend::Use::Permissions::None,
        "handle dereferenced in state without immediate access to data, with key: {" << get_key() << "}"
      );
      using return_t_decay = std::conditional_t<
        is_compile_time_immediate_read_only,
        T const,
        T
      >;
      return *static_cast<return_t_decay*>(current_use_->data_);
    }

    template <
      typename _Ignored = void,
      typename = std::enable_if_t<
        // Technically, if this is compile-time nothing, you can't even do this, so we can disable it
        not (traits::max_immediate_permissions == detail::AccessHandlePermissions::None
          and traits::max_schedule_permissions == detail::AccessHandlePermissions::None)
          and std::is_same<_Ignored, void>::value
      >
    >
    void
    release() const {
      DARMA_ASSERT_MESSAGE(
        _can_be_released(),
        "release() called on handle not in releasable state (most likely, uninitialized or already released)"
      );
      detail::backend_runtime->release_use(current_use_.get());
      current_use_.reset();
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
        current_use_->immediate_permissions_ == abstract::frontend::Use::Permissions::Modify,
        "set_value() called on handle not in immediately modifiable state, with key: {" << get_key() << "}"
      );
      emplace_value(std::forward<U>(val));
    }

    template <typename _Ignored = void, typename... Args>
    std::enable_if_t<
      not std::is_same<T, void>::value
        and is_compile_time_immediate_modifiable
        and std::is_same<_Ignored, void>::value
    >
    emplace_value(Args&&... args) const {
      DARMA_ASSERT_MESSAGE(
        current_use_->immediate_permissions_ == abstract::frontend::Use::Permissions::Modify,
        "emplace_value() called on handle not in immediately modifiable state, with key: {" << get_key() << "}"
      );
      // TODO do this more uniformly and/or expose it to the frontend somehow
      using alloc_t = std::allocator<T>;
      using allocator_traits_t = std::allocator_traits<alloc_t>;
      alloc_t alloc;
      if (value_constructed_) {
        allocator_traits_t::destroy(alloc, current_use_->data_);
      }
      allocator_traits_t::construct(alloc, current_use_->data_, std::forward<Args>(args)...);
      value_constructed_ = true;
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
        current_use_->immediate_permissions_ != abstract::frontend::Use::Permissions::None,
        "get_value() called on handle not in immediately readable state, with key: {" << get_key() << "}"
      );
      return *static_cast<T const *>(current_use_->data_);
    }

    const key_t&
    get_key() const {
      return var_handle_.get_key();
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
        current_use_->immediate_permissions_ == abstract::frontend::Use::Permissions::Modify,
        "get_reference() called on handle not in immediately modifiable state, with key: {" << get_key() << "}"
      );
      return *static_cast<T*>(current_use_->data_);
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
      using detail::HandleUse;
      DARMA_ASSERT_MESSAGE(
        current_use_->scheduling_permissions_ != HandleUse::None,
        "publish() called on handle that can't schedule at least read usage on data (most likely "
          "because it was already released"
      );
      static_assert(detail::only_allowed_kwargs_given<
          keyword_tags_for_publication::version,
          keyword_tags_for_publication::n_readers
        >::template apply<PublishExprParts...>::type::value,
        "Unknown keyword argument given to AccessHandle<>::publish()"
      );
      detail::publish_expr_helper<PublishExprParts...> helper;


      auto _pub_same = [&] {
        auto flow_to_publish = detail::backend_runtime->make_same_flow(
          current_use_->in_flow_,
          abstract::backend::Runtime::FlowPropagationPurpose::Input
        );
        detail::HandleUse use_to_publish(
          var_handle_.get(),
          flow_to_publish,
          detail::backend_runtime->make_same_flow(
            flow_to_publish, abstract::backend::Runtime::OutputFlowOfReadOperation
          ),
          detail::HandleUse::None, detail::HandleUse::Read
        );

        detail::backend_runtime->register_use(&use_to_publish);
        detail::PublicationDetails dets(
          helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
          helper.get_n_readers(std::forward<PublishExprParts>(parts)...)
        );
        detail::backend_runtime->publish_use(&use_to_publish, &dets);
        detail::backend_runtime->release_use(&use_to_publish);
      };

      auto _pub_from_modify = [&] {
        auto flow_to_publish = detail::backend_runtime->make_forwarding_flow(
          current_use_->in_flow_,
          abstract::backend::Runtime::FlowPropagationPurpose::ForwardingChanges
        );
        auto next_out = detail::backend_runtime->make_same_flow(
          current_use_->out_flow_,
          abstract::backend::Runtime::FlowPropagationPurpose::Output
        );
        auto next_in = detail::backend_runtime->make_same_flow(
          flow_to_publish,
          abstract::backend::Runtime::FlowPropagationPurpose::Input
        );

        detail::HandleUse use_to_publish(
          var_handle_.get(),
          flow_to_publish,
          detail::backend_runtime->make_same_flow(
            flow_to_publish, abstract::backend::Runtime::OutputFlowOfReadOperation
          ),
          detail::HandleUse::None, detail::HandleUse::Read
        );

        auto new_use = detail::make_unique<detail::HandleUse>(
          var_handle_.get(), next_in, next_out,
          current_use_->scheduling_permissions_,
          // Downgrade to read
          HandleUse::Read
        );

        detail::backend_runtime->register_use(&use_to_publish);
        detail::PublicationDetails dets(
          helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
          helper.get_n_readers(std::forward<PublishExprParts>(parts)...)
        );
        detail::backend_runtime->publish_use(&use_to_publish, &dets);

        _switch_to_new_use(std::move(new_use));
        detail::backend_runtime->release_use(&use_to_publish);
      };

      switch(current_use_->scheduling_permissions_) {
        case HandleUse::None: {
          // Error message above
          break;
        }

        case HandleUse::Read: {
          switch(current_use_->immediate_permissions_) {
            case HandleUse::None:
            case HandleUse::Read: {
              // Make a new flow for the publication
              _pub_same();
              break;
            }
            case HandleUse::Modify: {
              // Don't know when anyone would have HandleUse::Read_Modify, but we still know what to do...
              _pub_from_modify();
              break;
            }
          }
          break;
        }
        case HandleUse::Modify: {
          switch (current_use_->immediate_permissions_) {
            case HandleUse::None:
            case HandleUse::Read: {
              _pub_same();
              break;
            }
            case HandleUse::Modify: {
              _pub_from_modify();
              break;
            }
          }
          break;
        }
      }
    }

    ~AccessHandle() {
      if(_can_be_released()) {
        detail::backend_runtime->release_use(current_use_.get());
      }
    }

   private:

    void _switch_to_new_use(handle_use_ptr&& new_use) const {
      detail::backend_runtime->register_use(new_use.get());
      std::swap(current_use_, new_use);
      detail::backend_runtime->release_use(/* object swapped into */ new_use.get());
    }

    bool _can_be_released() const {
      // There are more conditions that could be checked, (e.g., if the state
      // is None, None), but we'll just use this one for now
      return (bool)current_use_ && current_use_->handle_ != nullptr;
    }

    ////////////////////////////////////////
    // private constructors

    AccessHandle(
      variable_handle_ptr var_handle,
      abstract::backend::Flow* in_flow,
      abstract::backend::Flow* out_flow,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions
    ) : var_handle_(var_handle),
        current_use_(detail::make_unique<detail::HandleUse>(
          var_handle_.get(),
          in_flow, out_flow,
          scheduling_permissions, immediate_permissions
        ))
    {
      detail::backend_runtime->register_use(current_use_.get());
    }

    ////////////////////////////////////////
    // private members

    mutable unsigned captured_as_ = CapturedAsInfo::Normal;
    mutable variable_handle_ptr var_handle_;
    mutable handle_use_ptr current_use_;
    // TODO switch to everything has to be constructed requirement
    mutable bool value_constructed_ = false;

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

template <typename T>
using ReadAccessHandle = AccessHandle<
  T, typename detail::make_access_handle_traits<
    detail::max_schedule_permissions<detail::AccessHandlePermissions::Read>,
    detail::max_immediate_permissions<detail::AccessHandlePermissions::Read>
  >::type
>;

//namespace serialization {

//template <typename... Args>
//struct Serializer<AccessHandle<Args...>> {
//  private:
//    using AccessHandleT = AccessHandle<Args...>;

//  public:
//    template <typename ArchiveT>
//    void compute_size(AccessHandleT const& val, ArchiveT& ar) const {
//      auto const& dep_handle = val.dep_handle_;
//      ar % dep_handle.get_key();
//      ar % dep_handle.get_version();
//      ar % dep_handle.version_is_pending();
//      ar % val.state_;
//      // Omit captured_as_; it should always be normal here
//      assert(val.captured_as_ == AccessHandleT::CapturedAsInfo::Normal);
//      // for whether or not the read-only holder is active
//      ar % bool();
//      // capturing_task will be replaced by task serialization, so we don't need to pack it here
//    }

//    template <typename ArchiveT>
//    void pack(AccessHandleT const& val, ArchiveT& ar) const {
//      auto const& dep_handle = val.dep_handle_;
//      ar << dep_handle.get_key();
//      ar << dep_handle.get_version();
//      ar << dep_handle.version_is_pending();
//      ar << val.state_;
//      // Omit captured_as_; it should always be normal here
//      assert(val.captured_as_ == AccessHandleT::CapturedAsInfo::Normal);
//      // only need to store whether or not the read-only holder is active
//      ar << bool(val.read_only_holder_);
//      // capturing_task will be replaced by task serialization, so we don't need to pack it here
//    }

//    template <typename ArchiveT>
//    void unpack(void* allocated, ArchiveT& ar) const {
//      // Call an unpacking constructor
//      new (allocated) AccessHandleT(serialization::unpack_constructor_tag, ar);
//    }
//};

//} // end namespace serialization

} // end namespace darma_runtime

#endif /* SRC_INCLUDE_DARMA_INTERFACE_APP_ACCESS_HANDLE_H_ */
