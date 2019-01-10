/*
//@HEADER
// ************************************************************************
//
//                          trace.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_TRACE_TRACE_H
#define INCLUDED_TRACE_TRACE_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/configs/arguments/args.h"
#include "vt/trace/trace_common.h"
#include "vt/trace/trace_registry.h"
#include "vt/trace/trace_constants.h"
#include "vt/trace/trace_event.h"
#include "vt/trace/trace_containers.h"
#include "vt/trace/trace_log.h"

#include <cstdint>
#include <cassert>
#include <unordered_map>
#include <stack>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <fstream>

#include <mpi.h>
#include <zlib.h>

namespace vt { namespace trace {

struct Trace {
  using LogType             = Log;
  using TraceConstantsType  = eTraceConstants;
  using TraceContainersType = TraceContainers<void>;
  using TimeIntegerType     = int64_t;
  using LogPtrType          = LogType*;
  using TraceContainerType  = std::vector<LogPtrType>;
  using TraceStackType      = std::stack<LogPtrType>;
  using ArgType             = vt::arguments::ArgConfig;

  Trace();
  Trace(std::string const& in_prog_name, std::string const& in_trace_name);

  virtual ~Trace();

  friend struct Log;

  std::string getTraceName() const { return full_trace_name; }
  std::string getSTSName()   const { return full_sts_name;   }
  std::string getDirectory() const { return full_dir_name;   }

  void initialize();
  void setupNames(
    std::string const& in_prog_name, std::string const& in_trace_name,
    std::string const& in_dir_name = ""
  );

  void beginProcessing(
    TraceEntryIDType const& ep, TraceMsgLenType const& len,
    TraceEventIDType const& event, NodeType const& from_node,
    double const& time = getCurrentTime(),
    uint64_t const idx = 0
  );
  void endProcessing(
    TraceEntryIDType const& ep, TraceMsgLenType const& len,
    TraceEventIDType const& event, NodeType const& from_node,
    double const& time = getCurrentTime(),
    uint64_t const idx = 0
  );

  void beginIdle(double const& time = getCurrentTime());
  void endIdle(double const& time = getCurrentTime());

  TraceEventIDType messageCreation(
    TraceEntryIDType const& ep, TraceMsgLenType const& len,
    double const& time = getCurrentTime()
  );
  TraceEventIDType messageCreationBcast(
    TraceEntryIDType const& ep, TraceMsgLenType const& len,
    double const& time = getCurrentTime()
  );
  TraceEventIDType messageRecv(
    TraceEntryIDType const& ep, TraceMsgLenType const& len,
    NodeType const& from_node, double const& time = getCurrentTime()
  );
  TraceEventIDType logEvent(LogPtrType log);

  void enableTracing();
  void disableTracing();
  bool checkEnabled();

  void writeTracesFile();
  void writeLogFile(gzFile file, TraceContainerType const& traces);
  bool inIdleEvent() const;

  static double getCurrentTime();
  static void outputControlFile(std::ofstream& file);
  static TimeIntegerType timeToInt(double const& time);
  static void traceBeginIdleTrigger();
  static void outputHeader(
    NodeType const& node, double const& start, gzFile file
  );
  static void outputFooter(
    NodeType const& node, double const& start, gzFile file
  );

private:
  TraceContainerType traces_;
  TraceStackType open_events_;
  TraceEventIDType cur_event_ = 1;
  std::string dir_name_       = "";
  std::string prog_name_      = "";
  std::string trace_name_     = "";
  bool enabled_               = true;
  bool idle_begun_            = false;
  bool use_directory_         = false;
  double start_time_          = 0.0;
  std::string full_trace_name = "";
  std::string full_sts_name   = "";
  std::string full_dir_name   = "";
};

}} //end namespace vt::trace

namespace vt {

backend_enable_if(
  trace_enabled,
  extern trace::Trace* theTrace();
);

}

#endif /*INCLUDED_TRACE_TRACE_H*/
