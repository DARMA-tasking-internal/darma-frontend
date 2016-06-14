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

#include <darma/interface/frontend/types.h>

#include <darma/interface/backend/flow.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/frontend/handle.h>
#include <darma/interface/frontend/task.h>
#include <darma/interface/frontend/use.h>
#include <darma/interface/frontend/publication_details.h>
#include <darma/interface/defaults/darma_main.h>

#include <thread>
#include <atomic>
#include <mutex>

#include <iostream>
#include <deque>
#include <utility>
#include <unordered_map>

/*
 * Debugging prints with mutex
 */
#define __THREADS_BACKEND_DEBUG__ 1
#if __THREADS_BACKEND_DEBUG__
std::mutex __output_mutex;
#define DEBUG_PRINT(fmt, arg...)					\
  do {									\
    std::unique_lock<std::mutex> __output_lg(__output_mutex);		\
    printf("(%zu): DEBUG threads: " fmt, threads_backend::this_rank, ##arg); \
    fflush(stdout);							\
  } while (0);
#else
  #define DEBUG_PRINT(fmt, arg...)
#endif

// TODO: hack this in here for now
namespace std {
  using darma_runtime::detail::SimpleKey;
  using darma_runtime::detail::key_traits;
  
  template<>
  struct hash<SimpleKey> {
    size_t operator()(SimpleKey const& in) const {
      return key_traits<SimpleKey>::hasher()(in);
    }
  };
}

namespace threads_backend {
  using namespace darma_runtime;
  using namespace darma_runtime::abstract::backend;

  // TODO: threads not implemented
  std::vector<std::thread> live_threads;
  std::vector<std::thread> live_ranks;

  // TL state
  __thread abstract::frontend::Task* current_task = 0;
  __thread size_t this_rank = 0;
  __thread size_t flow_label = 100;

  // global
  size_t n_ranks = 1;
  
  struct PackedDataBlock {
    virtual void *get_data() { return data_; }
    size_t size_;
    void *data_ = nullptr;
  };
  
  struct PublishedBlock {
    types::key_t key;
    PackedDataBlock* data = nullptr;
    std::atomic<size_t> expected = {0}, done = {0};

    PublishedBlock(PublishedBlock&& other)
      : data(std::move(other.data))
      , expected(other.expected.load())
      , done(other.done.load())
      , key(std::move(other.key)) {
    }

    PublishedBlock() = default;
  };

  // global publish
  std::mutex rank_publish;
  std::unordered_map<std::pair<types::key_t, types::key_t>, PublishedBlock> published;
  
  struct ThreadsFlow
    : public abstract::backend::Flow {

    // save all kinds of unnecessary info about each that we probably
    // do not need right now, along with inefficient explicit pointers
    ThreadsFlow* same, *next, *forward, *backward;
    darma_runtime::abstract::frontend::Handle* handle;
    void* mem;

    bool isNullFlow, isInitialFlow, ready;
    size_t label;

    ThreadsFlow(darma_runtime::abstract::frontend::Handle* handle_)
      : same(0)
      , next(0)
      , forward(0)
      , backward(0)
      , handle(handle_)
      , isNullFlow(false)
      , isInitialFlow(false)
      , ready(false)
      , label(++flow_label)
    { }

    bool check_ready() { return ready || (same ? check_same() : false); }
    bool check_same()  { return same->check_ready(); }
  };

  std::ostream& operator<<(std::ostream& st, const ThreadsFlow& f) {
    st <<
      "label = " << f.label << ", " <<
      "null = " << f.isNullFlow  << ", " <<
      "init = " << f.isInitialFlow  << ", " <<
      "ready = " << f.ready << ", " <<
      "same = " << (f.same ? f.same->label : 0) << ", " <<
      "next = " << (f.next ? f.next->label : 0) << ", " <<
      "forward = " << (f.forward ? f.forward->label : 0) << ", " <<
      "backward = " << (f.backward ? f.backward->label : 0);
    return st;
  }
  
  class ThreadsRuntime
    : public abstract::backend::Runtime {

  public:
    ThreadsRuntime(const ThreadsRuntime& tr) {}

    // TODO: fix any memory consistency bugs with data coordination we
    // need a fence at the least
    std::unordered_map<types::key_t, void*> data;

    // TODO: multi-threaded half-implemented not working..
    std::vector<std::atomic<size_t>*> deque_counter;
    std::vector<std::mutex> deque_mutex;
    std::vector<std::deque<types::unique_ptr_template<abstract::frontend::Task> > > deque;
    std::atomic<bool> finished;
    std::atomic<size_t> ranks;

    types::unique_ptr_template<abstract::frontend::Task> top_level_task;

    ThreadsRuntime() {
      std::atomic_init(&finished, false);
      std::atomic_init<size_t>(&ranks, 1);
    }

  protected:
    virtual void
    register_task(types::unique_ptr_template<abstract::frontend::Task>&& task) {
      DEBUG_PRINT("register task\n");

      #if defined(_BACKEND_MULTITHREADED_RUNTIME)
        // scoped lock for insertion
        {
	  std::lock_guard<std::mutex> guard(deque_mutex[this_rank]);
	  deque[this_rank].emplace_front(std::move(task));
        }

        std::atomic_fetch_add<size_t>(deque_counter[this_rank], 1);
      #endif

      check_dep_task(std::move(task));
    }

    void check_dep_task(types::unique_ptr_template<abstract::frontend::Task>&& task) {
      bool ready = true;
      
      for (auto&& dep : task->get_dependencies()) {
	ThreadsFlow* f_in  = reinterpret_cast<ThreadsFlow*>(dep->get_in_flow());
	ThreadsFlow* f_out = reinterpret_cast<ThreadsFlow*>(dep->get_out_flow());

	DEBUG_PRINT("Task dependency:\n");

	#if 0
	  std::cout << "\t FLOW in = "  << *f_in << std::endl;
	  std::cout << "\t FLOW out = " << *f_out << std::endl;
	#endif

	// set readiness of this flow based on symmetric flow structure
	const bool check_ready = f_in->check_ready();

	// try to set readiness if we can find a backward flow that is ready
	const bool check_back_ready = f_in->backward ? f_in->backward->ready : false;

	f_in->ready = check_ready || check_back_ready;
	
	ready &= f_in->ready;
      }

      // the task is ready
      if (ready) {
	abstract::frontend::Task* prev = current_task;
	types::unique_ptr_template<abstract::frontend::Task> cur = std::move(task);
	current_task = cur.get();
	cur.get()->run();
	current_task = prev;
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
      
      #if 0
        std::cout << "REGISTER Use with flows: " << std::endl;
        std::cout << "\t FLOW in = "  << *f_in << std::endl;
        std::cout << "\t FLOW out = " << *f_out << std::endl;
      #endif

      const auto& key = u->get_handle()->get_key();

      // ensure it exists in the hash
      if (data.find(key) == data.end()) {
	DEBUG_PRINT("Could not find data!!!!!!!\n");
	exit(240);
      }
      
      u->get_data_pointer_reference() = data[key];
    }
  
    virtual Flow*
    make_initial_flow(darma_runtime::abstract::frontend::Handle* handle) {
      ThreadsFlow* f = new ThreadsFlow(handle);
      f->isInitialFlow = true;
      f->ready = true;
      //std::cout << "MAKE initial: " << "FLOW = "  << *f << std::endl;

      // allocate some space for this object
      const size_t sz = handle->get_serialization_manager()->get_metadata_size();

      DEBUG_PRINT("make initial flow: size = %ld\n", sz);

      // TODO: we need to call "new" here or some memory pool or some
      // type of managed pointer. leaks like hell and is unsafe!
      void* const mem = operator new(sz);

      // save in hash
      const auto& key = handle->get_key();
      data[key] = mem;
      
      return f;
    }

    virtual Flow*
    make_fetching_flow(darma_runtime::abstract::frontend::Handle* handle,
		       types::key_t const& version_key) {
      DEBUG_PRINT("make fetching flow\n");

      bool found = false;

      do {
	{
	  // TODO: big lock to handling fetching
	  std::lock_guard<std::mutex> guard(threads_backend::rank_publish);
	  
	  const auto& key = handle->get_key();
	  const auto& iter = published.find({version_key, key});
	  
	  if (iter != published.end()) {
	    PublishedBlock* pub_ptr = &iter->second;
	    auto &pub = *pub_ptr;

	    void* unpack_to = operator new(pub.data->size_);

	    DEBUG_PRINT("unpacking data\n");
	    
	    handle->get_serialization_manager()->unpack_data(unpack_to, pub.data->data_);
	    data[key] = unpack_to;
	    
	    assert(pub.expected > 0);

	    ++(pub.done);
	    --(pub.expected);
	    
	    found = true;
	  }
	}
      } while (!found);

      ThreadsFlow* f = new ThreadsFlow(handle);
      /// set it ready immediately
      f->ready = true;
      return f;
    }

    virtual Flow*
    make_null_flow(darma_runtime::abstract::frontend::Handle* handle) {
      DEBUG_PRINT("make null flow\n");
      ThreadsFlow* f = new ThreadsFlow(handle);
      f->isNullFlow = true;
      //std::cout << "MAKE null: " << "FLOW = "  << *f << std::endl;
      return f;
    }
  
    virtual Flow*
    make_same_flow(Flow* from,
		   flow_propagation_purpose_t purpose) {
      DEBUG_PRINT("make same flow: %d\n", purpose);
      ThreadsFlow* f  = reinterpret_cast<ThreadsFlow*>(from);
      //std::cout << "MAKE input (same): " << "FLOW = "  << *f << std::endl;

      ThreadsFlow* f_same = new ThreadsFlow(0);
      f_same->same = f;
      //std::cout << "MAKE new flow with same: " << "FLOW = "  << f_same->label << std::endl;
      return f_same;
    }

    virtual Flow*
    make_forwarding_flow(Flow* from,
			 flow_propagation_purpose_t purpose) {
      DEBUG_PRINT("make forwarding flow: %d\n", purpose);

      ThreadsFlow* f  = reinterpret_cast<ThreadsFlow*>(from);
      //std::cout << "MAKE input (forwarding): " << "FLOW = "  << *f << std::endl;

      ThreadsFlow* f_forward = new ThreadsFlow(0);
      f->forward = f_forward;
      f_forward->backward = f;
      //std::cout << "MAKE new flow with forward: " << "FLOW = "  << f_forward->label << std::endl;
      return f_forward;
    }

    virtual Flow*
    make_next_flow(Flow* from,
		   flow_propagation_purpose_t purpose) {
      DEBUG_PRINT("make next flow: %d\n", purpose);

      ThreadsFlow* f  = reinterpret_cast<ThreadsFlow*>(from);
      //std::cout << "MAKE input (next): " << "FLOW = "  << *f << std::endl;

      ThreadsFlow* f_next = new ThreadsFlow(0);
      f->next = f_next;
      //std::cout << "MAKE new flow with next: " << "FLOW = "  << f_next->label << std::endl;
      return f_next;
    }
  
    virtual void
    release_use(darma_runtime::abstract::frontend::Use* u) {
      DEBUG_PRINT("release use\n");

      ThreadsFlow* f_in  = reinterpret_cast<ThreadsFlow*>(u->get_in_flow());
      ThreadsFlow* f_out = reinterpret_cast<ThreadsFlow*>(u->get_out_flow());
      
      // enable next forward flow
      if (f_in->forward) {
	f_in->forward->ready = true;
      }
      
      // enable each out flow
      f_out->ready = true;
    }
  
    virtual void
    publish_use(darma_runtime::abstract::frontend::Use* f,
		darma_runtime::abstract::frontend::PublicationDetails* details) {
      DEBUG_PRINT("publish use\n");

      {
	// TODO: big lock to handling publishing
	std::lock_guard<std::mutex> guard(threads_backend::rank_publish);

	const auto &expected = details->get_n_fetchers();
	auto &key = details->get_version_name();

	assert(expected >= 1);

	const darma_runtime::abstract::frontend::Handle* handle = f->get_handle();
	const size_t size = handle
	  ->get_serialization_manager()
	  ->get_packed_data_size(f->get_data_pointer_reference());

	PublishedBlock pub;
	pub.data = new PackedDataBlock();

	// set data block for published block
	pub.data->data_ = operator new(size);
	pub.data->size_ = handle
	  ->get_serialization_manager()
	  ->get_metadata_size();

	// call pack method to put it inside the allocated buffer
	handle
	  ->get_serialization_manager()
	  ->pack_data(f->get_data_pointer_reference(),
		      pub.data->data_);

	pub.expected = expected;
	pub.key = handle->get_key();

	published.emplace(std::piecewise_construct,
			  std::forward_as_tuple(key, pub.key),
			  std::forward_as_tuple(std::move(pub)));
      }
    }
  
    virtual void finalize() {
      DEBUG_PRINT("finalize\n");

      if (this_rank == 0) {
	DEBUG_PRINT("total threads to join is %zu\n", threads_backend::live_ranks.size());
    
	// TODO: memory consistency bug on live_ranks size here..with relaxed ordering
	for (size_t i = 0; i < threads_backend::live_ranks.size(); i++) {
	  DEBUG_PRINT("main thread joining %zu\n", i);
	  threads_backend::live_ranks[i].join();
	  DEBUG_PRINT("join complete %zu\n", i);
	}
      }
    }
  };

  // HACK: put an instance of the backend runtime here for the main thread..
  threads_backend::ThreadsRuntime* runtime_inst;
  
} // end namespace threads_backend

// TODO: not used, for multi-threaded runtime
void
scheduler_loop(size_t thd, threads_backend::ThreadsRuntime* runtime) {
  // read rank from TL variable
  const size_t local_rank = threads_backend::this_rank;
  // read number of ranks
  const size_t num_ranks = std::atomic_load<size_t>(&runtime->ranks);

  DEBUG_PRINT("starting scheduler for %zu, number of ranks %zu\n",
	      local_rank, num_ranks);
  
  // spin on termination
  while (!std::atomic_load(&runtime->finished)) {
    if (std::atomic_load<size_t>(runtime->deque_counter[local_rank]) > 0) {
      // scoped lock for removal attempt
      {
	std::lock_guard<std::mutex> guard(runtime->deque_mutex[local_rank]);
      }
    }
  }
}

void
start_thread_handler(const size_t thd, threads_backend::ThreadsRuntime* runtime) {
  //std::cout << "thread handler running" << std::endl;
  DEBUG_PRINT("%ld: thread handler starting\n", thd);

  // set thread-local rank
  threads_backend::this_rank = thd;

  // run scheduler, when this returns we have terminated!
  scheduler_loop(thd, runtime);
}

void
start_rank_handler(const size_t rank,
		   const int argc,
		   char** argv) {
  DEBUG_PRINT("%ld: rank handler starting\n", rank);

  // set TL variables
  threads_backend::this_rank = rank;

  // call into main
  const int ret = (*(darma_runtime::detail::_darma__generate_main_function_ptr<>()))(argc, argv);
}

int main(int argc, char **argv) {
  int ret = (*(darma_runtime::detail::_darma__generate_main_function_ptr<>()))(argc, argv);
  // TODO: check if runtime finalized before deleting
  // if (darma_runtime::detail::backend_runtime) {
  //   delete darma_runtime::detail::backend_runtime;
  // }

  //scheduler_loop(0, threads_backend::runtime_inst);

  #if defined(_BACKEND_MULTITHREADED_RUNTIME)
    for (size_t i = 0; i < threads_backend::live_threads.size(); i++) {
      DEBUG_PRINT("main thread joining %zu\n", i);
      threads_backend::live_threads[i].join();
    }
  #endif

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
  size_t ranks = 2, n_threads = 1;

  detail::ArgParser args = {
    {"t", "threads", 1},
    {"r", "ranks",   1}
  };

  args.parse(argc, argv);
  
  // read number of threads from the command line
  if (args["threads"].as<bool>()) {
    n_threads = args["threads"].as<size_t>();
    
    // TODO: require this backend not to run with multiple threads per rank
    assert(n_threads == 1);
  }

  // read number of ranks from the command line
  if (args["ranks"].as<bool>()) {
    ranks = args["ranks"].as<size_t>();

    assert(ranks > 0);
    // TODO: write some other sanity assertions here about the size of ranks...
  }

  if (threads_backend::this_rank == 0) {
    threads_backend::n_ranks = ranks;
  }
  
  DEBUG_PRINT("MY rank = %zu, n_ranks = %zu\n",
	      threads_backend::this_rank,
	      threads_backend::n_ranks);
  
  auto* runtime = new threads_backend::ThreadsRuntime();
  backend_runtime = runtime;

  if (threads_backend::this_rank == 0) {
    DEBUG_PRINT("rank = %zu, ranks = %zu, threads = %zu\n",
		threads_backend::this_rank,
		threads_backend::n_ranks,
		n_threads);
  
    // launch std::thread for each rank
    threads_backend::live_ranks.resize(threads_backend::n_ranks - 1);
    for (size_t i = 0; i < threads_backend::n_ranks - 1; ++i) {
      threads_backend::live_ranks[i] = std::thread(start_rank_handler, i + 1, argc, argv);
    }
    
    #if defined(_BACKEND_MULTITHREADED_RUNTIME)
      // store the launched threads in a vector
      threads_backend::live_threads.resize(n_threads - 1);
      threads_backend::runtime_inst = runtime;
    
      // init atomics/deques used to coordinate threads
      {
        std::vector<std::mutex> local_mutex(n_threads);
        runtime->deque_mutex.swap(local_mutex);
      }
    
      runtime->deque_counter.resize(n_threads);
      runtime->deque.resize(n_threads);
      std::atomic_store(&runtime->ranks, n_threads);
    
      for (size_t i = 0; i < n_threads; ++i) {
        runtime->deque_counter[i] = new std::atomic<size_t>();
        std::atomic_init<size_t>(runtime->deque_counter[i], 0);
      }
    
      // launch and save each std::thread
      for (size_t i = 0; i < n_threads - 1; ++i) {
        threads_backend::live_threads[i] = std::thread(start_thread_handler, i + 1, runtime);
      }
    #endif
  }

  // setup root task
  runtime->top_level_task = std::move(top_level_task);
  runtime->top_level_task->set_name(darma_runtime::make_key(DARMA_BACKEND_SPMD_NAME_PREFIX,
							    threads_backend::this_rank,
							    threads_backend::n_ranks));
  threads_backend::current_task = runtime->top_level_task.get();
}

#endif /* _THREADS_BACKEND_RUNTIME_ */
