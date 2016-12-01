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

namespace darma_runtime {

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

};

// </editor-fold> end MappedHandleCollection
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
      return current_use_->use.index_range;
    }

    AccessHandleCollection() = default;

  protected:

    using variable_handle_t = detail::VariableHandle<T>;
    using variable_handle_ptr = types::shared_ptr_template<detail::VariableHandle<T>>;
    using use_holder_ptr = types::shared_ptr_template<
      detail::GenericUseHolder<detail::CollectionManagingUse<index_range_type>>
    >;

  private:

    //==========================================================================
    // Customization/interaction point:

    template <typename... AccessorDetails>
    friend struct detail::AccessHandleCollectionAccess;

    template <typename, typename, typename, typename>
    friend struct detail::_task_collection_impl::_get_storage_arg_helper;

    template < typename, typename, size_t, typename >
    friend struct detail::_task_collection_impl::_get_task_stored_arg_helper;

    //==========================================================================

    template <typename, typename, typename...>
    friend struct detail::TaskCollectionImpl;

    //==========================================================================
    // private members:

    static constexpr auto unknown_backend_index = std::numeric_limits<size_t>::max();
    mutable std::size_t mapped_backend_index_ = unknown_backend_index;
    mutable variable_handle_ptr var_handle_ = nullptr;
    mutable use_holder_ptr current_use_ = nullptr;

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

namespace detail {

template <typename T>
struct is_access_handle_collection : std::false_type { };

template <typename T, typename IndexRangeT>
struct is_access_handle_collection<
  AccessHandleCollection<T, IndexRangeT>
> : std::true_type { };

template <typename T>
using decayed_is_access_handle_collection = typename is_access_handle_collection<
  std::decay_t<T>
>::type;

} // end namespace detail

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



} // end namespace darma_runtime

#endif //DARMA_HANDLE_COLLECTION_H
