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
#include <darma/impl/flow_handling.h>
#include <darma/impl/task_collection/task_collection_fwd.h>
#include <darma/impl/keyword_arguments/parse.h>

namespace darma_runtime {

namespace detail {

template <typename AccessHandleT>
class IndexedAccessHandle;

// forward declaration
template <typename Op>
struct all_reduce_impl;

struct unfetched_access_handle_tag { };

template <typename AccessHandleT>
struct _publish_impl;

} // end namespace detail


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
    using use_holder_ptr = types::shared_ptr_template<detail::UseHolder>;

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

    template <typename AccessHandleT>
    using is_convertible_from_access_handle =  std::integral_constant<bool,
      detail::is_access_handle<AccessHandleT>::value
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
    >;

    static constexpr auto is_collection_captured = traits::collection_capture_given;
    static constexpr auto is_collection_captured_as_shared_read =
      traits::collection_captured_as_shared_read;
    static constexpr auto is_collection_captured_as_unique_modify =
      traits::collection_captured_as_unique_modify;

  private:

  public:
    AccessHandle() = default;

    AccessHandle &
    operator=(AccessHandle &other) = default;

    //AccessHandle &
    //operator=(AccessHandle &other) const = default;

    AccessHandle &
    operator=(AccessHandle const &other) = default;

    //AccessHandle &
    //operator=(AccessHandle const &other) const = default;

    AccessHandle const &
    operator=(AccessHandle &&other) noexcept {
      // Forward to const move assignment operator
      return const_cast<AccessHandle const *>(this)->operator=(
        std::forward<AccessHandle>(other)
      );
    }

    AccessHandle const &
    operator=(AccessHandle&& other) const noexcept {
      std::swap(var_handle_, other.var_handle_);
      std::swap(current_use_, other.current_use_);
      value_constructed_ = other.value_constructed_;
      return *this;
    }

    AccessHandle const &
    operator=(std::nullptr_t) const {
      release();
      return *this;
    }

    AccessHandle(AccessHandle const & copied_from) noexcept {
      // get the shared_ptr from the weak_ptr stored in the runtime object
      detail::TaskBase* running_task = static_cast<detail::TaskBase* const>(
        abstract::backend::get_backend_context()->get_running_task()
      );
      capturing_task = running_task->current_create_work_context;
      var_handle_ = copied_from.var_handle_;

      // Now check if we're in a capturing context:
      if (capturing_task != nullptr) {
        //DARMA_ASSERT_MESSAGE(
        //  not copied_from.unfetched_,
        //  "Illegal capture of unfetched non-local AccessHandle"
        //);
        AccessHandle const* source = &copied_from;
        if(capturing_task->is_double_copy_capture) {
          assert(copied_from.prev_copied_from != nullptr);
          source = copied_from.prev_copied_from;
        }

        capturing_task->do_capture<AccessHandle>(*this, *source);

        // Only capture once...
        source->current_use_->captured_but_not_handled = true;
      } // end if capturing_task != nullptr
      else {
        // regular copy
        // TODO figure out how this works with respect to who is responsible for destruction
        current_use_ = copied_from.current_use_;
        // Also, save prev copied from in case this is a double capture, like in
        // create_condition.  This is the only time that the prev_copied_from ptr
        // should be valid (i.e., when task->is_double_copy_capture is set to true)
        prev_copied_from = &copied_from;
      }
    }

    ////////////////////////////////////////
    // Analogous type conversion constructor

