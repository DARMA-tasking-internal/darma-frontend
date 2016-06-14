/*
//@HEADER
// ************************************************************************
//
//                          task.h
//                         darma_new
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

#ifndef DARMA_RUNTIME_TASK_H_
#define DARMA_RUNTIME_TASK_H_

#include <typeindex>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <set>

#include <tinympl/greater.hpp>
#include <tinympl/int.hpp>
#include <tinympl/delay.hpp>
#include <tinympl/identity.hpp>
#include <tinympl/delay.hpp>
#include <tinympl/at.hpp>
#include <tinympl/erase.hpp>
#include <tinympl/bind.hpp>
#include <tinympl/logical_and.hpp>
#include <tinympl/vector.hpp>
#include <tinympl/transform2.hpp>
#include <tinympl/as_sequence.hpp>
#include <tinympl/tuple_as_sequence.hpp>
#include <tinympl/stl_integer_sequence.hpp>

#include <darma/interface/backend/types.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/frontend/task.h>

#include <darma/impl/util.h>
#include <darma/impl/runtime.h>
#include <darma/impl/handle_fwd.h>
#include <darma/impl/meta/callable_traits.h>
#include <darma/impl/handle.h>
#include <darma/impl/functor_traits.h>
#include <darma/impl/serialization/nonintrusive.h>
#include <darma/impl/use.h>
#include <darma/impl/util/smart_pointers.h>


namespace darma_runtime {

namespace detail {

class RunnableBase;

////////////////////////////////////////////////////////////////////////////////

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
    reg.emplace_back([](void *data) -> std::unique_ptr<RunnableBase> {
      return Runnable::construct_from_bytes(data);
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


////////////////////////////////////////////////////////////////////////////////

// <editor-fold desc="Runnable and RunnableBase">

class RunnableBase {
  public:
    virtual bool run() =0;
    virtual size_t get_index() const =0;
    virtual ~RunnableBase() { }
};

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

    static std::unique_ptr<RunnableBase>
    construct_from_bytes(void* data) {
      // TODO write this
      assert(false);
    }

    size_t get_index() const  { return index_; }

  private:
    std::remove_reference_t<Callable> run_this_;
};

template <typename Callable>
const size_t Runnable<Callable>::index_ = register_runnable<Runnable<Callable>>();

template <typename Callable>
struct RunnableCondition : public RunnableBase
{
  // Don't force an rvalue; caller might want to trigger a copy by not forwarding
  explicit
  RunnableCondition(Callable&& c)
    : run_this_(std::forward<Callable>(c))
  { }

  size_t get_index() const  { return 0; }

  bool run()  { return run_this_(); }

  std::remove_reference_t<Callable> run_this_;
};


template <
  typename Functor,
  typename... Args
>
class FunctorRunnable
  : public RunnableBase
{
  public:

    typedef functor_traits<Functor> traits;
    typedef functor_call_traits<Functor, Args&&...> call_traits;
    static constexpr auto n_functor_args_min = traits::n_args_min;
    static constexpr auto n_functor_args_max = traits::n_args_max;

    static_assert(
      sizeof...(Args) <= n_functor_args_max && sizeof...(Args) >= n_functor_args_min,
      "Functor task created with wrong number of arguments"
    );

  public:
    using args_tuple_t = typename call_traits::args_tuple_t;
  private:

    args_tuple_t args_;

    static const size_t index_;

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

  public:

    FunctorRunnable(
      args_tuple_t&& args_tup
    ) : args_(std::forward<args_tuple_t>(args_tup))
    { }

    FunctorRunnable(
      variadic_constructor_arg_t const,
      Args&&... args
    ) : args_(std::forward<Args>(args)...)
    { }

    bool run()  {
      meta::splat_tuple<AccessHandleBase>(
        _get_args_to_splat(),
        Functor()
      );
      return false;
    }


    static std::unique_ptr<RunnableBase>
    construct_from_bytes(void* data) {
      // TODO inplace rather than move construction?

      // Make an archive from the void*
      serialization::SimplePackUnpackArchive ar;
      using DependencyHandle_attorneys::ArchiveAccess;
      ArchiveAccess::start_unpacking(ar);
      ArchiveAccess::set_buffer(ar, data);

      // Reallocate
      using tuple_alloc_traits = serialization::detail::allocation_traits<args_tuple_t>;
      void* args_tup_spot = tuple_alloc_traits::allocate(ar, 1);
      args_tuple_t& args = *static_cast<args_tuple_t*>(args_tup_spot);

      // Unpack each of the arguments
      meta::tuple_for_each(
        args,
        [&ar](auto& arg) {
          ar.unpack_item(const_cast<
            std::remove_const_t<std::remove_reference_t<decltype(arg)>>&
          >(arg));
        }
      );

      // now cast to xvalue and invoke the argument move constructor
      return std::make_unique<FunctorRunnable>(std::move(args));
    }

    size_t get_index() const  { return index_; }
};

template <typename Functor, typename... Args>
const size_t FunctorRunnable<Functor, Args...>::index_ =
  register_runnable<FunctorRunnable<Functor, Args...>>();

// </editor-fold>

////////////////////////////////////////////////////////////////////////////////

// <editor-fold desc="TaskBase and its descendants">

class TaskBase : public abstract::frontend::Task<TaskBase>
{
  protected:

    using key_t = types::key_t;
    using abstract_use_t = abstract::frontend::Use;

    using get_deps_container_t = types::handle_container_template<abstract_use_t*>;

    get_deps_container_t dependencies_;

    key_t name_;

  public:

    TaskBase() = default;

    // Directly construct from a conditional callable
    template <typename LambdaCallable,
      typename = std::enable_if_t<
        not std::is_base_of<std::decay_t<LambdaCallable>, TaskBase>::value
      >
    >
    TaskBase(LambdaCallable&& bool_callable) {
      TaskBase* parent_task = static_cast<detail::TaskBase* const>(
        detail::backend_runtime->get_running_task()
      );
      parent_task->current_create_work_context = this;
      default_capture_as_info |= AccessHandleBase::CapturedAsInfo::ReadOnly;
      default_capture_as_info |= AccessHandleBase::CapturedAsInfo::Leaf;
      runnable_ =
        // *Intentionally* avoid perfect forwarding here, causing a copy to happen,
        // which then triggers all of the captures.  We do this by adding an lvalue reference
        // to the type and not forwarding the value
        detail::make_unique<RunnableCondition<std::remove_reference_t<LambdaCallable>&>>(
          bool_callable
        );
      default_capture_as_info = AccessHandleBase::CapturedAsInfo::Normal;
      parent_task->current_create_work_context = nullptr;
    }

    void add_dependency(HandleUse& use) {
      dependencies_.insert(&use);
    }

    template <typename AccessHandleT1, typename AccessHandleT2>
    void do_capture(
      AccessHandleT1& captured,
      AccessHandleT2 const& source_and_continuing
    );

    ////////////////////////////////////////////////////////////////////////////////
    // Implementation of abstract::frontend::Task

    get_deps_container_t const&
    get_dependencies() const {
      return dependencies_;
    }

    const key_t&
    get_name() const {
      return name_;
    }

    void
    set_name(const key_t& name) {
      name_ = name;
    }

    bool
    is_migratable() const {
      // Ignored for now:
      return false;
    }

    template <typename ReturnType = void>
    ReturnType run()  {
      static_assert(std::is_same<ReturnType, bool>::value or std::is_void<ReturnType>::value,
        "Only bool and void for ReturnType in Task::run<>() are currently supported"
      );
      assert(runnable_);
      pre_run_setup();
      return _do_run<ReturnType>(typename std::is_void<ReturnType>::type{});
    }


    size_t get_packed_size() const  {
      // TODO
      assert(false);
      return 0;
    }

    void pack(void* allocated) const  {
      // TODO
      assert(false);
    }

    // end implementation of abstract::frontend::Task
    ////////////////////////////////////////////////////////////////////////////////

  private:
    template <typename ReturnType>
    inline void
    _do_run(std::true_type&&) {
      runnable_->run();
      post_run_cleanup();
    }

    template <typename ReturnType>
    inline std::enable_if_t<
      not std::is_void<ReturnType>::value,
      ReturnType
    >
    _do_run(std::false_type&&) {
      ReturnType rv = runnable_->run();
      post_run_cleanup();
      return rv;
    }

  public:

    void pre_run_setup() { }

    void post_run_cleanup() { }

    void set_runnable(std::unique_ptr<RunnableBase>&& r) {
      runnable_ = std::move(r);
    }

    virtual ~TaskBase() noexcept { }

    TaskBase* current_create_work_context = nullptr;

    std::vector<std::function<void()>> registrations_to_run;
    std::vector<std::function<void()>> post_registration_ops;
    unsigned default_capture_as_info = AccessHandleBase::CapturedAsInfo::Normal;

  private:

    std::unique_ptr<RunnableBase> runnable_;

    friend types::unique_ptr_template<abstract::frontend::Task<TaskBase>>
    unpack_task(void* packed_data);


};

class TopLevelTask
  : public TaskBase
{
  public:

    bool run()  {
      // Abort, as specified.  This should never be called.
      assert(false);
      return false;
    }

};


// </editor-fold>

////////////////////////////////////////////////////////////////////////////////

} // end namespace detail

// implementation of abstract::frontend::unpack_task

namespace abstract {

namespace frontend {

inline backend::runtime_t::task_unique_ptr
unpack_task(void* packed_data) {
  serialization::SimplePackUnpackArchive ar;
  detail::DependencyHandle_attorneys::ArchiveAccess::start_unpacking(ar);
  detail::DependencyHandle_attorneys::ArchiveAccess::set_buffer(ar, packed_data);

  std::size_t runnable_index;
  ar >> runnable_index;

  // Now unpack the number of dependencies, their keys and versions
  //std::vector<std::pair<types::key_t, types::version_t>> deps;
  //ar >> deps;

  // TODO
  assert(false);
  return nullptr;
}

} // end namespace frontend

} // end namespace abstract

} // end namespace darma_runtime


#include <darma/impl/task_do_capture.impl.h>


#endif /* DARMA_RUNTIME_TASK_H_ */
