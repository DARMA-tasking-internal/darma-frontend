/*
//@HEADER
// ************************************************************************
//
//                          threads_backend.cc
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

#define DARMA_ASSERTION_BEGIN debug_output_mtx.lock(), std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl
#define DARMA_ASSERTION_END debug_output_mtx.unlock(), std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl

#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <tuple>
#include <utility>
#include <thread>
#include <algorithm>

static std::mutex debug_output_mtx;

#include <darma/meta/tuple_for_each.h>
#include <darma/meta/splat_tuple.h>
#include <darma/darma_assert.h>

#include "threads_backend.h"

using namespace darma_runtime;

#define DHARMA_THREADS_DEBUG 1

#if DHARMA_THREADS_DEBUG
#define DEBUG(...) { \
  std::lock_guard<std::mutex> ___dbg_lg(debug_output_mtx); \
  std::cout << __VA_ARGS__ << std::endl; \
}
#else
#define DEBUG(...)
#endif

#define DARMA_ABORT(...) { \
  std::stringstream sstr; \
  sstr << __VA_ARGS__; \
  dharma_abort_string(sstr.str()); \
}

void
dharma_abort_string(const std::string& str) {
  {
    std::lock_guard<std::mutex> ___dbg_lg(debug_output_mtx);
    std::cerr << "ERROR: " << str << std::endl;
  }
  abort();
}

struct locker {
  template <typename... Lockables>
  inline decltype(auto)
  operator()(Lockables&&... ls) const {
    std::lock(ls...);
    return std::forward_as_tuple(std::forward<Lockables>(ls)...);
  }
};

template <template <class...> class LockTemplate>
struct lock_maker {
  template <typename Lockable>
  inline decltype(auto)
  operator()(Lockable&& l) const {
    return LockTemplate<std::decay_t<Lockable>>(
      std::forward<Lockable>(l),
      std::defer_lock
    );
  }
};

template <template <class...> class LockTemplate, typename... Lockables>
decltype(auto)
make_lock_guards(Lockables&&... lockables) {
  return locker()(
    lock_maker<LockTemplate>()(std::forward<Lockables>(lockables))...
  );
}

template <typename... Lockables>
decltype(auto)
make_shared_lock_guards(Lockables&&... lockables) {
  return make_lock_guards<std::shared_lock>(std::forward<Lockables>(lockables)...);
}

template <typename... Lockables>
decltype(auto)
make_unique_lock_guards(Lockables&&... lockables) {
  return make_lock_guards<std::unique_lock>(std::forward<Lockables>(lockables)...);
}

template <typename Lockable>
struct mixed_lockable_wrapper {
  mixed_lockable_wrapper(Lockable* const mtx_ptr_in, bool needs_unique_in)
    : mtx_ptr(mtx_ptr_in), needs_unique(needs_unique_in)
  { }
  Lockable* mtx_ptr;
  bool needs_unique;
  bool operator<(const mixed_lockable_wrapper& other) const {
    return (intptr_t)mtx_ptr < (intptr_t)other.mtx_ptr;
  }
};

template <typename Container>
void do_safe_pointer_lock(const Container& shared, const Container& unique) {
  typedef typename std::remove_pointer<typename Container::value_type>::type lockable_t;
  std::vector<mixed_lockable_wrapper<lockable_t>> all_to_lock;
  all_to_lock.reserve(shared.size() + unique.size());
  for(auto&& s : shared) {
    all_to_lock.emplace_back(s, false);
  }
  for(auto&& u : unique) {
    all_to_lock.emplace_back(u, true);
  }
  std::sort(all_to_lock.begin(), all_to_lock.end());

  for(auto&& wrapper : all_to_lock) {
    if(wrapper.needs_unique) {
      wrapper.mtx_ptr->lock();
    }
    else {
      wrapper.mtx_ptr->lock_shared();
    }
  }
}

template <typename Container>
void do_safe_pointer_unlock(const Container& shared, const Container& unique) {
  typedef typename std::remove_pointer<typename Container::value_type>::type lockable_t;
  std::vector<mixed_lockable_wrapper<lockable_t>> all_to_unlock;
  all_to_unlock.reserve(shared.size() + unique.size());
  for(auto&& s : shared) {
    all_to_unlock.emplace_back(s, false);
  }
  for(auto&& u : unique) {
    all_to_unlock.emplace_back(u, true);
  }
  std::sort(all_to_unlock.begin(), all_to_unlock.end(), [](auto&& a, auto&& b){ return b < a; });

  for(auto&& wrapper : all_to_unlock) {
    if(wrapper.needs_unique) {
      wrapper.mtx_ptr->unlock();
    }
    else {
      wrapper.mtx_ptr->unlock_shared();
    }
  }
}

class DataBlock
  : public abstract::backend::DataBlock
{
  private:

    void* data_ = nullptr;

  public:

    void* get_data() override {
      return data_;
    }

    //void acquire_data(void* const data) override {
    //  abort(); // not implemented
    //}

    void allocate_data(size_t size) {
      DEBUG("Allocating data block of size " << size
        << " for DataBlock at 0x" << std::hex << intptr_t(this));
      data_ = malloc(size);
    }

    ~DataBlock() {
      if(data_) {
        DEBUG("Freeing data block DataBlock at 0x" << std::hex << intptr_t(this));
        free(data_);
      }
    }

};

static thread_local typename abstract::backend::runtime_t::task_t* running_task_ = nullptr;

void
set_running_task(
  typename abstract::backend::runtime_t::task_t* task
) {
  running_task_ = task;
}

// TODO handle waiting for publications to clear
class ThreadsRuntime
  : public abstract::backend::runtime_t
{
  protected:

    typedef typename abstract::backend::runtime_t::task_unique_ptr task_unique_ptr;
    typedef typename abstract::backend::runtime_t::key_t key_t;
    typedef typename abstract::backend::runtime_t::version_t version_t;
    typedef typename abstract::backend::runtime_t::task_t task_t;

    typedef typename std::shared_timed_mutex shared_mtx_t;
    typedef typename std::shared_lock<shared_mtx_t> shared_lock_t;
    typedef typename std::unique_lock<shared_mtx_t> unique_lock_t;

    typedef typename std::unordered_map<std::pair<key_t, version_t>, shared_mtx_t> handle_mtxs_t;
    typedef typename std::unordered_map<std::pair<key_t, version_t>, shared_lock_t> read_only_locks_t;
    typedef typename std::unordered_map<std::pair<key_t, version_t>, unique_lock_t> data_ready_locks_t;

    std::unordered_map<std::pair<key_t, version_t>, handle_t* const> handle_ptrs_;
    shared_mtx_t handle_ptrs_mtx_;

    handle_mtxs_t handle_mtxs_;
    shared_mtx_t handle_mtxs_mtx_;

    handle_mtxs_t data_ready_mtxs_;
    shared_mtx_t data_ready_mtxs_mtx_;

    read_only_locks_t read_only_locks_;
    shared_mtx_t read_only_locks_mtx_;

    data_ready_locks_t data_ready_locks_;
    shared_mtx_t data_ready_locks_mtx_;

    std::unordered_set<std::pair<key_t, version_t>> last_at_version_depth_;
    shared_mtx_t last_at_version_depth_mtx_;

    std::unordered_map<task_t*, std::vector<shared_mtx_t*>*> task_shared_locks_;
    shared_mtx_t task_shared_locks_mtx_;

    std::unordered_map<task_t*, std::vector<shared_mtx_t*>*> task_unique_locks_;
    shared_mtx_t task_unique_locks_mtx_;

    std::unordered_map<std::pair<key_t, version_t>, std::pair<key_t, version_t>> handle_to_satisfy_;
    shared_mtx_t handle_to_satisfy_mtx_;

  private:

    std::string
    get_key_version_string(
      const handle_t* const handle
    ) {
      std::stringstream sstr;
      sstr << "{key: ";
      handle->get_key().print_human_readable(", ", sstr);
      sstr << " | version: ";
      handle->get_version().print_human_readable(".", sstr);
      sstr << "}";
      return sstr.str();
    }

    std::deque<std::thread> task_threads_;

  public:

    void register_task(task_unique_ptr&& in_task) override {

      std::vector<shared_mtx_t*> to_lock_shared;
      std::vector<shared_mtx_t*> to_lock_unique;
      std::vector<shared_mtx_t*> shared_locks_to_transfer;
      std::vector<shared_mtx_t*> unique_locks_to_transfer;

      #if DHARMA_THREADS_DEBUG
      std::stringstream needs_locks_sstr;
      #endif

      task_t* parent_task = running_task_;
      const auto& deps = in_task->get_dependencies();
      { // RAII scope for _lg
        auto _lg = make_shared_lock_guards(
          data_ready_mtxs_mtx_, handle_mtxs_mtx_, task_shared_locks_mtx_,
          task_unique_locks_mtx_
        );

        for(auto&& dep : deps) {
          auto kver_pair = std::make_pair(dep->get_key(), dep->get_version());

          bool need_read = in_task->needs_read_data(dep);
          bool need_write = in_task->needs_write_data(dep);
          auto found_mtx = handle_mtxs_.find(kver_pair);
          if(found_mtx == handle_mtxs_.end()) {
            // TODO error here
            abort();
          }

          if(dep->is_satisfied()) {

            auto found_dr = data_ready_mtxs_.find(kver_pair);

            // must not be in the data_ready maps
            assert(found_dr == data_ready_mtxs_.end());

            // The parent task must be holding a lock
            // see if it should be shared or unique
            if(need_write) {
              // Look for a unique lock in the parent
              assert(dep->is_writable());
              auto found_parent = task_unique_locks_.find(parent_task);
              if(found_parent == task_unique_locks_.end()) {
                // TODO error here
                abort();
              }
              auto found_uniq = std::find(found_parent->second->begin(),
                 found_parent->second->end(), &(found_mtx->second));
              if(found_uniq == (*found_parent).second->end()) {
                // This is an error; parent didn't have write, but child wants it
                abort();
              }
              #if DHARMA_THREADS_DEBUG
              needs_locks_sstr << "    transfering unique lock for " << get_key_version_string(dep) << std::endl;
              #endif
              unique_locks_to_transfer.push_back(*found_uniq);
              found_parent->second->erase(found_uniq);
            }
            else {
              assert(need_read);
              // Check if the parent had read priviledges
              auto found_parent = task_shared_locks_.find(parent_task);
              if(found_parent == task_shared_locks_.end()) {
                // TODO error here
                abort();
              }
              auto found_shar = std::find(found_parent->second->begin(),
                 found_parent->second->end(), &(found_mtx->second));
              if(found_shar == found_parent->second->end()) {
                // This is not an error, (it's a priviledge downgrade);
                // it's just not implemented yet
                abort();
              }
              #if DHARMA_THREADS_DEBUG
              needs_locks_sstr << "    transfering shared lock for " << get_key_version_string(dep) << std::endl;
              #endif
              shared_locks_to_transfer.push_back(*found_shar);
              found_parent->second->erase(found_shar);
            }
          }
          else {
            if(dep->get_version() == version_t() and not dep->version_is_pending()) {
              // It's the first version of the dep, so we need to allocate in place
              auto found_dr = data_ready_mtxs_.find(kver_pair);

              // must not be in the data_ready maps
              if(found_dr != data_ready_mtxs_.end()) {
                // TODO error here
                abort();
              }

              // also, this must be a write-only use
              assert(need_write and not need_read);

              DataBlock* db = new DataBlock();
              db->allocate_data(dep->get_serialization_manager()->get_metadata_size(nullptr));

              DEBUG("satisfying handle " << get_key_version_string(dep)
                << " at 0x" << std::hex << intptr_t(dep) << " with empty (allocated) DataBlock: "
                << std::hex << "0x" << intptr_t(db));

              dep->satisfy_with_data_block(db);
              dep->allow_writes();

              DARMA_ASSERT_MESSAGE(dep->is_satisfied(),
                "handle not satisfied: " << get_key_version_string(dep));
            }
            else {
              // we'll need a shared lock the data_ready mutex for all deps
              auto found_dr = data_ready_mtxs_.find(kver_pair);
              if(found_dr == data_ready_mtxs_.end()) {
                // TODO error here
                abort();
              }
              #if DHARMA_THREADS_DEBUG
              needs_locks_sstr << "    needs shared data ready lock for " << get_key_version_string(dep) << std::endl;
              #endif
              to_lock_shared.push_back(&(found_dr->second));
            }

            // if we need read access and not write access, we want to acquire a
            // shared_lock to the handle mutex
            if(need_read and not need_write) {
              #if DHARMA_THREADS_DEBUG
              needs_locks_sstr << "    needs shared lock for " << get_key_version_string(dep) << std::endl;
              #endif
              to_lock_shared.push_back(&(found_mtx->second));
            }
            else if(need_read and need_write) {
              #if DHARMA_THREADS_DEBUG
              needs_locks_sstr << "    needs unique lock for " << get_key_version_string(dep) << std::endl;
              #endif
              to_lock_unique.push_back(&(found_mtx->second));
            }
            else if(not need_read and need_write) {
              // this will still work; it just won't be optimal because the data will get delivered anyway
              #if DHARMA_THREADS_DEBUG
              needs_locks_sstr << "    needs unique lock for " << get_key_version_string(dep) << std::endl;
              #endif
              to_lock_unique.push_back(&(found_mtx->second));
            }
            else { // not need_read and not need_write
              // TODO error here
              abort(); // not implemented
            }
          }
        }
      } // release shared access to handle_mtxs_, data_ready_mtxs_

      DEBUG("registering task with dependencies: " << std::endl;
        for(auto&& idep : in_task->get_dependencies()) {
          std::cout << "    " << get_key_version_string(idep) << ": "
                    << "needs_read=" << std::boolalpha << in_task->needs_read_data(idep)
                    << ", needs_write=" << std::boolalpha << in_task->needs_write_data(idep)
                    << ", satisfied=" << std::boolalpha << idep->is_satisfied()
                    << ", writable=" << std::boolalpha << idep->is_writable()
                    << std::endl;
        }
        std::cout << needs_locks_sstr.str();
        std::cout << "  (Task pointer: " << intptr_t(in_task.get()) << ")"
      );

      task_threads_.emplace_back([
          this,
          parent_task,
          to_lock_shared,
          to_lock_unique,
          unique_locks_to_transfer,
          shared_locks_to_transfer
      ](task_unique_ptr&& task){

        set_running_task(task.get());

        std::vector<shared_mtx_t*> to_unlock_shared;
        std::vector<shared_mtx_t*> to_unlock_unique;
        for(auto&& tls : to_lock_shared) to_unlock_shared.push_back(tls);
        for(auto&& tts : shared_locks_to_transfer) to_unlock_shared.push_back(tts);
        for(auto&& tlu : to_lock_unique) to_unlock_unique.push_back(tlu);
        for(auto&& ttu : unique_locks_to_transfer) to_unlock_unique.push_back(ttu);

        do_safe_pointer_lock(to_lock_shared, to_lock_unique);
        {
          auto _lg = make_unique_lock_guards(task_shared_locks_mtx_, task_unique_locks_mtx_);
          task_shared_locks_.emplace(task.get(), &to_unlock_shared);
          task_unique_locks_.emplace(task.get(), &to_unlock_unique);
        }

        DEBUG("running task with ptr 0x" << std::hex << intptr_t(task.get()));

        task->run();

        DEBUG("finished running task with ptr 0x" << std::hex << intptr_t(task.get()));

        {
          auto _lg = make_unique_lock_guards(task_shared_locks_mtx_, task_unique_locks_mtx_);
          task_unique_locks_.erase(task.get());
          task_shared_locks_.erase(task.get());
        }

        // TODO this is non-compliant!!! If a handle is released during a task, this won't work
        do_safe_pointer_unlock(to_unlock_shared, to_unlock_unique);

      }, std::move(in_task));
    }

    task_t* const get_running_task() const override {
      return running_task_;
    }

    void
    register_handle(handle_t* const handle) override {

      auto kver_pair = std::make_pair(handle->get_key(),handle->get_version());

      {
        auto _lg = make_unique_lock_guards(
          handle_mtxs_mtx_, data_ready_mtxs_mtx_, read_only_locks_mtx_,
          data_ready_locks_mtx_
        );

        DEBUG("registering handle: " << get_key_version_string(handle));

        auto handle_mtx_ipair = handle_mtxs_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(kver_pair),
          std::forward_as_tuple()
        );
        // emplace shared read only lock,
        // to be unlocked when release_read_only_usage() is called
        auto ro_lock_iter_pair = read_only_locks_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(kver_pair),
          std::forward_as_tuple(
            handle_mtx_ipair.first->second,
            std::defer_lock
          )
        );
        // emplace the unique data-ready lock
        // to be unlocked when the data is ready
        if(handle->get_version() == version_t() and not handle->version_is_pending()) {
          // the data needs to be allocated, so don't put anything in the data_ready_lock
          // array.  First use will allocate
          ro_lock_iter_pair.first->second.lock();
        }
        else {
          // we need to wait for the data to be ready
          auto dr_mtx_iter_pair = data_ready_mtxs_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(kver_pair),
            std::forward_as_tuple()
          );
          auto data_ready_ipair = data_ready_locks_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(kver_pair),
            std::forward_as_tuple(
              dr_mtx_iter_pair.first->second,
              std::defer_lock
            )
          );
          std::lock(
            ro_lock_iter_pair.first->second,
            data_ready_ipair.first->second
          );
        }

        if(!ro_lock_iter_pair.second || !handle_mtx_ipair.second) {
          DARMA_ABORT("Can't re-register handle "
            << get_key_version_string(handle) << " that hasn't been released yet"
          );
        }
      } // release a bunch of locks

      { // handle_ptrs RAII context
        auto _lg = unique_lock_t(handle_ptrs_mtx_);
        handle_ptrs_.emplace(kver_pair, handle);
      } // release handle_ptrs

    }

    void
    register_fetching_handle(
      handle_t* const handle,
      const key_t& user_version_tag
    ) override {
      // TODO implement this!
      abort();
    }

    void
    release_read_only_usage(
      const handle_t* const handle
    ) override
    {
      auto _lg = unique_lock_t(read_only_locks_mtx_);
      DARMA_ASSERT_NOT_NULL(handle);
      DEBUG("releasing read only usage for " << get_key_version_string(handle)
                << " at 0x" << std::hex << intptr_t(handle) << " with DataBlock: "
                << std::hex << "0x" << intptr_t(handle->get_data_block()));

      auto found = read_only_locks_.find(
        std::pair<key_t, version_t>(
          handle->get_key(),
          handle->get_version()
        )
      );
      // Raise an error if it's not registered
      if(found == read_only_locks_.end()) {
        auto found_mtx = handle_mtxs_.find(
          std::pair<key_t, version_t>(
            handle->get_key(),
            handle->get_version()
          )
        );
        if(found_mtx == handle_mtxs_.end()) {
          // it's unregistered, or it's already been released
          DARMA_ABORT("Can't release read-only usage for unregistered handle "
            << get_key_version_string(handle)
          );
        }
        else {
          // the lock is missing, but the mutex is there, so it's already been unlocked
          DARMA_ABORT("Can't release read-only usage more than once for handle "
            << get_key_version_string(handle)
          );
        }
      }
      // unlock the read-only usage
      found->second.unlock();
      read_only_locks_.erase(found);
    }

    void
    handle_done_with_version_depth(
      const handle_t* const handle
    ) override {
      auto _lg = unique_lock_t(last_at_version_depth_mtx_);
      DEBUG("handle done with version depth " << get_key_version_string(handle)
                << " at 0x" << std::hex << intptr_t(handle) << " with DataBlock: "
                << std::hex << "0x" << intptr_t(handle->get_data_block()));
      last_at_version_depth_.emplace(handle->get_key(), handle->get_version());
    }

    void
    release_handle(
      const handle_t* const handle
    ) override {
      auto _lg = make_unique_lock_guards(
        last_at_version_depth_mtx_, handle_ptrs_mtx_, data_ready_locks_mtx_, data_ready_mtxs_mtx_,
        handle_mtxs_mtx_, handle_to_satisfy_mtx_
      );
      auto kver_pair = std::make_pair(handle->get_key(), handle->get_version());

      DEBUG("releasing handle " << get_key_version_string(handle));

      // If there is a subsequent registered, we need to use this block to satisfy it
      version_t next_version_1 = handle->get_version();
      ++next_version_1;
      // see if there's a handle registered for the next version
      auto found = handle_ptrs_.find(std::make_pair(handle->get_key(), next_version_1));

      handle_t* to_satisfy = nullptr;

      if(found != handle_ptrs_.end()) {
        to_satisfy = found->second;
        DEBUG("found subsequent " << get_key_version_string(to_satisfy)
          << " to satisfy in release of " << get_key_version_string(handle));
      }
      else {
        auto found_lvd = last_at_version_depth_.find(kver_pair);
        if(found_lvd != last_at_version_depth_.end()) {
          version_t next_version_outer = handle->get_version();
          next_version_outer.pop_subversion();
          ++next_version_outer;
          auto found_next = handle_ptrs_.find(std::make_pair(handle->get_key(), next_version_outer));
          if(found_next != handle_ptrs_.end()) {
            to_satisfy = found_next->second;
            DEBUG("found version-depth-up subsequent " << get_key_version_string(to_satisfy)
              << " to satisfy in release of " << get_key_version_string(handle));
          }
          else {
            // if we still haven't found it, maybe there's a breadcrumb pointing up
            key_t next_key;
            version_t next_version;
            auto found_hts = handle_to_satisfy_.find(
              std::make_pair(handle->get_key(), next_version_outer)
            );
            if(found_hts != handle_to_satisfy_.end()) {
              do {
                next_key = found_hts->second.first;
                next_version = found_hts->second.second;
                // erase the old breadcrumb
                handle_to_satisfy_.erase(found_hts);
                found_hts = handle_to_satisfy_.find(
                  std::make_pair(next_key, next_version)
                );
              } while(found_hts != handle_to_satisfy_.end());
              auto found_ts_handle = handle_ptrs_.find(std::make_pair(next_key, next_version));
              if(found_ts_handle == handle_ptrs_.end()) {
                // error!
                abort();
              }
              to_satisfy = found_ts_handle->second;
              DEBUG("found multiple-version-depth-up breadcrumb subsequent " << get_key_version_string(to_satisfy)
                << " to satisfy in release of " << get_key_version_string(handle));
            }
          }
          last_at_version_depth_.erase(found_lvd);
        }
      }


      if(to_satisfy != nullptr) {

        if(handle->is_satisfied()) {
          DEBUG("satisfying handle " << get_key_version_string(to_satisfy)
            << " at 0x" << std::hex << intptr_t(to_satisfy) << " with DataBlock: "
            << std::hex << "0x" << intptr_t(to_satisfy->get_data_block())
            << " from released handle: " << get_key_version_string(handle)
          );
          to_satisfy->satisfy_with_data_block(handle->get_data_block());
          // release the data_ready_mtx for the subsequent
          auto found_drm = data_ready_locks_.find(std::make_pair(to_satisfy->get_key(), to_satisfy->get_version()));
          if(found_drm == data_ready_locks_.end()) {
            // TODO error here
            abort();
          }
          found_drm->second.unlock();
          data_ready_locks_.erase(found_drm);
        }
        else {
          // leave a breadcrumb
          DEBUG("leaving breadcrumb to satisfy handle " << get_key_version_string(to_satisfy)
            << " at 0x" << std::hex << intptr_t(to_satisfy) << " with DataBlock: "
            << std::hex << "0x" << intptr_t(to_satisfy->get_data_block())
            << " when released handle " << get_key_version_string(handle)
            << " is satisfied"
          );
          handle_to_satisfy_.emplace(kver_pair,
            std::make_pair(to_satisfy->get_key(), to_satisfy->get_version())
          );
        }
      }

      // If it isn't satisfied at this point, it was never used:
      // bool has_subsequent = false;
      // if(not handle->is_satisfied()) {
      //   if(found != handle_ptrs_.end()) {
      //     // indicate that a satisfaction of this handle actually means satisfying the
      //     // subsequent
      //     handle_to_satisfy_.emplace(kver_pair, found->second);
      //   }


      // }
      // else {
      //   if(found != handle_ptrs_.end()) {
      //     DEBUG("satisfying handle " << get_key_version_string(found->second)
      //               << " at 0x" << std::hex << intptr_t(found->second) << " with DataBlock: "
      //               << std::hex << "0x" << intptr_t(found->second->get_data_block())
      //               << " from released handle: " << get_key_version_string(handle)
      //     );
      //     DARMA_ASSERT_MESSAGE(handle->is_satisfied(), "handle not satisfied: " << get_key_version_string(handle));
      //     // TODO we could assert here that it's not in the last_at_version_depth_ map
      //     found->second->satisfy_with_data_block(handle->get_data_block());
      //     has_subsequent = true;
      //     // release the data_ready_mtx for the subsequent
      //     auto found_drm = data_ready_locks_.find(std::make_pair(handle->get_key(), next_version));
      //     if(found_drm == data_ready_locks_.end()) {
      //       // TODO error here
      //       abort();
      //     }
      //     found_drm->second.unlock();
      //     data_ready_locks_.erase(found_drm);
      //   }
      //   else {
      //     auto found_lvd = last_at_version_depth_.find(kver_pair);
      //     if(found_lvd != last_at_version_depth_.end()) {
      //       DARMA_ASSERT_MESSAGE(handle->is_satisfied(), "handle not satisfied: " << get_key_version_string(handle));
      //       version_t next_version_outer = handle->get_version();
      //       next_version_outer.pop_subversion();
      //       ++next_version_outer;
      //       auto found_next = handle_ptrs_.find(std::make_pair(handle->get_key(), next_version_outer));
      //       if(found_next != handle_ptrs_.end()) {
      //         DEBUG("satisfying handle " << get_key_version_string(found_next->second)
      //                   << " at 0x" << std::hex << intptr_t(found_next->second) << " with DataBlock: "
      //                   << std::hex << "0x" << intptr_t(found_next->second->get_data_block())
      //                   << " from released handle: " << get_key_version_string(handle)
      //         );
      //         found_next->second->satisfy_with_data_block(handle->get_data_block());
      //         has_subsequent = true;
      //         // release the data_ready_mtx for the subsequent
      //         auto found_drm = data_ready_locks_.find(std::make_pair(handle->get_key(), next_version_outer));
      //         if(found_drm == data_ready_locks_.end()) {
      //           // TODO error here
      //           abort();
      //         }
      //         found_drm->second.unlock();
      //         data_ready_locks_.erase(found_drm);
      //       }
      //       last_at_version_depth_.erase(kver_pair);
      //     }
      //   }
      // }

      // If there's nothing else using it, delete the data
      if(to_satisfy == nullptr and handle->is_satisfied()) {
        delete handle->get_data_block();
      }

      // Now delete stuff

      // delete the data ready mutex
      auto found_drm = data_ready_mtxs_.find(kver_pair);
      if(found_drm != data_ready_mtxs_.end()) {
        data_ready_mtxs_.erase(found_drm);
      }

      // delete the handle mutex
      auto found_hm = handle_mtxs_.find(kver_pair);
      if(found_hm == handle_mtxs_.end()) {
        // TODO error here
        abort();
      }
      handle_mtxs_.erase(found_hm);

      // delete the handle entry in the handle_ptrs map
      auto found_h = handle_ptrs_.find(kver_pair);
      if(found_h == handle_ptrs_.end()) {
        // TODO error here
        abort();
      }
      handle_ptrs_.erase(found_h);

      DEBUG("done releasing " << get_key_version_string(handle));

    }


    void
    publish_handle(
      const handle_t* const handle,
      const key_t& version_tag,
      const size_t n_fetchers = 1,
      bool is_final = false
    ) override {
      // TODO implement this
      abort();
    }

    void
    finalize() override {
      // TODO more efficient implementation (at least with a seperate joiner thread or something)
      for(auto&& tt : task_threads_) {
        tt.join();
      }
      // TODO mark something to make sure nothing else gets registered
    }


    //void
    //satisfy_handle(
    //  handle_t* const to_fill,
    //  bool needs_write_access = false
    //) override { abort(); };

    //void
    //establish_containment_relationship(
    //  const handle_t* const inner_handle,
    //  const handle_t* const outer_handle,
    //  containment_manager_t const& manager
    //) override { abort(); };

    ///**
    // * @todo Document this for 0.2.2 spec
    // */
    //void
    //establish_aliasing_relationship(
    //  const handle_t* const handle_a,
    //  const handle_t* const handle_b,
    //  aliasing_manager_t const& manager
    //) override { abort(); }

    std::unique_ptr<task_t> top_level_task;
};

void
darma_runtime::abstract::backend::darma_backend_initialize(
  int& argc, char**& argv,
  darma_runtime::abstract::backend::runtime_t*& backend_runtime,
  types::unique_ptr_template<
    typename darma_runtime::abstract::backend::runtime_t::task_t
  > top_level_task
) {
  auto* tmp_rt = new ThreadsRuntime;
  tmp_rt->top_level_task = std::move(top_level_task);
  running_task_ = tmp_rt->top_level_task.get();
  tmp_rt->top_level_task->set_name(
    darma_runtime::make_key(DARMA_BACKEND_SPMD_NAME_PREFIX, 0, 1)
  );
  backend_runtime = tmp_rt;
}


abstract::backend::runtime_t* darma_runtime::detail::backend_runtime = nullptr;