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

#include <darma/interface/backend/types.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/frontend/dependency_handle.h>
#include <darma/interface/frontend/task.h>

#include <darma/impl/util.h>
#include <darma/impl/runtime.h>
#include <darma/impl/handle_fwd.h>
#include <darma/impl/meta/callable_traits.h>
#include <darma/impl/handle.h>


namespace darma_runtime {

namespace detail {

////////////////////////////////////////////////////////////////////////////////

// <editor-fold desc="Task template parameter parsing nonsense (to be removed probably)" defaultstate="collapsed">

// TODO decide between this and "is_key<>", which would not check the concept but would be self-declared
template <typename T>
struct meets_key_concept;
template <typename T>
struct meets_version_concept;

namespace template_tags {
template <typename T> struct input { typedef T type; };
template <typename T> struct output { typedef T type; };
template <typename T> struct in_out { typedef T type; };
}

namespace m = tinympl;
namespace mv = tinympl::variadic;
namespace pl = tinympl::placeholders;
namespace tt = template_tags;

template <typename... Types>
struct task_traits {
  private:
    static constexpr const bool _first_is_key = m::and_<
        m::greater<m::int_<sizeof...(Types)>, m::int_<0>>,
        m::delay<
          meets_key_concept,
          mv::at<0, Types...>
        >
      >::value;

  public:
    typedef typename std::conditional<
      _first_is_key,
      mv::at<0, Types...>,
      m::identity<types::key_t>
    >::type::type key_t;

  private:
    static constexpr const size_t _possible_version_index =
      std::conditional<_first_is_key, m::int_<1>, m::int_<0>>::type::value;
    static constexpr const bool _version_given = m::and_<
      m::greater<m::int_<sizeof...(Types)>, m::int_<_possible_version_index>>,
      m::delay<
        meets_version_concept,
        mv::at<_possible_version_index, Types...>
      >
    >::value;


  public:
    typedef typename std::conditional<
      _version_given,
      mv::at<_possible_version_index, Types...>,
      m::identity<types::version_t>
    >::type::type version_t;

  private:

    typedef typename m::erase<
      (size_t)0, (size_t)(_first_is_key + _version_given),
      m::vector<Types...>, m::vector
    >::type other_types;

  public:

};

// </editor-fold>

////////////////////////////////////////////////////////////////////////////////

// <editor-fold desc="Functor traits">

// TODO strictness specifiers
template <
  typename Functor
>
struct functor_traits {

  template <typename T>
  using compile_time_modifiable_archetype = std::integral_constant<bool, T::is_compile_time_modifiable>;
  template <typename T>
  using decayed_is_compile_time_modifiable = meta::detected_or_t<std::false_type,
    compile_time_modifiable_archetype, std::decay_t<T>
  >;


  template <typename T>
  using decayed_is_access_handle = is_access_handle<std::decay_t<T>>;

  static constexpr auto n_args_min = meta::callable_traits<Functor>::n_args_min;
  static constexpr auto n_args_max = meta::callable_traits<Functor>::n_args_max;

  template <size_t N>
  struct arg_traits {
    struct is_access_handle
      : meta::callable_traits<Functor>::template arg_n_matches<decayed_is_access_handle, N>
    { };
    struct is_compile_time_modifiable
      : meta::callable_traits<Functor>::template arg_n_matches<decayed_is_compile_time_modifiable, N>
    { };
    struct is_nonconst_lvalue_reference
      : meta::callable_traits<Functor>::template arg_n_is_nonconst_lvalue_reference<N>
    { };
  };

  template <typename CallArg, typename IndexWrapped>
  struct call_arg_traits {
    template <typename T>
    using compile_time_read_access_analog_archetype = typename T::CompileTimeReadAccessAnalog;
    template <typename T>
    using value_type_archetype = typename T::value_type;

    static constexpr auto index = IndexWrapped::value;

    template <typename T>
    using detected_convertible_to_value = meta::is_detected_convertible<T, value_type_archetype, std::decay_t<CallArg>>;

    static constexpr bool is_access_handle = functor_traits::arg_traits<index>::is_access_handle::value;

    static constexpr bool is_convertible_to_value =
      meta::callable_traits<Functor>::template arg_n_matches<detected_convertible_to_value, index>::value;

    static constexpr bool is_nonconst_lvalue_reference =
      functor_traits::arg_traits<index>::is_nonconst_lvalue_reference::value;

    //DARMA_TYPE_DISPLAY(call_arg_traits);

    // Did the caller give a handle and the functor give an argument that was a const (reference to)
    // a type that is convertible to that handle's value_type?
    static constexpr bool is_const_conversion_capture =
      // Well, not if the argument is an AccessHandle
      (not is_access_handle) and
      // If so, the argument at index has to pass this convertibility predicate
      is_convertible_to_value and
      // also, it can't be an lvalue reference that expects modifications
      (not is_nonconst_lvalue_reference)
    ; // end is_const_conversion_capture

    static constexpr bool is_read_only_capture =
      is_const_conversion_capture or
      (functor_traits::arg_traits<index>::is_access_handle::value
      and not functor_traits::arg_traits<index>::is_compile_time_modifiable::value)
    ;

    typedef std::conditional<
      is_read_only_capture,
      meta::detected_t<compile_time_read_access_analog_archetype, std::decay_t<CallArg>> const,
      std::remove_cv_t<std::remove_reference_t<CallArg>> const
    > args_tuple_entry;

  };

  template <typename CallArgsAndIndexWrapped>
  using arg_traits_tuple_entry = typename
    tinympl::splat_to<CallArgsAndIndexWrapped, call_arg_traits>::type::args_tuple_entry;


  template <typename... CallArgs>
  using args_tuple_t = typename tinympl::transform<
    typename tinympl::zip<tinympl::sequence, tinympl::sequence,
      tinympl::as_sequence_t<tinympl::vector<CallArgs...>>,
      tinympl::as_sequence_t<typename tinympl::range_c<size_t, 0, sizeof...(CallArgs)>::type>
    >::type,
    arg_traits_tuple_entry,
    std::tuple
  >::type;


};

// </editor-fold>

////////////////////////////////////////////////////////////////////////////////

// <editor-fold desc="Runnable and RunnableBase">

class RunnableBase {
 public:
   virtual void run() =0;
   virtual ~RunnableBase() { }
};

template <typename Callable>
struct Runnable : public RunnableBase
{
 public:
  explicit
  Runnable(Callable&& c)
    : run_this_(std::forward<Callable>(c))
  { }
  void run() override { run_this_(); }

 private:
  Callable run_this_;
};

template <
  typename Functor,
  typename... Args
>
class FunctorRunnable
  : public RunnableBase
{
  private:

    typedef functor_traits<Functor> traits;
    static constexpr auto n_functor_args_min = traits::n_args_min;
    static constexpr auto n_functor_args_max = traits::n_args_max;

    static_assert(
      sizeof...(Args) <= n_functor_args_max && sizeof...(Args) >= n_functor_args_min,
      "Functor task created with wrong number of arguments"
    );

    typename traits::template args_tuple_t<Args...> args_;

  public:

    FunctorRunnable(
      variadic_constructor_arg_t const,
      Args&&... args
    ) : args_(std::forward<Args>(args)...)
    { }

    void run() override {
      meta::splat_tuple<AccessHandleBase>(
        std::move(args_),
        Functor()
      );
    }
};


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

} // end namespace darma_runtime


#include <darma/impl/task_capture_impl.h>


#endif /* DARMA_RUNTIME_TASK_H_ */
