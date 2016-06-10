/*
//@HEADER
// ************************************************************************
//
//                         threads.cc
//                           darma
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
// JL => Jonathan Lifflander (jliffla@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#if !defined(_THREADS_BACKEND_RUNTIME_)
#define _THREADS_BACKEND_RUNTIME_

#define DEBUG_PRINT(fmt, arg...)				\
  do {								\
    printf("DEBUG threads: " fmt, ##arg);			\
    fflush(stdout);						\
  } while (0);

#include <darma/interface/frontend/types.h>

#include <darma/interface/backend/flow.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/frontend/handle.h>
#include <darma/interface/frontend/task.h>
#include <darma/interface/frontend/use.h>
#include <darma/interface/frontend/publication_details.h>
#include <darma/interface/defaults/darma_main.h>

#include <thread>
#include <iostream>

namespace threads_backend {
  using namespace darma_runtime;
  using namespace darma_runtime::abstract::backend;

  std::vector<std::thread> live_threads;

  types::unique_ptr_template<abstract::frontend::Task> top_level_task;
  __thread abstract::frontend::Task* current_task = 0;
  size_t flow_label = 100;

  std::unordered_map<darma_runtime::abstract::frontend::Handle*, void*> data;
  
  struct ThreadsFlow
    : public abstract::backend::Flow {

    ThreadsFlow* same, *next;
    darma_runtime::abstract::frontend::Handle* handle;
    void* mem;

    bool isNullFlow, isInitialFlow, ready;
    size_t label;

    ThreadsFlow(darma_runtime::abstract::frontend::Handle* handle_)
      : same(0)
      , next(0)
      , handle(handle_)
      , isNullFlow(false)
      , isInitialFlow(false)
      , ready(false)
      , label(++flow_label)
    { }

    bool check_ready() { return ready || (same ? check_same() : false); }
    bool check_same() { return same->check_ready(); }
  };

  std::ostream& operator<<(std::ostream& st, const ThreadsFlow& f) {
    st <<
      "label = " << f.label << ", " <<
      "null = " << f.isNullFlow  << ", " <<
      "init = " << f.isInitialFlow  << ", " <<
      "ready = " << f.ready  << ", " <<
      "same = " << (f.same ? f.same->label : 0);
    return st;
  }
  
  class ThreadsRuntime
    : public abstract::backend::Runtime {
    
    virtual void
    register_task(types::unique_ptr_template<abstract::frontend::Task>&& task) {
      DEBUG_PRINT("register task\n");

      bool ready = true;
      
      for (auto&& dep : task->get_dependencies()) {
	std::cout << dep << std::endl;

	ThreadsFlow* f_in  = reinterpret_cast<ThreadsFlow*>(dep->get_in_flow());
	ThreadsFlow* f_out = reinterpret_cast<ThreadsFlow*>(dep->get_out_flow());
	std::cout << "Task dependency: " << std::endl;
	std::cout << "\t FLOW in = "  << *f_in << std::endl;
	std::cout << "\t FLOW out = " << *f_out << std::endl;

	ready &= f_in->check_ready();
      }

      if (ready) {
	abstract::frontend::Task* prev = current_task;
	current_task = task.get();
	task->run();
	current_task = prev;
      }

      for (auto&& dep : task->get_dependencies()) {
	ThreadsFlow* f_out = reinterpret_cast<ThreadsFlow*>(dep->get_out_flow());
	f_out->ready = true;
      }
    }

    virtual darma_runtime::abstract::frontend::Task*
    get_running_task() const {
      DEBUG_PRINT("get running task\n");
      return current_task;
    }

    virtual void
    register_use(darma_runtime::abstract::frontend::Use* u) {
      DEBUG_PRINT("register use\n");
      ThreadsFlow* f_in  = reinterpret_cast<ThreadsFlow*>(u->get_in_flow());
      ThreadsFlow* f_out = reinterpret_cast<ThreadsFlow*>(u->get_out_flow());
      std::cout << "REGISTER Use with flows: " << std::endl;
      std::cout << "\t FLOW in = "  << *f_in << std::endl;
      std::cout << "\t FLOW out = " << *f_out << std::endl;

      darma_runtime::abstract::frontend::Handle* h =
	const_cast<darma_runtime::abstract::frontend::Handle*>(u->get_handle());
      u->get_data_pointer_reference() = data[h];
    }
  
    virtual Flow*
    make_initial_flow(darma_runtime::abstract::frontend::Handle* handle) {
      DEBUG_PRINT("make initial flow\n");
      ThreadsFlow* f = new ThreadsFlow(handle);
      f->isInitialFlow = true;
      f->ready = true;
      std::cout << "MAKE initial: " << "FLOW = "  << *f << std::endl;

      // allocate some space for this object
      size_t sz = handle->get_serialization_manager()->get_metadata_size();
      void* mem = malloc(sz);

      // save in hash
      data[handle] = mem;
      
      return f;
    }

    virtual Flow*
    make_fetching_flow(darma_runtime::abstract::frontend::Handle* handle,
		       types::key_t const& version_key) {
      DEBUG_PRINT("make fetching flow\n");
    }

    virtual Flow*
    make_null_flow(darma_runtime::abstract::frontend::Handle* handle) {
      DEBUG_PRINT("make null flow\n");
      ThreadsFlow* f = new ThreadsFlow(handle);
      f->isNullFlow = true;
      std::cout << "MAKE null: " << "FLOW = "  << *f << std::endl;
      return f;
    }
  
    virtual Flow*
    make_same_flow(Flow* from,
		   flow_propagation_purpose_t purpose) {
      DEBUG_PRINT("make same flow: %d\n", purpose);
      ThreadsFlow* f  = reinterpret_cast<ThreadsFlow*>(from);
      std::cout << "MAKE input (same): " << "FLOW = "  << *f << std::endl;

      ThreadsFlow* f_same = new ThreadsFlow(0);
      f_same->same = f;
      std::cout << "MAKE new flow with same: " << "FLOW = "  << f_same->label << std::endl;
      return f_same;
    }

    virtual Flow*
    make_forwarding_flow(Flow* from,
			 flow_propagation_purpose_t purpose) {
      DEBUG_PRINT("make forwarding flow: %d\n", purpose);
    }

    virtual Flow*
    make_next_flow(Flow* from,
		   flow_propagation_purpose_t purpose) {
      DEBUG_PRINT("make next flow: %d\n", purpose);

      ThreadsFlow* f  = reinterpret_cast<ThreadsFlow*>(from);
      std::cout << "MAKE input (next): " << "FLOW = "  << *f << std::endl;

      ThreadsFlow* f_next = new ThreadsFlow(0);
      f_next->next = f;
      std::cout << "MAKE new flow with next: " << "FLOW = "  << f_next->label << std::endl;
      return f_next;
    }
  
    virtual void
    release_use(darma_runtime::abstract::frontend::Use* u) {
      DEBUG_PRINT("release use\n");
    }
  
    virtual void
    publish_use(darma_runtime::abstract::frontend::Use* f,
		darma_runtime::abstract::frontend::PublicationDetails* details) {
      DEBUG_PRINT("publish use\n");
    }
  
    virtual void finalize() {
      DEBUG_PRINT("finalize\n");
    }
  };

} // end namespace threads_backend

void
start_thread_handler(size_t thd) {
  //std::cout << "thread handler running" << std::endl;
  DEBUG_PRINT("%ld: thread handler starting\n", thd);
}

int main(int argc, char **argv) {
  int ret = (*(darma_runtime::detail::_darma__generate_main_function_ptr<>()))(argc, argv);
  // TODO: check if runtime finalized before deleting
  // if (darma_runtime::detail::backend_runtime) {
  //   delete darma_runtime::detail::backend_runtime;
  // }
  return ret;
}


void
darma_runtime::abstract::backend::darma_backend_initialize(
  int &argc, char **&argv,
  darma_runtime::abstract::backend::Runtime *&backend_runtime,
  types::unique_ptr_template<
    typename darma_runtime::abstract::backend::Runtime::task_t
  > top_level_task
) {
  size_t n_ranks = 1;
  detail::ArgParser args = { {"", "threads", 1} };

  args.parse(argc, argv);
  
  // read number of ranks from the command line
  if (args["threads"].as<bool>()) {
    n_ranks = args["threads"].as<size_t>();
  }

  // store the launched threads in a vector
  threads_backend::live_threads.resize(n_ranks);
  
  // launch and save each std::thread
  for (size_t i = 0; i < n_ranks - 1; ++i) {
    threads_backend::live_threads[i] = std::thread(start_thread_handler, i);
  }

  auto* runtime = new threads_backend::ThreadsRuntime();
  backend_runtime = runtime;

  // move task around
  threads_backend::top_level_task = std::move(top_level_task);
  threads_backend::top_level_task->set_name(darma_runtime::make_key(DARMA_BACKEND_SPMD_NAME_PREFIX, 0, n_ranks));
  threads_backend::current_task = threads_backend::top_level_task.get();
}

#endif /* _THREADS_BACKEND_RUNTIME_ */
