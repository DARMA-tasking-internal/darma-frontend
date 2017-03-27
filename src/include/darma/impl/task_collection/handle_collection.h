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

#include <darma/impl/handle.h>
#include <darma/impl/use.h>
#include <darma/impl/keyword_arguments/parse.h>
#include <darma/impl/keyword_arguments/macros.h>
#include <darma/interface/app/keyword_arguments/index_range.h>
#include <darma/impl/task_collection/task_collection_fwd.h>
#include <darma/impl/keyword_arguments/parse.h>
#include <darma/impl/access_handle_base.h>
#include <darma/impl/util/optional_boolean.h>
#include <darma/impl/use_collection.h>

namespace _darma__errors {

struct _____cannot_capture_return_of_AccessHandleCollection_subscript_directly {
  template <typename T>
  struct __for_access_handle_collection_wrapping_type {
    struct __must_call__local_access__or__read_access__method_first { };
  };
};

} // end namespace _darma__errors

namespace darma_runtime {

namespace detail {

template <
  OptionalBoolean IsOuter = OptionalBoolean::Unknown
>
struct access_handle_collection_traits {
  static constexpr auto is_outer = IsOuter;
};

} // end namespace detail

template <typename T, typename IndexRangeT,
  typename Traits=detail::access_handle_collection_traits<>
>
class AccessHandleCollection;

namespace detail {

template <typename... AccessorDetails>
struct AccessHandleCollectionAccess;

//==============================================================================
// <editor-fold desc="MappedHandleCollection">

template <typename AccessHandleCollectionT, typename Mapping>
struct MappedHandleCollection {
  public:  // all public for now...

    AccessHandleCollectionT collection;
    Mapping mapping;

    using mapping_t = Mapping;
    using access_handle_collection_t = AccessHandleCollectionT;

    // TODO remove meaningless default ctor once I write serdes stuff
    MappedHandleCollection() = default;

    template <typename AccessHandleCollectionTDeduced, typename MappingDeduced>
    MappedHandleCollection(
      AccessHandleCollectionTDeduced&& collection,
      MappingDeduced&& mapping
    ) : collection(std::forward<AccessHandleCollectionTDeduced>(collection)),
        mapping(std::forward<MappingDeduced>(mapping))
    { }

    // Just the compute_size and pack
    template <typename ArchiveT>
    void serialize(ArchiveT& ar) const {
      assert(not ar.is_unpacking());

      ar | mapping;

      ar | collection.var_handle_->get_key();

      ar | collection.get_index_range();

      ar | collection.current_use_->use->scheduling_permissions_;
      ar | collection.current_use_->use->immediate_permissions_;
      DARMA_ASSERT_MESSAGE(
        collection.mapped_backend_index_ == collection.unknown_backend_index,
        "Can't migrate a handle collection after it has been mapped to a task"
      );
      DARMA_ASSERT_MESSAGE(
        collection.local_use_holders_.empty(),
        "Can't migrate a handle collection after it has been mapped to a task"
      );
      auto* backend_runtime = abstract::backend::get_backend_runtime();
      if(ar.is_sizing()) {
        ar.add_to_size_indirect(
          backend_runtime->get_packed_flow_size(
            *collection.current_use_->use->in_flow_
          )
        );
        ar.add_to_size_indirect(
          backend_runtime->get_packed_flow_size(
            *collection.current_use_->use->out_flow_
          )
        );
      }
      else { // ar.is_packing()
        assert(ar.is_packing());
        using serialization::Serializer_attorneys::ArchiveAccess;
        backend_runtime->pack_flow(
          *collection.current_use_->use->in_flow_,
          reinterpret_cast<void*&>(ArchiveAccess::spot(ar))
        );
        backend_runtime->pack_flow(
          *collection.current_use_->use->out_flow_,
          reinterpret_cast<void*&>(ArchiveAccess::spot(ar))
        );
      }

    }

    template <typename ArchiveT>
    static MappedHandleCollection&
    reconstruct(void* allocated, ArchiveT& ar) {
      // just for offsets
      auto* rv_uninitialized = reinterpret_cast<MappedHandleCollection*>(allocated);

      // Mapping need not be default constructible, so unpack it here
      ar >> rv_uninitialized->mapping;

      // Collection is default constructible, so just construct it here
      // and unpack it in unpack
      new (&rv_uninitialized->collection) AccessHandleCollectionT();

      return *rv_uninitialized;
    }

