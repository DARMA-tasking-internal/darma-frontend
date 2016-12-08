/*
//@HEADER
// ************************************************************************
//
//                         common.h
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

#if !defined(_THREADS_COMMON_BACKEND_RUNTIME_H_)
#define _THREADS_COMMON_BACKEND_RUNTIME_H_

/*
 * Debugging prints with mutex
 */
#define __THREADS_BACKEND_DEBUG__	  0
#define __THREADS_BACKEND_SHUFFLE__	  0
#define __THREADS_BACKEND_DEBUG_VERBOSE__ 0
#define __THREADS_BACKEND_DEBUG_TRACE__   0
#define __THREADS_DEBUG_MODE__ (__THREADS_BACKEND_DEBUG__         ||    \
                                __THREADS_BACKEND_DEBUG_VERBOSE__ ||    \
                                __THREADS_BACKEND_DEBUG_TRACE__)

std::mutex __output_mutex;
#define THREADS_PRINTER(fmt, arg...)					\
  do {									\
    std::unique_lock<std::mutex> __output_lg(__output_mutex);		\
    printf("(%zu): DEBUG threads: " fmt,				\
	   inside_rank, ##arg);                                         \
    fflush(stdout);							\
  } while (0);

#define THREADS_PRINTER_THD(thd, fmt, arg...)                           \
  do {									\
    std::unique_lock<std::mutex> __output_lg(__output_mutex);		\
    printf("(%zu): DEBUG threads: " fmt,				\
	   thd, ##arg);                                                 \
    fflush(stdout);							\
  } while (0);

#define STARTUP_PRINT(fmt, arg...)					\
  do {									\
    std::unique_lock<std::mutex> __output_lg(__output_mutex);		\
    printf("THREADS : " fmt, ##arg);                                    \
    fflush(stdout);							\
  } while (0);

#define PRINT_STATE(FLOW)                                               \
  ((FLOW->state) == FlowWaiting ? "FlowWaiting" :                       \
   ((FLOW->state) == FlowWriteReady ? "FlowWriteReady" :                \
    ((FLOW->state) == FlowReadReady ? "FlowReadReady" :                 \
     ((FLOW->state) == FlowReadOnlyReady ? "FlowReadOnlyReady" :        \
      ((FLOW->state) == FlowAntiReady ? "FlowAntiReady" :               \
       ((FLOW->state) == FlowScheduleOnly ? "FlowScheduleOnly" :        \
        "InvalidFlowState"))))))

#if __THREADS_BACKEND_DEBUG_TRACE__
  #define DEBUG_TRACE(fmt, arg...)         THREADS_PRINTER(fmt, ##arg)
#else
  #define DEBUG_TRACE(fmt, arg...)
#endif

#if __THREADS_BACKEND_DEBUG_VERBOSE__
  #define DEBUG_VERBOSE_PRINT(fmt, arg...) THREADS_PRINTER(fmt, ##arg)
  #define DEBUG_PRINT(fmt, arg...)         THREADS_PRINTER(fmt, ##arg)
  #define DEBUG_PRINT_THD(thd, fmt, arg...) THREADS_PRINTER(fmt, ##arg)
#else
  #define DEBUG_VERBOSE_PRINT(fmt, arg...)
#endif

#if !defined(DEBUG_PRINT)
  #if __THREADS_BACKEND_DEBUG__
    #define DEBUG_PRINT(fmt, arg...)          THREADS_PRINTER(fmt, ##arg)
    #define DEBUG_PRINT_FORCE(fmt, arg...)    THREADS_PRINTER(fmt, ##arg)
    #define DEBUG_PRINT_THD(thd, fmt, arg...) THREADS_PRINTER_THD(thd, fmt, ##arg)
  #else
    #define DEBUG_PRINT(fmt, arg...)
    #define DEBUG_PRINT_FORCE(fmt, arg...)    THREADS_PRINTER(fmt, ##arg)
    #define DEBUG_PRINT_THD(thd, fmt, arg...)
  #endif
#endif

#define PRINT_BOOL_STR(IN) (((IN) ? "true" : "false"))
#define PRINT_KEY(IN) ((IN.human_readable_string().c_str()))
#define PRINT_LABEL(FLOW) (((FLOW)->label))
#define PRINT_LABEL_INNER(FLOW) (((FLOW)->label))
#define PRINT_PURPOSE(IN)						\
  ((IN) == Input ? "Input" :						\
   ((IN) == Output ? "Output" :						\
    ((IN) == ForwardingChanges ? "ForwardingChanges" :			\
     ((IN) == OutputFlowOfReadOperation ? "OutputFlowOfReadOperation" :	\
      "NONE"))))

#include <thread>
#include <atomic>

namespace threads_backend {
  using namespace darma_runtime;
  using namespace darma_runtime::abstract::backend;

  struct DataBlock {
    void* data = nullptr;
    size_t shared_ref_count = 0;
    size_t refs = 0;

    DataBlock(const DataBlock& in) = delete;

    DataBlock(void* data_)
      : data(data_)
    { }

    DataBlock(int refs_, size_t sz)
      : data(malloc(sz))
    { }

    virtual ~DataBlock() { free(data); }
  };

  struct CollectionID {
    size_t collection = 0;
    size_t index = 0;

    CollectionID() = default;
    CollectionID(CollectionID const&) = default;

    CollectionID(
      size_t const collection_in, size_t const index_in
    ) : collection(collection_in), index(index_in) { }

    bool operator==(CollectionID const& other) const {
      return other.collection == collection && other.index == index;
    }
  };
}

struct ArgsRemover {
  template <typename... Args>
  ArgsRemover(
    int& argc,
    char**& argv,
    Args&&... args
  ) {
    iterate(argc, argv, {args...});
  }

  std::list<std::string> new_args;

  void
  iterate(
    int& argc,
    char**& argv,
    std::initializer_list<std::string> args
  ) {
    std::list<std::string> arg_list, arg_list_new;
    for (auto i = 0; i < argc; i++) {
      arg_list.push_back(argv[i]);
    }

    for (auto&& arg : args) {
      bool remove_next = false;
      //std::cout << "iterate: arg = " << arg << std::endl;
      for (auto&& item : arg_list) {
        if ("--" + arg == item) {
          remove_next = true;
        } else if (!remove_next) {
          arg_list_new.push_back(item);
        } else {
          remove_next = false;
        }
      }
      arg_list = arg_list_new;
      arg_list_new.clear();
    }

    new_args = arg_list;
  }

};

struct ArgsHolder {
  std::unordered_map<std::string, std::string> args;

  ArgsHolder(int argc, char** argv) {
    for (auto i = 0; i < argc; i++) {
      //printf("parsing arg: %s\n", argv[i]);
      if (strlen(argv[i]) > 1 &&
          argv[i][0] == '-' &&
          argv[i][1] == '-' &&
          i+1 < argc &&
          argv[i+1][0] != '-') {
        //printf("parsing arg value: %s\n", argv[i+1]);
        args[std::string(argv[i])] = std::string(argv[i+1]);
        i++;
      } else {
        args[std::string(argv[i])];
      }
    }
  }

  bool
  exists(std::string const& str) {
    return args.find("--" + str) != args.end();
  }

  std::string
  get(std::string const& str) {
    return args[str];
  }

  template <typename T>
  T get(std::string const& str) {
    T val;
    assert(args.find("--" + str) != args.end() &&
           "Could not find argument");
    std::stringstream stream(args["--" + str]);
    stream >> val;
    if (!stream) {
      assert(0 && "Could not parse correctly");
    }
    return val;
  }
};

namespace union_find {
  template <typename UFArchetype>
  void make_set(
    UFArchetype const& node
  ) {
    node->alias = nullptr;
    node->uf_size = 1;
  }

  template <typename UFArchetype>
  void union_nodes(
    UFArchetype node1,
    UFArchetype node2
  ) {
    DEBUG_PRINT_THD(
      (size_t)0,
      "union_nodes: sz = %ld, sz2 = %ld\n",
      node1->uf_size,
      node2->uf_size
    );
    // optimization to pick the larger subtree
    // does not apply to this problem due to ordering problem
    // if (node1->uf_size < node2->uf_size) {
    //   node1.swap(node2);
    // }
    node2->alias = node1;
    node1->uf_size += node2->uf_size;
  }

  template <typename UFArchetype, typename Callable>
  UFArchetype find_call(
    UFArchetype const& node,
    Callable&& callable
  ) {
    DEBUG_PRINT_THD(
      (size_t)0,
      "find_call: node=%ld, alias=%ld\n",
      PRINT_LABEL(node),
      node->alias ? PRINT_LABEL(node->alias) : 0
    );
    if (node->alias != nullptr) {
      callable(node->alias);
      node->alias = find_call(
        node->alias,
        callable
      );
    }
    return node->alias ? node->alias : node;
  }
}


#endif /* _THREADS_COMMON_BACKEND_RUNTIME_H_ */
