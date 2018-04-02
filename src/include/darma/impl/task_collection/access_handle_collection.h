/*
//@HEADER
// ************************************************************************
//
//                      handle_collection.h
//                         DARMA
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

#ifndef DARMA_HANDLE_COLLECTION_H
#define DARMA_HANDLE_COLLECTION_H

#include <darma/impl/feature_testing_macros.h>
#if _darma_has_feature(create_concurrent_work)

// included forward declarations
#include <darma/impl/task_collection/task_collection_fwd.h>
#include <darma/impl/commutative_access_fwd.h>

// included interface files
#include <darma/interface/app/keyword_arguments/index_range.h>

// included from elsewhere in impl
#include <darma/impl/handle.h>
#include <darma/impl/use.h>
#include <darma/keyword_arguments/parse.h>
#include <darma/keyword_arguments/macros.h>
#include <darma/keyword_arguments/parse.h>
#include <darma/impl/access_handle_base.h>
#include <darma/utility/optional_boolean.h>
#include <darma/impl/use_collection.h>

// included from task_collection folder
#include <darma/impl/task_collection/errors.h>
#include <darma/impl/task_collection/access_handle_collection_traits.h>
#include <darma/impl/task_collection/mapped_handle_collection.h>
#include <darma/impl/task_collection/indexed_access_handle.h>

#include <darma/impl/access_handle/access_handle_collection_base.h>

#include <darma/impl/create_work/capture_permissions.h>

#include <darma/impl/collective/collective_fwd.h> // detail::op_not_given

#if _darma_has_feature(mpi_interop)
#include <darma/impl/mpi/piecewise_acquired_collection_fwd.h>
#endif // _darma_has_feature(mpi_interop)


namespace darma_runtime {

//==============================================================================
// <editor-fold desc="AccessHandleCollection">

template <typename T,
  typename IndexRangeT,
  typename Traits /*=detail::access_handle_collection_traits<>*/
