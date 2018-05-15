/*
//@HEADER
// ************************************************************************
//
//                          access_handle.h
//                         dharma_new
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef SRC_INCLUDE_DARMA_INTERFACE_APP_ACCESS_HANDLE_H_
#define SRC_INCLUDE_DARMA_INTERFACE_APP_ACCESS_HANDLE_H_

#ifdef DARMA_TEST_FRONTEND_VALIDATION
#  include <gtest/gtest_prod.h>
#endif

#include <darma/impl/commutative_access_fwd.h>

#include <darma/impl/handle.h>

#include <darma/impl/meta/splat_tuple.h>

#include <darma/impl/util/managing_ptr.h>

#include <darma/impl/task/task.h>
#include <darma/keyword_arguments/check_allowed_kwargs.h>
#include <darma/impl/util.h>
#include <darma/impl/use.h>
#include <darma/impl/publication_details.h>
#include <darma/impl/flow_handling.h>
#include <darma/impl/task_collection/task_collection_fwd.h>
#include <darma/keyword_arguments/parse.h>
#include <darma/impl/access_handle_base.h>
#include <darma/impl/access_handle/copy_captured_object.h>
#include <darma/impl/access_handle/basic_access_handle.h>
#include <darma/impl/create_work/create_work_while_fwd.h>

namespace darma {


// todo move this to a more appropriate place
template <typename T>
T darma_copy(T& lvalue) {
  return lvalue;
}

// TODO overload something to make it "release scheduling permissions and return reference" (i.e., release_into_reference())
// TODO unary +/-/! (maybe?) as a "const dereference" and give a mode that doesn't allow * or -> in read-only at runtime

template <typename T, typename Traits>
class AccessHandle
  : public detail::BasicAccessHandle,
    private detail::CopyCapturedObject<AccessHandle<T, Traits>>
{
  //===========================================================================
  // <editor-fold desc="type aliases"> {{{1

  public:

    typedef T value_type;

  protected:

    using variable_handle_t = detail::VariableHandle<T>;
    using variable_handle_ptr = types::shared_ptr_template<
      detail::VariableHandle<T>
    >;

    using key_t = types::key_t;

    using base_t = detail::BasicAccessHandle;
    using copy_capture_handler_t = detail::CopyCapturedObject<AccessHandle>;

  // </editor-fold> end type aliases }}}1
  //============================================================================

  //============================================================================
  // <editor-fold desc="Expression of traits as static member variables"> {{{1

  public:

    using traits = Traits;

    template <typename NewTraits>
    using with_traits = AccessHandle<T, NewTraits>;

    using CompileTimeReadAccessAnalog = with_traits<
      typename traits
        ::template with_static_scheduling_permissions<detail::AccessHandlePermissions::Read>::type
        ::template with_static_immediate_permissions<detail::AccessHandlePermissions::Read>::type
    >;


    static constexpr bool is_compile_time_immediate_modifiable =
      (traits::static_immediate_permissions
        == detail::AccessHandlePermissions::Modify
        or not traits::static_immediate_permissions_given);
    static constexpr bool is_compile_time_scheduling_modifiable =
      (traits::static_scheduling_permissions
        == detail::AccessHandlePermissions::Modify
        or not traits::static_scheduling_permissions_given);
    static constexpr bool is_compile_time_immediate_readable =
      (traits::static_immediate_permissions
        == detail::AccessHandlePermissions::Read
        or traits::static_immediate_permissions
          == detail::AccessHandlePermissions::Modify
        or not traits::static_immediate_permissions_given);
    static constexpr bool is_compile_time_scheduling_readable =
      (traits::static_scheduling_permissions == detail::AccessHandlePermissions::Read
        or traits::static_scheduling_permissions
          == detail::AccessHandlePermissions::Modify
        or not traits::static_scheduling_permissions_given);
    static constexpr bool is_compile_time_immediate_read_only =
      (is_compile_time_immediate_readable
        and not is_compile_time_immediate_modifiable);

    static constexpr bool is_leaf = (
      traits::static_scheduling_permissions == detail::AccessHandlePermissions::None
    );

    // Assert that static_scheduling_permissions, if given, is >= required_scheduling_permissions, if also given
    static_assert(
      detail::compatible_given_AH_permissions_greater_equal<
        traits::static_scheduling_permissions, traits::required_scheduling_permissions
      >::value,
      "Tried to create handle with static_scheduling_permissions < required_scheduling_permissions"
    );
    // Assert that static_immediate_permissions, if given, is >= required_immediate_permissions, if also given
    static_assert(
      detail::compatible_given_AH_permissions_greater_equal<
        traits::static_immediate_permissions, traits::required_immediate_permissions
      >::value,
      "Tried to create handle with static_immediate_permissions < required_immediate_permissions"
    );

    template <typename AccessHandleTIn,
      /* convenience temporary */
      typename AccessHandleT = std::decay_t<AccessHandleTIn>
    >
    using is_convertible_from_access_handle =  std::integral_constant<
      bool,
      detail::is_access_handle<AccessHandleT>::value
        // Check if the conversion is allowed based on min permissions and max permissions
        and detail::compatible_given_AH_permissions_greater_equal<
          AccessHandleT::traits::static_immediate_permissions, traits::required_immediate_permissions
        >::value
        // same thing for schedule case
        and detail::compatible_given_AH_permissions_greater_equal<
          AccessHandleT::traits::static_scheduling_permissions, traits::required_scheduling_permissions
        >::value
        // Special members need to be convertible also
        and std::is_convertible<
          typename AccessHandleT::traits::special_members_t,
          typename traits::special_members_t
        >::value
    >;

    template <typename AccessHandleTIn,
      /* convenience temporary */
      typename AccessHandleT = std::decay_t<AccessHandleTIn>
    >
    using is_reinterpret_castable_from_access_handle = std::integral_constant<
      bool, is_convertible_from_access_handle<AccessHandleT>::value
        and traits::template is_reinterpret_castable_from<typename AccessHandleT::traits>::type::value
    >;

    static constexpr auto
      is_collection_captured = traits::collection_capture_given;
    static constexpr auto is_collection_captured_as_shared_read =
      traits::collection_captured_as_shared_read;
    static constexpr auto is_collection_captured_as_unique_modify =
      traits::collection_captured_as_unique_modify;

    static constexpr auto is_known_not_copy_assignable =
      traits::semantic_traits::is_copy_assignable == OptionalBoolean::KnownFalse;

  // </editor-fold> end Expression of traits as static member variables }}}1
  //============================================================================

  //============================================================================
  // <editor-fold desc="assignment operator overloads"> {{{1

  public:

    // TODO make read-only AccessHandles copy assignable

    template <typename AccessHandleT>
    std::enable_if_t<
      is_convertible_from_access_handle<AccessHandleT>::value
        and not AccessHandleT::is_known_not_copy_assignable,
      AccessHandle&
    >
    operator=(AccessHandleT const& other) {
      // Don't need go through copy ctor, since it shouldn't be a capture
      assert(not _is_capturing_copy());
      this->base_t::_do_assignment(other);
      other_private_members_ = other.other_private_members_;
      return *this;
    }

    template <typename AccessHandleT,
      typename=std::enable_if_t<
        is_convertible_from_access_handle<AccessHandleT>::value
          and std::is_rvalue_reference<AccessHandleT&&>::value
      >
    >
    AccessHandle&
    operator=(AccessHandleT&& /*note: not a universal reference! */ other) {
      this->base_t::_do_assignment(std::move(other));
      other_private_members_ = std::move(other.other_private_members_);
      return *this;
    };

    AccessHandle const&
    operator=(std::nullptr_t) const {
      release();
      return *this;
    }

  // </editor-fold> end assignment operator overloads }}}1
  //============================================================================

  //============================================================================
  // <editor-fold desc="Public Ctors"> {{{1

  public:

    AccessHandle() { }

    AccessHandle(AccessHandle const& copied_from);

    //--------------------------------------------------------------------------
    // <editor-fold desc="Analogous type conversion constructor"> {{{2

    // This *has* to be distinct from the regular copy constructor above because
    // of the handling of the double-copy capture
    template <
      typename AccessHandleT,
      typename = std::enable_if_t<
        // Check if it's a convertible AccessHandle type that's not this type
        is_convertible_from_access_handle<AccessHandleT>::value
          and not std::is_same<std::decay_t<AccessHandleT>, AccessHandle>::value
      >
    >
    AccessHandle(
      AccessHandleT const& copied_from
    )
    {
      auto result =
        this->copy_capture_handler_t::handle_compatible_analog_construct(
          copied_from
        );

      // There's no lambda serdes case to worry about here
      assert(not result.argument_is_garbage);

      if(not result.did_capture and not result.argument_is_garbage and not result.required_permissions) {
        // then we need to propagate stuff here, since no capture handler was invoked
        // in other words, we're acting like an assignment
        this->base_t::_do_assignment(copied_from);
        other_private_members_ = copied_from.other_private_members_;
      }

      // added by gb -- 02-08-2017 
      if (result.required_permissions) {
         this->base_t::_reset_handles();
         other_private_members_ = copied_from.other_private_members_;
      }

    }

    // </editor-fold> end Analogous type conversion constructor }}}2
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // <editor-fold desc="move constructors"> {{{2

  public:

    template <typename _Ignored_SFINAE=void,
      typename=std::enable_if_t<std::is_void<_Ignored_SFINAE>::value>
    >
    AccessHandle(AccessHandle&& other)
      : base_t(std::move(other)),
        copy_capture_handler_t(std::move(other)),
        other_private_members_(std::move(other.other_private_members_))
    { }

    // Analogous type move constructor
    // Can't use forwarding constructor here, apparently
    template <typename AccessHandleT>
    AccessHandle(
      AccessHandleT&& /* note: not universal reference! */ other,
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
          and not is_reinterpret_castable_from_access_handle<AccessHandleT>::value,
        utility::_not_a_type
      > _nat = {utility::_not_a_type_ctor_tag}
    ) : base_t(std::move(other)),
        copy_capture_handler_t(),
        other_private_members_(std::move(other.other_private_members_))
    { }

    // need to make this a template to avoid deleting templated assignment operators
    template <typename _Ignored_SFINAE=void,
      typename=std::enable_if_t<std::is_void<_Ignored_SFINAE>::value>
    >
    AccessHandle(AccessHandle const&& other) noexcept
      : AccessHandle(std::move(const_cast<AccessHandle&>(other)))
    { /* forwarding ctor, must be empty */ }

    // </editor-fold> end move constructors }}}2
    //--------------------------------------------------------------------------

  // </editor-fold> end Public Ctors }}}1
  //============================================================================

  //============================================================================
  // <editor-fold desc="Public interface methods"> {{{1

  private:


  public:

    // added by gb -- 02-09-2018
    AccessHandle& get_source_access_handle() const {
       return *(this->copy_capture_handler_t::get_source_object());
    }

    template <
      typename _Ignored=void,
      typename = std::enable_if_t<
        is_compile_time_immediate_readable
          and std::is_same<_Ignored, void>::value
      >
    >
    std::conditional_t<
      is_compile_time_immediate_read_only,
      T const*,
      T*
    >
    operator->() const {
      _check_at_least_read_immediate_permissions("operator->()");
      using return_t_decay = std::conditional_t<
        is_compile_time_immediate_read_only,
        T const,
        T
      >;
      return static_cast<return_t_decay*>(get_current_use()->use()->data_);
    }

    template <
      typename _Ignored=void,
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
      _check_at_least_read_immediate_permissions("operator*()");
      using return_t_decay = std::conditional_t<
        is_compile_time_immediate_read_only,
        T const,
        T
      >;
      return *static_cast<return_t_decay*>(get_current_use()->use()->data_);
    }

    template <
      typename _Ignored = void,
      typename = std::enable_if_t<
        // Technically, if this is compile-time nothing, you can't even do this, so we can disable it
        not(traits::static_immediate_permissions
          == detail::AccessHandlePermissions::None
          and traits::static_scheduling_permissions
            == detail::AccessHandlePermissions::None)
          and std::is_same<_Ignored, void>::value
      >
    >
    void
    release() const {
      _check_use_exists("release()");
      DARMA_ASSERT_MESSAGE(
        _can_be_released(),
        "release() called on handle not in releasable state (most likely, uninitialized or already released)"
      );
      current_use_base_ = nullptr;
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
      _check_at_least_write_immediate_permissions("set_value()");
      *static_cast<T*>(get_current_use()->use()->data_) = std::forward<U>(val);
    }

    template <typename _Ignored = void, typename... Args>
    std::enable_if_t<
      not std::is_same<T, void>::value
        and is_compile_time_immediate_modifiable
        and std::is_same<_Ignored, void>::value
    >
    emplace_value(Args&& ... args) const {
      _check_at_least_write_immediate_permissions("emplace_value()");

      using alloc_t = typename traits::allocation_traits::allocator_t;
      using allocator_traits_t = std::allocator_traits<alloc_t>;
      alloc_t alloc;
      allocator_traits_t::destroy(alloc, (T*)(get_current_use()->use()->data_));
      allocator_traits_t::construct(
        alloc,
        (T*)(get_current_use()->use()->data_),
        std::forward<Args>(args)...
      );
    }

    template <
      typename = std::enable_if<
        not std::is_same<T, void>::value
          and is_compile_time_immediate_readable
      >
    >
    const T&
    get_value() const {
      _check_at_least_read_immediate_permissions("get_value()");
      return *static_cast<T const*>(get_current_use()->use()->data_);
    }

    const key_t&
    get_key() const {
      return var_handle_base_->get_key();
    }

    template <
      typename = std::enable_if<
        not std::is_same<T, void>::value
          and is_compile_time_immediate_modifiable
      >
    >
    T&
    get_reference() const {
      _check_modify_immediate_permissions("get_reference()");
      return *static_cast<T*>(get_current_use()->use()->data_);
    }

    template <
      typename = std::enable_if<
        not std::is_same<T, void>::value
          and is_compile_time_immediate_modifiable
      >
    >
    T&
    obtain_reference() const {
      _check_modify_immediate_permissions("obtain_reference()");
      if(get_current_use()->use()->scheduling_permissions_ != frontend::Permissions::None) {
        // TODO decide if this needs to register a new use and release the old one
        // for now, go ahead and do that

        get_current_use()->replace_use(
          get_current_use()->use()->with_different_permissions(
            frontend::Permissions::None,
            get_current_use()->use()->immediate_permissions_
          )
        );
      }

      return *static_cast<T*>(get_current_use()->use()->data_);
    }