    template <
      typename AccessHandleT,
      typename = std::enable_if_t<
        // Check if it's a convertible AccessHandle type that's not this type
        is_convertible_from_access_handle<AccessHandleT>::value
          and not std::is_same<std::decay_t<AccessHandleT>, AccessHandle>::value
      >
    >
    AccessHandle(
      AccessHandleT const &copied_from
    ) noexcept {
      using detail::analogous_access_handle_attorneys::AccessHandleAccess;
      // get the shared_ptr from the weak_ptr stored in the runtime object
      detail::TaskBase* running_task = static_cast<detail::TaskBase* const>(
        abstract::backend::get_backend_context()->get_running_task()
      );
      capturing_task = running_task->current_create_work_context;
      var_handle_ = AccessHandleAccess::var_handle(copied_from);

      // Now check if we're in a capturing context:
      if (capturing_task != nullptr) {
        DARMA_ASSERT_MESSAGE(
          not copied_from.unfetched_,
          "Illegal capture of unfetched non-local AccessHandle"
        );
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
        if (
          // If this type doesn't have scheduling permissions, mark it as a leaf
          traits::max_schedule_permissions == detail::AccessHandlePermissions::None
        ) {
          AccessHandleAccess::captured_as(copied_from) |= CapturedAsInfo::Leaf;
        }
        // TODO schedule-only, etc
        // TODO require dynamic modify from RHS if this class is static modify
        capturing_task->do_capture<AccessHandle>(*this, copied_from);
      } // end if capturing_task != nullptr
      else {
        // regular copy
        DARMA_ASSERT_FAILURE(
          "Copying an AccessHandle<T> to an analogous type (e.g., ReadAccessHandle<T>) is not"
          " allowed (except for the implicit copy that occurs when the handle is being captured)"
        );
      }
    }

    // end analogous type conversion constructor
    ////////////////////////////////////////

    ////////////////////////////////////////
    // Collection capture

  protected:

    template <typename U>
    using _is_collection_captured_archetype = tinympl::bool_<U::is_collection_captured>;

    template <typename U>
    using _is_not_collection_captured_archetype = tinympl::bool_<not U::is_collection_captured>;

    template <typename U>
    using access_handle_is_collection_captured = meta::detected_or_t<
      std::false_type, _is_collection_captured_archetype, U
    >;

    template <typename U>
    using access_handle_is_not_collection_captured = meta::detected_or_t<
      std::false_type, _is_not_collection_captured_archetype, U
    >;

  public:

    template <typename AccessHandleT>
    AccessHandle(AccessHandleT&& other,
      std::enable_if_t<
        is_convertible_from_access_handle<AccessHandleT>::value
          and not is_collection_captured
          and access_handle_is_collection_captured<std::decay_t<AccessHandleT>>::value,
        detail::_not_a_type
      > = { detail::_not_a_type_ctor_tag }
    ) {
      // Don't do a copy-based capture and registration; the TaskCollection
      // creation process will handle it.  Just copy over things...
      var_handle_ = std::forward<AccessHandleT>(other).var_handle_;
      current_use_ = std::forward<AccessHandleT>(other).current_use_;
      // Everything else will be set up by the TaskCollection setup process
    }

    template <typename AccessHandleT>
    AccessHandle(AccessHandleT&& other,
      std::enable_if_t<
        is_convertible_from_access_handle<AccessHandleT>::value
          and is_collection_captured
          and access_handle_is_not_collection_captured<std::decay_t<AccessHandleT>>::value,
        detail::_not_a_type
      > = { detail::_not_a_type_ctor_tag }
    ) {
      // Don't do a copy-based capture and registration; the TaskCollection
      // creation process will handle it.  Just copy over things...
      var_handle_ = std::forward<AccessHandleT>(other).var_handle_;
      current_use_ = std::forward<AccessHandleT>(other).current_use_;
      // Everything else will be set up by the TaskCollection setup process
    }

    // end Collection capture
    ////////////////////////////////////////

    // Allow casting to a non-const reference
    operator AccessHandle&() const {
      return *const_cast<AccessHandle*>(this);
    };

    ////////////////////////////////////////
    // <editor-fold desc="move constructors">

    AccessHandle(AccessHandle &&) noexcept = default;

