/*
//@HEADER
// ************************************************************************
//
//                          trace.h
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

#if !defined(_THREADS_TRACE_BACKEND_RUNTIME_H_)
#define _THREADS_TRACE_BACKEND_RUNTIME_H_

#include <darma/interface/backend/flow.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/defaults/darma_main.h>

#include <common.h>

#include <vector>
#include <stack>
#include <iostream>
#include <fstream>
#include <chrono>

using std::chrono::high_resolution_clock;
using std::chrono::time_point;
using std::chrono::duration;

namespace threads_backend {
  enum EventType {
    GROUP_BEGIN=2,
    GROUP_END=3,
    DEP_CREATE=1
  };

  typedef size_t Entry;
  typedef size_t Event;
  typedef std::string EntryName;
  
  struct TraceLog {
    time_point<high_resolution_clock> time;
    Entry entry;
    EventType type;

    // pointers to end and begin
    TraceLog* end   = nullptr;
    TraceLog* begin = nullptr;

    // post creation of log
    std::vector<TraceLog*> deps;
    Event event;
    size_t rank;
  };

  struct TraceEntrys {
    std::mutex event_lock;
    std::unordered_map<EntryName, Entry> eventNames;
    Entry nextEntry = 0;

    TraceEntrys(const TraceEntrys& other) = delete;
    TraceEntrys() = default;
    
    Entry findEntryID(const EntryName& event) {
      // TODO: fix this. registry must not be locked
      // per-thd registry with rank resolution at end
      std::lock_guard<std::mutex> guard(event_lock);
      if (eventNames.find(event) != eventNames.end()) {
        return eventNames[event];
      } else {
        eventNames[event] = nextEntry++;
        return eventNames[event];
      }
    }

    void outputControlFile(const size_t numRanks,
                           std::ofstream& file) {
      file << "PROJECTIONS_ID\n"
           << "VERSION 7.0\n"
           << "TOTAL_PHASES 1\n"
           << "MACHINE unknown\n"
           << "PROCESSORS " << numRanks << "\n"
           << "TOTAL_CHARES " << eventNames.size() << "\n"
           << "TOTAL_EPS "    << eventNames.size() << "\n"
           << "TOTAL_MSGS 0\n"
           << "TOTAL_PSEUDOS 0\n"
           << "TOTAL_EVENTS 0"
           << std::endl;
      
      for (auto&& event : eventNames) {
        file << "CHARE "
             << event.second << " "
             << event.first
             << std::endl;
      }

      for (auto&& event : eventNames) {
        const auto& name = event.first;
        const auto& id = event.second;
        
        file << "ENTRY CHARE "
             << id << " "
             << name << " "
             << id << " "
             << id << " "
             << std::endl;
      }

      file << "TOTAL_FUNCTIONS 0\n"
           << "END\n"
           << std::endl;
    }
  };

  struct TraceTimer {
    time_point<high_resolution_clock> currentTime() {
      return high_resolution_clock::now();
    }
  };

  static TraceTimer timer;
  static TraceEntrys entryReg;

  struct TraceModule {
    const size_t rank;
    const size_t nranks;
    const std::string trace_name;
    const std::string name;
    const size_t flushSize;
    const size_t stackMaxDepth;
    const time_point<high_resolution_clock> startTime;

    std::vector<TraceLog*> traces;
    std::stack<TraceLog*> open;

    TraceModule(const TraceModule& other) = delete;

    TraceModule(const size_t rank_,
                const size_t nranks_,
                const std::string name_,
                const size_t flushSize_ = 0,
                const size_t stackMaxDepth_ = 0)
      : rank(rank_)
      , nranks(nranks_)
      , name(name_)
      , trace_name(name_ + "." + std::to_string(rank) + ".log")
      , flushSize(flushSize_)
      , stackMaxDepth(stackMaxDepth_)
      , startTime(timer.currentTime())
      , enabled(true)
    {
      DEBUG_TRACE("TraceModule starting: name=\"%s\", rank=%ld\n",
                  trace_name.c_str(),
                  rank);
      traces.reserve(10000);
    }

    virtual ~TraceModule() {
      DEBUG_TRACE("TraceModule finalizing: name=\"%s\", rank=%ld\n",
                  trace_name.c_str(),
                  rank);

      writeTracesDisk();
    }

    void
    writeLogDisk(std::ofstream& file,
                 std::vector<TraceLog*> traces) {
      for (auto&& log : traces) {
        const long convertedTime = time2long(computeDiff(log->time,startTime));

        switch (log->type) {
        case GROUP_BEGIN:
          file << log->type << " 0 "
               << log->entry << " "
               << convertedTime << " "
               << log->event << " "
               << log->rank << " "
               << "0 0 0 0 0 0 0\n";
          break;
        case GROUP_END:
          file << log->type << " 0 "
               << log->entry << " "
               << convertedTime << " "
               << log->event << " "
               << log->rank << " "
               << "0 0 0 0 0 0 0 0\n";
          break;
        case DEP_CREATE:
          file << log->type << " 0 "
               << log->entry << " "
               << convertedTime << " "
               << log->event << " "
               << log->rank << " "
               << "0 0 0 0 0 0 0 0\n";
          break;
        default:
          DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
        }

        // recursive call to unfold trace structure
        if (log->deps.size() > 0) {
          // TODO: hackish block to space out send events
          {
            size_t cur = 0;
            for (auto&& sublog : log->deps) {
              sublog->time -= std::chrono::nanoseconds(1000 * cur);
              cur++;
            }
          }
          
          writeLogDisk(file,log->deps);
        }
      }
      traces.empty();
    }
    
    void writeTracesDisk() {
      std::ofstream file;
      file.open(trace_name);
      TraceModule::outputHeader(rank,startTime,file);
      writeLogDisk(file,traces);
      TraceModule::outputFooter(rank,startTime,file);
      file.close();

      if (rank == 0) {
        std::ofstream file;
        file.open(name + ".sts");
        entryReg.outputControlFile(nranks,file);
        file.close();
      }
    }

    inline TraceLog*
    eventStartNow(const EntryName& entry) {
      return eventStart(timer.currentTime(), entry);
    }

    inline TraceLog*
    eventStopNow(const EntryName& entry) {
      return eventStop(timer.currentTime(), entry);
    }
    
    TraceLog*
    eventStart(const time_point<high_resolution_clock> time,
               const EntryName entry) {
      auto log = new TraceLog{time,
                              entryReg.findEntryID(entry),
                              GROUP_BEGIN};
      log->rank = this_rank;
      logEvent(log);
      return log;
    }

    TraceLog*
    eventStop(const time_point<high_resolution_clock> time,
              const EntryName entry) {
      auto log = new TraceLog{time,
                              entryReg.findEntryID(entry),
                              GROUP_END};
      log->rank = this_rank;
      logEvent(log);
      return log;
    }

    TraceLog*
    depCreate(const time_point<high_resolution_clock> time,
              const Entry entry) {
      auto log = new TraceLog{time,
                              entry,
                              DEP_CREATE};
      log->rank = this_rank;
      return log;
    }
    
    size_t logEvent(TraceLog* const log) {
      if (!enabled) {
        return 0;
      }      

      auto grouped_begin = [&]{
        if (!open.empty()) {
          traces.push_back(new TraceLog{log->time,// - 0.0000001,
                                        open.top()->entry,
                                        GROUP_END});
        }
        // push on open stack.
        open.push(log);
        traces.push_back(log);

        const auto event = traces.size() - 1;
        log->event = event;

        return event;
      };

      auto grouped_end = [&]{
        // sanity assertions
        assert(!open.empty());
        assert(open.top()->entry == log->entry &&
               open.top()->type == GROUP_BEGIN);

        // match event with the one that this ends
        log->event = open.top()->event;

        // set up begin/end links
        open.top()->end = log;
        log->begin = open.top();
        
        open.pop();

        traces.push_back(log);
        
        if (!open.empty()) {
          traces.push_back(new TraceLog{log->time,// + 0.0000001,
                                        open.top()->entry,
                                        GROUP_BEGIN});
        }

        return log->event;
      };

      auto dep_create = [&]{
        traces.push_back(log);

        const auto event = traces.size() - 1;
        log->event = event;
        
        return event;
      };

      switch (log->type) {
      case GROUP_BEGIN:
        return grouped_begin();
        break;
      case GROUP_END:
        return grouped_end();
        break;
      case DEP_CREATE:
        return dep_create();
        break;
      default:
        DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
        return 0;
        break;
      }
    }
    
    void start() {
      enabled = true;
    }

    void stop() {
      enabled = false;
    }

    static void outputHeader(const size_t rank,
                             const time_point<high_resolution_clock> startTime,
                             std::ofstream& file) {
      // Output header for projections file
      file << "PROJECTIONS-RECORD 0"
           << std::endl;
      // '6' means COMPUTATION_BEGIN to Projections: this starts a
      // trace
      file << "6 "
           << 0
           << std::endl;
    }

    static void outputFooter(const size_t rank,
                             const time_point<high_resolution_clock> startTime,
                             std::ofstream& file) {
      /// Output footer for projections file, '7' means
      /// COMPUTATION_END to Projections
      file << "7 "
           << time2long(computeDiff(timer.currentTime(),startTime))
           << std::endl;
    }

    static duration<double> computeDiff(const time_point<high_resolution_clock> t0,
                                        const time_point<high_resolution_clock> t1) {
      duration<double> dur(t0-t1);
      return dur;
    }
    
    static long time2long(const duration<double> time) {
      return (long)(time.count() * 1e6);
    }
    
  private:
    bool enabled;
  };
}

#endif /* _THREADS_TRACE_BACKEND_RUNTIME_H_ */