#if _darma_has_feature(publish_fetch)
    template <typename _Ignored=void, typename... PublishExprParts>
    std::enable_if_t<
      is_compile_time_scheduling_readable
        and std::is_same<_Ignored, void>::value
    >
    publish(
      PublishExprParts&& ... parts
    ) const;
#endif // _darma_has_feature(publish_fetch)

    template <
      typename _for_SFINAE_only=void,
      typename=std::enable_if_t<
        not is_collection_captured
          and std::is_void<_for_SFINAE_only>::value
      >
    >
    with_traits<
      typename traits::template with_collection_capture_mode<
        detail::AccessHandleTaskCollectionCaptureMode::SharedRead
      >::type
    >
    shared_read() const {
      using return_type = with_traits<
        typename traits::template with_collection_capture_mode<
          detail::AccessHandleTaskCollectionCaptureMode::SharedRead
        >::type
      >;
      return return_type(*this);
    };


#if _darma_has_feature(commutative_access_handles)
    template <typename _Ignored_SFINAE=void>
    std::enable_if_t<
      is_compile_time_scheduling_modifiable
        and std::is_void<_Ignored_SFINAE>::value
    >
    begin_commutative_usage() const {
      DARMA_ASSERT_MESSAGE(
        current_use_.get() != nullptr,
        "begin_commutative_usage called on uninitialized or released AccessHandle"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_->use->immediate_permissions_ == frontend::Permissions::None,
        "begin_commutative_usage called on use with immediate permissions that aren't None"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_->use->scheduling_permissions_ == frontend::Permissions::Modify,
        "begin_commutative_usage called on use without scheduling modify permissions"
      );

      using namespace darma::detail::flow_relationships;

      // Need to make next flow to be the output of the commutative usage
      auto old_out_flow = current_use_->use->out_flow_;

      current_use_->replace_use(
        detail::HandleUse(
          var_handle_,
          /* scheduling permissions */
          frontend::Permissions::Modify,
          /* immediate permissions */
          frontend::Permissions::None,
          same_flow(&current_use_->use->in_flow_),
          next_of_in_flow()
#if _darma_has_feature(anti_flows)
          , same_anti_flow(&current_use_->use->anti_out_flow_),
          insignificant_flow()
#endif // _darma_has_feature(anti_flows)
          , frontend::CoherenceMode::Commutative
        ),
        true
      );

      assert(current_use_->use->suspended_out_flow_ == nullptr);
      current_use_->use->suspended_out_flow_ = std::make_unique<types::flow_t>(
        std::move(old_out_flow)
      );

      set_is_commutative_dynamic(true);
    }

    template <typename _Ignored_SFINAE=void>
    std::enable_if_t<
      is_compile_time_scheduling_modifiable
        and std::is_void<_Ignored_SFINAE>::value
    >
    end_commutative_usage() const {
      // TODO We'll need to swap uses here when all of the uses are registered
      DARMA_ASSERT_MESSAGE(
        current_use_.get() != nullptr,
        "end_commutative_usage called on uninitialized or released AccessHandle"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_->use->immediate_permissions_ == frontend::Permissions::None,
        "end_commutative_usage called on use with immediate permissions that aren't None"
      );
      DARMA_ASSERT_MESSAGE(
        current_use_->use->coherence_mode_ == frontend::CoherenceMode::Commutative,
        "end_commutative_usage called on use without being in Commutative coherence mode"
      );

      using namespace darma::detail::flow_relationships;

      set_is_commutative_dynamic(false);

#if _darma_has_feature(anti_flows)
      DARMA_ASSERT_NOT_IMPLEMENTED("new anti-flow semantics with commutative");
#endif

      current_use_->replace_use(
        detail::HandleUse(
          var_handle_,
          /* scheduling permissions */
          frontend::Permissions::Modify,
          /* immediate permissions */
          frontend::Permissions::None,
          same_flow(&current_use_->use->out_flow_),
          //FlowRelationship::Same, &current_use_->use->out_flow_,
          same_flow(current_use_->use->suspended_out_flow_.get())
          //FlowRelationship::Same, current_use_->use->suspended_out_flow_.release()
#if _darma_has_feature(anti_flows)
          , insignificant_flow(),
          anti_next_of_in_flow()
#endif // _darma_has_feature(anti_flows)
        ),
        true
      );
    }
