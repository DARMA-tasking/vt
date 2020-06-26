/*
//@HEADER
// *****************************************************************************
//
//                                  read_lb.cc
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
#include "vt/context/context.h"
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/vrt/collection/balance/lb_type.h"
#include "vt/configs/arguments/args.h"

#include <string>
#include <fstream>
#include <cassert>
#include <cctype>
#include <cmath>

namespace vt { namespace vrt { namespace collection { namespace balance {

/*static*/ std::string ReadLBSpec::filename = {};
/*static*/ SpecIndex ReadLBSpec::num_entries_ = 0;
/*static*/ typename ReadLBSpec::SpecMapType ReadLBSpec::spec_mod_ = {};
/*static*/ typename ReadLBSpec::SpecMapType ReadLBSpec::spec_exact_ = {};
/*static*/ std::vector<SpecIndex> ReadLBSpec::spec_prec_ = {};
/*static*/ bool ReadLBSpec::read_complete_ = false;

/*static*/ bool ReadLBSpec::hasSpec() {
  if (not theArgConfig()->vt_lb_file) {
    return false;
  }
  if (read_complete_) {
    return true;
  } else {
    auto const file_name = theArgConfig()->vt_lb_file_name;
    if (file_name == "") {
      vtAbort(
        "--vt_lb_file enabled but no file name is specified: --vt_lb_file_name"
      );
      return false;
    } else {
      bool good = openFile(file_name);
      if (not good) {
        auto str =
          fmt::format("--vt_lb_file_name={} is not a valid file", file_name);
        vtAbort(str);
      }
      return good;
    }
  }
}

/*static*/ bool ReadLBSpec::openFile(std::string const name) {
  std::ifstream file(name);
  filename = name;
  return file.good();
}

/*static*/ LBType ReadLBSpec::getLB(SpecIndex const& idx) {
  if (not read_complete_) {
    readFile();
  }
  auto const lb = entry(idx);
  if (lb) {
    return lb->getLB();
  } else {
    return LBType::NoLB;
  }
}

/*static*/ SpecEntry* ReadLBSpec::entry(SpecIndex const& idx) {
  // First, search the exact iter spec for this iteration: it has the highest
  // precedence
  auto spec_iter = spec_exact_.find(idx);
  if (spec_iter != spec_exact_.end()) {
    return &spec_iter->second;
  }

  // Second, walk through the spec precedence map for the mod overloads
  for (auto mod : spec_prec_) {
    auto iter = spec_mod_.find(mod);
    if (iter != spec_mod_.end()) {
      // Check if this mod is applicable to the idx
      if (idx % mod == 0) {
        auto iter_mod = spec_mod_.find(mod);
        if (iter_mod != spec_mod_.end()) {
          return &iter_mod->second;
        }
      }
    }
  }

  // Else, return nullptr---no applicable entry found
  return nullptr;
}

int eatWhitespace(std::ifstream& file) {
  while (not file.eof() and std::isspace(file.peek()) and file.peek() != '\n') {
    file.get();
  }
  return file.eof() ? 0 : file.peek();
}

/*static*/ void ReadLBSpec::readFile() {
  if (read_complete_) {
    return;
  }

  std::ifstream file(filename);
  vtAssert(file.good(), "must be valid");

  while (!file.eof()) {
    bool is_mod = false;
    int64_t mod = -1;
    std::string lb_name;
    std::vector<std::string> params;

    int c = eatWhitespace(file);

    /*
     * Parse an entry that starts with an LB: "% ..."
     */
    if (static_cast<char>(c) == '%') {
      is_mod = true;
      // Eat up the '%', move to next
      file.get();
      c = eatWhitespace(file);
    }

    /*
     * Parse entry starting LB iter/mod: "[%] 10 GreedyLB ..."
     */
    if (std::isdigit(c)) {
      file >> mod;
    }

    c = eatWhitespace(file);

    /*
     * Parse the name of the LB: "GreedyLB ..."
     */
    if (std::isalpha(c)) {
      file >> lb_name;
    }

    c = eatWhitespace(file);

    /*
     * Parse out all the parameters for the LB: "x=1 y=2 test=3 ..."
     */
    while (file.peek() != '\n' and not file.eof()) {
      std::string param;
      file >> param;
      params.push_back(param);
    }

    eatWhitespace(file);

    while (file.peek() == '\n') {
      file.get();
    }

    /*
     * Split params into 'key=value'
     */
    auto const param_map = parseParams(params);

    /*
     * Check to make sure we have a valid LB name
     */
    bool valid_lb_found = false;
    for (auto&& elm : lb_names_) {
      if (lb_name == elm.second) {
        valid_lb_found = true;
      }
    }
    if (not valid_lb_found) {
      auto err_msg = fmt::format("Valid LB not found: \"name={}\"\n", lb_name);
      vtAbort(err_msg);
    }

    /*
     * If the line is specified as a mod '%' or not line is specified (assume
     * mod 1)
     */
    SpecMapType* map = nullptr;
    if (is_mod or mod == -1) {
      if (mod == -1) {
        mod = 1;
      }
      spec_prec_.push_back(mod);
      map = &spec_mod_;
    } else {
      map = &spec_exact_;
    }

    if (map->find(mod) != map->end()) {
      auto err_msg = fmt::format(
        "Iter {} specified twice: name={}, mod={}\n", mod, lb_name, is_mod
      );
      vtAbort(err_msg);
    }

    map->emplace(
      std::piecewise_construct,
      std::forward_as_tuple(mod),
      std::forward_as_tuple(SpecEntry{mod, lb_name, param_map})
    );
  }

  read_complete_ = true;
}

/*static*/ void ReadLBSpec::clear() {
  read_complete_ = false;
  filename = "";
  num_entries_ = 0;
  spec_mod_.clear();
  spec_exact_.clear();
  spec_prec_.clear();
}

/*static*/ typename ReadLBSpec::ParamMapType
ReadLBSpec::parseParams(std::vector<std::string> params) {
  ParamMapType param_map;

  /*
   * Split params into 'key=value'
   */
  for (auto&& p : params) {
    if (p == "") {
      continue;
    }
    std::string key, value;
    bool found = false;
    for (std::size_t i = 0; i < p.size(); i++) {
      if (p[i] == '=') {
        key = p.substr(0, i);
        value = p.substr(i + 1, p.length() - 1);
        param_map[key] = value;
        found = true;
      }
    }
    if (not found) {
      auto err = fmt::format("LB file reader: could not parse param: \"{}\"", p);
      vtAbort(err);
    }
  }

  return param_map;
}

/*static*/ SpecEntry ReadLBSpec::makeSpecFromParams(std::string param_str) {
  std::istringstream stream(param_str);
  std::vector<std::string> params;
  while (not stream.eof()) {
    std::string param;
    stream >> param;
    params.push_back(param);
  }

  auto param_map = parseParams(params);

  return SpecEntry{0, "", param_map};
}

}}}} /* end namespace vt::vrt::collection::balance */
