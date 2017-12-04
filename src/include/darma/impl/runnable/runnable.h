/*
//@HEADER
// ************************************************************************
//
//                      runnable.h
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

#ifndef DARMA_IMPL_RUNNABLE_H
#define DARMA_IMPL_RUNNABLE_H

#include <memory>
#include <cstddef>
#include <vector>
#include <type_traits>
#include <cassert>

#include <tinympl/all_of.hpp>
#include <tinympl/logical_or.hpp>
#include <tinympl/tuple_as_sequence.hpp>

#include <darma/interface/backend/types.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/frontend/task.h>

#include <darma/serialization/traits.h>
#include <darma/impl/capture/functor_traits.h>
#include <darma/impl/util.h>
#include <darma/impl/handle.h>
#include <darma/impl/util/smart_pointers.h>

#include <darma/impl/util/static_assertions.h>

//==============================================================================
// <editor-fold desc="Errors for calls with unserializable arguments">

namespace _darma__errors {

template <typename Functor>
struct __________asynchronous_call_to_functor__ {
  template <typename ArgType>
  struct _____with_unserializable_argument_of_type__ { };
};

} // end namespace _darma__errors

// </editor-fold> end Errors for calls with unserializable arguments
//==============================================================================

#include "runnable_fwd.h"
#include "registry.h"
#include "runnable_base.h"
#include "lambda_runnable.h"

namespace darma_runtime {
namespace detail {

template <typename Callable>
struct RunnableCondition : public RunnableBase
{
  // Force this one to be an lvalue reference
  explicit
  RunnableCondition(std::remove_reference_t<Callable>& c)
    : run_this_(c)
  { }

#if _darma_has_feature(task_migration)
  size_t get_index() const  { DARMA_ASSERT_NOT_IMPLEMENTED(); return 0; }

  virtual size_t get_packed_size() const {
    DARMA_ASSERT_NOT_IMPLEMENTED();
    return 0; // Unreachable; silence missing return warnings
  }

  virtual void pack(void* allocated) const { DARMA_ASSERT_NOT_IMPLEMENTED(); }
#endif // _darma_has_feature(task_migration)

  bool run()  { return run_this_(); }

  std::remove_reference_t<Callable> run_this_;
};


////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="FunctorRunnable">

template <
  typename Callable,
  typename... Args
>
class FunctorLikeRunnableBase
  : public RunnableBase
{
  public:

    typedef functor_traits<Callable> traits;
    typedef functor_call_traits<Callable, Args&&...> call_traits;
    static constexpr auto n_functor_args_min = traits::n_args_min;
    static constexpr auto n_functor_args_max = traits::n_args_max;

    STATIC_ASSERT_VALUE_LESS_EQUAL(sizeof...(Args), n_functor_args_max);

    static_assert(
      sizeof...(Args) <= n_functor_args_max && sizeof...(Args) >= n_functor_args_min,
      "FunctorWrapper or Method task created with wrong number of arguments"
    );

    using args_tuple_t = typename call_traits::args_tuple_t;

  protected:

    args_tuple_t args_;

    // This method makes the appropriate transformations of arguments from the
    // types they are stored as to the types they need to by passed to the
    // callable as.  For instance, a formal parameter of const T& will be
    // stored as a ReadAccessHandle<T>, but needs to be passed to the callable
    // as a const T& corresponding to the dereference of the ReadAccessHandle<T>
    decltype(auto)
    _get_args_to_splat() {
      return static_get_args_to_splat(args_);
    }

    static decltype(auto)
    static_get_args_to_splat(args_tuple_t& args_) {
      return meta::tuple_for_each_zipped(
        // iterate over the arguments yielding a pair of
        // (argument, call_arg_traits) and call the lambda below for each
        // pair
        args_,
        typename tinympl::transform<
          std::make_index_sequence<std::tuple_size<args_tuple_t>::value>,
          call_traits::template call_arg_traits_types_only,
          std::tuple
        >::type(),
        [&](auto&& arg, auto&& call_arg_traits_i_val) -> decltype(auto) {
          // Forward to the get_converted_arg function which does the appropriate
          // conversion from stored arg type to call arg type
          using call_traits_i = std::decay_t<decltype(call_arg_traits_i_val)>;
          return call_traits_i::template get_converted_arg(
            std::forward<decltype(arg)>(arg)
          );
        } // end lambda
      ); // end tuple_for_each_zipped
    }

    using get_args_splat_tuple_t = decltype(static_get_args_to_splat(
      std::declval<args_tuple_t&>()
    ));

    ////////////////////////////////////////////////////////////////////////////
#if _darma_has_feature(task_migration)

    // TODO there should also be a version of construct_from_archive where
    //      all of the entries in args_tuple_t are in-place constructible
    //      with an archive object (once we work out the syntax for expressing
    //      this in-place constructibility).  That should be the preferred
    //      mode of action.

    // Things are a lot simpler if we can default construct all of the arguments
    template <typename ArchiveT, typename RunnableToMake, typename... CTorArgs>
    static types::unique_ptr_template<RunnableToMake>
    _construct_from_archive(
      std::enable_if_t<
        std::is_default_constructible<args_tuple_t>::value,
        ArchiveT&
      > ar,
      CTorArgs&&... ctor_args
    ) {

      // Default-construct args_ by calling the default constructor
      auto rv = detail::make_unique<RunnableToMake>(
        std::forward<CTorArgs>( ctor_args )...
      );

      ar >> rv->args_;

      return std::move(rv);
    };

    // If args_tuple_t isn't default constructible, we need to do something
    // a bit more messy
    template <typename ArchiveT, typename RunnableToMake, typename... CTorArgs>
    static types::unique_ptr_template<RunnableToMake>
    _construct_from_archive(
      std::enable_if_t<
        not std::is_default_constructible<args_tuple_t>::value,
        ArchiveT&
      > ar,
      CTorArgs&&... ctor_args
    ) {
      // If all of the args aren't default constructible, we have no choice but
      // to move construct the arguments

      // Reallocate
      // TODO request this allocation from the backend instead
      // (or make the default allocator such that it asks the backend for an
      // allocation)
      using tuple_alloc_traits =
        serialization::detail::allocation_traits<args_tuple_t>;
      void* args_tup_spot = tuple_alloc_traits::allocate(ar, 1);
      args_tuple_t& args = *static_cast<args_tuple_t*>(args_tup_spot);

      // Unpack each of the arguments
      meta::tuple_for_each(
        args,
        [&ar](auto& arg) {
          ar.unpack_item(
            const_cast<
              std::remove_const_t<std::remove_reference_t<decltype(arg)>>&
              >(arg)
          );
        }
      );

      // now cast to xvalue and invoke the argument move constructor
      auto rv = detail::make_unique<RunnableToMake>(
        std::move(args),
        std::forward<CTorArgs>(ctor_args)...
      );

      // now that the move has happened, call the destructors of the
      // elements in the args tuple
      tuple_alloc_traits::destroy(
        ar, static_cast<args_tuple_t*>(args_tup_spot)
      );
      // And deallocate the memory
      tuple_alloc_traits::deallocate(
        ar, static_cast<args_tuple_t*>(args_tup_spot), 1
      );

      return std::move(rv);
    }
#endif // _darma_has_feature(task_migration)


    ////////////////////////////////////////////////////////////////////////////

  public:

#if _darma_has_feature(task_migration)

    size_t get_packed_size() const override {
      using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
      serialization::SimplePackUnpackArchive ar;

      ArchiveAccess::start_sizing(ar);

      ar % args_;

      return ArchiveAccess::get_size(ar);
    }

    void pack(void* allocated) const override {
      using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
      serialization::SimplePackUnpackArchive ar;

      ArchiveAccess::start_packing(ar);
      ArchiveAccess::set_buffer(ar, allocated);

      // pack the arguments
      ar << args_;

    }
#endif // _darma_has_feature(task_migration)

    template <typename _used_only_for_SFINAE = void,
      typename = std::enable_if_t<
        std::is_default_constructible<args_tuple_t>::value
        and std::is_void<_used_only_for_SFINAE>::value
      >
    >
    FunctorLikeRunnableBase()
      : args_()
    { }


    FunctorLikeRunnableBase(
      args_tuple_t&& args_tup
    ) : args_(std::forward<args_tuple_t>(args_tup))
    { }

    FunctorLikeRunnableBase(
      variadic_constructor_arg_t const,
      Args&&... args
    ) : args_(std::forward<Args>(args)...)
    { }

    ////////////////////////////////////////////////////////////////////////////
};

template <typename Functor, typename... Args>
class FunctorRunnable
  : public FunctorLikeRunnableBase<Functor, Args...>
{
  private:

    using base_t = FunctorLikeRunnableBase<Functor, Args...>;

    using args_tuple_t = typename base_t::args_tuple_t;

    template <typename T>
    using _arg_is_serializable_or_is_access_handle = tinympl::or_<
      typename serialization::detail::serializability_traits<T>
        ::template is_serializable_with_archive<
          serialization::PolicyAwareArchive
        >,
      typename serialization::detail::serializability_traits<T>
        ::template is_serializable_with_archive<
          serialization::SimplePackUnpackArchive
        >,
      is_access_handle<T>
    >;


  public:

    template <
      /* convenience */
      typename _first_unserializable =
        tinympl::at_or_t<int /* ignored*/,
          tinympl::find_if<
            args_tuple_t,
            tinympl::negate_metafunction<_arg_is_serializable_or_is_access_handle>::template apply
          >::type::value,
          args_tuple_t
        >
    >
    using static_assert_all_args_serializable = decltype(std::conditional_t<
      tinympl::all_of<args_tuple_t, _arg_is_serializable_or_is_access_handle>::value,
      tinympl::identity<int>,
      _darma__static_failure<
        _____________________________________________________________________,
        _____________________________________________________________________,
        typename _darma__errors::__________asynchronous_call_to_functor__<Functor>
          ::template _____with_unserializable_argument_of_type__<
            _first_unserializable
          >,
        _____________________________________________________________________,
        _____________________________________________________________________
      >
    >());

  private:

    template <typename... FArgs>
    using _return_of_functor_t = std::result_of_t<Functor(FArgs...)>;

    using splat_args_t = typename base_t::get_args_splat_tuple_t;


  public:

    // Use the base class constructors
    using base_t::base_t;


  private:

    template <typename _Ignored=void>
    std::enable_if_t<
      std::is_void<_Ignored>::value
        and std::is_void<
          typename tinympl::splat_to<
            splat_args_t,
            _return_of_functor_t
          >::type
        >::value,
      bool
    >
    do_run() {
      meta::splat_tuple<AccessHandleBase>(
        this->base_t::_get_args_to_splat(),
        Functor()
      );
      return false; /* should be ignored */
    }

    template <typename _Ignored=void>
    std::enable_if_t<
      std::is_void<_Ignored>::value
      and not std::is_void<
        typename tinympl::splat_to<
          splat_args_t,
          _return_of_functor_t
        >::type
      >::value,
      bool
    >
    do_run() {
      return meta::splat_tuple<AccessHandleBase>(
        this->base_t::_get_args_to_splat(),
        Functor()
      );
    }

  public:

    bool run() override {
      return do_run();
    }


#if _darma_has_feature(task_migration)
    template <typename ArchiveT>
    static types::unique_ptr_template<RunnableBase>
    construct_from_archive(ArchiveT& ar) {
      return base_t::template _construct_from_archive<
        ArchiveT,
        FunctorRunnable<Functor, Args...>
      >(ar);
    };

    size_t get_index() const override { return register_runnable<FunctorRunnable>(); }
#endif // _darma_has_feature(task_migration)

};

//template <typename FunctorWrapper, typename... Args>
//const size_t FunctorRunnable<FunctorWrapper, Args...>::index_ =
//  register_runnable<FunctorRunnable<FunctorWrapper, Args...>>();

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////





} // end namespace detail
} // end namespace darma_runtime


#endif //DARMA_IMPL_RUNNABLE_H