>
class AccessHandleCollection
  : public detail::BasicAccessHandleCollection<IndexRangeT>,
    private detail::CopyCapturedObject<AccessHandleCollection<T, IndexRangeT, Traits>>
{
  public:

    using value_type = T;
    using traits = Traits;

    template <typename... NewTraitsFlags>
    using with_trait_flags = AccessHandleCollection<
      T, IndexRangeT, typename detail::make_access_handle_collection_traits<
        T, IndexRangeT, NewTraitsFlags...
      >::template from_traits<traits>::type
    >;

    template <typename NewTraits>
    using with_traits = AccessHandleCollection<T, IndexRangeT, NewTraits>;

  protected:

    using base_t = detail::BasicAccessHandleCollection<IndexRangeT>;
    using copy_capture_handler_t =
      detail::CopyCapturedObject<AccessHandleCollection<T, IndexRangeT, Traits>>;

  //============================================================================
  // <editor-fold desc="public interface functions"> {{{1

  public:

    template <typename MappingT>
    auto
    mapped_with(MappingT&& mapping) const {
      return detail::MappedHandleCollection<
        darma_runtime::AccessHandleCollection<T, IndexRangeT,
          detail::access_handle_collection_traits<T, IndexRangeT,
            typename traits::permissions_traits,
            detail::ahc_traits::semantic_traits<
              /* IsOuter= */ OptionalBoolean::KnownFalse,
              typename traits::semantic_traits::handle_semantic_traits
            >,
            typename traits::allocation_traits
          >
        >,
        std::decay_t<MappingT>
      >(
        *this, std::forward<MappingT>(mapping)
      );
    };


    IndexRangeT const&
    get_index_range() const {
      return this->get_current_use()->use()->collection_->index_range_;
    }


    template <
      typename _Ignored_SFINAE=void,
      typename=std::enable_if_t<
        std::is_void<_Ignored_SFINAE>::value // should always be true
          and traits::semantic_traits::is_outer != OptionalBoolean::KnownTrue
      >
    >
    auto
    operator[](
      typename base_t::index_range_traits_t::index_type const& idx
    ) const;


#if _darma_has_feature(handle_collection_based_collectives)
    template <typename ReduceOp=detail::op_not_given, typename... Args>
    auto
    reduce(Args&&... args) const;
#endif //_darma_has_feature(handle_collection_based_collectives)


  // </editor-fold> end public interface functions }}}1
  //============================================================================


  public:

    ~AccessHandleCollection() = default;


  //============================================================================
  // <editor-fold desc="AccessHandleBase pure virtual method overloads"> {{{1

  protected:

    std::unique_ptr<detail::CaptureDescriptionBase>
    get_capture_description(
      detail::CapturedObjectBase& captured,
      detail::CapturedObjectBase::capture_op_t schedule_capture_op,
      detail::CapturedObjectBase::capture_op_t immediate_capture_op
    ) const override;

//    void call_add_dependency(
//      detail::TaskBase* task
//    ) override {
//      if(local_use_holders_->size()) {
//        task->add_dependency(*this->current_use_base_->use_base);
//      }
//      else {
//        for(auto&& local_holder_pair : local_use_holders_) {
//          task->add_dependency(*local_holder_pair.second->use_base);
//        }
//      }
//    }

//    void call_make_captured_use_holder(
//      std::shared_ptr<detail::VariableHandleBase> var_handle,
//      frontend::permissions_t req_sched_perms,
//      frontend::permissions_t req_immed_perms,
//      detail::AccessHandleBase const& source_in,
//      bool register_continuation_use = true
//    ) override {
//      if(dynamic_is_outer) {
//        assert(local_use_holders_.size() == 0);
//        current_use_ = _call_make_captured_use_holder_impl(
//          var_handle,
//          (frontend::permissions_t)std::max((int)req_sched_perms, (int)req_immed_perms),
//          frontend::Permissions::None,
//          source_in,
//          register_continuation_use
//        );
//      }
//      else {
//        // TODO fix this for create_work_while!!!
//        //auto& source = reinterpret_cast<AccessHandleCollection const&>(source_in);
//        auto& source = utility::safe_static_cast<AccessHandleCollection const&>(source_in);
//        for(auto&& local_holder_pair : source.local_use_holders_) {
//          local_use_holders_[local_holder_pair.first] = detail::make_captured_use_holder(
//            var_handle,
//            req_sched_perms,
//            req_immed_perms,
//            local_holder_pair.second.get(),
//            register_continuation_use
//          );
//          // Continuation should be unchanged
//          // TODO test this
//        }
//        current_use_ = source.current_use_;
//      }
//    }

    std::shared_ptr<detail::AccessHandleBase>
    copy() const override {
      auto rv = std::allocate_shared<AccessHandleCollection>(
        std::allocator<AccessHandleCollection>{}
      );
      rv->_do_assignment(*this);
      return rv;
    }

//    void
//    replace_use_holder_with(detail::AccessHandleBase const& other_handle) override {
//      auto& other_as_this_type = utility::safe_static_cast<AccessHandleCollection const&>(
//        other_handle
//      );
//      current_use_ = other_as_this_type.current_use_;
//      if(not dynamic_is_outer) {
//        // Also replace the use holders in the local_use_holders_
//        assert(&other_as_this_type != this);
//        local_use_holders_ = other_as_this_type.local_use_holders_;
//      }
//    }

    // really should be something like "release_current_use_holders"...
    void
    release_current_use() const override {
      this->current_use_base_ = nullptr;
      if(local_use_holders_.size()) {
        local_use_holders_.clear();
      }
    }

  // </editor-fold> end AccessHandleBase pure virtual method overloads }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="public ctors"> {{{1

  public:

    AccessHandleCollection() { }

    AccessHandleCollection(
      AccessHandleCollection&& other
    ) : base_t(std::move(other)),
        copy_capture_handler_t(std::move(other)),
        mapped_backend_index_(std::move(other.mapped_backend_index_)),
        local_use_holders_(std::move(other.local_use_holders_)),
        dynamic_is_outer(std::move(other.dynamic_is_outer))
        #if _darma_has_feature(task_collection_token)
        , task_collection_token_(std::move(other.task_collection_token_))
        #endif // _darma_has_feature(task_collection_token)
    { }

    AccessHandleCollection(
      AccessHandleCollection const& other
    ) : /* can't do anything yet because we don't know if the argument is garbage or not */
        base_t(), copy_capture_handler_t()
    {

      auto capture_management_result =
        this->copy_capture_handler_t::handle_copy_construct(other);

      // If we didn't do a capture and the argument isn't garbage (e.g., from
      // lambda serdes process), then we need to do the normal copy constructor
      // operations
      if(
        not capture_management_result.did_capture
        and not capture_management_result.argument_is_garbage
      ) {
        _do_assignment(other);
      }

    }

    template <typename OtherTraits>
    AccessHandleCollection(
      AccessHandleCollection<T, IndexRangeT, OtherTraits> const& other,
      std::enable_if_t<
        traits::template is_convertible_from<OtherTraits>::value
          and not std::is_same<OtherTraits, traits>::value,
        utility::_not_a_type
      > = { }
    )
    {
      auto capture_management_result =
        this->copy_capture_handler_t::handle_compatible_analog_construct(other);

      // If we didn't do a capture and the argument isn't garbage (e.g., from
      // lambda serdes process), then we need to do the normal copy constructor
      // operations
      // Analogous type ctor shouldn't be part of a lambda serdes or anything
      // like that
      assert(not capture_management_result.argument_is_garbage);
      if(not capture_management_result.did_capture) {
        _do_assignment(other);
      }
    }

  // </editor-fold> end public ctors }}}1
  //============================================================================

  public:

    // TODO delete the copy assignment operator
    AccessHandleCollection& operator=(AccessHandleCollection const&) = default;

    AccessHandleCollection& operator=(AccessHandleCollection&&) = default;

    template <typename OtherTraits, typename _Ignored_SFINAE=void,
      typename=std::enable_if_t<
        // TODO generalize this into an is_assignable_from_... metafunction
        traits::semantic_traits::is_outer != OptionalBoolean::KnownTrue
          and traits::semantic_traits::is_outer != OtherTraits::semantic_traits::is_outer
          and std::is_void<_Ignored_SFINAE>::value // should always be true
      >
    >
    AccessHandleCollection& operator=(
      AccessHandleCollection<T, IndexRangeT, OtherTraits> const& other
    ) {
      _do_assignment(other);
      return *this;
    };


  //============================================================================
  // <editor-fold desc="friends"> {{{1

  private:

    template <typename, typename>
    friend struct detail::AccessHandleCollectionCaptureDescription;

    template <typename, typename, typename>
    friend
    class AccessHandleCollection;

    template <typename... AccessorDetails>
    friend
    struct detail::AccessHandleCollectionAccess;

    template <typename, typename, typename, typename>
    friend
    struct detail::_task_collection_impl::_get_storage_arg_helper;

    template <typename, typename, size_t, typename>
    friend
    struct detail::_task_collection_impl::_get_task_stored_arg_helper;

    template <typename, typename, typename>
    friend
    struct detail::_task_collection_impl::_get_call_arg_helper;

    template <typename, typename>
    friend
    struct detail::MappedHandleCollection;

    template <typename, typename>
    friend
    struct detail::IndexedAccessHandle;

    template <typename, typename, typename...>
    friend
    struct detail::TaskCollectionImpl;

    friend
    struct serialization::Serializer<AccessHandleCollection>;


    template <typename, typename...>
    friend struct detail::_commutative_access_impl;

    template <typename, typename...>
    friend struct detail::_noncommutative_collection_access_impl;

    template <typename>
    friend struct detail::CopyCapturedObject;

#if _darma_has_feature(mpi_interop)
    template <typename, typename>
    friend struct PiecewiseCollectionHandle;
#endif // _darma_has_feature(mpi_interop)

  // </editor-fold> end friends }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="private members"> {{{1

  private:

    static constexpr auto
      unknown_backend_index = std::numeric_limits<size_t>::max();

    mutable std::size_t mapped_backend_index_ = unknown_backend_index;

    // TODO get rid of this in favor of mapped/unmapped
    mutable bool dynamic_is_outer =
      traits::semantic_traits::is_outer == OptionalBoolean::KnownTrue;

    mutable std::map<
      typename base_t::index_range_traits_t::index_type,
      typename base_t::element_use_holder_ptr
    > local_use_holders_;

    // TODO move this to a special members class that leaves it out if it's known to be not captured
#if _darma_has_feature(task_collection_token)
    mutable types::task_collection_token_t task_collection_token_;
#endif // _darma_has_feature(task_collection_token)

  // </editor-fold> end private members }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="private implementation detail methods"> {{{1

  private:

    template <typename AccessHandleCollectionT>
    void _do_assignment(AccessHandleCollectionT const& other) {
      this->base_t::_do_assignment(other);
      local_use_holders_ = other.local_use_holders_;
      mapped_backend_index_ = other.mapped_backend_index_;
      dynamic_is_outer = other.dynamic_is_outer;
      #if _darma_has_feature(task_collection_token)
      task_collection_token_ = other.task_collection_token_;
      #endif // _darma_has_feature(task_collection_token)
    }


    void _setup_local_uses(detail::TaskBase& task) const;


    template <typename CompatibleAHC>
    void prepare_for_capture(CompatibleAHC const& copied_from) {
      // Not the same as _do_assignment because it doesn't copy over the Uses
      this->var_handle_base_ = copied_from.var_handle_base_;
      mapped_backend_index_ = copied_from.mapped_backend_index_;
      dynamic_is_outer = copied_from.dynamic_is_outer;
      #if _darma_has_feature(task_collection_token)
      task_collection_token_ = copied_from.task_collection_token_;
      #endif // _darma_has_feature(task_collection_token)
    }

    template <
      typename CompatibleAHC,
      typename CaptureManagerT
    >
    void report_capture(
      CompatibleAHC const* source,
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

    template <typename TaskLikeT>
    void report_dependency(TaskLikeT* task) {
      task->add_dependency(*this->get_current_use()->use_base);
      if(local_use_holders_.size()) {
        for(auto&& pair : local_use_holders_) {
          task->add_dependency(*pair.second->use_base);
        }
      }
    }


    template <typename Archive>
    void unpack_from_archive(Archive& ar) {

      // TODO Better encapsulation here!!!

      types::key_t key = darma_runtime::make_key();
      ar >> key;
      this->var_handle_base_ = std::make_shared<
        ::darma_runtime::detail::VariableHandle<value_type>
      >(key);

      auto use_base = serialization::PolymorphicSerializableObject<detail::HandleUseBase>
        ::unpack(*reinterpret_cast<char const**>(&ar.data_pointer_reference()));

      this->set_current_use(
        base_t::use_holder_t::recreate_migrated(
          std::move(*use_base.get())
        )
      );

    }


  // </editor-fold> end private implementation detail methods }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="private ctors"> {{{1

  private:

    explicit AccessHandleCollection(
      std::shared_ptr<detail::VariableHandleBase> const& var_handle,
      std::shared_ptr<typename base_t::use_holder_t> const& use_holder
    ) : base_t(var_handle, use_holder)
    {
      assert(utility::safe_static_cast<detail::VariableHandle<T>*>(var_handle.get()) != nullptr);
    }

  // </editor-fold> end private ctors }}}1
  //============================================================================
};

// </editor-fold> end AccessHandleCollection
//==============================================================================


//==============================================================================
// <editor-fold desc="is_access_handle_collection"> {{{1

namespace detail {


template <typename T, typename IndexRangeT, typename Traits>
struct is_access_handle_collection<
  AccessHandleCollection<T, IndexRangeT, Traits>
>: std::true_type {
};

template <typename T>
using decayed_is_access_handle_collection = typename is_access_handle_collection<
  std::decay_t<T>
>::type;

template <typename T>
using is_access_handle_collection_d = decayed_is_access_handle<T>;

} // end namespace detail

// </editor-fold> end is_access_handle_collection }}}1
//==============================================================================


//==============================================================================
// <editor-fold desc="initial_access_collection">

namespace detail {

struct initial_access_collection_tag { };

template <typename ValueType, typename... TraitFlags>
struct AccessHandleCollectionAccess<initial_access_collection_tag, ValueType,
  TraitFlags...
> {

  using ah_analog_traits = typename make_access_handle_traits<ValueType,
    TraitFlags...
  >::template from_traits<
    make_access_handle_traits_t<ValueType,
      static_scheduling_permissions<AccessHandlePermissions::Modify>,
      required_scheduling_permissions<AccessHandlePermissions::Modify>
    >
  >::type;

  template <typename IndexRangeT>
  decltype(auto)
  _impl(
    types::key_t const& key,
    IndexRangeT&& range
  ) const {

    using index_range_t = indexing::index_range_type_from_arguments_t<IndexRangeT>;

    auto var_handle = std::make_shared<VariableHandle<ValueType>>(key);

    using return_type = AccessHandleCollection<
      ValueType, index_range_t,
      access_handle_collection_traits<
        ValueType, std::decay_t<index_range_t>,
        typename ah_analog_traits::permissions_traits,
        ahc_traits::semantic_traits<
          /* IsOuter = */ OptionalBoolean::KnownTrue,
          typename ah_analog_traits::semantic_traits,
          /* IsMapped = */ OptionalBoolean::Unknown
        >,
        typename ah_analog_traits::allocation_traits
      >
    >;

    using namespace darma_runtime::abstract::frontend;
    using namespace darma_runtime::detail::flow_relationships;

    auto use_holder = detail::UseHolder<
      BasicCollectionManagingUse<index_range_t>
    >::create(
      make_unmapped_use_collection(
        std::forward<IndexRangeT>(range)
      ),
      var_handle,
      darma_runtime::frontend::Permissions::Modify,
      darma_runtime::frontend::Permissions::None,
      initial_flow().as_collection_relationship(),
      null_flow().as_collection_relationship(),
      insignificant_flow().as_collection_relationship(),
      initial_anti_flow().as_collection_relationship(),
      frontend::CoherenceMode::Sequential
    );

    use_holder->could_be_alias = true;

    return return_type(
      var_handle, std::move(use_holder)
    );
  }

  template <typename IndexRangeT, typename Arg1, typename... Args>
  inline decltype(auto)
  operator()(
    IndexRangeT&& range,
    variadic_arguments_begin_tag,
    Arg1&& arg1,
    Args&&... args
  ) const {

    auto key = make_key(std::forward<Arg1>(arg1), std::forward<Args>(args)...);

    return _impl(key, std::forward<IndexRangeT>(range));

  }

  template <typename IndexRangeT>
  inline decltype(auto)
  operator()(
    IndexRangeT&& range,
    variadic_arguments_begin_tag
  ) const {

    auto key = detail::key_traits<types::key_t>::make_awaiting_backend_assignment_key();

    return _impl(key, std::forward<IndexRangeT>(range));

  }

};

} // end namespace detail


template <typename T, typename... TraitFlags, typename... Args>
auto
initial_access_collection(Args&&... args) {
  using namespace darma_runtime::detail;
  using darma_runtime::keyword_tags_for_create_concurrent_work::index_range;
  using parser = kwarg_parser<
    variadic_positional_overload_description<
      _keyword<deduced_parameter, index_range>
    >
    // TODO other overloads
  >;

  // This is on one line for readability of compiler error; don't respace it please!
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;


  return parser()
    .parse_args(std::forward<Args>(args)...)
    .invoke(detail::AccessHandleCollectionAccess<initial_access_collection_tag,
      T, TraitFlags...
    >());
};

// </editor-fold> end initial_access_collection
//==============================================================================

template <typename T, typename IndexRange>
using ReadAccessHandleCollection = AccessHandleCollection<
  T, IndexRange,
  detail::access_handle_collection_traits<T, IndexRange,
    detail::access_handle_permissions_traits<
      /* Required scheduling = */ detail::AccessHandlePermissions::Read,
      /* Required immediate = */ detail::AccessHandlePermissions::Read,
      /* Static scheduling = */ detail::AccessHandlePermissions::Read,
      /* Static immediate = */ detail::AccessHandlePermissions::Read
    >
    // All of the other categories are defaulted
  >
>;

template <typename T, typename IndexRange,
  typename OtherTraits=detail::make_access_handle_collection_traits_t<T, IndexRange>
>
using CommutativeAccessHandleCollection = typename AccessHandleCollection<
  T, IndexRange
>::template with_traits<
  typename detail::make_access_handle_collection_traits<T, IndexRange,
    detail::required_immediate_permissions<detail::AccessHandlePermissions::None>,
    // For now, at least, a CommutativeAccessHandleCollection can never be indexed
    // and have local_access() called on the indexed return value, so it never
    // has immediate permissions other than None
    detail::static_immediate_permissions<detail::AccessHandlePermissions::None>
  >::template from_traits<OtherTraits>::type
>;

template <typename T, typename IndexRange, typename... TraitsFlags>
using CommutativeAccessHandleCollectionWithTraits = CommutativeAccessHandleCollection<
  T, IndexRange, detail::make_access_handle_collection_traits_t<T, IndexRange, TraitsFlags...>
>;

//==============================================================================
// <editor-fold desc="serialization for AccessHandleCollection"> {{{1

#if _darma_has_feature(task_migration)
namespace serialization {

template <typename T, typename IndexRangeT, typename Traits>
struct Serializer<darma_runtime::AccessHandleCollection<T, IndexRangeT, Traits>> {

  using access_handle_collection_t = darma_runtime::AccessHandleCollection<
    T, IndexRangeT, Traits
  >;


  static void _do_migratability_assertions(access_handle_collection_t const& ahc) {
    DARMA_ASSERT_MESSAGE(
      ahc.mapped_backend_index_ == ahc.unknown_backend_index,
      "Can't migrate a handle collection after it has been mapped to a task"
    );
    DARMA_ASSERT_MESSAGE(
      ahc.local_use_holders_.empty(),
      "Can't migrate a handle collection after it has been mapped to a task"
    );
  }

  template <typename ArchiveT>
  static void _do_compute_size_or_pack(access_handle_collection_t const& ahc, ArchiveT& ar) {
    _do_migratability_assertions(ahc);

    ar | ahc.var_handle_base_->get_key();
    ar | ahc.get_index_range();
    ar | ahc.get_current_use()->use()->scheduling_permissions_;
    ar | ahc.get_current_use()->use()->immediate_permissions_;
    ar | ahc.get_current_use()->use()->coherence_mode_;
    ar | ahc.get_current_use()->use()->in_flow_;
    ar | ahc.get_current_use()->use()->out_flow_;
    ar | ahc.get_current_use()->use()->anti_in_flow_;
    ar | ahc.get_current_use()->use()->anti_out_flow_;
  }

  template <typename ArchiveT>
  static void compute_size(access_handle_collection_t const& ahc, ArchiveT& ar) {
    ar | ahc.var_handle_base_->get_key();
    ar.add_to_size_raw(ahc.current_use_base_->use_base->get_packed_size());
  }

  template <typename ConvertiblePackingArchive>
  _darma_requires(requires(ConvertiblePackingArchive a) {
    PointerReferenceSerializationHandler<>::make_packing_archive(ar);
  })
  static void pack(
    access_handle_collection_t const& val,
    ConvertiblePackingArchive& ar,
    std::enable_if_t<
      not is_packable_with_archive<types::flow_t, ConvertiblePackingArchive>::value
        or not is_packable_with_archive<types::anti_flow_t, ConvertiblePackingArchive>::value,
      darma_runtime::utility::_not_a_type
    > = { }
  ) {
    // Packing flows uses pointer references, so we need to convert to an archive
    // that exposes the pointer (unless the archive type can serialize flows directly
    auto ptr_ar = PointerReferenceSerializationHandler<>::make_packing_archive_referencing(ar);
    ar << val;
  }

  template <typename ArchiveT>
  static void pack(
    access_handle_collection_t const& ahc, ArchiveT& ar,
    std::enable_if_t<
      is_packable_with_archive<types::flow_t, ArchiveT>::value
        and is_packable_with_archive<types::anti_flow_t, ArchiveT>::value,
      darma_runtime::utility::_not_a_type
    > = { }
  ) {
    ar | ahc.var_handle_base_->get_key();
    ahc.current_use_base_->use_base->pack(*reinterpret_cast<char**>(&ar.data_pointer_reference()));
  }

  template <
    typename ConvertibleUnpackingArchive
  >
  _darma_requires(requires(ConvertibleUnpackingArchive a) {
    PointerReferenceSerializationHandler<>::make_unpacking_archive(ar);
  })
  static void unpack(
    void* allocated,
    ConvertibleUnpackingArchive& ar,
    std::enable_if_t<
      not is_unpackable_with_archive<types::flow_t, ConvertibleUnpackingArchive>::value
        or not is_unpackable_with_archive<types::anti_flow_t, ConvertibleUnpackingArchive>::value,
      darma_runtime::utility::_not_a_type
    > = { }
  )
  {
    // Unpacking flows uses pointer references, so we need to convert to an archive
    // that exposes the pointer (unless the archive type can serialize flows directly
    auto ptr_ar = PointerReferenceSerializationHandler<>::make_unpacking_archive_referencing(ar);
    darma_unpack(allocated_buffer_for<access_handle_collection_t>(allocated), ptr_ar);
  }

  template <
    typename ArchiveT,
    typename=std::enable_if_t<
      is_unpackable_with_archive<types::flow_t, ArchiveT>::value
        and is_unpackable_with_archive<types::anti_flow_t, ArchiveT>::value
    >
  >
  static void unpack(void* allocated, ArchiveT& ar) {
    auto& collection = *(new (allocated) access_handle_collection_t());
    collection.unpack_from_archive(ar);
  };

};

} // end namespace serialization
#endif // _darma_has_feature(task_migration)

// </editor-fold> end serialization for AccessHandleCollection (currently disabled code) }}}1
//==============================================================================

} // end namespace darma_runtime

#include <darma/impl/task_collection/access_handle_collection_capture_description.h>

#endif // _darma_has_feature(create_concurrent_work)

#endif //DARMA_HANDLE_COLLECTION_H
