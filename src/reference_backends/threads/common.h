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
#define __THREADS_BACKEND_DEBUG__         0
#define __THREADS_BACKEND_DEBUG_VERBOSE__ 0

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
#define PRINT_LABEL(FLOW) (((FLOW)->inner->label))
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
    void* data;

    DataBlock(const DataBlock& in) = delete;

    DataBlock(void* data_)
      : refs(1)
      , data(data_)
    { }
    
    DataBlock(int refs_, size_t sz)
      : refs(refs_)
      , data(malloc(sz))
    { }

    int get_refs() const { return refs; }
    void ref(int num = 1) { refs += num; }
    void deref() { refs--; }

    virtual ~DataBlock() { free(data); }

  private:
    int refs;
  };
}

#endif /* _THREADS_COMMON_BACKEND_RUNTIME_H_ */
