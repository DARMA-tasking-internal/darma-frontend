/*
//@HEADER
// ************************************************************************
//
//                      handle_collection.h
//                         DARMA
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
#include <darma/impl/keyword_arguments/parse.h>
#include <darma/impl/keyword_arguments/macros.h>
#include <darma/impl/keyword_arguments/parse.h>
#include <darma/impl/access_handle_base.h>
#include <darma/impl/util/optional_boolean.h>
#include <darma/impl/use_collection.h>

// included from task_collection folder
#include <darma/impl/task_collection/errors.h>
#include <darma/impl/task_collection/access_handle_collection_traits.h>
#include <darma/impl/task_collection/mapped_handle_collection.h>
#include <darma/impl/task_collection/indexed_access_handle.h>

namespace darma_runtime {

namespace detail {

template <typename... AccessorDetails>
struct AccessHandleCollectionAccess;

} // end namespace detail

namespace serialization {

// Forward declaration
template <typename T, typename IndexRangeT, typename Traits>
struct Serializer<darma_runtime::AccessHandleCollection<T, IndexRangeT, Traits>>;

} // end namespace serialization

//==============================================================================
// <editor-fold desc="AccessHandleCollection">

template <typename T, typename IndexRangeT,
  typename Traits /*=detail::access_handle_collection_traits<>*/
>
class AccessHandleCollection : public detail::AccessHandleBase {
  public:

    using value_type = T;
    using index_range_type = IndexRangeT;
    using traits_t = Traits; // TODO remove this one
    using traits = Traits;

    template <typename... NewTraitsFlags>
    using with_trait_flags = AccessHandleCollection<
      T, IndexRangeT, typename detail::make_access_handle_collection_traits<
        T, IndexRangeT, NewTraitsFlags...
      >::template from_traits<traits_t>::type
    >;

    template <typename NewTraits>
    using with_traits = AccessHandleCollection<T, IndexRangeT, NewTraits>;

  protected:

    using variable_handle_ptr = detail::managing_ptr<
      std::shared_ptr<detail::VariableHandle<T>>,
      std::shared_ptr<detail::VariableHandleBase>
    >;
    using use_t = detail::CollectionManagingUse<index_range_type>;
    using use_holder_t = detail::GenericUseHolder<use_t>;
    using use_holder_ptr = detail::managing_ptr<
      std::shared_ptr<use_holder_t>,
      detail::UseHolderBase*
    >;
    using element_use_holder_ptr = types::shared_ptr_template<detail::UseHolder>;

    using _range_traits = indexing::index_range_traits<index_range_type>;

    auto
    _call_make_captured_use_holder_impl(
      std::shared_ptr<detail::VariableHandleBase> var_handle,
      detail::HandleUse::permissions_t req_sched_perms,
      detail::HandleUse::permissions_t req_immed_perms,
      detail::AccessHandleBase const& source_in
    ) const {
      auto* source =
        detail::safe_static_cast<AccessHandleCollection const*>(&source_in);

      auto continuing_use_maker = [&](
        auto handle,
        auto scheduling_permissions,
        auto immediate_permissions,
        auto&& in_flow_rel,
        auto&& out_flow_rel
#if _darma_has_feature(anti_flows)
        , auto&& anti_in_flow_rel,
        auto&& anti_out_flow_rel
#endif // _darma_has_feature(anti_flows)
      ) {
        return darma_runtime::detail::CollectionManagingUse<
          std::decay_t<IndexRangeT>
        >(
          handle,
          scheduling_permissions, immediate_permissions,
          std::move(in_flow_rel).as_collection_relationship(),
          std::move(out_flow_rel).as_collection_relationship(),
#if _darma_has_feature(anti_flows)
          std::move(anti_in_flow_rel).as_collection_relationship(),
          std::move(anti_out_flow_rel).as_collection_relationship(),
#endif // _darma_has_feature(anti_flows)
          source->current_use_->use->index_range
        );
      };

      auto next_use_holder_maker = [&](
        auto handle,
        auto scheduling_permissions,
        auto immediate_permissions,
        auto&& in_flow_rel,
        auto&& out_flow_rel
 #if _darma_has_feature(anti_flows)
        , auto&& anti_in_flow_rel,
        auto&& anti_out_flow_rel
#endif // _darma_has_feature(anti_flows)
      ) {
        return std::make_shared<
          darma_runtime::detail::GenericUseHolder<
            darma_runtime::detail::CollectionManagingUse<
              std::decay_t<IndexRangeT>
            >
          >>(
          darma_runtime::detail::CollectionManagingUse<std::decay_t<IndexRangeT>>(
            handle,
            scheduling_permissions, immediate_permissions,
            std::move(in_flow_rel).as_collection_relationship(),
            std::move(out_flow_rel).as_collection_relationship(),
#if _darma_has_feature(anti_flows)
            std::move(anti_in_flow_rel).as_collection_relationship(),
            std::move(anti_out_flow_rel).as_collection_relationship(),
#endif // _darma_has_feature(anti_flows)
            source->current_use_->use->index_range
          ), true, true
        );
      };

      // Do the capture
      return detail::make_captured_use_holder(
        var_handle_base_,
        req_sched_perms,
        req_immed_perms,
        source->current_use_.get(),
        next_use_holder_maker,
        continuing_use_maker
      );

    }