    template <typename ArchiveT>
    void unpack(ArchiveT& ar) {
      // Mapping already unpacked in reconstruct()

      // Set up the access handle collection here, though
      types::key_t key = darma_runtime::make_key();
      ar >> key;
      auto var_handle = std::make_shared<
        detail::VariableHandle<typename AccessHandleCollectionT::value_type>
      >(key);
      collection.var_handle_ = var_handle;

      using handle_range_t = typename AccessHandleCollectionT::index_range_type;
      using handle_range_traits = indexing::index_range_traits<handle_range_t>;
      using handle_range_serdes_traits = serialization::detail::serializability_traits<handle_range_t>;

      // Unpack index range of the handle itself
      char hr_buffer[sizeof(handle_range_t)];
      handle_range_serdes_traits::unpack(reinterpret_cast<void*>(hr_buffer), ar);
      auto& hr = *reinterpret_cast<handle_range_t*>(hr_buffer);

      // unpack permissions
      HandleUse::permissions_t sched_perm, immed_perm;
      ar >> sched_perm >> immed_perm;

      // unpack the flows
      using serialization::Serializer_attorneys::ArchiveAccess;
      auto* backend_runtime = abstract::backend::get_backend_runtime();
      char const*& archive_spot = const_cast<char const*&>(
        ArchiveAccess::spot(ar)
      );
      auto inflow = detail::make_flow_ptr(
        backend_runtime->make_unpacked_flow(
          reinterpret_cast<void const*&>(archive_spot)
        )
      );
      auto outflow = detail::make_flow_ptr(
        backend_runtime->make_unpacked_flow(
          reinterpret_cast<void const*&>(archive_spot)
        )
      );


      // remake the use:
      collection.current_use_ = std::make_shared<
        GenericUseHolder<CollectionManagingUse<handle_range_t>>
      >(
        CollectionManagingUse<handle_range_t>(
          var_handle, inflow, outflow, sched_perm, immed_perm, std::move(hr)
          // the mapping will be re-set up in the task collection unpack,
          // so don't worry about it here
        ),
        /* do_register_in_ctor= */ false // MappedHandleCollection unpack handles this
      );

      // the use will be reregistered after the mapping is added back in, so
      // don't worry about it here

    }

};

// </editor-fold> end MappedHandleCollection
//==============================================================================


//==============================================================================
// <editor-fold desc="IndexedAccessHandle"> {{{1

// Proxy type returned by the AccessHandleCollection index operation
template <typename AccessHandleT>
class IndexedAccessHandle {
  private:

    AccessHandleT access_handle_;
    bool has_local_access_;
    std::size_t backend_index_;

    template <typename, typename, typename>
    friend class darma_runtime::AccessHandleCollection;

    IndexedAccessHandle(
      AccessHandleT&& access_handle,
      bool has_local_access,
      std::size_t backend_index = std::numeric_limits<std::size_t>::max()
    ) : access_handle_(std::move(access_handle)),
        has_local_access_(has_local_access),
        backend_index_(backend_index)
    { }

  public:

    template <typename _for_SFINAE_only=void,
      typename=tinympl::and_<
        std::is_void<_for_SFINAE_only>,
        _darma__static_failure<
          typename
            _darma__errors::_____cannot_capture_return_of_AccessHandleCollection_subscript_directly
              ::template __for_access_handle_collection_wrapping_type<typename AccessHandleT::value_type>
              ::__must_call__local_access__or__read_access__method_first
        >
      >
    >
    IndexedAccessHandle(IndexedAccessHandle const&) { /* unreachable */ }       // LCOV_EXCL_LINE

    IndexedAccessHandle(IndexedAccessHandle&&) = default;

    AccessHandleT
    local_access() const {
      DARMA_ASSERT_MESSAGE(has_local_access_,
        "Attempted to obtain local access to index of AccessHandleCollection"
          " that is not local"
      );
      return access_handle_;
    }

    template <typename... Args>
    typename AccessHandleT::template with_traits<
      typename AccessHandleT::traits
        ::template with_static_immediate_permissions<
          AccessHandlePermissions::Read
        >::type
        ::template with_static_scheduling_permissions<
          AccessHandlePermissions::Read
        >::type
    >
    read_access(Args&&... args) const {
      DARMA_ASSERT_MESSAGE(not has_local_access_,
        "Attempted to fetch an AccessHandle corresponding to an index of an"
        " AccessHandleCollection that is local to the fetching context"
      );

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
            backend_runtime->make_indexed_fetching_flow(
              *access_handle_.current_use_->use->in_flow_.get(),
              version_key, backend_index_
            )
          );

