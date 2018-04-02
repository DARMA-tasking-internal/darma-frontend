/*
//@HEADER
// ************************************************************************
//
//                       darma_mpi.h
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

#ifndef DARMA_MPI_H
#define DARMA_MPI_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(mpi_interop)

#include <tinympl/is_instantiation_of.hpp>

#include <darma/keyword_arguments/parse.h>
#include <darma/keyword_arguments/macros.h>
#include <darma/impl/array/index_range.h>
#include <darma/impl/mpi/piecewise_acquired_collection.h>
#include <darma/impl/mpi/persistent_collection.h>

DeclareDarmaTypeTransparentKeyword(mpi_context, data);
DeclareDarmaTypeTransparentKeyword(mpi_context, size);
DeclareDarmaTypeTransparentKeyword(mpi_context, index);
DeclareDarmaTypeTransparentKeyword(mpi_context, indices);
DeclareDarmaTypeTransparentKeyword(mpi_context, index_range);
DeclareDarmaTypeTransparentKeyword(mpi_context, copy_callback);
DeclareDarmaTypeTransparentKeyword(mpi_context, copy_back_callback);

namespace darma_runtime {

namespace detail {

template <typename ValueType>
struct
_persistent_collection_creation_impl {

  public:

    _persistent_collection_creation_impl(
      types::runtime_context_token_t context_token
    ) : context_token_(context_token) { } 

  private:

    // Variadic key generator
    template <typename FirstArg,
      typename... LastArgs
    >
    auto
    _make_key_impl(FirstArg&& arg, LastArgs&&... args) {
       return make_key(std::forward<FirstArg>(arg), std::forward<LastArgs>(args)...);
    }

    // Default key generator
    auto
    _make_key_impl() {
       return darma_runtime::detail::key_traits<
         darma_runtime::types::key_t
       >::make_awaiting_backend_assignment_key();
    } 

  public:

    /* index_range keyword version */
    template <typename IndexRangeT,
      typename = std::enable_if_t<
        std::is_base_of<abstract::frontend::IndexRange, IndexRangeT>::value
      >,
      typename... Args
    >
    auto
    operator()(
      IndexRangeT&& index_range,
      variadic_arguments_begin_tag,
      Args&&... args
    ) {

      // Create key and handle
      auto key = _make_key_impl(std::forward<Args>(args)...);
      auto var_handle = std::make_shared<VariableHandle<ValueType>>(key);

      // Register handle with the backend as part of a piecewise collection
      auto persistent_collection_token = register_persistent_collection(
        context_token_,
        var_handle,
        index_range.size()
      );

      // Create a PersistentCollectionHandle object...
      auto persistent_collection = PersistentCollectionHandle<ValueType, std::decay_t<IndexRangeT>>(
        var_handle,
        std::forward<IndexRangeT>(index_range),
        context_token_,
        persistent_collection_token
      );

      // ... and return it
      return persistent_collection;
    }


    /* size keyword version */
    template <typename...Args>
    auto
    operator()(
      size_t size,
      variadic_arguments_begin_tag,
      Args&&... args
    ) {

      // Create key and handle
      auto key = _make_key_impl(std::forward<Args>(args)...);
      auto var_handle = std::make_shared<VariableHandle<ValueType>>(key);

      // Register handle with the backend as part of a piecewise collection
      auto persistent_collection_token = register_persistent_collection(
        context_token_,
        var_handle,
        size
      );

      // Create a one-dimension index range
      auto index_range = Range1D<int>(size);

      // Create a PersistentCollectionHandle object...
      auto persistent_collection = PersistentCollectionHandle<ValueType, Range1D<int>>(
        var_handle,
        index_range,
        context_token_,
        persistent_collection_token
      );

      // ... and return it
      return persistent_collection;
    }

  private:

    types::runtime_context_token_t context_token_;


};