#endif // _darma_has_feature(commutative_access_handles)

  // </editor-fold> end Public interface methods }}}1
  //============================================================================

    ~AccessHandle() = default;

  private:

  //============================================================================
  // <editor-fold desc="Private implementation detail methods"> {{{1

  private:

    /** @internal
     *  Reusable assertions, called for debugging purposes only
     */
    inline void _check_use_exists(const char* operation) const {
      DARMA_ASSERT_MESSAGE(
        has_use_holder(),
        "Invalid operation `" << operation << "` performed on AccessHandle after"
          " release or before assignment (i.e., UseHolder was null)"
      );
      DARMA_ASSERT_MESSAGE(
        get_current_use()->has_use(),
        "Internal error: Invalid operation `" << operation << "` performed on"
          " AccessHandle with non-null UseHolder but with null Use"
      );
    }

    /** @internal
     *  Reusable assertions, called for debugging purposes only
     */
    inline void _check_at_least_read_immediate_permissions(
      const char* operation
    ) const {
      _check_use_exists(operation);
      DARMA_ASSERT_MESSAGE(
        get_current_use()->use()->immediate_permissions_ == frontend::Permissions::Read
          || get_current_use()->use()->immediate_permissions_ == frontend::Permissions::Modify,
        "Invalid operation `" << operation << "` performed on AccessHandle"
          " with immediate permissions " << permissions_to_string(
          get_current_use()->use()->immediate_permissions_
        ) << " (needs to have at least immediate Read permissions)"
      );
    }

    /** @internal
     *  Reusable assertions, called for debugging purposes only
     */
    inline void _check_at_least_write_immediate_permissions(
      const char* operation
    ) const {
      _check_use_exists(operation);
      DARMA_ASSERT_MESSAGE(
        get_current_use()->use()->immediate_permissions_ == frontend::Permissions::Write
          || get_current_use()->use()->immediate_permissions_ == frontend::Permissions::Modify,
        "Invalid operation `" << operation << "` performed on AccessHandle"
          " with immediate permissions " << permissions_to_string(
          get_current_use()->use()->immediate_permissions_
        ) << " (needs to have at least immediate Write permissions)"
      );
    }

    /** @internal
     *  Reusable assertions, called for debugging purposes only
     */
    inline void _check_modify_immediate_permissions(
      const char* operation
    ) const {
      _check_use_exists(operation);
      DARMA_ASSERT_MESSAGE(
        get_current_use()->use()->immediate_permissions_ == frontend::Permissions::Modify,
        "Invalid operation `" << operation << "` performed on AccessHandle"
          " with immediate permissions " << permissions_to_string(
          get_current_use()->use()->immediate_permissions_
        ) << " (needs to have Modify permissions)"
      );
    }

    bool _can_be_released() const {
      // There are more conditions that could be checked, (e.g., if the state
      // is None, None), but we'll just use this one for now
      return (bool)current_use_base_.get();
    }

    bool
    _is_capturing_copy() const {
      // This method is basically only used in debug mode (inside an assert)
      detail::TaskBase* running_task = static_cast<detail::TaskBase* const>(
        abstract::backend::get_backend_context()->get_running_task()
      );
      if (running_task) {
        return running_task->current_create_work_context != nullptr;
      } else {
        return false;
      }
    }

  // </editor-fold> end Private implementation detail methods }}}1
  //============================================================================

  //==============================================================================
  // <editor-fold desc="implicit conversions and reinterpretations"> {{{1

  public:

    template <
      typename AccessHandleT,
      typename=std::enable_if_t<
        std::decay_t<AccessHandleT>
          ::template is_reinterpret_castable_from_access_handle<AccessHandle>::value
          and not std::is_const<AccessHandleT>::value
      >
    >
    operator AccessHandleT&() &
    {
      static_assert(sizeof(AccessHandle) == sizeof(std::decay_t<AccessHandleT>),
        "Something went horribly wrong"
      );
      return *reinterpret_cast<AccessHandleT*>(this);
    }

    operator AccessHandle&() const &
    {
      return *const_cast<AccessHandle*>(this);
    }

    template <
      typename AccessHandleT,
      typename=std::enable_if_t<
        std::decay_t<AccessHandleT>
          ::template is_reinterpret_castable_from_access_handle<AccessHandle>::value
          // This works around a bug in GCC.  See impl/meta/any_convertible.h
          and meta::_impl::rvalue_ref_operator_needs_const_t::value
      >
    >
    operator AccessHandleT&&() &&
    {
      static_assert(sizeof(AccessHandle) == sizeof(std::decay_t<AccessHandleT>),
        "Something went horribly wrong"
      );
      return reinterpret_cast<AccessHandleT&&>(*this);
    }

  private:

  // </editor-fold> end implicit conversions and reinterpretations }}}1
  //==============================================================================

  //============================================================================
  // <editor-fold desc="AccessHandleBase pure virtual function implementations"> {{{1

  private:

    std::shared_ptr<detail::AccessHandleBase>
    copy() const override {
      auto rv = std::allocate_shared<AccessHandle>(
        std::allocator<AccessHandle>{}
      );
      rv->base_t::_do_assignment(*this);
      rv->other_private_members_ = other_private_members_;
      return rv;
    }