  //============================================================================
  // <editor-fold desc="public interface functions"> {{{1

  public:

    template <typename MappingT>
    auto
    mapped_with(MappingT&& mapping) const {
      return detail::MappedHandleCollection<
        darma_runtime::AccessHandleCollection<T, IndexRangeT,
          detail::access_handle_collection_traits<T, IndexRangeT,
            typename traits_t::permissions_traits,
            detail::ahc_traits::semantic_traits<
              OptionalBoolean::KnownFalse,
              typename traits_t::semantic_traits::handle_semantic_traits
            >,
            typename traits_t::allocation_traits
          >
        >,
        std::decay_t<MappingT>
      >(
        *this, std::forward<MappingT>(mapping)
      );
    };


    IndexRangeT const&
    get_index_range() const {
      return current_use_->use->index_range;
    }


    template <
      typename _Ignored_SFINAE=void,
      typename=std::enable_if_t<
        std::is_void<_Ignored_SFINAE>::value // should always be true
          and traits_t::semantic_traits::is_outer != KnownTrue
      >
    >
    auto
    operator[](typename _range_traits::index_type const& idx) const {
      using return_type = detail::IndexedAccessHandle<
        AccessHandleCollection,
        element_use_holder_ptr
      >;

      auto use_iter = local_use_holders_.find(idx);
      if (use_iter == local_use_holders_.end()) {

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // make an unfetched handle

        auto const& idx_range = get_index_range();
        auto map_dense = _range_traits::mapping_to_dense(idx_range);
        using _map_traits = indexing::mapping_traits<typename _range_traits::mapping_to_dense_type>;
        auto
          backend_index = _map_traits::map_forward(map_dense, idx, idx_range);

        return return_type(
          *this,
          nullptr,
          /* has_local_access = */ false,
          backend_index
        );
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      } else {

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Local access case
        return return_type(
          *this,
          use_iter->second,
          /* has local access = */ true
        );
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      }
    }


#if _darma_has_feature(handle_collection_based_collectives)
    template <typename ReduceOp=detail::op_not_given, typename... Args>
    auto
    reduce(Args&&... args) const {
      using namespace darma_runtime::detail;
      using parser = detail::kwarg_parser<
        overload_description<
          _keyword< /* required for now */
            deduced_parameter, darma_runtime::keyword_tags_for_collectives::output
          >,
          _optional_keyword<
            converted_parameter, darma_runtime::keyword_tags_for_collectives::tag
          >
        >
      >;
      using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

      // TODO somehow check if we're inside a task collection, and fail if we're not

      return parser()
        .with_converters(
          [](auto&&... parts) {
            return darma_runtime::make_key(std::forward<decltype(parts)>(parts)...);
          }
        )
        .with_default_generators(
          darma_runtime::keyword_arguments_for_collectives::tag=[]{
            return darma_runtime::make_key();
          }
        )
        .parse_args(std::forward<Args>(args)...)
        .invoke([this](
          auto& output_handle,
          types::key_t const& tag
        ) -> decltype(auto) {

          auto* backend_runtime = abstract::backend::get_backend_runtime();

          auto cap_result_holder = detail::make_captured_use_holder(
            output_handle.var_handle_,
            /* requested_scheduling_permissions */
            darma_runtime::detail::HandleUse::None,
            /* requested_immediate_permissions */
            darma_runtime::detail::HandleUse::Modify,
            output_handle.current_use_.get()
          );

          auto cap_collection_holder = this->_call_make_captured_use_holder_impl(
            this->var_handle_base_,
            detail::HandleUse::None,
            detail::HandleUse::Read,
            *this
          );

          assert(cap_collection_holder->use->scheduling_permissions_ == detail::HandleUse::None);
          assert(cap_collection_holder->use->immediate_permissions_ == detail::HandleUse::Read);

          // piece and n_pieces are ignored now
          auto coll_dets = detail::_get_collective_details_t<
            ReduceOp,
            AccessHandleCollection,
            decltype(output_handle)
          >(0, 0);
          auto* rt = abstract::backend::get_backend_runtime();
          rt->reduce_collection_use(
            cap_collection_holder->use.release_smart_ptr(),
            cap_result_holder->use.release_smart_ptr(),
            &coll_dets,
            tag
          );

          assert(cap_collection_holder->use.get_smart_ptr().get() == nullptr);
          assert(cap_result_holder->use.get_smart_ptr().get() == nullptr);

          cap_collection_holder->is_registered = false;
          cap_collection_holder->could_be_alias = false;

          cap_result_holder->is_registered = false;
          cap_result_holder->could_be_alias = false;
        });



    }
#endif //_darma_has_feature(handle_collection_based_collectives)