          access_handle_.current_use_->use->in_flow_ = fetched_in_flow;

          access_handle_.current_use_->use->out_flow_ = detail::make_flow_ptr(
            backend_runtime->make_null_flow( access_handle_.var_handle_ )
          );

          access_handle_.current_use_->could_be_alias = true;
          access_handle_.unfetched_ = false;

          // Still, don't register until it gets captured...

          return access_handle_;
        });
    }

};


// </editor-fold> end IndexedAccessHandle }}}1
//==============================================================================

} // end namespace detail


//==============================================================================
// <editor-fold desc="AccessHandleCollection">

template <typename T, typename IndexRangeT,
  typename Traits /*=detail::access_handle_collection_traits<>*/
>
class AccessHandleCollection : public detail::AccessHandleBase {
  public:

    using value_type = T;
    using index_range_type = IndexRangeT;
    using traits_t = Traits;

    template <typename MappingT>
    auto mapped_with(MappingT&& mapping) const {
      return detail::MappedHandleCollection<
        ::darma_runtime::AccessHandleCollection<T, IndexRangeT,
          detail::access_handle_collection_traits<OptionalBoolean::KnownFalse>
        >,
        std::decay_t<MappingT>
      >(
        *this, std::forward<MappingT>(mapping)
      );
    };

    IndexRangeT const& get_index_range() const {
      return current_use_->use->index_range;
    }


  protected:

    using variable_handle_ptr = detail::managing_ptr<
      std::shared_ptr<detail::VariableHandle<T>>,
      std::shared_ptr<detail::VariableHandleBase>
    >;
    using use_holder_ptr = detail::managing_ptr<
      std::shared_ptr<
        detail::GenericUseHolder<detail::CollectionManagingUse<index_range_type>>
      >,
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
        auto const& in_flow, auto const& out_flow,
        auto scheduling_permissions,
        auto immediate_permissions
      ) {
        return darma_runtime::detail::CollectionManagingUse<
          std::decay_t<IndexRangeT>
        >(
          handle, in_flow, out_flow,
          scheduling_permissions, immediate_permissions,
          source->current_use_->use->index_range
        );
      };

      auto next_use_holder_maker = [&](
        auto handle,
        auto const& in_flow, auto const& out_flow,
        auto scheduling_permissions,
        auto immediate_permissions
      ) {
        return std::make_shared<
          darma_runtime::detail::GenericUseHolder<
            darma_runtime::detail::CollectionManagingUse<
              std::decay_t<IndexRangeT>
            >
          >>(
          darma_runtime::detail::CollectionManagingUse<std::decay_t<IndexRangeT>>(
            handle, in_flow, out_flow,
            scheduling_permissions, immediate_permissions,
            source->current_use_->use->index_range
          )
        );
      };

      // Custom "next flow maker"
      auto next_flow_maker = [](auto&& flow, auto* backend_runtime) {
        return darma_runtime::detail::make_flow_ptr(
          backend_runtime->make_next_flow_collection(
            *std::forward<decltype(flow)>(flow).get()
          )
        );
      };

      // Do the capture
      return detail::make_captured_use_holder(
        var_handle_base_,
        req_sched_perms,
        req_immed_perms,
        source->current_use_.get(),
        next_use_holder_maker,
        next_flow_maker,
        continuing_use_maker
      );

    }


  //============================================================================
  // <editor-fold desc="public interface functions"> {{{1

  public:

    template <
      typename _Ignored_SFINAE=void,
      typename=std::enable_if_t<
        traits_t::is_outer != KnownTrue
          and std::is_void<_Ignored_SFINAE>::value // should always be true
      >
    >
    auto
    operator[](typename _range_traits::index_type const& idx) const {
      auto use_iter = local_use_holders_.find(idx);
      if (use_iter == local_use_holders_.end()) {
        // make an unfetched handle

        auto* backend_runtime = abstract::backend::get_backend_runtime();
        // stash the in flow that you should get the fetched flow from in the in flow
        auto unfetched_in_flow = current_use_->use->in_flow_;

        auto const& idx_range = get_index_range();
        auto map_dense = _range_traits::mapping_to_dense(idx_range);
        using _map_traits = indexing::mapping_traits<typename _range_traits::mapping_to_dense_type>;
        auto
          backend_index = _map_traits::map_forward(map_dense, idx, idx_range);

        auto idx_use_holder = std::make_shared<detail::UseHolder>(
          detail::HandleUse(
            var_handle_base_, unfetched_in_flow, unfetched_in_flow,
            detail::HandleUse::Read, detail::HandleUse::None
          )
        );
        // DO NOT REGISTER IT YET!!!

        return detail::IndexedAccessHandle<AccessHandle<T>>(
          AccessHandle<T>(
            detail::unfetched_access_handle_tag{},
            var_handle_.get_smart_ptr(), idx_use_holder
          ), false,
          backend_index
        );
      } else {
        return detail::IndexedAccessHandle<AccessHandle<T>>(
          AccessHandle<T>(
            var_handle_.get_smart_ptr(), use_iter->second
          ), true
        );
      }
    }

    template <typename ReduceOp=detail::op_not_given, typename... Args>
    auto
    reduce(Args&&... args) const  {
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
            HandleUse::None,
            /* requested_immediate_permissions */
            HandleUse::Modify,
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



  // </editor-fold> end public interface functions }}}1
  //============================================================================


  public:

    ~AccessHandleCollection() = default;


  //============================================================================
  // <editor-fold desc="AccessHandleBase pure virtual method overloads"> {{{1

  protected:


    void call_make_captured_use_holder(
      std::shared_ptr<detail::VariableHandleBase> var_handle,
      detail::HandleUse::permissions_t req_sched_perms,
      detail::HandleUse::permissions_t req_immed_perms,
      detail::AccessHandleBase const& source_in
    ) override {
      current_use_ = _call_make_captured_use_holder_impl(
        var_handle,
        std::max(req_sched_perms, req_immed_perms, detail::compatible_permissions_less{}),
        detail::HandleUse::None,
        source_in
      );
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
    { }

    AccessHandleCollection(
      AccessHandleCollection const& other
    ) : mapped_backend_index_(other.mapped_backend_index_),
        var_handle_(var_handle_base_, other.var_handle_),
        current_use_(current_use_base_, other.current_use_),
        local_use_holders_(other.local_use_holders_),
        dynamic_is_outer(other.dynamic_is_outer)
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

        source->captured_as_ |= CapturedAsInfo::ScheduleOnly;

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

    template <typename _Ignored_SFINAE=void, typename OtherTraits>
    AccessHandleCollection(
      AccessHandleCollection<
        T, IndexRangeT, OtherTraits
      > const& other,
      std::enable_if_t<
        traits_t::is_outer != KnownTrue
          and traits_t::is_outer != OtherTraits::is_outer
          and std::is_void<_Ignored_SFINAE>::value, // should always be true
        int
      > = 0
    ) : mapped_backend_index_(other.mapped_backend_index_),
        var_handle_(var_handle_base_, other.var_handle_),
        current_use_(current_use_base_, other.current_use_),
        local_use_holders_(other.local_use_holders_),
        dynamic_is_outer(other.dynamic_is_outer)
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

    AccessHandleCollection& operator=(AccessHandleCollection const&) = default;
    AccessHandleCollection& operator=(AccessHandleCollection&&) = default;
    template <typename OtherTraits, typename _Ignored_SFINAE=void,
      typename=std::enable_if_t<
        traits_t::is_outer != KnownTrue
          and traits_t::is_outer != OtherTraits::is_outer
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

    template <typename, typename>
    friend
    struct detail::MappedHandleCollection;

    template <typename, typename, typename...>
    friend
    struct detail::TaskCollectionImpl;

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
    mutable bool dynamic_is_outer = traits_t::is_outer;

    mutable std::map<
      typename _range_traits::index_type,
      element_use_holder_ptr
    > local_use_holders_;

  // </editor-fold> end private members }}}1
  //============================================================================


  //============================================================================
  // <editor-fold desc="private implementation detail methods"> {{{1

  private:

    void _setup_local_uses(detail::TaskBase& task) const {
      auto& current_use_use = current_use_->use;
      auto
        local_idxs = current_use_use->local_indices_for(mapped_backend_index_);
      auto const& idx_range = get_index_range();
      auto map_dense = _range_traits::mapping_to_dense(idx_range);
      auto* backend_runtime = abstract::backend::get_backend_runtime();
      using _map_traits = indexing::mapping_traits<typename _range_traits::mapping_to_dense_type>;
      for (auto&& idx : local_idxs) {
        auto fe_idx = _map_traits::map_backward(map_dense, idx, idx_range);

        auto local_in_flow = detail::make_flow_ptr(
          backend_runtime->make_indexed_local_flow(
            *current_use_->use->in_flow_.get(), idx
          )
        );

        detail::flow_ptr local_out_flow;
        if (
          current_use_->use->immediate_permissions_ == detail::HandleUse::Modify
            or current_use_->use->scheduling_permissions_
              == detail::HandleUse::Modify
          ) {
          local_out_flow = detail::make_flow_ptr(
            backend_runtime->make_indexed_local_flow(
              *current_use_->use->out_flow_.get(), idx
            )
          );
        } else {
          local_out_flow = local_in_flow;
        }

        // TODO UPDATE THIS!!!
        auto idx_use_holder = std::make_shared<detail::UseHolder>(
          detail::HandleUse(
            var_handle_base_, local_in_flow, local_out_flow,
            current_use_->use->scheduling_permissions_,
            current_use_->use->immediate_permissions_
          )
        );
        idx_use_holder->do_register();
        task.add_dependency(*idx_use_holder->use_base);

        local_use_holders_.insert(std::make_pair(fe_idx, idx_use_holder));

      }
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

template <typename T>
struct is_access_handle_collection: std::false_type {
};

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

template <typename ValueType, typename Traits>
struct AccessHandleCollectionAccess<initial_access_collection_tag, ValueType, Traits> {

  template <typename IndexRangeT>
  decltype(auto) _impl(
    types::key_t const& key,
    IndexRangeT&& range
  ) const {

    auto var_handle = std::make_shared<VariableHandle<ValueType>>(key);

    auto* backend_runtime = abstract::backend::get_backend_runtime();
    auto use_holder = std::make_shared<GenericUseHolder<
      CollectionManagingUse<std::decay_t<IndexRangeT>>
    >>(
      CollectionManagingUse<std::decay_t<IndexRangeT>>(
        var_handle,
        detail::make_flow_ptr(backend_runtime->make_initial_flow_collection(var_handle)),
        detail::make_flow_ptr(backend_runtime->make_null_flow_collection(var_handle)),
        HandleUse::Modify,
        HandleUse::None,
        std::forward<IndexRangeT>(range)
      )
    );

    return AccessHandleCollection<ValueType, std::decay_t<IndexRangeT>, Traits>(
      var_handle, use_holder
    );
  }

  template <typename IndexRangeT, typename Arg1, typename... Args>
  inline auto
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
  inline auto
  operator()(
    IndexRangeT&& range,
    variadic_arguments_begin_tag
  ) const {

    auto key = types::key_t(types::key_t::request_backend_assigned_key_tag{});

    return _impl(key, std::forward<IndexRangeT>(range));

  }

};

} // end namespace detail


template <typename T, typename... Args>
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
      T, detail::access_handle_collection_traits</* IsOuter = */ KnownTrue>
    >());
};