//    void
//    call_make_captured_use_holder(
//      std::shared_ptr<detail::VariableHandleBase> var_handle,
//      frontend::permissions_t req_sched,
//      frontend::permissions_t req_immed,
//      detail::AccessHandleBase const& source,
//      bool register_continuation_use = true
//    ) override {
//      set_current_use(
//        darma::detail::make_captured_use_holder(
//          var_handle_base_,
//          req_sched, req_immed,
//          get_current_use(),
//          register_continuation_use
//        )
//      );
//    }

    void release_current_use() const override {
      current_use_base_ = nullptr;
    }

    std::unique_ptr<detail::CaptureDescriptionBase>
    get_capture_description(
      detail::CapturedObjectBase& captured,
      detail::CapturedObjectBase::capture_op_t schedule_capture_op,
      detail::CapturedObjectBase::capture_op_t immediate_capture_op
    ) const override;


  // </editor-fold> end AccessHandleBase pure virtual function implementations }}}1
  //============================================================================

  //==============================================================================
  // <editor-fold desc="CopyCapturedObject hook implementations"> {{{1

  private:

    template <typename CompatibleAccessHandleT>
    void _mark_static_capture_flags(
      CompatibleAccessHandleT const* source
    ) {
      if (
        // If this type is a compile-time read-only handle, mark it as such here
        traits::static_immediate_permissions == detail::AccessHandlePermissions::Read
          // If the other type is compile-time read-only and we don't know, mark it as a read
          or (
            CompatibleAccessHandleT::traits::static_immediate_permissions
              == detail::AccessHandlePermissions::Read
              and not traits::static_immediate_permissions_given
          )
        ) {
        detail::CapturedObjectAttorney::captured_as_info(*source).ReadOnly = true;
        //source->captured_as_ |= CapturedAsInfo::ReadOnly;
      }
      if (
        // If this type doesn't have scheduling permissions, mark it as a leaf
        traits::static_scheduling_permissions == detail::AccessHandlePermissions::None
      ) {
        detail::CapturedObjectAttorney::captured_as_info(*source).Leaf = true;
        //source->captured_as_ |= CapturedAsInfo::Leaf;
      }
      if (
        // If this type doesn't have scheduling permissions, mark it as a leaf
        traits::required_immediate_permissions == detail::AccessHandlePermissions::None
      ) {
        detail::CapturedObjectAttorney::captured_as_info(*source).ScheduleOnly = true;
        //source->captured_as_ |= CapturedAsInfo::ScheduleOnly;
      }
    }

    template <typename CompatibleAccessHandleT>
    void prepare_for_capture(
      CompatibleAccessHandleT const& copied_from
    ) {
      // We should copy these over before reporting the capture
      var_handle_base_ = copied_from.var_handle_base_; // this should probably be somewhere else
      other_private_members_ = copied_from.other_private_members_;
      _mark_static_capture_flags(&copied_from);
    }

    template <
      typename CompatibleAccessHandleT,
      typename CaptureManagerT
    >
    void report_capture(
      CompatibleAccessHandleT const* source,
      CaptureManagerT* capturing_task
    ) {
      static_assert(
        std::is_base_of<detail::CaptureManager, CaptureManagerT>::value,
        "Report capture requires an object conforming to the CaptureManager interface"
      );

      capturing_task->do_capture(*this, *source);

      source->current_use_base_->use_base->already_captured = true;
      // TODO this flag should be on the AccessHandleBase itself
      capturing_task->uses_to_unmark_already_captured.insert(
        source->current_use_base_->use_base
      );
    }

    //--------------------------------------------------------------------------
    // <editor-fold desc="DARMA feature: task_migration"> {{{2
    #if _darma_has_feature(task_migration)

    template <typename Archive>
    void unpack_from_archive(Archive& ar);

    #endif // _darma_has_feature(task_migration)
    // </editor-fold> end DARMA feature: task_migration }}}2
    //--------------------------------------------------------------------------

    template <
      typename TaskLikeT
    >
    void report_dependency(
      TaskLikeT* task
    ) {
      task->add_dependency(*current_use_base_->use_base);
    }

  // </editor-fold> end CopyCapturedObject hook implementations }}}1
  //==============================================================================

  //============================================================================
  // <editor-fold desc="private ctors"> {{{1

  private:

    explicit
    AccessHandle(
      std::shared_ptr<detail::VariableHandleBase> const& var_handle,
      std::shared_ptr<typename base_t::use_holder_t> const& use_holder
    ) : base_t(var_handle, use_holder)
    {
      assert(utility::safe_static_cast<detail::VariableHandle<T>*>(var_handle.get()) != nullptr);
    }

    explicit
    AccessHandle(
      std::shared_ptr<detail::VariableHandleBase> const& var_handle,
      std::shared_ptr<typename base_t::use_holder_t> const& use_holder,
      std::unique_ptr<types::flow_t>&& suspended_out_flow
    ) : base_t(var_handle, use_holder)
    {
      assert(utility::safe_static_cast<detail::VariableHandle<T>*>(var_handle.get()) != nullptr);
      current_use_base_->use_base->suspended_out_flow_ = std::move(suspended_out_flow);
    }

  // </editor-fold> end private ctors }}}1
  //============================================================================

  //============================================================================
  // <editor-fold desc="private members and their accessors"> {{{1

  private:

    typename traits::special_members_t other_private_members_;

    bool
    get_is_outermost_scope_dynamic() const {
      return other_private_members_.is_outermost_scope_dynamic;
    }
    bool
    set_is_outermost_scope_dynamic(bool new_val) {
      other_private_members_.is_outermost_scope_dynamic = new_val;
      return new_val;
    }

    typename traits::allocation_traits::allocator_t const&
    get_allocator() const {
      return other_private_members_.allocator;
    }
    typename traits::allocation_traits::allocator_t&
    get_allocator() {
      return other_private_members_.allocator;
    }


  // </editor-fold> end private members }}}1
  //============================================================================

  //============================================================================
  // <editor-fold desc="friends"> {{{1

  public:

    friend class detail::create_work_attorneys::for_AccessHandle;
    friend class detail::access_attorneys::for_AccessHandle;

    template <typename, typename, size_t, typename>
    friend
    struct detail::_task_collection_impl::_get_task_stored_arg_helper;

    template <typename, typename, typename, typename>
    friend
    struct detail::_task_collection_impl::_get_storage_arg_helper;

    friend class detail::TaskBase;

    template <typename, typename>
    friend
    class AccessHandle;

    template <typename, typename, typename>
    friend
    class AccessHandleCollection;

    template <typename, typename, typename...>
    friend
    class detail::TaskCollectionImpl;

    template <typename Op>
    friend
    struct detail::all_reduce_impl;

    template <typename AccessHandleT>
    friend
    struct detail::_publish_impl;

    template <typename, typename...>
    friend struct detail::_commutative_access_impl;
    template <typename, typename...>
    friend struct detail::_noncommutative_access_impl;

    template <typename, typename>
    friend
    class detail::IndexedAccessHandle;

    // Analogs with different privileges are friends too
    friend struct detail::analogous_access_handle_attorneys::AccessHandleAccess;

    // Allow implicit conversion to value in the invocation of the task
    friend struct meta::splat_tuple_access<detail::AccessHandleBase>;

    friend struct darma::serialization::Serializer<AccessHandle>;

    template <typename, typename...>
    friend struct detail::_initial_access_key_helper;

    template <typename>
    friend struct detail::_read_access_helper;

    template <typename>
    friend class detail::CopyCapturedObject;


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

  // </editor-fold> end friends }}}1
  //============================================================================

};