    // Analogous type move constructor
    template <typename AccessHandleT>
    AccessHandle(AccessHandleT&& /* note: not universal reference! */ other,
      std::enable_if_t<
        // Check if this is convertible from AccessHandleT
        is_convertible_from_access_handle<std::decay_t<AccessHandleT>>::value
          and not std::is_same<std::decay_t<AccessHandleT>, AccessHandle>::value
            // also, turn this into a non-universal ref (i.e., only use for rvalue references)
            // and ignore consts as well (they should go to the const move constructor below)
          and std::is_same<
            std::remove_const_t<std::remove_reference_t<AccessHandleT>>,
            AccessHandleT
          >::value
          and access_handle_is_not_collection_captured<AccessHandleT>::value
          and not is_collection_captured
          and not access_handle_is_collection_captured<AccessHandleT>::value,
        detail::_not_a_type
      > _nat = { detail::_not_a_type_ctor_tag }
    ) noexcept
      : AccessHandle( reinterpret_cast<AccessHandle&&>(std::move(other)) )
    { }

    AccessHandle(AccessHandle const&& other) noexcept
      : AccessHandle(std::move(const_cast<AccessHandle&>(other)))
    { }

    // Analogous type const move constructor
    template <
      typename AccessHandleT,
      typename = std::enable_if_t<
        // Check if this is convertible from AccessHandleT
        is_convertible_from_access_handle<
          std::decay_t<AccessHandleT>>::value
          and not std::is_same<std::decay_t<AccessHandleT>, AccessHandle>::value
      >
    >
    AccessHandle(AccessHandleT const && other) noexcept
      : AccessHandle(std::move(const_cast<AccessHandleT&>(other)))
    { }

