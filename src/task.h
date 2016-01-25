/*
//@HEADER
// ************************************************************************
//
//                          task.h
//                         dharma_new
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

#ifndef DHARMA_RUNTIME_TASK_H_
#define DHARMA_RUNTIME_TASK_H_

#include <unordered_map>

#include "abstract/frontend/task.h"
#include "handle.h"

#include <tinympl/greater.hpp>
#include <tinympl/int.hpp>
#include <tinympl/delay.hpp>
#include <tinympl/identity.hpp>
#include <tinympl/at.hpp>
#include <tinympl/erase.hpp>

namespace dharma_runtime {

namespace detail {

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
      m::identity<default_key_t>
    >::type::type key_t;

  private:
    static constexpr const size_t _possible_version_index =
      std::conditional<_first_is_key, m::int_<1>, m::int_<0>>::type::value
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
      m::identity<default_version_t>
    >::type::type version_t;

  private:

    typedef typename m::erase<
      (size_t)0, (size_t)(_first_is_key + _version_given),
      m::vector<Types...>, m::vector
    >::type other_types;

  public:

};

}

namespace detail {

template <
  typename key_type,
  typename version_type
>
class TaskBase
  : public abstract::frontend::Task<key_type, version_type, std::vector, std::shared_ptr>
{
  protected:

    template <typename... Ts>
    using container = std::vector<Ts...>;
    template <typename... Ts>
    using smart_ptr_template = std::shared_ptr<Ts...>;

    typedef abstract::frontend::DependencyHandle<key_type, version_type> handle_t;
    typedef smart_ptr_template<handle_t> handle_ptr;

    container<handle_ptr> inputs_;
    container<handle_ptr> outputs_;
    container<handle_ptr> in_outs_;

    virtual void
    do_run() const =0;

  public:

    const container<handle_ptr>&
    get_inputs() const override {
      return inputs_;
    }

    const container<handle_ptr>&
    get_outputs() const override {
      return outputs_;
    }

    const container<handle_ptr>&
    get_in_outs() const override {
      return in_outs_;
    }

    void run() const override {
      do_run();
    }

    virtual ~TaskBase() noexcept { }


};

template <
  typename Lambda,
  typename... Types
>
class Task
  : public TaskBase<
      typename task_traits<Types...>::key_t,
      typename task_traits<Types...>::version_t
    >,
    std::enable_shared_from_this<Task>
{

  protected:

    template <typename T>
    using dep_handle_t = DependencyHandle<T, key_type, version_type>;
    template <typename... Ts>
    using smart_ptr_template = std::shared_ptr<Ts...>;
    template <typename T>
    using dep_handle_ptr = smart_ptr_template<dep_handle_t<T>>;

    typedef smart_ptr_template<Lambda> lambda_ptr;

    typedef typename smart_ptr_traits<smart_ptr_template>::template maker<lambda> lambda_ptr_maker;


  public:

    template <typename T>
    void add_input(const dep_handle_ptr<T>& dep)
    {
      this->inputs_.push_back(dep);
    }

    template <typename T>
    void add_output(const dep_handle_ptr<T>& dep)
    {
      this->outputs_.push_back(dep);
    }

    template <typename T>
    void add_in_out(const dep_handle_ptr<T>& dep)
    {
      this->in_outs_.push_back(dep);
    }

    void set_lambda(
      lambda_t&& the_lambda
    ) {
      // this is sloppy and should will make it nearly impossible to inline tasks, but for now...
      lambda_ = lambda_ptr_maker()(std::forward(the_lambda));
    }

    void do_run() const override {
      auto& rtc = detail::thread_runtime;
      rtc.current_running_task = shared_from_this();
      (*lambda_)();
      rtc.current_running_task = 0;
    }


  private:

    lambda_ptr lambda_;


};

} // end namespace detail

} // end namespace dharma_runtime



#endif /* DHARMA_RUNTIME_TASK_H_ */
