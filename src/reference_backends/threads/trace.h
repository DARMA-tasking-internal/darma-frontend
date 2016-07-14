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

namespace threads_backend {
  enum EventType {
    GROUP_BEGIN=2,
    GROUP_END=3,
    NON_GROUP=1
  };

  typedef std::string EventName;
  typedef size_t Event;
  
  struct TraceLog {
    double time;
    Event event;
    EventType type;
  };

  struct TraceEvents {
    std::mutex event_lock;
    std::unordered_map<EventName, Event> eventNames;
    Event nextEvent = 0;

    TraceEvents(const TraceEvents& other) = delete;
    TraceEvents() = default;
    
    Event findEventID(const EventName& event) {
      // TODO: fix this. registry must not be locked
      // per-thd registry with rank resolution at end
      std::lock_guard<std::mutex> guard(event_lock);
      if (eventNames.find(event) != eventNames.end()) {
        return eventNames[event];
      } else {
        eventNames[event] = nextEvent++;
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
           << "TOTAL_EPS"     << eventNames.size() << "\n"
           << "TOTAL_MSGS 0\n"
           << "TOTAL_PSEUDOS 0\n"
           << "TOTAL_EVENTS 0\n"
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
    double currentTime() {
      return 0.0;
    }
  };

  static TraceTimer timer;
  static TraceEvents eventReg;
  
  struct TraceModule {
    const size_t rank;
    const size_t nranks;
    const std::string trace_name;
    const std::string name;
    const size_t flushSize;
    const size_t stackMaxDepth;
    double startTime;

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

    void writeLogDisk(std::ofstream& file) {
      for (auto&& log : traces) {
        const double convertedTime = TraceModule::time2long(log->time - startTime);

        switch (log->type) {
        case GROUP_BEGIN:
          file << log->type << " 0 "
               << log->event << " "
               << convertedTime << " "
               << "0 0 0 0 0 0 0 0 0 0\n";
          break;
        case GROUP_END:
          break;
        case NON_GROUP:
          break;
        default:
          DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
        }
      }
      traces.empty();
    }
    
    void writeTracesDisk() {
      std::ofstream file;
      file.open(trace_name);
      TraceModule::outputHeader(rank,startTime,file);
      writeLogDisk(file);
      TraceModule::outputFooter(rank,startTime,file);
      file.close();

      if (rank == 0) {
        std::ofstream file;
        file.open(name + ".sts");
        eventReg.outputControlFile(nranks,file);
        file.close();
      }
    }
    
    void eventStart(const double time,
                    const EventName event) {
      logEvent(new TraceLog{time,
                            eventReg.findEventID(event),
                            GROUP_BEGIN});
    }

    void eventStop(const double time,
                   const EventName event) {
      logEvent(new TraceLog{time,
                            eventReg.findEventID(event),
                            GROUP_END});
    }
    
    void logEvent(TraceLog* const log) {
      if (!enabled) {
        return;
      }      

      auto grouped_begin = [&]{
        if (!open.empty()) {
          traces.push_back(new TraceLog{log->time - 0.0000001,
                                        open.top()->event,
                                        GROUP_END});
        }
        // push on open stack.
        open.push(log);
        traces.push_back(log);
      };

      auto grouped_end = [&]{
        // sanity assertions
        assert(!open.empty());
        assert(open.top()->event == log->event &&
               open.top()->type == GROUP_BEGIN);

        open.pop();

        traces.push_back(log);
        
        if (!open.empty()) {
          traces.push_back(new TraceLog{log->time + 0.0000001,
                                        open.top()->event,
                                        GROUP_BEGIN});
        }
      };

      auto nongroup = [&]{
        traces.push_back(log);
      };

      switch (log->type) {
      case GROUP_BEGIN:
        grouped_begin();
        break;
      case GROUP_END:
        grouped_end();
        break;
      case NON_GROUP:
        nongroup();
        break;
      default:
        DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
      }
    }
    
    void start() {
      enabled = true;
    }

    void stop() {
      enabled = false;
    }

    static void outputHeader(const size_t rank,
                             const double startTime,
                             std::ofstream& file) {
      // Output header for projections file
      file << "PROJECTIONS-RECORD 0"
           << std::endl;
      // '6' means COMPUTATION_BEGIN to Projections: this starts a
      // trace
      file << "6 "
           << TraceModule::time2long(startTime)
           << std::endl;
    }

    static void outputFooter(const size_t rank,
                             const double startTime,
                             std::ofstream& file) {
      /// Output footer for projections file, '7' means
      /// COMPUTATION_END to Projections
      file << "7 "
           << TraceModule::time2long(timer.currentTime() - startTime)
           << std::endl;
    }

    static long time2long(const double time) {
      return (long)(time * 1e6);
    }
    
  private:
    bool enabled;
  };
}

#endif /* _THREADS_TRACE_BACKEND_RUNTIME_H_ */
