/*
//@HEADER
// ************************************************************************
//
//                      top_level.h
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

#ifndef DARMA_IMPL_TOP_LEVEL_H
#define DARMA_IMPL_TOP_LEVEL_H


#include <darma/interface/frontend/top_level.h>
#include <darma/impl/task/task.h>

#include <functional>
#include <darma/interface/frontend/top_level_task.h>


namespace darma_runtime {

namespace detail {

namespace _impl {

using top_level_callable = std::function<void(std::vector<std::string>)>;

template <typename=void>
top_level_callable&
_get_top_level_callable_ref() {
  static top_level_callable tlc;
  return tlc;
}

template <typename Callable>
struct TopLevelCallableRegistrar {
  TopLevelCallableRegistrar() {
    top_level_callable& tlc = _get_top_level_callable_ref<void>();
    tlc = top_level_callable([](std::vector<std::string> cl_args) {
      Callable()(cl_args);
    });
  }
};

template <typename ReturnType, typename... Args>
struct TopLevelCallableRegistrar<ReturnType (*)(Args...)> {
  TopLevelCallableRegistrar(ReturnType (*fp)(Args...)) {
    top_level_callable& tlc = _get_top_level_callable_ref<void>();
    tlc = top_level_callable([fp](std::vector<std::string> cl_args) {
      (*fp)(cl_args);
    });
  }
};

} // end namespace _impl

#define DARMA_REGISTER_TOP_LEVEL_FUNCTOR(...) \
  static const auto _darma__top_level_task_registration \
    = ::darma_runtime::detail::_impl::TopLevelCallableRegistrar<__VA_ARGS__>();
#define DARMA_REGISTER_TOP_LEVEL_FUNCTION(...) \
  static const auto _darma__top_level_task_registration \
    = ::darma_runtime::detail::_impl::TopLevelCallableRegistrar<decltype(&(__VA_ARGS__))>(&(__VA_ARGS__));


struct TopLevelTaskImpl
#if _darma_has_feature(task_migration)
  : PolymorphicSerializationAdapter<
      TopLevelTaskImpl,
      abstract::frontend::Task,
      abstract::frontend::TopLevelTask<TaskBase>
    >
#else
  : abstract::frontend::TopLevelTask<TaskBase>
#endif // _darma_Has_feature(task_migration)
{
  private:

    std::vector<std::string> cl_args_;

  public:

    void run() override {
      _impl::_get_top_level_callable_ref()(cl_args_);
    }

    void set_command_line_arguments(std::vector<std::string> clargs) {
      cl_args_ = std::move(clargs);
    }

    template <typename ArchiveT>
    void serialize(ArchiveT& ar) {
      ar | cl_args_;
    }
};

} // end namespace detail

namespace frontend {

inline
std::unique_ptr<abstract::frontend::TopLevelTask<darma_runtime::detail::TaskBase>>
darma_top_level_setup(int& argc, char**& argv) {
  std::vector<std::string> cl_args;
  cl_args.reserve(argc);
  for(int i = 0; i < argc; ++i) cl_args.emplace_back(argv[i]);
  auto tlt = std::make_unique<darma_runtime::detail::TopLevelTaskImpl>();
  tlt->set_command_line_arguments(std::move(cl_args));
  return { std::move(tlt) };
}

inline
std::unique_ptr<abstract::frontend::TopLevelTask<darma_runtime::detail::TaskBase>>
darma_top_level_setup(std::vector<std::string>&& args) {
  auto tlt = std::make_unique<darma_runtime::detail::TopLevelTaskImpl>();
  tlt->set_command_line_arguments(std::move(args));
  return { std::move(tlt) };
}


} // end namespace frontend
} // end namespace darma_runtime

#endif //DARMA_IMPL_TOP_LEVEL_H