  // </editor-fold> end public interface functions }}}1
  //============================================================================


  public:

    ~AccessHandleCollection() = default;


  //============================================================================
  // <editor-fold desc="AccessHandleBase pure virtual method overloads"> {{{1

  protected:

    void call_add_dependency(
      detail::TaskBase* task
    ) override {
      if(dynamic_is_outer) {
        task->add_dependency(*current_use_base_->use_base);
      }
      else {
        for(auto&& local_holder_pair : local_use_holders_) {
          task->add_dependency(*local_holder_pair.second->use_base);
        }
      }
    }


    void call_make_captured_use_holder(
      std::shared_ptr<detail::VariableHandleBase> var_handle,
      detail::HandleUse::permissions_t req_sched_perms,
      detail::HandleUse::permissions_t req_immed_perms,
      detail::AccessHandleBase const& source_in
    ) override {
      if(dynamic_is_outer) {
        assert(local_use_holders_.size() == 0);
        current_use_ = _call_make_captured_use_holder_impl(
          var_handle,
          std::max(req_sched_perms, req_immed_perms, detail::compatible_permissions_less{}),
          detail::HandleUse::None,
          source_in
        );
      }
      else {
        //DARMA_ASSERT_NOT_IMPLEMENTED("AccessHandleCollection capture in task inside create_concurrent_work");
        auto& source = reinterpret_cast<AccessHandleCollection const&>(source_in);
        for(auto&& local_holder_pair : source.local_use_holders_) {
          local_use_holders_[local_holder_pair.first] = detail::make_captured_use_holder(
            var_handle,
            req_sched_perms,
            req_immed_perms,
            local_holder_pair.second.get()
          );
          // TODO !!! register a new Use for the collection as well
          //current_use_ = _call_make_captured_use_holder_impl(
          //  var_handle,
          //  std::max(req_sched_perms, req_immed_perms, detail::compatible_permissions_less{}),
          //  detail::HandleUse::None,
          //  source_in
          //);
        }

        // TODO Finish this!!!
      }
    }

    std::shared_ptr<detail::AccessHandleBase>
    copy(
      bool check_context = true
    ) const override {
      if (check_context) {
        return std::allocate_shared<AccessHandleCollection>(
          darma_runtime::serialization::darma_allocator<AccessHandleCollection>{},
          *this
        );
      } else {
        auto rv = std::allocate_shared<AccessHandleCollection>(
          darma_runtime::serialization::darma_allocator<AccessHandleCollection>{}
        );
        rv->current_use_ = current_use_;
        rv->var_handle_ = var_handle_;
        rv->local_use_holders_ = local_use_holders_;
        rv->mapped_backend_index_ = mapped_backend_index_;
        rv->dynamic_is_outer = dynamic_is_outer;
        return rv;
      }
    }

