/*
//@HEADER
// ************************************************************************
//
//                          serial_backend.cc
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

#define DHARMA_SERIAL_DEBUG 0

#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>

#if DHARMA_SERIAL_DEBUG
std::mutex __output_mutex;
#define DEBUG(...) { \
  std::unique_lock<std::mutex> __output_lg(__output_mutex); \
  std::cout << "RANK " << rank << "::" <<  __VA_ARGS__ << std::endl; \
}
#define DARMA_ASSERTION_BEGIN __output_mutex.lock(), std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl
#define DARMA_ASSERTION_END __output_mutex.unlock(), std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl
#else
#define DEBUG(...)
#endif

#include "serial_backend.h"

namespace serial_backend {

using namespace darma_runtime;

class SerialDataBlock
  : public abstract::backend::DataBlock {
 public:

  void *get_data() override {
    return data_;
  }

  void *data_ = nullptr;

};

class SerialRuntime
  : public abstract::backend::runtime_t {
 protected:

  typedef typename abstract::backend::runtime_t::task_t task_t;
  typedef typename abstract::backend::runtime_t::task_unique_ptr task_unique_ptr;
  typedef typename abstract::backend::runtime_t::handle_t handle_t;
  typedef typename abstract::backend::runtime_t::version_t version_t;


  std::unordered_map<std::pair<key_t, version_t>, handle_t *const> registered_handles;

  struct published_data_block {
    published_data_block() = default;
    published_data_block(
      published_data_block &&other
    ) : data_block(std::move(other.data_block)),
        fetch_handles_expected(other.fetch_handles_expected.load()),
        fetch_handles_out(other.fetch_handles_out.load()),
        published_version(std::move(other.published_version)) { }

    SerialDataBlock *data_block = nullptr;
    std::atomic<size_t> fetch_handles_expected = {0};
    std::atomic<size_t> fetch_handles_out = {0};
    version_t published_version;
  };

 public:

  typedef std::unordered_map<std::pair<key_t, key_t>, published_data_block> published_data_blocks_t;
  typedef std::mutex shared_mutex_t;
  typedef std::unique_lock<shared_mutex_t> unique_lock_t;
  typedef std::unique_lock<shared_mutex_t> shared_lock_t;

 protected:

  static published_data_blocks_t published_data_blocks;
  static shared_mutex_t published_data_blocks_mtx;

  std::unordered_map<intptr_t, key_t> fetch_handles;

  std::string
  get_key_string(
    key_t const &key
  ) const {
    std::stringstream sstr;
    key.print_human_readable(", ", sstr);
    return sstr.str();
  }

  std::string
  get_key_version_string(
    const handle_t *const handle
  ) const {
    std::stringstream sstr;
    sstr << "{key: ";
    handle->get_key().print_human_readable(", ", sstr);
    sstr << " | version: ";
    handle->get_version().print_human_readable(".", sstr);
    sstr << "}";
    return sstr.str();
  }

 public:


  void
  register_task(
    task_unique_ptr &&task
  ) override {
    auto &deps = task->get_dependencies();


    for (auto &&dep : deps) {
      // TODO check that dependency is registered

      if (task->needs_read_data(dep)) {
        DARMA_ASSERT_MESSAGE(dep->is_satisfied(), "Handle " << get_key_version_string(dep) <<
          " not satisfied upon usage in task");
      }
      if (task->needs_write_data(dep)) {
        assert(not dep->version_is_pending());
        if (dep->get_version() != version_t()) {
          DARMA_ASSERT_MESSAGE(dep->is_writable(), "Handle " << get_key_version_string(dep) <<
            " not writable upon usage in modifying task");
        }
        else {
          assert(not task->needs_read_data(dep));
          auto *serman = dep->get_serialization_manager();
          size_t size = serman->get_metadata_size(nullptr);
          SerialDataBlock *db = new SerialDataBlock();
          db->data_ = malloc(size);
          dep->satisfy_with_data_block(db);
        }
      }
    }

    running_tasks.push(task.get());
    task->run();
    running_tasks.pop();

  }


  task_t *const
  get_running_task() const override {
    return running_tasks.top();
  }


  void register_handle(
    handle_t *const handle
  ) override {
    DEBUG("register_handle called with handle " << get_key_version_string(handle));
    auto found = registered_handles.find({handle->get_key(), handle->get_version()});
    assert(found == registered_handles.end());
    registered_handles.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(handle->get_key(), handle->get_version()),
      std::forward_as_tuple(handle)
    );
  }

  void register_fetching_handle(
    handle_t *const handle,
    const key_t &user_version_tag
  ) override {

    DEBUG("register_fetching_handle called for handle " << get_key_version_string(handle) << " and version tag "
            << get_key_string(user_version_tag));

    published_data_block *pdb_ptr;
    bool ready = false;

    // Spin lock until handle is published by someone else
    while (not ready) {
      shared_lock_t _lg(published_data_blocks_mtx);
      auto found_pdb = published_data_blocks.find({handle->get_key(), user_version_tag});
      auto end_iter = published_data_blocks.end();
      if (found_pdb != published_data_blocks.end()) {
        pdb_ptr = &found_pdb->second;
        ready = true;
        auto &pdb = *pdb_ptr;

        assert(pdb.fetch_handles_expected > 0);
        handle->satisfy_with_data_block(pdb.data_block);
        ++(pdb.fetch_handles_out);
        --(pdb.fetch_handles_expected);
        handle->set_version(pdb.published_version);
      }
    }


    fetch_handles.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(intptr_t(handle)),
      std::forward_as_tuple(user_version_tag)
    );

  }

  void
  release_read_only_usage(
    handle_t *const handle
  ) override {
    DEBUG("releasing read only usage of handle " << get_key_version_string(handle));
    auto found = registered_handles.find({handle->get_key(), handle->get_version()});
    if (found == registered_handles.end()) {
      assert(fetch_handles.find(intptr_t(handle)) != fetch_handles.end());
    }
    auto found_fh = fetch_handles.find(intptr_t(handle));
    if (found_fh == fetch_handles.end()) {
      handle->allow_writes();
    }
  }

  void
  release_handle(
    const handle_t *const handle
  ) override {
    DEBUG("release called for handle " << get_key_version_string(handle));
    auto found_fetcher = fetch_handles.find(intptr_t(handle));
    if (found_fetcher == fetch_handles.end()) {

      auto &k = handle->get_key();
      auto &v = handle->get_version();

      auto found = registered_handles.find({k, v});
      assert(found != registered_handles.end());
      // remove ourselves from the registered handles
      registered_handles.erase(found);

      handle_t *subsequent = nullptr;

      // 1. First, if a handle with key k and version ++(v.push_subversion())}
      //    is registered, that handle is the subsequent.
      auto v_rule_1 = v;
      v_rule_1.push_subversion();
      ++v_rule_1;
      auto found_rule_1 = registered_handles.find({k, v_rule_1});
      if (found_rule_1 != registered_handles.end()) {
        subsequent = found_rule_1->second;
      }
      else {
        // 2. If no handle with {k, ++(v.push_subversion())} is registered, but a
        //    handle with {k, ++v} is registered, then {k, ++v} is the subsequent.
        auto v_rule_2 = v;
        ++v_rule_2;
        auto found_rule_2 = registered_handles.find({k, v_rule_2});
        if (found_rule_2 != registered_handles.end()) {
          subsequent = found_rule_2->second;
        }
        else {
          // 3. If neither {k, ++(v.push_subversion())} nor {k, ++v} is registered,
          //    then the following procedure should be followed to determine the
          //    subsequent:
          //      a. Set v'=++(v.pop_subversion()).
          //      b. If {k, v'} exists, it is the subsequent.
          //      c. If {k, v'} does not exist and v'.depth() == 1, no subsequent
          //         exists.
          //      d. Set v'=++(v'.pop_subversion()) and return to step b.
          // NOTE:  In the serial version, we know there won't be out of order releases like this, so all we need
          // to do is handle the last at version depth case here.

          if (v.depth() > 1) {
            auto v_rule_3 = v;
            v_rule_3.pop_subversion();
            ++v_rule_3;
            auto found_rule_3 = registered_handles.find({k, v_rule_3});
            if (found_rule_3 != registered_handles.end()) {
              subsequent = found_rule_3->second;
            }

          }

        }

      }

      if (subsequent) {
        // we can only do this in the serial version
        DARMA_ASSERT_MESSAGE(handle->is_satisfied(), "Handle " << get_key_version_string(handle) <<
          " with subsequent " << get_key_version_string(subsequent) << " not satisfied before release");
        subsequent->satisfy_with_data_block(handle->get_data_block());
        DEBUG("satifying subsequent " << get_key_version_string(subsequent) << " with handle "
                << get_key_version_string(handle));
      }
      else if (handle->is_satisfied()) {
        if (handle->get_data_block()->get_data() != nullptr) {
          free(handle->get_data_block()->get_data());
        }
        delete handle->get_data_block();
      }
    }
    else {
      // fetching handle
      shared_lock_t _lg(published_data_blocks_mtx);
      auto found_pdb = published_data_blocks.find({handle->get_key(), found_fetcher->second});
      assert(found_pdb != published_data_blocks.end());
      auto &pdb = found_pdb->second;
      --(pdb.fetch_handles_out);
      if (pdb.fetch_handles_expected.load() == 0 and pdb.fetch_handles_out.load() == 0) {
        free(pdb.data_block->get_data());
        delete pdb.data_block;
        published_data_blocks.erase(found_pdb);
      }
      fetch_handles.erase(found_fetcher);
    }

  }

  void publish_handle(
    handle_t *const handle,
    const key_t &version_tag,
    const size_t n_fetchers = 1,
    bool is_final = false
  ) override {
    assert(not is_final); // version 0.2.0
    if (n_fetchers == 0) return;

    DEBUG("publish called on handle " << get_key_version_string(handle));

    unique_lock_t _lg(published_data_blocks_mtx);

    auto &k = handle->get_key();

    auto found_pdb = published_data_blocks.find({k, version_tag});
    assert(found_pdb == published_data_blocks.end());

    assert(handle->is_satisfied());
    assert(not handle->is_writable());

    assert(n_fetchers >= 1);

    // Just do a copy for now
    published_data_block pdb;
    pdb.data_block = new SerialDataBlock();
    size_t data_size = handle->get_serialization_manager()->get_metadata_size(nullptr);
    pdb.data_block->data_ = malloc(data_size);
    memcpy(pdb.data_block->data_, handle->get_data_block()->get_data(), data_size);

    pdb.fetch_handles_expected = n_fetchers;
    pdb.published_version = handle->get_version();

    published_data_blocks.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(k, version_tag),
      std::forward_as_tuple(std::move(pdb))
    );
  }


  void
  finalize() override {
    // nothing to do here
    DEBUG("finalize called");
    if (rank == 0) {
      for (auto &&thr : other_ranks_if_rank_0) {
        thr.join();
      }
    }
  }


  task_unique_ptr top_level_task;
  std::stack<task_t *> running_tasks;

  size_t rank;
  size_t n_ranks;
  std::vector<std::thread> other_ranks_if_rank_0;

};

SerialRuntime::published_data_blocks_t SerialRuntime::published_data_blocks;
SerialRuntime::shared_mutex_t SerialRuntime::published_data_blocks_mtx;

} // end namespace serial_backend


//thread_local darma_runtime::abstract::backend::runtime_t* darma_runtime::detail::backend_runtime = nullptr;

int main(int argc, char **argv) {
  int ret = (*(darma_runtime::detail::_darma__generate_main_function_ptr<>()))(argc, argv);
  // TODO: check if runtime finalized before deleting
  if (darma_runtime::detail::backend_runtime) {
    delete darma_runtime::detail::backend_runtime;
  }
  return ret;
}

void
darma_runtime::abstract::backend::darma_backend_initialize(
  int &argc, char **&argv,
  darma_runtime::abstract::backend::runtime_t *&backend_rt,
  types::unique_ptr_template<
    typename darma_runtime::abstract::backend::runtime_t::task_t
  > top_level_task
) {
  using namespace darma_runtime::detail;
  size_t n_ranks = 1;
  ArgParser args = {
    {"h", "help", "print help (not implemented)"},
    {"", "serial-backend-n-ranks", 1}
  };
  args.parse(argc, argv);
  if (args["serial-backend-n-ranks"].as<bool>()) {
    n_ranks = args["serial-backend-n-ranks"].as<size_t>();
  }
  if (args.program_name() != DARMA_SERIAL_BACKEND_SPAWNED_RANKS_PROCESS_STRING) {

    auto *tmp_rt = new serial_backend::SerialRuntime;
    tmp_rt->top_level_task = std::move(top_level_task);
    tmp_rt->running_tasks.push(tmp_rt->top_level_task.get());
    tmp_rt->top_level_task->set_name(
      darma_runtime::make_key(DARMA_BACKEND_SPMD_NAME_PREFIX, 0, n_ranks)
    );
    tmp_rt->rank = 0;
    tmp_rt->n_ranks = n_ranks;
    backend_rt = tmp_rt;

    int tmp_argc = argc;
    char **tmp_argv = argv;
    for (size_t irank = 1; irank < n_ranks; ++irank) {
      tmp_rt->other_ranks_if_rank_0.emplace_back([irank, n_ranks, tmp_argc, tmp_argv] {
        char *new_argv[tmp_argc + 4];
        int new_argv_spot = 0;
        std::string new_prog_name(DARMA_SERIAL_BACKEND_SPAWNED_RANKS_PROCESS_STRING);
        new_argv[new_argv_spot++] = const_cast<char *>(new_prog_name.c_str());
        for (int iarg = 1; iarg < tmp_argc; ++iarg) {
          new_argv[new_argv_spot++] = tmp_argv[iarg];
        }
        std::string rank_arg("--" + std::string(DARMA_SERIAL_BACKEND_SPAWNED_RANK_NUM_OPTION));
        std::string rank_num = std::to_string(irank);
        new_argv[new_argv_spot++] = const_cast<char *>(rank_arg.c_str());
        new_argv[new_argv_spot++] = const_cast<char *>(rank_num.c_str());
        std::string n_rank_arg("--serial-backend-n-ranks");
        std::string n_rank_num = std::to_string(n_ranks);
        new_argv[new_argv_spot++] = const_cast<char *>(n_rank_arg.c_str());
        new_argv[new_argv_spot++] = const_cast<char *>(n_rank_num.c_str());


        int new_argc = new_argv_spot;

        int thread_main_return = (*(darma_runtime::detail::_darma__generate_main_function_ptr<>()))(
          new_argc, new_argv
        );

        assert(thread_main_return == 0);

        // TODO: check if runtime finalized before deleting
        if (darma_runtime::detail::backend_runtime) {
          delete darma_runtime::detail::backend_runtime;
        }
      });
    }

  }
  else {
    // This is a spawned program
    ArgParser new_args = {
      {"", DARMA_SERIAL_BACKEND_SPAWNED_RANK_NUM_OPTION, 1}
    };
    new_args.parse(argc, argv);
    size_t irank = new_args[DARMA_SERIAL_BACKEND_SPAWNED_RANK_NUM_OPTION].as<size_t>();
    auto *my_tmp_rt = new serial_backend::SerialRuntime;
    my_tmp_rt->top_level_task = std::move(top_level_task);
    my_tmp_rt->running_tasks.push(my_tmp_rt->top_level_task.get());
    my_tmp_rt->top_level_task->set_name(
      darma_runtime::make_key(DARMA_BACKEND_SPMD_NAME_PREFIX, irank, n_ranks)
    );
    my_tmp_rt->rank = irank;
    my_tmp_rt->n_ranks = n_ranks;
    backend_rt = my_tmp_rt;
  }
}
