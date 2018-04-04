/*
//@HEADER
// ************************************************************************
//
//              piecewise_collection_handle.h
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

#ifndef DARMA_PIECEWISE_COLLECTION_HANDLE_H
#define DARMA_PIECEWISE_COLLECTION_HANDLE_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(mpi_interop)

#include <darma/keyword_arguments/parse.h>
#include <darma/keyword_arguments/macros.h>
#include <darma/impl/task_collection/access_handle_collection.h>
#include <darma/interface/backend/mpi_interop.h>

DeclareDarmaTypeTransparentKeyword(piecewise_handle, index);
DeclareDarmaTypeTransparentKeyword(piecewise_handle, copy_callback);
DeclareDarmaTypeTransparentKeyword(piecewise_handle, copy_back_callback);

namespace darma_runtime {

namespace detail {

struct _default_callback {
  auto operator()() {return nullptr;}
};

template <typename ValueType>
struct 
_piecewise_collection_handle_impl {
 
  public:
 
    _piecewise_collection_handle_impl(
      types::runtime_context_token_t context_token,
      types::piecewise_collection_token_t collection_token
    ) : context_token_(context_token),
        collection_token_(collection_token)
    { }

  public:
 
    template<
      typename CopyCallbackT,
      typename CopyBackCallbackT
    >
    void operator()(
      size_t index,
      CopyCallbackT&& copy_callback,
      CopyBackCallbackT&& copy_back_callback,  
      variadic_arguments_begin_tag, 
      ValueType& data
    ) { 
      darma_runtime::backend::register_piecewise_collection_piece(
        context_token_,
        collection_token_,
        index,
        data,
        std::forward<CopyCallbackT>(copy_callback),
        std::forward<CopyBackCallbackT>(copy_back_callback)
      );
    }

  private:
 
    mutable types::runtime_context_token_t context_token_;
    mutable types::piecewise_collection_token_t collection_token_;

};

} // end namespace detail

template<
  typename ValueType,
  typename IndexRangeT
>
class PiecewiseCollectionHandle {

  private:

    // AccessHandleCollection type for conversion operator
    using ah_analog_traits = typename detail::make_access_handle_traits<ValueType
    >::template from_traits<
      detail::make_access_handle_traits_t<ValueType,
        detail::static_scheduling_permissions<detail::AccessHandlePermissions::Modify>,
        detail::required_scheduling_permissions<detail::AccessHandlePermissions::Modify>
      >
    >::type;

    using access_handle_collection_t = AccessHandleCollection<
      ValueType, IndexRangeT,
      detail::access_handle_collection_traits<
        ValueType, std::decay_t<IndexRangeT>,
        typename ah_analog_traits::permissions_traits,
        detail::ahc_traits::semantic_traits<
          /* IsOuter = */ OptionalBoolean::KnownTrue,
          typename ah_analog_traits::semantic_traits,
          /* IsMapped = */ OptionalBoolean::Unknown
        >,
        typename ah_analog_traits::allocation_traits
      >
    >; 

  public: 

    PiecewiseCollectionHandle() = delete;

  public:

    template <typename IndexRangeDeducedT> 
    explicit PiecewiseCollectionHandle(
      std::shared_ptr<detail::VariableHandle<ValueType>> const& var_handle,
      IndexRangeDeducedT range,
      types::runtime_context_token_t context_token,
      types::piecewise_collection_token_t collection_token
    ) : var_handle_(var_handle),
        range_(std::forward<IndexRangeDeducedT>(range)), 
        context_token_(context_token),
        collection_token_(collection_token) 
    {

      // TODO: change flow relationships
      using namespace darma_runtime::detail;
      using namespace darma_runtime::detail::flow_relationships;
 
      use_holder_ = detail::UseHolder<
        BasicCollectionManagingUse<IndexRangeT>
      >::create_with_unregistered_use(
        make_unmapped_use_collection(
          std::forward<IndexRangeT>(range_)
        ),
        var_handle_,
        darma_runtime::frontend::Permissions::Modify,
        darma_runtime::frontend::Permissions::None,
        initial_imported_flow().as_collection_relationship(),
        null_flow().as_collection_relationship(),
        insignificant_flow().as_collection_relationship(),
        initial_imported_anti_flow().as_collection_relationship(),
        frontend::CoherenceMode::Sequential
      );
      use_holder_->could_be_alias = true;
    }

  public:
 
    template <typename... Args>
    void 
    acquire_access(Args&&... args) const {

      using namespace darma_runtime::detail;
      using darma_runtime::keyword_tags_for_piecewise_handle::index;
      using darma_runtime::keyword_tags_for_piecewise_handle::copy_callback;
      using darma_runtime::keyword_tags_for_piecewise_handle::copy_back_callback;

      using parser = kwarg_parser<
        variadic_positional_overload_description<
          _keyword<size_t, index>,
          _optional_keyword<deduced_parameter, copy_callback>,
          _optional_keyword<deduced_parameter, copy_back_callback>
        >
      >;
      using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

      parser()
        .with_default_generators(
          keyword_arguments_for_piecewise_handle::copy_callback=_default_callback{},
          keyword_arguments_for_piecewise_handle::copy_back_callback=_default_callback{}
        )
        .parse_args(std::forward<Args>(args)...)
        .invoke(detail::_piecewise_collection_handle_impl<ValueType>(context_token_, collection_token_));
    }

  public:
 
    operator AccessHandleCollection<ValueType, IndexRangeT>() {

      // Register use and return access handle collection
      use_holder_->register_unregistered_use();
      return access_handle_collection_t(
        var_handle_, use_holder_
      );
    }

  public: 

    access_handle_collection_t collection() {
     
      // Register use and return access handle collection 
      use_holder_->register_unregistered_use();
      return access_handle_collection_t(
        var_handle_, use_holder_);
    }

  private:

    IndexRangeT range_;
    std::shared_ptr<detail::VariableHandle<ValueType>> var_handle_;
    std::shared_ptr<detail::UseHolder<detail::BasicCollectionManagingUse<IndexRangeT>>> use_holder_;

    mutable types::runtime_context_token_t context_token_;
    mutable types::piecewise_collection_token_t collection_token_;

};

} // end namespace darma_runtime

#endif // _darma_has_feature(mpi_interop)

#endif //DARMA_PIECEWISE_COLLECTION_HANDLE_H
