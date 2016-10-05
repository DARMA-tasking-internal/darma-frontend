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
	   threads_backend::this_rank, ##arg);				\
    fflush(stdout);							\
  } while (0);

#define STARTUP_PRINT(fmt, arg...)					\
  do {									\
    std::unique_lock<std::mutex> __output_lg(__output_mutex);		\
    printf("(%zu) THREADS : " fmt, threads_backend::this_rank, ##arg);	\
    fflush(stdout);							\
  } while (0);

#define PRINT_STATE(FLOW)                                               \
  ((FLOW->state) == FlowWaiting ? "FlowWaiting" :                       \
   ((FLOW->state) == FlowWriteReady ? "FlowWriteReady" :                \
    ((FLOW->state) == FlowReadReady ? "FlowReadReady" :                 \
     ((FLOW->state) == FlowReadOnlyReady ? "FlowReadOnlyReady" :        \
      ((FLOW->state) == FlowAntiReady ? "FlowAntiReady" :               \
       "InvalidFlowState")))))

#if __THREADS_BACKEND_DEBUG_TRACE__
  #define DEBUG_TRACE(fmt, arg...)         THREADS_PRINTER(fmt, ##arg)
#else
  #define DEBUG_TRACE(fmt, arg...)
#endif

#if __THREADS_BACKEND_DEBUG_VERBOSE__
  #define DEBUG_VERBOSE_PRINT(fmt, arg...) THREADS_PRINTER(fmt, ##arg)
  #define DEBUG_PRINT(fmt, arg...)         THREADS_PRINTER(fmt, ##arg)
#else
  #define DEBUG_VERBOSE_PRINT(fmt, arg...)
#endif

#if !defined(DEBUG_PRINT)
  #if __THREADS_BACKEND_DEBUG__
    #define DEBUG_PRINT(fmt, arg...) THREADS_PRINTER(fmt, ##arg)
  #else
    #define DEBUG_PRINT(fmt, arg...)
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

  extern __thread size_t this_rank;

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

  struct DataStore
    : DataStoreHandle {
    size_t rank = 0;
    size_t handle_id = 0;

    DataStore(size_t in_rank,
              size_t in_handle_id)
      : rank(in_rank)
      , handle_id(in_handle_id)
    { }

    bool
    operator==(DataStore const& other) {
      return other.rank == rank && other.handle_id == handle_id;
    }

    DataStore(DataStore const& other) = default;
  };

  struct ConcurrentRegionContext
    : ConcurrentRegionContextHandle {
    size_t index = 0;

    ConcurrentRegionContext(
      size_t const in_index
    )
      : index(in_index)
    { }

    size_t
    get_backend_index() const {
      return index;
    }
  };
}

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
    return args.find(str) != args.end();
  }

  std::string
  get(std::string const& str) {
    return args[str];
  }

  template <typename T>
  T operator[](std::string const& str) {
    return get(str);
  }

  template <typename T>
  T get(std::string const& str) {
    T val;
    assert(args.find(str) != args.end() &&
           "Could not find argument");
    std::stringstream stream(args[str]);
    stream >> val;
    if (!stream) {
      assert(0 && "Could not parse correctly");
    }
    return val;
  }
};

#endif /* _THREADS_COMMON_BACKEND_RUNTIME_H_ */