template <typename T>
using ReadAccessHandle = AccessHandle<
  T, typename detail::make_access_handle_traits<T,
    detail::required_immediate_permissions<detail::AccessHandlePermissions::Read>,
    detail::static_immediate_permissions<detail::AccessHandlePermissions::Read>,
    detail::required_scheduling_permissions<detail::AccessHandlePermissions::Read>
  >::type
>;

template <typename AccessHandleT>
using ScheduleOnly = typename AccessHandleT::template with_traits<
  detail::make_access_handle_traits_t<typename AccessHandleT::value_type,
    detail::required_immediate_permissions<detail::AccessHandlePermissions::None>
  >
>;
template <typename AccessHandleT, typename IndexType>
using UniquelyOwned = typename AccessHandleT::template with_traits<
  detail::make_access_handle_traits_t<typename AccessHandleT::value_type,
    detail::owning_index_type<IndexType>,
    detail::collection_capture_mode<
      detail::AccessHandleTaskCollectionCaptureMode::UniqueModify
    >
  >
>;

template <typename T, typename... TraitModifiers>
using AccessHandleWithTraits = AccessHandle<T,
  detail::make_access_handle_traits_t<T,
    TraitModifiers...
  >
>;

} // end namespace darma


#include <darma/impl/access_handle/access_handle_serialization.h>

#include <darma/impl/access_handle/access_handle.impl.h>

#include <darma/impl/access_handle_publish.impl.h>

#endif /* SRC_INCLUDE_DARMA_INTERFACE_APP_ACCESS_HANDLE_H_ */
