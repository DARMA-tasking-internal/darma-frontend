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
#include <iomanip>
#include <list>

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


enum { NO_VALUE, OPTIONAL_VALUE, REQUIRED_VALUE, MULTIPLE_VALUES };

#define NO_SHORT_NAME 0
#define NO_LONG_NAME nullptr

struct ArgsConfig {
  int argEnum;
  char letter;
  const char* name;
  int valueType;
  const char* docstring;
};

struct ArgGetter {
  const char* value;

  ArgGetter(const char* val) : value(val){}

  template <class T>
  void
  get(T& val) const {
    std::stringstream stream(value);
    stream >> val;
    std::cout << "value " << value << " became " << val << std::endl;
    if (!stream) {
      assert(0 && "Could not parse correctly");
    }
  }

  const char*
  get() const {
    return value;
  }

};

struct ArgEntry {
  int argEnum;

  /** Or this could have multiple values */
  std::list<ArgGetter> entries;

  ArgEntry() : argEnum(-1)
  {
  }

  ArgEntry(int en, const char* val) :
    argEnum(en)
  {
    entries.emplace_back(val);
  }

  template <class T>
  void
  get(T& val) const {
    return entries.front().get<T>(val);
  }

  const char*
  get() const {
    return entries.front().get();
  }

  typedef std::list<ArgGetter>::iterator iterator;
  typedef std::list<ArgGetter>::const_iterator const_iterator;

  iterator begin() {
    return entries.begin();
  }

  const_iterator begin() const {
    return entries.begin();
  }

  iterator end() {
    return entries.end();
  }

  const_iterator end() const {
    return entries.end();
  }

};

struct ArgName {
  char shortName;
  const char* longName;
  ArgName(char s, const char* l) :
    shortName(s), longName(l)
  {
  }
};

struct ArgNameCompare
{
  bool operator()(const ArgName& l, const ArgName& r) const {
    if (l.shortName != 0 && r.shortName != 0){
      return l.shortName < r.shortName;
    } else {
      return strcmp(l.longName, r.longName) < 0;
    }
  }
};


struct ArgsHolder {

  std::map<ArgName, ArgsConfig*, ArgNameCompare> configs;
  std::list<ArgEntry> entries;

  typedef  std::list<ArgEntry>::iterator iterator;
  typedef  std::list<ArgEntry>::const_iterator const_iterator;

  iterator begin() {
    return entries.begin();
  }

  const_iterator begin() const {
    return entries.begin();
  }

  iterator end() {
    return entries.end();
  }

  const_iterator end() const {
    return entries.end();
  }

  void
  usage(std::ostream& os){
    using std::setw;
    os << "Usage: app [OPTION]\n"
       << "Run a given DARMA application with OPTIONS configuring the backend and app"
       << std::endl;
    for (auto& pair : configs){
      ArgsConfig* cfg = pair.second;
      int stride = 22;
      if (cfg->letter != 0){
        os << setw(2) << " -" << cfg->letter << ", ";
      } else {
        os << " ";
        //we didn't print
        stride += 4;
      }

      if (cfg->name){
        os << "--" << cfg->name;
        int len = strlen(cfg->name);
        os << setw(stride-2-len) << "";
      } else {
        os << setw(stride) << "";
      }

      switch (cfg->valueType){
        case OPTIONAL_VALUE:
          os << "[Optional value]";
          break;
        case REQUIRED_VALUE:
          os << "[Required value]";
          break;
        case NO_VALUE:
          break;
        case MULTIPLE_VALUES:
          os << "[List of values]";
          break;
      }

      os << "\n ";
      int lineskip = 2;
      //print a well-formatted docstring
      int col = 0;
      int idx = 0;
      int len = strlen(cfg->docstring);
      while (idx < len){
        int startIdx = idx;
        while (idx < len && (cfg->docstring[idx] != ' ')){
          ++idx;
        }
        int wordSize = idx - startIdx;
        if (col != 0 && (col + wordSize) > 60){
          os << "\n" << setw(lineskip) << "";
          col = 0;
        } else {
          col += wordSize;
          os << " ";
        }

        for (int i=startIdx; i < idx; ++i){
          os << cfg->docstring[i];
        }

        while (idx < len && cfg->docstring[idx] == ' '){
          ++idx;
        }
      }
      os << std::endl;
    }
  }

  void
  parse(const std::vector<std::string>& args)
  {
    char* argv[100];
    for (int i=0; i < args.size(); ++i){
      argv[i] = const_cast<char*>(args[i].data());
    }
    parse(args.size(), argv);
  }

  bool
  parseNext(ArgsConfig* cfg, ArgEntry& entry, char* next, char* str)
  {
    switch (cfg->valueType){
      case OPTIONAL_VALUE:
      case MULTIPLE_VALUES:
        if (next[0] != '-'){
          entry.entries.emplace_back(next);
          return true;
        }
        return false;
      case REQUIRED_VALUE:
        if (next[0] == '-'){
          std::cerr << str << " requires argument but found flag " << next << std::endl;
          abort();
        }
        entry.entries.emplace_back(next);
        return true;
      case NO_VALUE:
        if (next[0] != '-'){
          std::cerr << str << " takes no arguments, but found " << next << std::endl;
          abort();
        }
        return false;
    }
    return false; //never reached, get rid compiler warnings
  }

  void
  parse(int argc, char** argv)
  {
    int i=0;
    for (i=1; i < argc; ++i){
      char* str = argv[i];
      int len = strlen(str);
      if (len >= 2){
        if (str[0] != '-') {
          std::cerr << "value " << str << " not consumed by any flag" << std::endl;
          abort();
        }

        //look for '=' in string
        char* valptr = str;
        bool valEquals = false;
        while (*valptr != '\0'){
          if (*valptr == '='){
            //argument is contained in the string
            valEquals = true;
            *valptr = '\0';
            break;
          }
          ++valptr;
        }
        entries.emplace_back();
        ArgEntry& entry = entries.back();
        ArgsConfig* cfg = nullptr;
        if (str[1] == '-'){
          ArgName n(NO_SHORT_NAME, &str[2]);
          cfg = configs[n];
        } else {
          if (len != 2) {
            std::cerr << "flag " << str << " has single dash, but is not a single char" << std::endl;
            abort();
          }
          ArgName n(str[1], NO_LONG_NAME);
          cfg = configs[n];
        }

        if (cfg == nullptr){
          std::cerr << "unknown flag " << str << std::endl;
          abort();
        }

        entry.argEnum = cfg->argEnum;

        if (valEquals){
          parseNext(cfg, entry, valptr+1, str);
          *valptr = '=';
        } else if ((i+1) == argc){
          //reached the end
          if (cfg->valueType == REQUIRED_VALUE){
            std::cerr << str << " requires argument" << std::endl;
            abort();
          }
        } else if (cfg->valueType == MULTIPLE_VALUES){
          while ((i+1) < argc && argv[i+1][0] != '-'){
            //keep reading arguments until we find next flag
            //or run out of argv
            ++i;
            entry.entries.emplace_back(argv[i]);
          }
        } else {
          if (parseNext(cfg, entry, argv[i+1], str)){
            //if this contains an argument, increment index
            ++i;
          }
        }
      }
    }
  }

  ArgsHolder(ArgsConfig config[]) {
    int idx = 0;
    while(1){
      ArgsConfig& cfg = config[idx];
      if (cfg.letter == 0 && cfg.name == nullptr)
        break;
      configs[{cfg.letter,cfg.name}] = &cfg;
      ++idx;
    }
  }
};


#endif /* _THREADS_COMMON_BACKEND_RUNTIME_H_ */
