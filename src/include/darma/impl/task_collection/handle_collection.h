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

#include <darma/impl/handle.h>
#include <darma/impl/use.h>
#include <darma/impl/keyword_arguments/parse.h>
#include <darma/impl/keyword_arguments/macros.h>
#include <darma/interface/app/keyword_arguments/index_range.h>
#include <darma/impl/task_collection/task_collection_fwd.h>
#include <darma/impl/keyword_arguments/parse.h>

namespace _darma__errors {

struct _____cannot_capture_return_of_AccessHandleCollection_subscript_directly {
  template <typename T>
  struct __for_access_handle_collection_wrapping_type {
    struct __must_call__local_access__or__read_access__method_first { };
  };
};

} // end namespace _darma__errors

namespace darma_runtime {

template <typename T, typename IndexRangeT>
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

    template <typename MappingDeduced>
    MappedHandleCollection(
      AccessHandleCollectionT const& collection,
      MappingDeduced&& mapping
    ) : collection(collection),
        mapping(std::forward<MappingDeduced>(mapping))
    { }

    // Just the compute_size and pack
    template <typename ArchiveT>
    void serialize(ArchiveT& ar) const {
      assert(not ar.is_unpacking());

      ar | mapping;

      ar | collection.var_handle_->get_key();

      ar | collection.get_index_range();

      ar | collection.current_use_->use.scheduling_permissions_;
      ar | collection.current_use_->use.immediate_permissions_;
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
            *collection.current_use_->use.in_flow_
          )
        );
        ar.add_to_size_indirect(
          backend_runtime->get_packed_flow_size(
            *collection.current_use_->use.out_flow_
          )
        );
      }
      else { // ar.is_packing()
        assert(ar.is_packing());
        using serialization::Serializer_attorneys::ArchiveAccess;
        backend_runtime->pack_flow(
          *collection.current_use_->use.in_flow_,
          reinterpret_cast<void*&>(ArchiveAccess::spot(ar))
        );
        backend_runtime->pack_flow(
          *collection.current_use_->use.out_flow_,
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
      types::key_t key;
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
        )
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

    template <typename, typename>
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
        ::template with_max_immediate_permissions<
          AccessHandlePermissions::Read
        >::type
        ::template with_max_schedule_permissions<
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
              *access_handle_.current_use_->use.in_flow_.get(),
              version_key, backend_index_
            )
          );

          access_handle_.current_use_->use.in_flow_ = fetched_in_flow;

          access_handle_.current_use_->use.out_flow_ = detail::make_flow_ptr(
            backend_runtime->make_null_flow( access_handle_.var_handle_ )
          );

          access_handle_.current_use_->could_be_alias = true;

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

template <typename T, typename IndexRangeT>
class AccessHandleCollection {
  public:

    using value_type = T;
    using index_range_type = IndexRangeT;

    template <typename MappingT>
    auto mapped_with(MappingT&& mapping) const {
      return detail::MappedHandleCollection<
        ::darma_runtime::AccessHandleCollection<T, IndexRangeT>,
        std::decay_t<MappingT>
      >(
        *this, std::forward<MappingT>(mapping)
      );
    };

    IndexRangeT const& get_index_range() const {
      return current_use_->use.use_->index_range;
    }

    AccessHandleCollection() = default;

  protected:

    using variable_handle_t = detail::VariableHandle<T>;
    using variable_handle_ptr = types::shared_ptr_template<detail::VariableHandle<T>>;
    using use_holder_ptr = types::shared_ptr_template<
      detail::GenericUseHolder<detail::CollectionManagingUse<index_range_type>>
    >;
    using element_use_holder_ptr = types::shared_ptr_template<detail::UseHolder>;

    using _range_traits = indexing::index_range_traits<index_range_type>;

  public:

    auto
    operator[](typename _range_traits::index_type const& idx) const {
      auto use_iter = local_use_holders_.find(idx);
      if(use_iter == local_use_holders_.end()) {
        // make an unfetched handle

        auto* backend_runtime = abstract::backend::get_backend_runtime();
        // stash the in flow that you should get the fetched flow from in the in flow
        auto unfetched_in_flow = current_use_->use.in_flow_;

        // Shouldn't do this...
        //auto fetched_out_flow = detail::make_flow_ptr(
        //  backend_runtime->make_null_flow(
        //    var_handle_
        //  )
        //);

        auto const& idx_range = get_index_range();
        auto map_dense = _range_traits::mapping_to_dense(idx_range);
        using _map_traits = indexing::mapping_traits<typename _range_traits::mapping_to_dense_type>;
        auto backend_index = _map_traits::map_forward(map_dense, idx, idx_range);

        auto idx_use_holder = std::make_shared<detail::UseHolder>(
          detail::HandleUse(
            var_handle_, unfetched_in_flow, unfetched_in_flow,
            detail::HandleUse::Read, detail::HandleUse::None
          )
        );
        // DO NOT REGISTER IT YET!!!

        return detail::IndexedAccessHandle<AccessHandle<T>>(
          AccessHandle<T>(
            detail::unfetched_access_handle_tag{},
            var_handle_, idx_use_holder
          ), false,
          backend_index
        );
      }
      else {
        return detail::IndexedAccessHandle<AccessHandle<T>>(
          AccessHandle<T>(
            var_handle_, use_iter->second
          ), true
        );
      }
    }