    void
    replace_use_holder_with(detail::AccessHandleBase const& other_handle) override {
      current_use_ = detail::safe_static_cast<AccessHandleCollection const*>(
        &other_handle
      )->current_use_;
    }

    void
    release_current_use() const override {
      current_use_ = nullptr;
    }

  // </editor-fold> end AccessHandleBase pure virtual method overloads }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="public ctors"> {{{1

  public:

    AccessHandleCollection()
      : current_use_(current_use_base_),
        var_handle_(var_handle_base_)
    { }

    AccessHandleCollection(
      AccessHandleCollection&& other
    ) : mapped_backend_index_(std::move(other.mapped_backend_index_)),
        var_handle_(var_handle_base_, std::move(other.var_handle_)),
        current_use_(current_use_base_, std::move(other.current_use_)),
        local_use_holders_(std::move(other.local_use_holders_)),
        dynamic_is_outer(std::move(other.dynamic_is_outer)),
        copied_from(std::move(other.copied_from))
#if _darma_has_feature(task_collection_token)
        , task_collection_token_(std::move(other.task_collection_token_))
#endif // _darma_has_feature(task_collection_token)
    { }

    AccessHandleCollection(
      AccessHandleCollection const& other
    ) : mapped_backend_index_(other.mapped_backend_index_),
        var_handle_(var_handle_base_, other.var_handle_),
        current_use_(current_use_base_, other.current_use_),
        local_use_holders_(other.local_use_holders_),
        dynamic_is_outer(other.dynamic_is_outer)
#if _darma_has_feature(task_collection_token)
        , task_collection_token_(std::move(other.task_collection_token_))
#endif // _darma_has_feature(task_collection_token)
    {
      // get the shared_ptr from the weak_ptr stored in the runtime object
      detail::TaskBase* running_task = static_cast<detail::TaskBase* const>(
        abstract::backend::get_backend_context()->get_running_task()
      );
      darma_runtime::detail::TaskBase* capturing_task = nullptr;
      if (running_task) {
        capturing_task = running_task->current_create_work_context;
      }

      if (capturing_task != nullptr) {
        AccessHandleCollection const* source = &other;
        if (capturing_task->is_double_copy_capture) {
          assert(other.copied_from != nullptr);
          source = other.copied_from;
        }

        if(dynamic_is_outer) {
          source->captured_as_ |= CapturedAsInfo::ScheduleOnly;
        }

        capturing_task->do_capture(*this, *source);

        if(source->current_use_) {
          source->current_use_->use->already_captured = true;
          capturing_task->uses_to_unmark_already_captured.push_back(
            source->current_use_->use.get()
          );
        }

      } else {
        copied_from = &other;
        current_use_ = other.current_use_;
      }

    }

