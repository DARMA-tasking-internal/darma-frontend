/*
//@HEADER
// ************************************************************************
//
//              persistent_collection_handle.h
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

#ifndef DARMA_PERSISTENT_COLLECTION_HANDLE_H
#define DARMA_PERSISTENT_COLLECTION_HANDLE_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(mpi_interop)

#include <darma/keyword_arguments/parse.h>
#include <darma/keyword_arguments/macros.h>
#include <darma/impl/task_collection/access_handle_collection.h>


namespace darma_runtime {

template<
  typename ValueType,
  typename IndexRangeT
>
class PersistentCollectionHandle {

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

    PersistentCollectionHandle() = delete;

  public:

    template <typename IndexRangeDeducedT> 
    explicit PersistentCollectionHandle(
      std::shared_ptr<detail::VariableHandle<ValueType>> const& var_handle,
      IndexRangeDeducedT range,
      types::runtime_context_token_t context_token,
      types::persistent_collection_token_t collection_token
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
        initial_flow().as_collection_relationship(),
        null_flow().as_collection_relationship(),
        insignificant_flow().as_collection_relationship(),
        initial_anti_flow().as_collection_relationship(),
        frontend::CoherenceMode::Sequential
      );
      use_holder_->could_be_alias = true;
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
         var_handle_, use_holder_
       );
    }    

  private:

    IndexRangeT range_;
    std::shared_ptr<detail::VariableHandle<ValueType>> var_handle_;
    std::shared_ptr<detail::UseHolder<detail::BasicCollectionManagingUse<IndexRangeT>>> use_holder_;

    mutable types::runtime_context_token_t context_token_;
    mutable types::persistent_collection_token_t collection_token_;

};

} // end namespace darma_runtime

#endif // _darma_has_feature(mpi_interop)

#endif //DARMA_PERSISTENT_COLLECTION_HANDLE_H
