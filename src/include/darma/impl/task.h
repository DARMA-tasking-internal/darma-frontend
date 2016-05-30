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
#include <darma/interface/frontend/dependency_handle.h>
#include <darma/interface/frontend/task.h>

#include <darma/impl/util.h>
#include <darma/impl/runtime.h>
#include <darma/impl/handle_fwd.h>
#include <darma/impl/meta/callable_traits.h>
#include <darma/impl/handle.h>
#include <darma/impl/functor_traits.h>
#include <darma/impl/serialization/nonintrusive.h>


#include <typeindex>


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
    virtual void run() =0;
    virtual size_t get_index() const =0;
    virtual ~RunnableBase() { }
};

template <typename Callable>
struct Runnable : public RunnableBase
{
  private:
  public:
    explicit
    Runnable(Callable&& c)
      : run_this_(std::forward<Callable>(c))
    { }
    void run() override { run_this_(); }

    static const size_t index_;

    static std::unique_ptr<RunnableBase>
    construct_from_bytes(void* data) {
      // TODO write this
      assert(false);
    }

    size_t get_index() const override { return index_; }

  private:
    Callable run_this_;
};

template <typename Callable>
const size_t Runnable<Callable>::index_ = register_runnable<Runnable<Callable>>();

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

    void run() override {
      meta::splat_tuple<AccessHandleBase>(
        _get_args_to_splat(),
        Functor()
      );
    }


    static std::unique_ptr<RunnableBase>
    construct_from_bytes(void* data) {
      // TODO inplace rather than move construction?
      // TODO !!! special treatment for handles

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

    size_t get_index() const override { return index_; }
};

template <typename Functor, typename... Args>
const size_t FunctorRunnable<Functor, Args...>::index_ =
  register_runnable<FunctorRunnable<Functor, Args...>>();

// </editor-fold>

////////////////////////////////////////////////////////////////////////////////

// <editor-fold desc="TaskBase and its descendants">

class TaskBase : public abstract::backend::runtime_t::task_t
{
  protected:

    typedef abstract::backend::runtime_t::handle_t handle_t;
    typedef abstract::backend::runtime_t::key_t key_t;
    typedef abstract::backend::runtime_t::version_t version_t;
    typedef types::shared_ptr_template<handle_t> handle_ptr;
    typedef types::handle_container_template<handle_t*> get_deps_container_t;
    typedef std::unordered_set<handle_t*> needs_handle_container_t;


    get_deps_container_t dependencies_;

    needs_handle_container_t needs_read_deps_;
    needs_handle_container_t needs_write_deps_;
    std::vector<handle_ptr> all_deps_;

    key_t name_;

  public:

    template <typename DependencyHandleSharedPtr>
    void add_dependency(
      const DependencyHandleSharedPtr& dep,
      bool needs_read_data,
      bool needs_write_data
    ) {
      dependencies_.insert(dep.get());
      all_deps_.push_back(dep);
      assert(needs_read_data || needs_write_data);
      if(needs_read_data) needs_read_deps_.insert(dep.get());
      if(needs_write_data) needs_write_deps_.insert(dep.get());
    }

    template <typename AccessHandleT1, typename AccessHandleT2>
    void do_capture(
      AccessHandleT1& captured,
      AccessHandleT2 const& source_and_continuing
    );

    ////////////////////////////////////////////////////////////////////////////////
    // Implementation of abstract::frontend::Task

    const get_deps_container_t&
    get_dependencies() const override {
      return dependencies_;
    }

    bool
    needs_read_data(
      const handle_t* handle
    ) const override {
      // TODO figure out why we need a const cast here
      return needs_read_deps_.find(const_cast<handle_t*>(handle)) != needs_read_deps_.end();
    }

    bool
    needs_write_data(
      const handle_t* handle
    ) const override {
      // TODO figure out why we need a const cast here
      return needs_write_deps_.find(const_cast<handle_t*>(handle)) != needs_write_deps_.end();
    }

    const key_t&
    get_name() const override {
      return name_;
    }

    void
    set_name(const key_t& name) override {
      name_ = name;
    }

    bool
    is_migratable() const override {
      // Ignored for now:
      return false;
    }

    virtual void run() override {
      assert(runnable_);
      pre_run_setup();
      runnable_->run();
      post_run_cleanup();
    }

    size_t get_packed_size() const override {
      // TODO
      assert(false);
      return 0;
    }

    void pack(void* allocated) const override {
      // TODO
      assert(false);
    }

    // end implementation of abstract::frontend::Task
    ////////////////////////////////////////////////////////////////////////////////

    void pre_run_setup() {
      for(auto& dep_ptr : this->all_deps_) {
        // Make sure the access handle does in fact have it
        assert(not dep_ptr.unique());
        dep_ptr.reset();
      }
    }

    void post_run_cleanup() { }

    void set_runnable(std::unique_ptr<RunnableBase>&& r) {
      runnable_ = std::move(r);
    }

    virtual ~TaskBase() noexcept { }

    TaskBase* current_create_work_context = nullptr;

    std::vector<std::function<void()>> registrations_to_run;
    std::vector<std::function<void()>> post_registration_ops;

  private:

    std::unique_ptr<RunnableBase> runnable_;

    friend abstract::frontend::Task* unpack_task(void* packed_data);

};

class TopLevelTask
  : public TaskBase
{
  public:

    void run() override {
      // Abort, as specified.  This should never be called.
      abort();
    }

};


// </editor-fold>

////////////////////////////////////////////////////////////////////////////////

} // end namespace detail

// implementation of abstract::frontend::unpack_task

namespace abstract {

namespace frontend {

inline types::unique_ptr_template<abstract::frontend::Task>
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


#include <darma/impl/task_capture_impl.h>


#endif /* DARMA_RUNTIME_TASK_H_ */