    template <typename OtherTraits>
    AccessHandleCollection(
      AccessHandleCollection<T, IndexRangeT, OtherTraits> const& other,
      std::enable_if_t<
        traits_t::template is_convertible_from<OtherTraits>::value
          and not std::is_same<OtherTraits, traits_t>::value,
        detail::_not_a_type
      > = { }
    ) : mapped_backend_index_(other.mapped_backend_index_),
        var_handle_(var_handle_base_, other.var_handle_),
        current_use_(current_use_base_, other.current_use_),
        local_use_holders_(other.local_use_holders_),
        dynamic_is_outer(other.dynamic_is_outer)
#if _darma_has_feature(task_collection_token)
        , task_collection_token_(other.task_collection_token_)
#endif // _darma_has_feature(task_collection_token)
    {
      // get the shared_ptr from the weak_ptr stored in the runtime object
      detail::TaskBase* running_task = static_cast<detail::TaskBase* const>(
        abstract::backend::get_backend_context()->get_running_task()
      );
      darma_runtime::detail::TaskBase* capturing_task = nullptr;
      if (running_task) {
        capturing_task = running_task->current_create_work_context;
      }

      if (capturing_task != nullptr) {
        DARMA_ASSERT_FAILURE("Shouldn't be capturing here");
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
        traits_t::semantic_traits::is_outer != KnownTrue
          and traits_t::semantic_traits::is_outer != OtherTraits::semantic_traits::is_outer
          and std::is_void<_Ignored_SFINAE>::value // should always be true
      >
    >
    AccessHandleCollection& operator=(
      AccessHandleCollection<T, IndexRangeT, OtherTraits> const& other
    ) {
      this->~AccessHandleCollection();
      new (this) AccessHandleCollection(other);
      return *this;
    };


  //============================================================================
  // <editor-fold desc="friends"> {{{1

  private:

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

  // </editor-fold> end friends }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="private members"> {{{1

  private:

    static constexpr auto
      unknown_backend_index = std::numeric_limits<size_t>::max();

    mutable std::size_t mapped_backend_index_ = unknown_backend_index;
    mutable variable_handle_ptr var_handle_ = {var_handle_base_};
    mutable use_holder_ptr current_use_ = {current_use_base_};
    mutable AccessHandleCollection const* copied_from = nullptr;
    mutable bool dynamic_is_outer =
      traits_t::semantic_traits::is_outer == KnownTrue;

    mutable std::map<
      typename _range_traits::index_type,
      element_use_holder_ptr
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

    void _setup_local_uses(detail::TaskBase& task) const {
      auto local_idxs =
        current_use_->use->local_indices_for(mapped_backend_index_);

      auto const& idx_range = get_index_range();

      auto map_dense = _range_traits::mapping_to_dense(idx_range);

      using _map_traits =
        indexing::mapping_traits<typename _range_traits::mapping_to_dense_type>;

      for (auto&& idx : local_idxs) {
        auto fe_idx = _map_traits::map_backward(map_dense, idx, idx_range);

        using namespace darma_runtime::detail::flow_relationships;

        if(
          current_use_->use->immediate_permissions_ == detail::HandleUse::Modify
            or current_use_->use->scheduling_permissions_ == detail::HandleUse::Modify
        ) {

          auto idx_use_holder = std::make_shared<detail::UseHolder>(
            detail::HandleUse(
              var_handle_base_,
              current_use_->use->scheduling_permissions_,
              current_use_->use->immediate_permissions_,
              // In relationship
              indexed_local_flow(&current_use_->use->in_flow_, idx),
              //FlowRelationship::IndexedLocal, &current_use_->use->in_flow_,
              // Out relationship
              indexed_local_flow(&current_use_->use->out_flow_, idx)
              //FlowRelationship::IndexedLocal, &current_use_->use->out_flow_, false,
#if _darma_has_feature(anti_flows)
              ,
              /* anti-in flow */
              indexed_local_anti_flow(&current_use_->use->anti_in_flow_, idx),
              //* anti-out flow */
              indexed_local_anti_flow(&current_use_->use->anti_out_flow_, idx)

#endif // _darma_has_feature(anti_flows)
            ),
            /* reg_in_ctor = */ true,
            /* will_be_dependency = */ true
          );
          // TODO change this to call_add_dependency
          task.add_dependency(*idx_use_holder->use_base);
          local_use_holders_.insert(std::make_pair(fe_idx, idx_use_holder));

        }
        else {

          auto idx_use_holder = std::make_shared<detail::UseHolder>(
            detail::HandleUse(
              var_handle_base_,
              current_use_->use->scheduling_permissions_,
              current_use_->use->immediate_permissions_,
              // In relationship and source
              indexed_local_flow(&current_use_->use->in_flow_, idx),
              //FlowRelationship::IndexedLocal, &current_use_->use->in_flow_,
#if _darma_has_feature(anti_flows)
              // out relationship
              insignificant_flow(),
              // anti-in relationship
              insignificant_flow(),
              // anti-out
              indexed_local_anti_flow(&current_use_->use->anti_out_flow_, idx)
#else
              // Out relationship
              same_flow_as_in()
              //FlowRelationship::Same, nullptr, true,
#endif // _darma_has_feature(anti_flows)
            ),
            /* reg_in_ctor = */ true,
            /* will_be_dependency = */ true
          );
          task.add_dependency(*idx_use_holder->use_base);
          local_use_holders_.insert(std::make_pair(fe_idx, idx_use_holder));
        }

      } // end loop over local idxs

    }



  // </editor-fold> end private implementation detail methods }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="private ctors"> {{{1

  private:

    explicit AccessHandleCollection(
      std::shared_ptr<detail::VariableHandle<value_type>> const& var_handle,
      typename use_holder_ptr::smart_ptr_t const& use_holder
    ) : var_handle_(var_handle_base_, var_handle),
        current_use_(current_use_base_, use_holder)
    { }

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

    auto var_handle = std::make_shared<VariableHandle<ValueType>>(key);

    using return_type = AccessHandleCollection<
      ValueType, std::decay_t<IndexRangeT>,
      access_handle_collection_traits<ValueType, std::decay_t<IndexRangeT>,
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

    auto use_holder = std::make_shared<GenericUseHolder<
      CollectionManagingUse<std::decay_t<IndexRangeT>>
    >>(
      CollectionManagingUse<std::decay_t<IndexRangeT>>(
        var_handle,
        darma_runtime::detail::HandleUse::Modify,
        darma_runtime::detail::HandleUse::None,
        initial_flow().as_collection_relationship(),
        //FlowRelationship::InitialCollection, nullptr,
        null_flow().as_collection_relationship(),
        //FlowRelationship::NullCollection, nullptr, false,
#if _darma_has_feature(anti_flows)
        // anti-in
        insignificant_flow().as_collection_relationship(),
        // anti-out
        initial_anti_flow().as_collection_relationship(),
#endif // _darma_has_feature(anti_flows)
        std::forward<IndexRangeT>(range)
      )
    );

    use_holder->could_be_alias = true;

    return return_type(
      std::move(var_handle), std::move(use_holder)
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

    auto key = types::key_t(types::key_t::request_backend_assigned_key_tag{});

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
    detail::static_immediate_permissions<detail::AccessHandlePermissions::None>,
    detail::required_scheduling_permissions<detail::AccessHandlePermissions::Commutative>,
    detail::static_scheduling_permissions<detail::AccessHandlePermissions::Commutative>
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


  void _do_migratability_assertions(access_handle_collection_t const& ahc) const {
    DARMA_ASSERT_MESSAGE(
      ahc.mapped_backend_index_ == ahc.unknown_backend_index,
      "Can't migrate a handle collection after it has been mapped to a task"
    );
    DARMA_ASSERT_MESSAGE(
      ahc.local_use_holders_.empty(),
      "Can't migrate a handle collection after it has been mapped to a task"
    );
  }

  // This is the sizing and packing method in one...
  template <typename ArchiveT>
  void compute_size(access_handle_collection_t const& ahc, ArchiveT& ar) const {

    _do_migratability_assertions(ahc);

    ar % ahc.var_handle_->get_key();
    ar % ahc.get_index_range();
    ar % ahc.current_use_->use->scheduling_permissions_;
    ar % ahc.current_use_->use->immediate_permissions_;

    auto* backend_runtime = abstract::backend::get_backend_runtime();

    ar.add_to_size_indirect(
      backend_runtime->get_packed_flow_size(
        ahc.current_use_->use->in_flow_
      )
    );
    ar.add_to_size_indirect(
      backend_runtime->get_packed_flow_size(
        ahc.current_use_->use->out_flow_
      )
    );
  }

  template <typename ArchiveT>
  void pack(access_handle_collection_t const& ahc, ArchiveT& ar) const {
    _do_migratability_assertions(ahc);

    ar << ahc.var_handle_->get_key();
    ar << ahc.get_index_range();
    ar << ahc.current_use_->use->scheduling_permissions_;
    ar << ahc.current_use_->use->immediate_permissions_;

    auto* backend_runtime = abstract::backend::get_backend_runtime();
    using serialization::Serializer_attorneys::ArchiveAccess;

    backend_runtime->pack_flow(
      ahc.current_use_->use->in_flow_,
      reinterpret_cast<void*&>(ArchiveAccess::spot(ar))
    );
    backend_runtime->pack_flow(
      ahc.current_use_->use->out_flow_,
      reinterpret_cast<void*&>(ArchiveAccess::spot(ar))
    );
  }

  template <typename ArchiveT, typename AllocatorT>
  void unpack(void* allocated, ArchiveT& ar, AllocatorT const& alloc) const {
    // TODO allocator support

    auto& collection = *(new (allocated) access_handle_collection_t());

    types::key_t key = darma_runtime::make_key();
    ar >> key;
    collection.var_handle_ = std::make_shared<
      ::darma_runtime::detail::VariableHandle<typename access_handle_collection_t::value_type>
    >(key);

    using handle_range_t = typename access_handle_collection_t::index_range_type;
    using handle_range_traits = indexing::index_range_traits<handle_range_t>;
    using handle_range_serdes_traits = serialization::detail::serializability_traits<handle_range_t>;

    // Unpack index range of the handle itself
    char hr_buffer[sizeof(handle_range_t)];
    handle_range_serdes_traits::unpack(reinterpret_cast<void*>(hr_buffer), ar);
    auto& hr = *reinterpret_cast<handle_range_t*>(hr_buffer);

    // unpack permissions
    darma_runtime::detail::HandleUse::permissions_t sched_perm, immed_perm;
    ar >> sched_perm >> immed_perm;

    // unpack the flows
    using serialization::Serializer_attorneys::ArchiveAccess;
    auto* backend_runtime = abstract::backend::get_backend_runtime();
    char const*& archive_spot = const_cast<char const*&>(
      ArchiveAccess::spot(ar)
    );
    auto inflow = backend_runtime->make_unpacked_flow(
      reinterpret_cast<void const*&>(archive_spot)
    );
    auto outflow = backend_runtime->make_unpacked_flow(
      reinterpret_cast<void const*&>(archive_spot)
    );


    // remake the use:
    collection.current_use_ = std::make_shared<
      darma_runtime::detail::GenericUseHolder<
        darma_runtime::detail::CollectionManagingUse<handle_range_t>
      >
    >(
      darma_runtime::detail::CollectionManagingUse<handle_range_t>(
        collection.var_handle_.get_smart_ptr(), sched_perm, immed_perm,
        std::move(inflow), std::move(outflow), std::move(hr)
        // the mapping will be re-set up in the task collection unpack,
        // so don't worry about it here
      ),
      // The task collection unpack handles registration for mapped handles;
      // otherwise, we need to register here
      /* do_register_in_ctor= */ access_handle_collection_t
        ::traits_t::semantic_traits::is_mapped != KnownTrue
    );

  };

};

} // end namespace serialization
#endif // _darma_has_feature(task_migration)

// </editor-fold> end serialization for AccessHandleCollection (currently disabled code) }}}1
//==============================================================================

//template <typename T, typename Traits>
//template <typename AccessHandleCollectionT, typename ReduceOp, detail::handle_collective_label_t Label>
//AccessHandle<T, Traits>::AccessHandle(
//  detail::_collective_awaiting_assignment<AccessHandleCollectionT, ReduceOp, Label>&& awaiting
//) : current_use_(current_use_base_)
//{
//  var_handle_ = std::make_shared<detail::VariableHandle<T>>(
//    detail::key_traits<types::key_t>::make_awaiting_backend_assignment_key()
//  );
//  var_handle_base_ = var_handle_;
//  auto* runtime = abstract::backend::get_backend_runtime();
//  auto in_flow = detail::make_flow_ptr(runtime->make_initial_flow(var_handle_));
//  auto out_flow = detail::make_next_flow_ptr(in_flow, runtime);
//
//  // TODO make sure using an initial flow this way doesn't mess up the backend
//  current_use_ = detail::UseHolder(detail::HandleUse(
//    var_handle_,
//    in_flow, out_flow,
//    detail::HandleUse::None, /* scheduling permissions */
//    detail::HandleUse::Modify /* immediate permissions */ // TODO change this to Write
//  ));
//
//  darma_runtime::detail::SimpleCollectiveDetails<ReduceOp, T>(
//
//  );
//  runtime->allreduce_collection_use(
//    awaiting.collection.current_use_.get(),
//    current_use_.get(),
//
//
//  )

//};

} // end namespace darma_runtime

#endif // _darma_has_feature(create_concurrent_work)

#endif //DARMA_HANDLE_COLLECTION_H