template <typename ValueType>
struct 
_piecewise_acquired_collection_creation_impl {

  private:

    template<typename IndexTupleDeducedT,
      typename DataTupleDeducedT,
      typename CopyCallbackT,
      typename CopyBackCallbackT,
      size_t... Idxs
    >
    void _register_piecewise_collection_pieces_impl( 
      types::piecewise_collection_token_t collection_token,
      IndexTupleDeducedT&& index_tuple, 
      DataTupleDeducedT&& data_tuple,
      std::index_sequence<Idxs...> /** unused **/,
      CopyCallbackT&& copy_callback,
      CopyBackCallbackT&& copy_back_callback
    ) {

      // Create functor
      auto functor = _piecewise_collection_handle_impl<ValueType>(context_token_, collection_token); 

      std::make_tuple( // fold emulation through comma operator
        (functor(std::get<Idxs>(index_tuple),
           std::forward<CopyCallbackT>(copy_callback),
           std::forward<CopyBackCallbackT>(copy_back_callback),
           variadic_arguments_begin_tag{},
           std::forward<decltype(std::get<Idxs>(data_tuple))>(std::get<Idxs>(data_tuple))           
        ), 0)...
      );

    }

  public:

    _piecewise_acquired_collection_creation_impl(
      types::runtime_context_token_t context_token
    ) : context_token_(context_token) {}


  public:

    /* index_range keyword version */
    template <typename IndexRangeT, 
      typename = std::enable_if_t<
        std::is_base_of<abstract::frontend::IndexRange, IndexRangeT>::value
      >,
      typename FirstArg,
      typename... LastArgs
    >
    auto
    operator()(
      IndexRangeT&& index_range,
      variadic_arguments_begin_tag,
      FirstArg&& arg,
      LastArgs&&... args
    ) {

      // Create key and handle
      auto key = make_key(std::forward<FirstArg>(arg), std::forward<LastArgs>(args)...);
      auto var_handle = std::make_shared<VariableHandle<ValueType>>(key);

      // Register handle with the backend as part of a piecewise collection
      auto collection_token = register_piecewise_collection(
        context_token_,
        var_handle,
        index_range.size()
      );    

      // Create a PiecewiseCollectionHandle object ...
      auto piecewise_collection = PiecewiseCollectionHandle<ValueType, std::decay_t<IndexRangeT>>(
        var_handle,
        std::forward<IndexRangeT>(index_range),
        context_token_,
        collection_token
      );

      // ... and return it
      return piecewise_collection;
    }  
 

    /* size keyword version */
    template <typename FirstArg,
      typename... LastArgs
    >
    auto
    operator()(
      size_t size,
      variadic_arguments_begin_tag,
      FirstArg&& arg,
      LastArgs&&... args
    ) {

      // Create key and handle
      auto key = make_key(std::forward<FirstArg>(arg), std::forward<LastArgs>(args)...);
      auto var_handle = std::make_shared<VariableHandle<ValueType>>(key);

      // Register handle with the backend as part of a piecewise collection
      auto collection_token = register_piecewise_collection(
        context_token_,
        var_handle,
        size
      );

      // Create a one-dimension index range
      auto index_range = Range1D<int>(size);

      // Create a PiecewiseCollectionHandle object ...
      auto piecewise_collection = PiecewiseCollectionHandle<ValueType, Range1D<int>>(
        var_handle,
        index_range,
        context_token_,
        collection_token
      );

      // ... and return it
      return piecewise_collection;
    }

  public:

    /* index_range keyword version */ 
    template <typename IndexRangeT, 
      typename CopyCallbackT,
      typename CopyBackCallbackT,
      typename = std::enable_if_t<
        std::is_base_of<abstract::frontend::IndexRange, IndexRangeT>::value
      >,
      typename FirstArg, 
      typename... LastArgs
    >
    auto
    operator()(
      IndexRangeT&& index_range,
      ValueType& data,
      size_t index,
      CopyCallbackT&& copy_callback,
      CopyBackCallbackT&& copy_back_callback,
      variadic_arguments_begin_tag,
      FirstArg&& arg,
      LastArgs&&... args
    ) {

      // Create key and handle
      auto key = make_key(std::forward<FirstArg>(arg), std::forward<LastArgs>(args)...);
      auto var_handle = std::make_shared<VariableHandle<ValueType>>(key);

      // Register handle with the backend as part of a piecewise collection
      auto collection_token = register_piecewise_collection(
        context_token_,
        var_handle,
        index_range.size()
      );

      // Create a PiecewiseCollectionHandle object 
      auto piecewise_collection = PiecewiseCollectionHandle<ValueType, std::decay_t<IndexRangeT>>(
        var_handle,
        std::forward<IndexRangeT>(index_range),
        context_token_,
        collection_token
      );
  
      // Register the piecewise collection piece
      _piecewise_collection_handle_impl<ValueType>(context_token_, collection_token)(
        index,
        std::forward<CopyCallbackT>(copy_callback),
        std::forward<CopyBackCallbackT>(copy_back_callback),
        variadic_arguments_begin_tag{}, 
        data
      );
   
      // Return the PiecewiseCollectionHandle object created above
      return piecewise_collection;
    }


    /* size keyword version */
    template <typename CopyCallbackT,
      typename CopyBackCallbackT,
      typename FirstArg,
      typename... LastArgs
    >
    auto
    operator()(
      size_t size,
      ValueType& data,
      size_t index,
      CopyCallbackT&& copy_callback,
      CopyBackCallbackT&& copy_back_callback,
      variadic_arguments_begin_tag,
      FirstArg&& arg,
      LastArgs&&... args
    ) {

      // Create key and handle
      auto key = make_key(std::forward<FirstArg>(arg), std::forward<LastArgs>(args)...);
      auto var_handle = std::make_shared<VariableHandle<ValueType>>(key);

      // Register handle with the backend as part of a piecewise collection
      auto collection_token = register_piecewise_collection(
        context_token_,
        var_handle,
        size
      );

      // Create a one-dimension index range
      auto index_range = Range1D<int>(size);
 
      // Create a PiecewiseCollectionHandle object 
      auto piecewise_collection = PiecewiseCollectionHandle<ValueType, Range1D<int>>(
        var_handle,
        index_range,
        context_token_,
        collection_token
      );
  
      // Register the piecewise collection piece
      _piecewise_collection_handle_impl<ValueType>(context_token_, collection_token)(
        index,
        std::forward<CopyCallbackT>(copy_callback),
        std::forward<CopyBackCallbackT>(copy_back_callback),
        variadic_arguments_begin_tag{}, 
        data
      );
  
      // Return the PiecewiseCollectionHandle object created above
      return piecewise_collection;
    }


  public:

    /* index_range keyword version */
    template <typename IndexRangeT,
      typename DataTupleDeducedT,
      typename IndexTupleDeducedT,
      typename CopyCallbackT,
      typename CopyBackCallbackT,
      typename = std::enable_if_t<
        std::is_base_of<abstract::frontend::IndexRange, IndexRangeT>::value &&
        tinympl::is_instantiation_of<std::tuple, DataTupleDeducedT>::value
      >,
      typename FirstArg,
      typename... LastArgs
    >
    auto
    operator()(
      IndexRangeT&& index_range,
      DataTupleDeducedT&& data_tuple,
      IndexTupleDeducedT&& index_tuple,
      CopyCallbackT&& copy_callback,
      CopyBackCallbackT&& copy_back_callback, 
      variadic_arguments_begin_tag,
      FirstArg&& arg,
      LastArgs&&... args 
    ) {

      // Require data and index tuple to have the same number of elements
      static_assert(
        std::tuple_size<std::decay_t<IndexTupleDeducedT>>::value == std::tuple_size<std::decay_t<DataTupleDeducedT>>::value, 
        "Error: data and index tuple passed to piecewise_acquired_collection must have the same number of elements !!"
      ); 

      // Create key and handle
      auto key = make_key(std::forward<FirstArg>(arg), std::forward<LastArgs>(args)...);
      auto var_handle = std::make_shared<VariableHandle<ValueType>>(key);

      // Register handle with the backend as part of a piecewise collection
      auto collection_token = register_piecewise_collection(
        context_token_,
        var_handle,
        index_range.size()
      );

      // Create a PiecewiseCollectionHandle object
      auto piecewise_collection = PiecewiseCollectionHandle<ValueType, std::decay_t<IndexRangeT>>(
        var_handle,
        std::forward<IndexRangeT>(index_range),
        context_token_,
        collection_token
      );

      // Call private helper method to register the piecewise collection pieces
      _register_piecewise_collection_pieces_impl(
        collection_token,
        std::forward<IndexTupleDeducedT>(index_tuple),
        std::forward<DataTupleDeducedT>(data_tuple),
        std::make_index_sequence<std::tuple_size<std::decay_t<IndexTupleDeducedT>>::value>(),
        std::forward<CopyCallbackT>(copy_callback),
        std::forward<CopyBackCallbackT>(copy_back_callback) 
      );

      // Return the PiecewiseCollectionHandle created above
      return piecewise_collection;
    }

    
    /* size keyword version */
    template <typename DataTupleDeducedT,
      typename IndexTupleDeducedT,
      typename CopyCallbackT,
      typename CopyBackCallbackT,
      typename = std::enable_if_t<
        tinympl::is_instantiation_of<std::tuple, DataTupleDeducedT>::value
      >,
      typename FirstArg,
      typename... LastArgs
    >
    auto
    operator()(
      size_t size,
      DataTupleDeducedT&& data_tuple,
      IndexTupleDeducedT&& index_tuple,
      CopyCallbackT&& copy_callback,
      CopyBackCallbackT&& copy_back_callback,
      variadic_arguments_begin_tag,
      FirstArg&& arg,
      LastArgs&&... args
    ) {

      // Require data and index tuple to have the same number of elements
      static_assert(
        std::tuple_size<std::decay_t<IndexTupleDeducedT>>::value == std::tuple_size<std::decay_t<DataTupleDeducedT>>::value, 
        "Error: data and index tuple passed to piecewise_acquired_collection must have the same number of elements. \n"
      ); 

      // Create key and handle
      auto key = make_key(std::forward<FirstArg>(arg), std::forward<LastArgs>(args)...);
      auto var_handle = std::make_shared<VariableHandle<ValueType>>(key);

      // Register handle with the backend as part of a piecewise collection
      auto collection_token = register_piecewise_collection(
        context_token_,
        var_handle,
        size
      );

      // Create a one-dimension index range
      auto index_range = Range1D<int>(size);

      // Create a PiecewiseCollectionHandle object
      auto piecewise_collection = PiecewiseCollectionHandle<ValueType, Range1D<int>>(
        var_handle,
        index_range,
        context_token_,
        collection_token
      );

      // Call private helper method to register the piecewise collection pieces
      _register_piecewise_collection_pieces_impl(
        collection_token,
        std::forward<IndexTupleDeducedT>(index_tuple),
        std::forward<DataTupleDeducedT>(data_tuple),
        std::make_index_sequence<std::tuple_size<std::decay_t<IndexTupleDeducedT>>::value>(),
        std::forward<CopyCallbackT>(copy_callback),
        std::forward<CopyBackCallbackT>(copy_back_callback) 
      );

      // Return the PiecewiseCollectionHandle created above
      return piecewise_collection;
    }  

  private:
 
    types::runtime_context_token_t context_token_;

};

} // end namespace detail

