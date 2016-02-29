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

#include <stack>
#include <unordered_set>
#include <unordered_map>

#include "serial_backend.h"

#define DHARMA_SERIAL_DEBUG 1

#if DHARMA_SERIAL_DEBUG
#define DEBUG(...) { \
  std::cout << __VA_ARGS__ << std::endl; \
}
#else
#define DEBUG(...)
#endif

namespace serial_backend {

using namespace darma_runtime;

class SerialDataBlock
  : public abstract::backend::DataBlock
{
  public:

    void* get_data() override {
      return data_;
    }

    void* data_ = nullptr;

};

class SerialRuntime
  : public abstract::backend::runtime_t
{
  protected:

    typedef typename abstract::backend::runtime_t::task_t task_t;
    typedef typename abstract::backend::runtime_t::task_unique_ptr task_unique_ptr;
    typedef typename abstract::backend::runtime_t::handle_t handle_t;
    typedef typename abstract::backend::runtime_t::version_t version_t;


    std::unordered_map<std::pair<key_t, version_t>, handle_t* const> registered_handles;

    std::unordered_set<std::pair<key_t, version_t>> last_at_version_depth;

    std::string
    get_key_version_string(
      const handle_t* const handle
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
      task_unique_ptr&& task
    ) override {
      auto& deps = task->get_dependencies();

      // TODO check that dependency is registered

      for(auto&& dep : deps) {
        if(task->needs_read_data(dep)) {
          DARMA_ASSERT_MESSAGE(dep->is_satisfied(), "Handle " << get_key_version_string(dep) <<
            " not satisfied upon usage in task");
        }
        if(task->needs_write_data(dep)) {
          if(dep->get_version() != version_t()) {
            DARMA_ASSERT_MESSAGE(dep->is_writable(), "Handle " << get_key_version_string(dep) <<
              " not writable upon usage in modifying task");
          }
          else {
            assert(not task->needs_read_data(dep));
            auto* serman = dep->get_serialization_manager();
            size_t size = serman->get_metadata_size(nullptr);
            SerialDataBlock* db = new SerialDataBlock();
            db->data_ = malloc(size);
            dep->satisfy_with_data_block(db);
          }
        }
      }

      running_tasks.push(task.get());
      task->run();
      running_tasks.pop();

    }


    task_t* const
    get_running_task() const override {
      return running_tasks.top();
    }


    void register_handle(
      handle_t* const handle
    ) override {
      auto found = registered_handles.find({handle->get_key(), handle->get_version()});
      assert(found == registered_handles.end());
      registered_handles.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(handle->get_key(), handle->get_version()),
        std::forward_as_tuple(handle)
      );
    }

    void register_fetching_handle(
      handle_t* const handle,
      const key_t& user_version_tag
    ) override {
      // TODO
      assert(false); // not implemented
    }

    void
    release_read_only_usage(
      handle_t* const handle
    ) override {
      auto found = registered_handles.find({handle->get_key(), handle->get_version()});
      assert(found != registered_handles.end());
      DEBUG("releasing read only usage of handle " << get_key_version_string(handle));
      handle->allow_writes();
    }

    void
    handle_done_with_version_depth(
      const handle_t* const handle
    ) override {
      auto found = registered_handles.find({handle->get_key(), handle->get_version()});
      assert(found != registered_handles.end());
      auto found_lavd = last_at_version_depth.find({handle->get_key(), handle->get_version()});
      assert(found_lavd == last_at_version_depth.end());
      last_at_version_depth.emplace(handle->get_key(), handle->get_version());
    }