    // </editor-fold> end move constructors
    ////////////////////////////////////////

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
        not unfetched_,
        "Illegal operation on unfetched non-local AccessHandle"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_.get() != nullptr,
        "handle dereferenced in context without immediate permissions"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_->use.immediate_permissions_ != abstract::frontend::Use::Permissions::None,
        "handle dereferenced in state without immediate access to data, with key: {" << get_key() << "}"
      );
      using return_t_decay = std::conditional_t<
        is_compile_time_immediate_read_only,
        T const,
        T
      >;
      return static_cast<return_t_decay*>(current_use_->use.data_);
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
        not unfetched_,
        "Illegal operation on unfetched non-local AccessHandle"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_.get() != nullptr,
        "handle dereferenced in context without immediate permissions"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_->use.immediate_permissions_ != abstract::frontend::Use::Permissions::None,
        "handle dereferenced in state without immediate access to data, with key: {" << get_key() << "}"
      );
      using return_t_decay = std::conditional_t<
        is_compile_time_immediate_read_only,
        T const,
        T
      >;
      return *static_cast<return_t_decay*>(current_use_->use.data_);
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
        not unfetched_,
        "Illegal operation on unfetched non-local AccessHandle"
      );
      DARMA_ASSERT_MESSAGE(
        _can_be_released(),
        "release() called on handle not in releasable state (most likely, uninitialized or already released)"
      );
      current_use_ = nullptr;
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
        not unfetched_,
        "Illegal operation on unfetched non-local AccessHandle"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_.get() != nullptr,
        "set_value() called on handle in context without immediate permissions"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_->use.immediate_permissions_ == abstract::frontend::Use::Permissions::Modify,
        "set_value() called on handle not in immediately modifiable state, with key: {" << get_key() << "}"
      );
      DARMA_ASSERT_MESSAGE(value_constructed_,
        "Tried to call set_value on non-default-constructible type before calling emplace_value to construct"
          " the initial instance"
      );
      *static_cast<T*>(current_use_->use.data_) = std::forward<U>(val);
    }

    template <typename _Ignored = void, typename... Args>
    std::enable_if_t<
      not std::is_same<T, void>::value
        and is_compile_time_immediate_modifiable
        and std::is_same<_Ignored, void>::value
    >
    emplace_value(Args&&... args) const {
      DARMA_ASSERT_MESSAGE(
        not unfetched_,
        "Illegal operation on unfetched non-local AccessHandle"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_.get() != nullptr,
        "emplace_value() called on handle in context without immediate permissions"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_->use.immediate_permissions_ == abstract::frontend::Use::Permissions::Modify,
        "emplace_value() called on handle not in immediately modifiable state, with key: {" << get_key() << "}"
      );
      // TODO do this more uniformly and/or expose it to the frontend somehow
      using alloc_t = std::allocator<T>;
      using allocator_traits_t = std::allocator_traits<alloc_t>;
      alloc_t alloc;
      if (value_constructed_) {
        allocator_traits_t::destroy(alloc, (T*)(current_use_->use.data_));
      }
      allocator_traits_t::construct(alloc, (T*)(current_use_->use.data_), std::forward<Args>(args)...);
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
        not unfetched_,
        "Illegal operation on unfetched non-local AccessHandle"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_.get() != nullptr,
        "get_value() called on handle in context without immediate permissions"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_->use.immediate_permissions_ != abstract::frontend::Use::Permissions::None,
        "get_value() called on handle not in immediately readable state, with key: {" << get_key() << "}"
      );
      return *static_cast<T const *>(current_use_->use.data_);
    }

    const key_t&
    get_key() const {
      return var_handle_->get_key();
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
        not unfetched_,
        "Illegal operation on unfetched non-local AccessHandle"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_.get() != nullptr,
        "get_reference() called on handle in context without immediate permissions"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_->use.immediate_permissions_ == abstract::frontend::Use::Permissions::Modify,
        "get_reference() called on handle not in immediately modifiable state, with key: {" << get_key() << "}"
      );
      return *static_cast<T*>(current_use_->use.data_);
    }

    template <typename _Ignored=void, typename... PublishExprParts>
    std::enable_if_t<
      is_compile_time_schedule_readable
        and std::is_same<_Ignored, void>::value
    >
    publish(
      PublishExprParts&&... parts
    ) const;

    template <typename... Args>
    auto const& read_access(
      Args&&... args
    ) const {

      DARMA_ASSERT_MESSAGE(
        unfetched_,
        "Illegal operation on AccessHandle not in an unfetched state"
      );

      unfetched_ = false;

      using namespace darma_runtime::detail;
      using parser = detail::kwarg_parser<
        overload_description<
          _optional_keyword<converted_parameter, keyword_tags_for_publication::version>
        >
      >;
      using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

      return parser()
        .with_converters(
          [](auto&&... parts) {
            return darma_runtime::make_key(std::forward<decltype(parts)>(parts)...);
          }
        )
        .with_default_generators(
          keyword_arguments_for_publication::version=[]{ return make_key(); }
        )
        .parse_args(std::forward<Args>(args)...)
        .invoke([this](
          types::key_t&& version_key
        ) -> decltype(auto) {

          auto* backend_runtime = abstract::backend::get_backend_runtime();
          auto fetched_in_flow = make_flow_ptr(
            backend_runtime->make_fetching_flow(
              var_handle_,
              version_key
            )
          );

          auto fetched_out_flow = make_flow_ptr(
            backend_runtime->make_null_flow(
              var_handle_
            )
          );

          current_use_ = make_shared<GenericUseHolder<HandleUse>>(
            var_handle_,
            fetched_in_flow,
            fetched_out_flow,
            HandleUse::Read,
            HandleUse::Read
          );

          current_use_->could_be_alias = true;
          current_use_->use.use_->collection_owner_ = owning_index_;

          return *this;

        });
    }


    template <
      typename Index,
      typename _for_SFINAE_only=void,
      typename=std::enable_if_t<
        not is_collection_captured
        and std::is_void<_for_SFINAE_only>::value
      >
    >
    with_traits<
      typename traits::template with_collection_capture_mode<
        detail::AccessHandleTaskCollectionCaptureMode::UniqueModify
      >::type::template with_owning_index_type<std::decay_t<Index>>::type
    >
    owned_by(Index&& idx) const {
      using return_type = with_traits<
        typename traits::template with_collection_capture_mode<
          detail::AccessHandleTaskCollectionCaptureMode::UniqueModify
        >::type::template with_owning_index_type<std::decay_t<Index>>::type
      >;
      auto rv = return_type(*this);
      rv.owning_index_ = std::forward<Index>(idx);
      return rv;
    };



    ~AccessHandle() noexcept = default;

  private:

    //void _switch_to_new_use(use_holder_ptr&& new_use) const {
    //  std::swap(current_use_, new_use);
    //}

    bool _can_be_released() const {
      // There are more conditions that could be checked, (e.g., if the state
      // is None, None), but we'll just use this one for now
      return (bool)current_use_.get() && current_use_->use.handle_ != nullptr;
    }

  //==============================================================================
  // <editor-fold desc="private ctors"> {{{1

  private:

    explicit
    AccessHandle(
      variable_handle_ptr const& var_handle,
      use_holder_ptr const& use_holder
    )
      : var_handle_(var_handle),
      current_use_(use_holder) {}

    AccessHandle(
      variable_handle_ptr var_handle,
      detail::flow_ptr const& in_flow,
      detail::flow_ptr const& out_flow,
      abstract::frontend::Use::permissions_t scheduling_permissions,
      abstract::frontend::Use::permissions_t immediate_permissions
    ) : var_handle_(var_handle),
        current_use_(
          detail::make_shared<detail::UseHolder>(
            detail::HandleUse(
              var_handle_,
              in_flow, out_flow,
              scheduling_permissions, immediate_permissions
            )
          )
        )
    {
      current_use_->could_be_alias = true;
    }

    template <typename Archive>
    AccessHandle(
      serialization::unpack_constructor_tag_t const&,
      Archive& ar
    ) {
      key_t k;
      ar >> k;
      var_handle_ = detail::make_shared<detail::VariableHandle<T>>(k);
      detail::HandleUse::permissions_t immed, sched;

      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wuninitialized"

      ar >> sched >> immed;

      auto* backend_runtime = abstract::backend::get_backend_runtime();

      using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
      auto in_flow = detail::make_flow_ptr(
        backend_runtime->make_unpacked_flow(
          ArchiveAccess::get_const_spot(ar)
        )
      );

      // Note that the backend function advances the underlying pointer, so the
      // pointer returned by get_spot is different in the call below from the
      // call above
      auto out_flow = detail::make_flow_ptr(
        backend_runtime->make_unpacked_flow(
          ArchiveAccess::get_const_spot(ar)
        )
      );

      current_use_ = std::make_shared<detail::UseHolder>(
        detail::migrated_use_arg,
        detail::HandleUse(
          var_handle_,
          in_flow, out_flow, sched, immed
        )
      );

      #pragma clang diagnostic pop
    }

    AccessHandle(
      detail::unfetched_access_handle_tag,
      variable_handle_ptr const& var_handle,
      use_holder_ptr const& unreg_use_ptr
    ) : var_handle_(var_handle),
        current_use_(unreg_use_ptr),
        unfetched_(true)
    { }


  // </editor-fold> end private ctors }}}1
  //==============================================================================

  //==============================================================================
  // <editor-fold desc="private members"> {{{1

  private:

    mutable variable_handle_ptr var_handle_ = nullptr;
    mutable use_holder_ptr current_use_ = nullptr;
    // TODO switch to everything has to be constructed requirement
    mutable bool value_constructed_ = std::is_default_constructible<T>::value;
    mutable bool unfetched_ = false;
    mutable typename traits::owning_index_t owning_index_;

    AccessHandle const* prev_copied_from = nullptr;

  // </editor-fold> end private members }}}1
  //==============================================================================

   public:

    ////////////////////////////////////////
    // Attorneys for create_work and *_access functions
    friend class detail::create_work_attorneys::for_AccessHandle;
    friend class detail::access_attorneys::for_AccessHandle;

    template <typename, typename, size_t, typename>
    friend struct detail::_task_collection_impl::_get_task_stored_arg_helper;

    template <typename, typename, typename, typename>
    friend struct detail::_task_collection_impl::_get_storage_arg_helper;

    ////////////////////////////////////////
    // TaskBase is also a friend
    friend class detail::TaskBase;

    template <typename, typename>
    friend class AccessHandle;

    template <typename, typename>
    friend class AccessHandleCollection;

    template <typename Op>
    friend struct detail::all_reduce_impl;

    template <typename AccessHandleT>
    friend struct detail::_publish_impl;

    template <typename>
    friend class detail::IndexedAccessHandle;

    ////////////////////////////////////////
    // Analogs with different privileges are friends too
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