class mpi_context {

  public:
     
    mpi_context() = delete;
    mpi_context(mpi_context const&) = default;
    mpi_context(mpi_context&&) = default;

  public:

    explicit mpi_context(types::MPI_Comm comm) {
      runtime_token_ = create_runtime_context(comm); 
    } 

  public:

    template <typename T, typename... Args>
    auto
    piecewise_acquired_collection(Args&&... args) const {
 
      using namespace darma_runtime::detail; 
      using darma_runtime::keyword_tags_for_mpi_context::data; 
      using darma_runtime::keyword_tags_for_mpi_context::size;
      using darma_runtime::keyword_tags_for_mpi_context::index; 
      using darma_runtime::keyword_tags_for_mpi_context::indices;
      using darma_runtime::keyword_tags_for_mpi_context::index_range;
      using darma_runtime::keyword_tags_for_mpi_context::copy_callback;
      using darma_runtime::keyword_tags_for_mpi_context::copy_back_callback;

      using parser = kwarg_parser<
        variadic_positional_overload_description<
          _keyword<deduced_parameter, index_range>
        >,
        variadic_positional_overload_description<
          _keyword<size_t, size>
        >,
        variadic_positional_overload_description<
          _keyword<deduced_parameter, index_range>,
          _keyword<deduced_parameter, data>,
          _keyword<size_t, index>,
          _optional_keyword<deduced_parameter, copy_callback>,
          _optional_keyword<deduced_parameter, copy_back_callback>
        >,
        variadic_positional_overload_description<
          _keyword<size_t, size>,
          _keyword<deduced_parameter, data>,
          _keyword<size_t, index>,
          _optional_keyword<deduced_parameter, copy_callback>,
          _optional_keyword<deduced_parameter, copy_back_callback>
        >,
        variadic_positional_overload_description<
          _keyword<deduced_parameter, index_range>,
          _keyword<converted_parameter, data>,
          _keyword<converted_parameter, indices>,
          _optional_keyword<deduced_parameter, copy_callback>,
          _optional_keyword<deduced_parameter, copy_back_callback> 
        >,
        variadic_positional_overload_description<
          _keyword<size_t, size>,
          _keyword<converted_parameter, data>,
          _keyword<converted_parameter, indices>,
          _optional_keyword<deduced_parameter, copy_callback>,
          _optional_keyword<deduced_parameter, copy_back_callback>
        >
      >;
      using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

      // Invoke parser
      return parser()
        .with_default_generators(
          keyword_arguments_for_mpi_context::copy_callback=_default_callback{},
          keyword_arguments_for_mpi_context::copy_back_callback=_default_callback{}
        )
        .with_converters(
          [](auto&&... data_parts) {
            return std::forward_as_tuple(std::forward<decltype(data_parts)>(data_parts)...);
          },
          [](auto... index_parts) {
            return std::make_tuple(index_parts...);
          }
        )
        .parse_args(std::forward<Args>(args)...)
        .invoke(detail::_piecewise_acquired_collection_creation_impl<T>(runtime_token_));
    }

  public:

    template <typename T, typename... Args>
    auto
    persistent_collection(Args&&... args) const {

      using namespace darma_runtime::detail;
      using darma_runtime::keyword_tags_for_mpi_context::size;
      using darma_runtime::keyword_tags_for_mpi_context::index_range;

      using parser = kwarg_parser<
        variadic_positional_overload_description<
          _keyword<deduced_parameter, index_range>
        >,
        variadic_positional_overload_description<
          _keyword<size_t, size>
        >
      >;
      using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

      // Invoke parser
      return parser()
        .parse_args(std::forward<Args>(args)...)
        .invoke(detail::_persistent_collection_creation_impl<T>(runtime_token_));
    }

  public:

    template <typename Callable>
    void
    run_distributed_region_blocking(Callable&& callable) {
      run_distributed_region(
        runtime_token_,
        std::forward<Callable>(callable)
      );
    }

    void run_distributed_region_worker_blocking() {
      run_distributed_region_worker(runtime_token_);
    }

  private:

    types::runtime_context_token_t runtime_token_;
};

} // end namespace darma_runtime

#endif // _darma_has_feature(mpi_interop)

#endif //DARMA_MPI_H 
