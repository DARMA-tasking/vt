/*
//@HEADER
// *****************************************************************************
//
//                                   spec.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "vt/config.h"
#include "vt/trace/file_spec/spec.h"
#include "vt/objgroup/manager.h"

#include <fstream>

namespace vt { namespace trace { namespace file_spec {

void TraceSpec::init(ProxyType in_proxy) {
  proxy_ = in_proxy;
}

bool TraceSpec::checkTraceEnabled(SpecIndex in_phase) {
  vtAssertExpr(has_spec_);

  for (auto&& elm : spec_mod_) {
    if (elm.second.testTraceEnabledFor(in_phase)) {
      return true;
    }
  }
  for (auto&& elm : spec_exact_) {
    if (elm.second.testTraceEnabledFor(in_phase)) {
      return true;
    }
  }
  return false;
}

bool TraceSpec::hasSpec() {
  if (ArgType::vt_trace_spec) {
    if (ArgType::vt_trace_spec_file == "") {
      vtAbort(
        "--vt_trace_spec enabled but no file name is specified:"
        " --vt_trace_spec_file"
      );
      return false;
    } else {
      auto& filename = ArgType::vt_trace_spec_file;
      std::ifstream file(filename);
      if (not file.good()) {
        auto str = fmt::format(
          "--vt_trace_spec_file={} is not a valid file", filename
        );
        vtAbort(str);
        return false;
      } else {
        return true;
      }
    }
  } else {
    return false;
  }
}

void TraceSpec::parse() {
  if (not hasSpec()) {
    return;
  }

  auto& filename = ArgType::vt_trace_spec_file;
  std::ifstream file(filename);
  vtAssertExpr(file.good());

  while (!file.eof()) {
    bool is_mod  = false;
    SpecIndex phase = -1;
    SpecIndex phase_negative_offset = 0;
    SpecIndex phase_positive_offset = 0;

    int c = eatWhitespace(file);

    /*
     * Optionally parse an entry that starts with an mod phase: "% ..."
     *                                                           ^
     */
    if (static_cast<char>(c) == '%') {
      is_mod = true;
      // Eat up the '%', move to next
      file.get();
      c = eatWhitespace(file);
    }

    /*
     * Parse phase/mod phase: "[%]10..."
     *                            ^^
     */
    if (std::isdigit(c)) {
      file >> phase;
    }

    c = eatWhitespace(file);

    /*
     * Parse negative offset for phase/mod: "[%]10 -5..."
     *                                             ^^
     * This offset must be negative or zero!
     */
    if (std::isdigit(c) or static_cast<char>(c) == '-') {
      file >> phase_negative_offset;
      if (phase_negative_offset > 0) {
        auto str = fmt::format(
          "Parsing file \"{}\" error: found offset in negative offset position"
          " that is > 0: value \"{}\"",
          filename,
          phase_negative_offset
        );
        vtAbort(str);
      }
    }

    c = eatWhitespace(file);

    /*
     * Parse positive offset for phase/mod: "[%]10 -5 5..."
     *                                                ^
     * This offset must be positive or zero!
     */
    if (std::isdigit(c)) {
      file >> phase_positive_offset;
      if (phase_positive_offset < 0) {
        auto str = fmt::format(
          "Parsing file \"{}\" error: found offset in positive offset position"
          " that is < 0: value \"{}\"",
          filename,
          phase_positive_offset
        );
        vtAbort(str);
      }
    }

    eatWhitespace(file);

    /*
     * Entry is complete; eat all the following newlines
     */
    while (file.peek() == '\n') {
      file.get();
    }

    debug_print(
      trace, node,
      "TraceSpec::parser: is_mod={}, phase={}, neg={}, pos={}\n",
      is_mod, phase, phase_negative_offset, phase_positive_offset
    );

    // We have a whole entry, put in the spec; it should not already exist
    insertSpec(
      phase, phase_negative_offset, phase_positive_offset, is_mod,
      is_mod ? spec_mod_ : spec_exact_
    );
  }

  has_spec_ = true;
}

void TraceSpec::broadcastSpec() {
  auto root = theContext()->getNode();
  auto msg = makeMessage<SpecMsg>(spec_mod_, spec_exact_, root);
  proxy_.template broadcast<SpecMsg, &TraceSpec::transferSpec>(msg.get());
}

void TraceSpec::transferSpec(SpecMsg* msg) {
  // The broadcast will hit all nodes, so the node that it exists on will
  // ignore it
  if (not has_spec_) {
    spec_mod_ = msg->spec_mod_;
    spec_exact_ = msg->spec_exact_;
    has_spec_ = true;
  }
}

void TraceSpec::insertSpec(
  SpecIndex phase, SpecIndex neg, SpecIndex pos, bool is_mod, SpecMapType& map
) {
  auto iter = map.find(phase);
  if (iter != map.end()) {
    auto str = fmt::format(
      "Parsing file \"{}\" error: multiple lines start with the same {}:"
      " value \"{}{}\"",
      is_mod ? "mod phase" : "phase",
      ArgType::vt_trace_spec_file,
      is_mod ? "%" : "",
      phase
    );
    vtAbort(str);
  } else {
    map.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(phase),
      std::forward_as_tuple(SpecEntry{phase, neg, pos, is_mod})
    );
  }
}

int TraceSpec::eatWhitespace(std::ifstream& file) {
  while (not file.eof() and std::isspace(file.peek()) and file.peek() != '\n') {
    file.get();
  }
  return file.eof() ? 0 : file.peek();
}

/*static*/ typename TraceSpec::ProxyType TraceSpec::construct() {
  auto proxy = theObjGroup()->makeCollective<TraceSpec>();
  proxy.get()->init(proxy);
  return proxy;
}

}}} /* end namespace vt::trace::file_spec */