#ifdef DARMA_TEST_FRONTEND_VALIDATION_CREATE_WORK
    FRIEND_TEST(::TestCreateWork, mod_capture_MM);
#endif

};

template <typename T>
using ReadAccessHandle = AccessHandle<
  T, typename detail::make_access_handle_traits<
    detail::min_immediate_permissions<detail::AccessHandlePermissions::Read>,
    detail::max_immediate_permissions<detail::AccessHandlePermissions::Read>,
    detail::min_schedule_permissions<detail::AccessHandlePermissions::Read>
  >::type
>;

namespace serialization {

template <typename... Args>
struct Serializer<AccessHandle<Args...>> {
  private:
    using AccessHandleT = AccessHandle<Args...>;

    bool handle_is_serializable_assertions(AccessHandleT const& val) const {
      // The handle has to be set up and valid
      assert(val.var_handle_.get() != nullptr);
      // The only AccessHandle objects that should ever be migrated are ones that
      // are already registered as part of a task.
      assert(val.current_use_->is_registered == true);
      // Also, since this has to be before the task runs, whether or not the use
      // can establish an alias should be exactly determined by whether or not it
      // has modify scheduling permissions (and less than modify immediate permissions)
      assert(
        ((
          val.current_use_->use.scheduling_permissions_ == ::darma_runtime::detail::HandleUse::Modify
            and val.current_use_->use.immediate_permissions_ != ::darma_runtime::detail::HandleUse::Modify
        ) and val.current_use_->could_be_alias)
          or (not (
            val.current_use_->use.scheduling_permissions_ == ::darma_runtime::detail::HandleUse::Modify
              and val.current_use_->use.immediate_permissions_ != ::darma_runtime::detail::HandleUse::Modify
          ) and not val.current_use_->could_be_alias)
      );
      // captured_as_ should always be normal here
      assert(val.captured_as_ == AccessHandleT::CapturedAsInfo::Normal);
      return true;
    }