    void
    release_handle(
      const handle_t* const handle
    ) override {
      auto& k = handle->get_key();
      auto& v = handle->get_version();

      auto found = registered_handles.find({k, v});
      assert(found != registered_handles.end());
      // remove ourselves from the registered handles
      registered_handles.erase(found);


      auto found_lavd = last_at_version_depth.find({k, v});
      bool is_lavd = found_lavd != last_at_version_depth.end();
      if(is_lavd) last_at_version_depth.erase(found_lavd);

      handle_t* subsequent = nullptr;

      // 1. first, if a handle is registered with key k and version ++(v.push(0)), that handle is the subsequent
      //    and should be satisfied.  In this case, there must also be a handle registered with key k and version
      //    ++v which is \b not the subsequent (but debug mode should check for that handle's existence).
      auto v_rule_1 = v;
      v_rule_1.push_subversion();
      ++v_rule_1;
      auto found_rule_1 = registered_handles.find({k, v_rule_1});
      if(found_rule_1 != registered_handles.end()) {
        subsequent = found_rule_1->second;
      }
      else {
        //  2. If no handle with {k, ++(v.push(0))} is registered, the runtime should check for {k, ++v}.  If that
        //     handle exists, it is the subsequent.  (Further, none of the following rules should yield a subsequent,
        //     and debug mode should check for this and raise an error).
        auto v_rule_2 = v;
        ++v_rule_2;
        auto found_rule_2 = registered_handles.find({k, v_rule_2});
        if(found_rule_2 != registered_handles.end()) {
          assert(!is_lavd);
          subsequent = found_rule_2->second;
          // TODO check no other subsequents
        }
        else {
          // 3. If handle_done_with_version_depth() was called with handle as an argument before this release call, or
          //    if {k, ++v} is not found to exist (e.g., potentially it has already been released) but a handle h' with
          //    {k, ++v} was an argument ot handle_done_with_version_depth() before the release of h', the runtime should
          //    check for the existence of {k', v'} = {k, ++(v.pop())}.
          //    Then,
          //       * if {k', v'} exists, it is the subsequent
          //       * if {k', v'} does not exist but handle_done_with_version_depth() was called on a handle with it
          //         during that handle's life cycle, repeat with {k', v'} = {k', ++(v'.pop())}
          //       * otherwise, {k, v} has no subsequent.  This is also true if a {k', v'} is reached for which
          //         handle_done_with_version_depth() was called with v'.depth() == 1 (or if v'.depth() == 1 at
          //         any time in this process taht pop() would need to be called)
          // NOTE:  In the serial version, we know there won't be out of order releases like this, so all we need
          // to do is handle the last at version depth case here.

          if(is_lavd and v.depth() > 1) {
            auto v_rule_3 = v;
            v_rule_3.pop_subversion();
            ++v_rule_3;
            auto found_rule_3 = registered_handles.find({k, v_rule_3});
            if(found_rule_3 != registered_handles.end()) {
              subsequent = found_rule_3->second;
            }
            else {
              assert(false); // shouldn't get here
            }

          }

        }

      }

      if(subsequent) {
        // we can only do this in the serial version
        DARMA_ASSERT_MESSAGE(handle->is_satisfied(), "Handle " << get_key_version_string(handle) <<
          " with subsequent " << get_key_version_string(subsequent) << " not satisfied before release");
        subsequent->satisfy_with_data_block(handle->get_data_block());
        DEBUG("Satifying subsequent " << get_key_version_string(subsequent) << " with handle " << get_key_version_string(handle));
      }
      else if(handle->is_satisfied()) {
        if(handle->get_data_block()->get_data() != nullptr) {
          free(handle->get_data_block()->get_data());
        }
        delete handle->get_data_block();
      }

    }


    void publish_handle(
      const handle_t* const handle,
      const key_t& version_tag,
      const size_t n_fetchers = 1,
      bool is_final = false
    ) override {
      assert(not is_final); // version 0.2.0

      assert(false); // not implemented
      // TODO
    }


    void
    finalize() override {
      // nothing to do here
    }


    task_unique_ptr top_level_task;
    std::stack<task_t*> running_tasks;



};

} // end namespace serial_backend

void
darma_runtime::abstract::backend::darma_backend_initialize(
  int& argc, char**& argv,
  darma_runtime::abstract::backend::runtime_t*& backend_runtime,
  types::unique_ptr_template<
    typename darma_runtime::abstract::backend::runtime_t::task_t
  > top_level_task
) {
  auto* tmp_rt = new serial_backend::SerialRuntime;
  tmp_rt->top_level_task = std::move(top_level_task);
  tmp_rt->running_tasks.push(tmp_rt->top_level_task.get());
  tmp_rt->top_level_task->set_name(
    darma_runtime::make_key(DARMA_BACKEND_SPMD_NAME_PREFIX, 0, 1)
  );
  backend_runtime = tmp_rt;
}

darma_runtime::abstract::backend::runtime_t* darma_runtime::detail::backend_runtime = nullptr;