    //==========================================================================

    ~AccessHandleCollection() { }

  private:

    //==========================================================================
    // Customization/interaction point:

    template <typename... AccessorDetails>
    friend struct detail::AccessHandleCollectionAccess;

    template <typename, typename, typename, typename>
    friend struct detail::_task_collection_impl::_get_storage_arg_helper;

    template < typename, typename, size_t, typename >
    friend struct detail::_task_collection_impl::_get_task_stored_arg_helper;

    template <typename, typename>
    friend struct detail::MappedHandleCollection;

    //friend struct serialization::Serializer<AccessHandleCollection>;

    //==========================================================================

    template <typename, typename, typename...>
    friend struct detail::TaskCollectionImpl;

    //==========================================================================
    // private members:

    static constexpr auto unknown_backend_index = std::numeric_limits<size_t>::max();

    mutable std::size_t mapped_backend_index_ = unknown_backend_index;
    mutable variable_handle_ptr var_handle_ = nullptr;
    mutable use_holder_ptr current_use_ = nullptr;

    mutable std::map<
      typename _range_traits::index_type,
      element_use_holder_ptr
    > local_use_holders_;


    //==========================================================================
    // private methods:

    void _setup_local_uses(detail::TaskBase& task) const {
      auto& current_use_use = current_use_->use;
      auto local_idxs = current_use_use.use_->local_indices_for(mapped_backend_index_);
      auto const& idx_range = get_index_range();
      auto map_dense = _range_traits::mapping_to_dense(idx_range);
      auto* backend_runtime = abstract::backend::get_backend_runtime();
      using _map_traits = indexing::mapping_traits<typename _range_traits::mapping_to_dense_type>;
      for(auto&& idx : local_idxs) {
        auto fe_idx = _map_traits::map_backward(map_dense, idx, idx_range);

        auto local_in_flow = detail::make_flow_ptr(
          backend_runtime->make_indexed_local_flow(
            *current_use_->use.in_flow_.get(), idx
          )
        );

        detail::flow_ptr local_out_flow;
        if(
          current_use_->use.immediate_permissions_ == detail::HandleUse::Modify
          or current_use_->use.scheduling_permissions_ == detail::HandleUse::Modify
        ) {
          local_out_flow = detail::make_flow_ptr(
            backend_runtime->make_indexed_local_flow(
              *current_use_->use.out_flow_.get(), idx
            )
          );
        }
        else {
          local_out_flow = local_in_flow;
        }

        auto idx_use_holder = std::make_shared<detail::UseHolder>(
          detail::HandleUse(
            var_handle_, local_in_flow, local_out_flow,
            current_use_->use.scheduling_permissions_,
            current_use_->use.immediate_permissions_
          )
        );
        idx_use_holder->do_register();
        task.add_dependency(idx_use_holder->use);

        local_use_holders_.insert(std::make_pair(fe_idx, idx_use_holder));

      }
    }


    //==========================================================================
    // private ctors:

    explicit AccessHandleCollection(
      std::shared_ptr<detail::VariableHandle<value_type>> const& var_handle,
      use_holder_ptr const& use_holder
    ) : var_handle_(var_handle),
        current_use_(use_holder)
    { }
};

// </editor-fold> end AccessHandleCollection
//==============================================================================


//==============================================================================
// <editor-fold desc="is_access_handle_collection"> {{{1

namespace detail {

template <typename T>
struct is_access_handle_collection: std::false_type {
};

template <typename T, typename IndexRangeT>
struct is_access_handle_collection<
  AccessHandleCollection<T, IndexRangeT>
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

template <typename ValueType>
struct AccessHandleCollectionAccess<initial_access_collection_tag, ValueType> {

  template <typename IndexRangeT, typename... Args>
  inline auto
  operator()(
    IndexRangeT&& range,
    variadic_arguments_begin_tag,
    Args&&... args
  ) const {

    auto* backend_runtime = abstract::backend::get_backend_runtime();

    auto var_handle = std::make_shared<VariableHandle<ValueType>>(
      make_key(std::forward<Args>(args)...)
    );

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

    auto rv = AccessHandleCollection<ValueType, std::decay_t<IndexRangeT>>(
      var_handle, use_holder
    );

    return rv;

  }

};

} // end namespace detail


template <typename T, typename... Args>
auto
initial_access_collection(Args&&... args) {
  using namespace darma_runtime::detail;
  using darma_runtime::keyword_tags_for_create_concurrent_region::index_range;
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
    .invoke(detail::AccessHandleCollectionAccess<initial_access_collection_tag, T>());
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


} // end namespace darma_runtime

#endif //DARMA_HANDLE_COLLECTION_H
