/*
//@HEADER
// ************************************************************************
//
//                      functor_task.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_FUNCTOR_TASK_H
#define DARMAFRONTEND_FUNCTOR_TASK_H

#include "task_fwd.h"

#include <darma/impl/serialization/traits.h>
#include <darma/impl/functor_traits.h>
#include <darma/impl/util.h>
#include <darma/impl/handle.h>
#include <darma/impl/util/smart_pointers.h>
#include <darma/impl/polymorphic_serialization.h>

#include "task_base.h"
#include "task_ctor_helper.h"

namespace darma_runtime {
namespace detail {

struct FunctorCaptureSetupHelper : protected CaptureSetupHelperBase {

  FunctorCaptureSetupHelper() = default;

  FunctorCaptureSetupHelper(
    TaskBase* parent_task,
    CaptureManager* current_capture_context
  ) {
    pre_capture_setup(parent_task, current_capture_context);
  }

};


template <
  typename Functor,
  typename... Args
>
struct FunctorCapturer : protected FunctorCaptureSetupHelper {

  using traits = functor_traits<Functor>;
  using call_traits = functor_call_traits<Functor, Args&&...>;
  using stored_args_tuple_t = typename call_traits::args_tuple_t;

  static constexpr auto n_functor_args_min = traits::n_args_min;
  static constexpr auto n_functor_args_max = traits::n_args_max;

  // We only support fixed argument count functors for now:
  static constexpr auto n_functor_args = n_functor_args_min;
  STATIC_ASSERT_VALUE_EQUAL(n_functor_args, n_functor_args_max);
  STATIC_ASSERT_VALUE_EQUAL(n_functor_args, sizeof...(Args));


  template <typename... ArgsDeduced>
  FunctorCapturer(
    TaskBase* parent_task,
    CaptureManager* capture_manager,
    ArgsDeduced&&... args_deduced
  ) : FunctorCaptureSetupHelper(parent_task, capture_manager),
      stored_args_(std::forward<ArgsDeduced>(args_deduced)...)
  {
    post_capture_cleanup(parent_task, capture_manager);
  }

  explicit
  FunctorCapturer(
    stored_args_tuple_t&& args_moved
  ) : stored_args_(std::move(args_moved))
  { }

  template <size_t... Idxs>
  void run_functor(std::integer_sequence<size_t, Idxs...>) {
    Functor{}(
      call_traits::template call_arg_traits<Idxs>::get_converted_arg(
        std::get<Idxs>(stored_args_)
      )...
    );
  }

  //============================================================================
  // <editor-fold desc="data members"> {{{1

  stored_args_tuple_t stored_args_;

  // </editor-fold> end data members }}}1
  //============================================================================

};


template <
  typename Functor,
  typename... Args
>
struct FunctorTask
#if _darma_has_feature(task_migration)
    : PolymorphicSerializationAdapter<
        FunctorTask<Functor, Args...>,
        abstract::frontend::Task,
        TaskBase
      >,
#else
    : TaskCtorHelper,
#endif
      FunctorCapturer<Functor, Args...>
{
  public:

    #if _darma_has_feature(task_migration)
    using base_t = PolymorphicSerializationAdapter<
      FunctorTask<Functor, Args...>,
      abstract::frontend::Task,
      TaskBase
    >;
    #else
    using base_t = TaskBase;
    #endif

    using capturer_t = FunctorCapturer<Functor, Args...>;
    using stored_args_tuple_t = typename capturer_t::stored_args_tuple_t;

  private:


    explicit
    FunctorTask(
      typename capturer_t::stored_args_tuple_t&& stored_args_in
    ) : capturer_t(
          std::move(stored_args_in)
        )
    { }

  public:

    template <
      typename AllowAliasingDescription,
      typename... ArgsDeduced
    >
    FunctorTask(
      variadic_constructor_arg_t /*unused*/,
      TaskBase* parent_task,
      types::key_t const& name_key,
      AllowAliasingDescription&& aliasing_desc,
      bool is_data_parallel,
      ArgsDeduced&&... args_in
    ) : base_t(
          parent_task,
          name_key,
          std::forward<AllowAliasingDescription>(aliasing_desc),
          is_data_parallel
        ),
        capturer_t(
          parent_task,
          this,
          std::forward<ArgsDeduced&&>(args_in)...
        )
    { }


    //==============================================================================
    // <editor-fold desc="run() method"> {{{1

  public:

    void run() override {
      this->run_functor(std::index_sequence_for<Args...>{});
    }

    // </editor-fold> end run() method }}}1
    //==============================================================================


    //==========================================================================
    // <editor-fold desc="Serialization"> {{{1

    template <typename ArchiveT>
    static FunctorTask& reconstruct(void* allocated, ArchiveT& ar);

  private:

    template <typename StoredArgT>
    int _add_if_dep(std::true_type, StoredArgT& arg) {
      this->add_dependency(*arg.current_use_base_->use_base);
      // return something to make the make_tuple work
      // (the compiler should remove the unused variable anyway)
      return 0;
    }

    template <typename StoredArgT>
    int _add_if_dep(std::false_type, StoredArgT&) {
      /* do nothing */
      // return something to make the make_tuple work
      // (the compiler should remove the unused variable anyway)
      return 0;
    }

    template <size_t... Idxs>
    void _unpack_deps(
      std::integer_sequence<size_t, Idxs...>
    ) {
      auto _unused = std::make_tuple(
        _add_if_dep(
          typename std::is_base_of<
            AccessHandleBase,
            std::decay_t<std::tuple_element_t<Idxs, typename capturer_t::stored_args_tuple_t>>
          >::type{},
          std::get<Idxs>(this->stored_args_)
        )...
      );
    }

  public:

    template <typename ArchiveT>
    void serialize(ArchiveT& ar) {
      if(not ar.is_unpacking()) {
        // Need to pack in this order for reconstruct
        ar | this->stored_args_;
        this->TaskBase::template do_serialize(ar);
      }
      else {
        // unpack of args happens in reconstruct, so only need to invoke the base
        // TODO serdes framework should work better with class hierarchies
        // there should be something here that allows me to call unpack instead of
        // calling serialize
        this->TaskBase::template do_serialize(ar);

       // Now we need to iterate over the arguments and add the uses as dependencies
        _unpack_deps(std::index_sequence_for<Args...>{});
      }
    }

    // </editor-fold> end Serialization }}}1
    //==========================================================================


    bool is_migratable() const override {
      return true;
    }

    ~FunctorTask() override = default;

};

template <typename Functor, typename... Args>
template <typename ArchiveT>
FunctorTask<Functor, Args...>&
FunctorTask<Functor, Args...>::reconstruct(
  void* allocated, ArchiveT& ar
) {

  auto* allocated_args = abstract::backend::get_backend_memory_manager()->allocate(
    sizeof(stored_args_tuple_t),
    serialization::detail::DefaultMemoryRequirementDetails{}
  );

  ar.template unpack_item<stored_args_tuple_t>(allocated_args);

  auto& stored_args_unpacked = *static_cast<stored_args_tuple_t*>(allocated_args);

  auto* rv = new (allocated) FunctorTask(std::move(stored_args_unpacked));

  stored_args_unpacked.~stored_args_tuple_t();

  abstract::backend::get_backend_memory_manager()->deallocate(
    allocated_args, sizeof(stored_args_tuple_t)
  );

  return *rv;
}

} // end namespace detail
} // end namespace darma_runtime


#endif //DARMAFRONTEND_FUNCTOR_TASK_H
