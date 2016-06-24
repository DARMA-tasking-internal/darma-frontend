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

#include <darma/interface/backend/types.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/frontend/task.h>

#include <darma/impl/serialization/traits.h>
#include <darma/impl/functor_traits.h>
#include <darma/impl/util.h>
#include <darma/impl/handle.h>
#include <darma/impl/util/smart_pointers.h>

namespace darma_runtime {
namespace detail {

// Forward declaration
class RunnableBase;

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="Runnable registry and helpers">

typedef std::vector<std::function<std::unique_ptr<RunnableBase>(void*)>> runnable_registry_t;

// TODO make sure this pattern works on all compilers at all optimization levels
template <typename = void>
runnable_registry_t&
get_runnable_registry()  {
  static runnable_registry_t reg;
  return reg;
}

namespace _impl {

template <typename Runnable>
struct RunnableRegistrar {
  size_t index;
  RunnableRegistrar() {
    runnable_registry_t &reg = get_runnable_registry<>();
    index = reg.size();
    reg.emplace_back([](void* archive_as_void) -> std::unique_ptr<RunnableBase> {
      using ArchiveT = serialization::SimplePackUnpackArchive;

      return Runnable::template construct_from_archive<
        serialization::SimplePackUnpackArchive
      >(*static_cast<serialization::SimplePackUnpackArchive*>(archive_as_void));
    });
  }
};

template <typename Runnable>
struct RunnableRegistrarWrapper {
  static RunnableRegistrar<Runnable> registrar;
};

template <typename Runnable>
RunnableRegistrar<Runnable> RunnableRegistrarWrapper<Runnable>::registrar = { };

} // end namespace _impl

template <typename Runnable>
const size_t
register_runnable() {
  return _impl::RunnableRegistrarWrapper<Runnable>::registrar.index;
}

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="RunnableBase">

class RunnableBase {
  public:
    virtual bool run() =0;
    virtual size_t get_index() const =0;
    virtual size_t get_packed_size() const =0;
    virtual void pack(void* allocated) const =0;
    virtual ~RunnableBase() { }
};

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="Runnable (for lambdas)">

template <typename Callable>
struct Runnable : public RunnableBase
{
  private:
  public:
    // Force it to be an rvalue reference
    explicit
    Runnable(std::remove_reference_t<Callable>&& c)
      : run_this_(std::move(c))
    { }
    bool run()  { run_this_(); return false; }

    static const size_t index_;

    template <typename ArchiveT>
    static std::unique_ptr<RunnableBase>
    construct_from_archive(ArchiveT& data) {
      // TODO write this (or don't...)
      assert(false);
      return nullptr;
    }

    virtual size_t get_packed_size() const { DARMA_ASSERT_NOT_IMPLEMENTED(); }
    virtual void pack(void* allocated) const { DARMA_ASSERT_NOT_IMPLEMENTED(); }

    size_t get_index() const  { return index_; }

  private:
    std::remove_reference_t<Callable> run_this_;
};

template <typename Callable>
const size_t Runnable<Callable>::index_ =
  register_runnable<Runnable<Callable>>();

template <typename Callable>
struct RunnableCondition : public RunnableBase
{
  // Don't force an rvalue; caller might want to trigger a copy by not forwarding
  explicit
  RunnableCondition(Callable&& c)
    : run_this_(std::forward<Callable>(c))
  { }

  size_t get_index() const  { DARMA_ASSERT_NOT_IMPLEMENTED(); return 0; }
  virtual size_t get_packed_size() const { DARMA_ASSERT_NOT_IMPLEMENTED(); }
  virtual void pack(void* allocated) const { DARMA_ASSERT_NOT_IMPLEMENTED(); }

  bool run()  { return run_this_(); }

  std::remove_reference_t<Callable> run_this_;
};

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////


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

    static_assert(
      sizeof...(Args) <= n_functor_args_max && sizeof...(Args) >= n_functor_args_min,
      "Functor or Method task created with wrong number of arguments"
    );

  protected:

    using args_tuple_t = typename call_traits::args_tuple_t;

    args_tuple_t args_;

    // TODO comment this
    decltype(auto)
    _get_args_to_splat() {
      return meta::tuple_for_each_zipped(
        args_,
        typename tinympl::transform<
          std::make_index_sequence<std::tuple_size<args_tuple_t>::value>,
          call_traits::template call_arg_traits_types_only,
          std::tuple
        >::type(),
        [this](auto&& arg, auto&& call_arg_traits_i_val) -> decltype(auto) {
          using call_traits_i = std::decay_t<decltype(call_arg_traits_i_val)>;
          return call_traits_i::template get_converted_arg(
            std::forward<decltype(arg)>(arg)
          );
        }
      );
    }


    ////////////////////////////////////////////////////////////////////////////

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


    ////////////////////////////////////////////////////////////////////////////

  public:

    size_t get_packed_size() const override {
      using detail::DependencyHandle_attorneys::ArchiveAccess;
      serialization::SimplePackUnpackArchive ar;

      ArchiveAccess::start_sizing(ar);

      ar % args_;

      return ArchiveAccess::get_size(ar);
    }

    void pack(void* allocated) const override {
      using detail::DependencyHandle_attorneys::ArchiveAccess;
      serialization::SimplePackUnpackArchive ar;

      ArchiveAccess::start_packing(ar);
      ArchiveAccess::set_buffer(ar, allocated);

      // pack the arguments
      ar << args_;

    }

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

    static const size_t index_;
    using base_t = FunctorLikeRunnableBase<Functor, Args...>;

    using args_tuple_t = typename base_t::args_tuple_t;

  public:

    // Use the base class constructors
    using base_t::base_t;


    bool run()  {
      meta::splat_tuple<AccessHandleBase>(
        this->base_t::_get_args_to_splat(),
        Functor()
      );
      return false;
    }


    template <typename ArchiveT>
    static types::unique_ptr_template<RunnableBase>
    construct_from_archive(ArchiveT& ar) {
      return base_t::template _construct_from_archive<
        ArchiveT,
        FunctorRunnable<Functor, Args...>
      >(ar);
    };

    size_t get_index() const  { return index_; }
};

template <typename Functor, typename... Args>
const size_t FunctorRunnable<Functor, Args...>::index_ =
  register_runnable<FunctorRunnable<Functor, Args...>>();

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////





} // end namespace detail
} // end namespace darma_runtime


#endif //DARMA_IMPL_RUNNABLE_H