  public:
    template <typename ArchiveT>
    void compute_size(AccessHandleT const& val, ArchiveT& ar) const {

      assert(handle_is_serializable_assertions(val));

      ar % val.var_handle_->get_key();
      ar % val.current_use_->use.scheduling_permissions_;
      ar % val.current_use_->use.immediate_permissions_;

      auto* backend_runtime = abstract::backend::get_backend_runtime();
      // TODO if we add operator==() to the requirements of flow_t, we don't have to pack the outflow when it's the same as the inflow
      ar.add_to_size_indirect(
        backend_runtime->get_packed_flow_size(*(val.current_use_->use.in_flow_))
      );
      ar.add_to_size_indirect(
        backend_runtime->get_packed_flow_size(*(val.current_use_->use.out_flow_))
      );

    }

    template <typename ArchiveT>
    void pack(AccessHandleT const& val, ArchiveT& ar) const {

      assert(handle_is_serializable_assertions(val));

      ar << val.var_handle_->get_key();
      ar << val.current_use_->use.scheduling_permissions_;
      ar << val.current_use_->use.immediate_permissions_;

      using detail::DependencyHandle_attorneys::ArchiveAccess;
      auto* backend_runtime = abstract::backend::get_backend_runtime();
      // TODO if we add operator==() to the requirements of flow_t, we don't have to pack the outflow when it's the same as the inflow
      backend_runtime->pack_flow(
        *(val.current_use_->use.in_flow_),
        ArchiveAccess::get_spot(ar)
      );
      backend_runtime->pack_flow(
        *(val.current_use_->use.out_flow_),
        ArchiveAccess::get_spot(ar)
      );

    }

    template <typename ArchiveT>
    void unpack(void* allocated, ArchiveT& ar) const {
      // Call an unpacking constructor
      new (allocated) AccessHandleT(serialization::unpack_constructor_tag, ar);
    }
};

} // end namespace serialization

} // end namespace darma_runtime

#include <darma/impl/access_handle_publish.impl.h>

#endif /* SRC_INCLUDE_DARMA_INTERFACE_APP_ACCESS_HANDLE_H_ */
