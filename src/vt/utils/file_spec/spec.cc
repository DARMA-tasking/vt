/*
//@HEADER
// *****************************************************************************
//
//                                   spec.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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
#include "vt/utils/file_spec/spec.h"
#include "vt/objgroup/manager.h"

#include <fstream>

namespace vt { namespace utils { namespace file_spec {

void FileSpec::init(ProxyType in_proxy, FileSpecType in_type) {
  proxy_ = in_proxy;
  type_ = in_type;
}

bool FileSpec::checkEnabled(SpecIndex in_phase) {
  vtAssertExpr(has_spec_);

  for (auto&& elm : spec_mod_) {
    if (elm.second.testEnabledFor(in_phase)) {
      return true;
    }
  }
  for (auto&& elm : spec_exact_) {
    if (elm.second.testEnabledFor(in_phase)) {
      return true;
    }
  }
  return false;
}

bool FileSpec::hasSpec() {
  auto checkSpecFile = [](
                         bool spec_enabled, std::string const& spec_file,
                         std::string const& spec_type) {
    if (spec_enabled) {
      if (spec_file == "") {
        vtAbort(fmt::format(
          "--vt_{}_spec enabled but no file name is specified:"
          " --vt_{}_spec_file",
          spec_type, spec_type));
        return false;
      } else {
        std::ifstream file(spec_file);
        if (not file.good()) {
          auto str = fmt::format(
            "--vt_{}_spec_file={} is not a valid file", spec_type, spec_file);
          vtAbort(str);
          return false;
        } else {
          return true;
        }
      }
    } else {
      return false;
    }
  };

  if (type_ == FileSpecType::TRACE) {
    return checkSpecFile(
      theConfig()->vt_trace_spec, theConfig()->vt_trace_spec_file, "trace");
  } else if (type_ == FileSpecType::LB) {
    return checkSpecFile(
      theConfig()->vt_lb_spec, theConfig()->vt_lb_spec_file, "lb");
  }
  else{
    vtAbort("Unknown Spec Type");
    return false;
  }
}

void FileSpec::parse() {
  if (not hasSpec()) {
    return;
  }

  auto& filename = type_ == FileSpecType::TRACE ?
    theConfig()->vt_trace_spec_file :
    theConfig()->vt_lb_spec_file;
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
      vtAbortIf(
        phase_negative_offset > 0,
        fmt::format(
          "Parsing file \"{}\" error: found offset in negative offset position"
          " that is > 0: value \"{}\"",
          filename,
          phase_negative_offset
        )
      );
    }

    c = eatWhitespace(file);

    /*
     * Parse positive offset for phase/mod: "[%]10 -5 5..."
     *                                                ^
     * This offset must be positive or zero!
     */
    if (std::isdigit(c)) {
      file >> phase_positive_offset;
      vtAbortIf(
        phase_positive_offset < 0,
        fmt::format(
          "Parsing file \"{}\" error: found offset in positive offset position"
          " that is < 0: value \"{}\"",
          filename,
          phase_positive_offset
        )
      );
    }

    eatWhitespace(file);

    /*
     * Entry is complete; eat all the following newlines
     */
    while (file.peek() == '\n') {
      file.get();
    }

    vt_debug_print(
      verbose, trace,
      "FileSpec::parser: is_mod={}, phase={}, neg={}, pos={}\n",
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

void FileSpec::broadcastSpec() {
  auto root = theContext()->getNode();
  proxy_.template broadcast<SpecMsg, &FileSpec::transferSpec>(
    spec_mod_, spec_exact_, root
  );
}

void FileSpec::transferSpec(SpecMsg* msg) {
  // The broadcast will hit all nodes, so the node that it exists on will
  // ignore it
  if (not has_spec_) {
    spec_mod_ = msg->spec_mod_;
    spec_exact_ = msg->spec_exact_;
    has_spec_ = true;
  }
}

void FileSpec::insertSpec(
  SpecIndex phase, SpecIndex neg, SpecIndex pos, bool is_mod, SpecMapType& map
) {
  vtAbortIf(
    map.find(phase) != map.end(),
    fmt::format(
      "Parsing file \"{}\" error: multiple lines start with the same {}:"
      " value \"{}{}\"",
      is_mod ? "mod phase" : "phase",
      type_ == FileSpecType::TRACE ? theConfig()->vt_trace_spec_file : theConfig()->vt_lb_spec_file,
      is_mod ? "%" : "",
      phase
    )
  );
  map.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(phase),
    std::forward_as_tuple(SpecEntry{phase, neg, pos, is_mod})
  );
}

int FileSpec::eatWhitespace(std::ifstream& file) {
  while (not file.eof() and std::isspace(file.peek()) and file.peek() != '\n') {
    file.get();
  }
  return file.eof() ? 0 : file.peek();
}

/*static*/ typename FileSpec::ProxyType FileSpec::construct(FileSpecType type) {
  auto proxy = theObjGroup()->makeCollective<FileSpec>(GetSpecName(type));
  proxy.get()->init(proxy, type);
  return proxy;
}

}}} /* end namespace vt::utils::file_spec */