// </editor-fold> end initial_access_collection
//==============================================================================


//==============================================================================
// <editor-fold desc="serialization for AccessHandleCollection (currently disabled code)"> {{{1

//namespace serialization {
//
//template <typename T, typename IndexRangeT>
//struct Serializer<darma_runtime::AccessHandleCollection<T, IndexRangeT>> {
//
//  using access_handle_collection_t = darma_runtime::AccessHandleCollection<
//    T,
//    IndexRangeT
//  >;
//
//  // This is the sizing and packing method in one...
//  template <typename ArchiveT>
//  void compute_size(access_handle_collection_t& ahc, ArchiveT& ar) const {
//    ar % ahc.var_handle_->get_key();
//    ar % ahc.current_use_->use.scheduling_permissions_;
//    ar % ahc.current_use_->use.immediate_permissions_;
//    DARMA_ASSERT_MESSAGE(
//      ahc.mapped_backend_index_ == ahc.unknown_backend_index,
//      "Can't migrate a handle collection after it has been mapped to a task"
//    );
//    DARMA_ASSERT_MESSAGE(
//      ahc.local_use_holders_.empty(),
//      "Can't migrate a handle collection after it has been mapped to a task"
//    );
//    auto* backend_runtime = abstract::backend::get_backend_runtime();
//    ar.add_to_size_indirect(
//      backend_runtime->get_packed_flow_size(
//        *ahc.current_use_->use.in_flow_
//      )
//    );
//    ar.add_to_size_indirect(
//      backend_runtime->get_packed_flow_size(
//        *ahc.current_use_->use.out_flow_
//      )
//    );
//  }
//
//};
//
//} // end namespace serialization

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
