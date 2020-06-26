/*
//@HEADER
// *****************************************************************************
//
//                                  read_lb.h
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_READ_LB_H
#define INCLUDED_VRT_COLLECTION_BALANCE_READ_LB_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_type.h"

#include <string>
#include <fstream>
#include <unordered_map>
#include <cstdlib>

namespace vt { namespace vrt { namespace collection { namespace balance {

using SpecIndex = int64_t;

template <typename T>
struct Converter {
  static T convert(std::string val, T default_);
};

template <>
struct Converter<double> {
  static double convert(std::string val, double default_) {
    try {
      return std::stod(val);
    } catch (...) {
      return default_;
    }
  }
};

template <>
struct Converter<bool> {
  static bool convert(std::string val, bool default_) {
    if (val == "1" or val == "true" or val == "t") {
      return true;
    }
    if (val == "0" or val == "false" or val == "f") {
      return false;
    }
    return default_;
  }
};

template <>
struct Converter<int32_t> {
  static int32_t convert(std::string val, int32_t default_) {
    try {
      return std::stoi(val);
    } catch (...) {
      return default_;
    }
  }
};

template <>
struct Converter<std::string> {
  static std::string convert(std::string val, std::string default_) {
    if (val.size() > 0) {
      return val;
    } else {
      return default_;
    }
  }
};

struct SpecEntry {
  SpecEntry(
    SpecIndex const in_idx, std::string const in_name,
    std::unordered_map<std::string, std::string> in_params
  ) : idx_(in_idx), lb_name_(in_name), params_(in_params)
  {}

  SpecIndex getIdx() const { return idx_; }
  std::string getName() const { return lb_name_; }
  LBType getLB() const {
    for (auto&& elm : lb_names_) {
      if (lb_name_ == elm.second) {
        return elm.first;
      }
    }
    return LBType::NoLB;
  }

  template <typename T>
  T getOrDefault(std::string const& key, T default_) const {
    auto iter = params_.find(key);
    if (iter == params_.end()) {
      return default_;
    } else {
      auto val = iter->second;
      return Converter<T>::convert(val, default_);
    }
  }

  void checkAllowedKeys(std::vector<std::string> const& allowed) {
    for (auto&& p : params_) {
      bool found = false;
      for (auto&& key : allowed) {
        if (key == p.first) {
          found = true;
        }
      }
      if (not found) {
        std::string allowed_str;
        for (auto&& key : allowed) {
          allowed_str += fmt::format("\"{}\"; ", key);
        }
        auto err = fmt::format(
          "LB does not support key={}, allowed keys: {}", p.first, allowed_str
        );
        vtAbort(err);
      }
    }
  }

private:
  SpecIndex idx_;
  std::string lb_name_;
  std::unordered_map<std::string, std::string> params_;
};

/*
 * Reads the following file format for LB spec---example:
 *
 *     %10 GossipLB c=1 k=5 f=2 i=10
 *     0 HierarchicalLB min=0.9 max=1.1 auto=false
 *     % 5 GreedyLB min=1.0
 *     120 GreedyLB c=0 k=2 f=3 i=3
 *
 */

struct ReadLBSpec {
  using SpecMapType  = std::unordered_map<SpecIndex,SpecEntry>;
  using ParamMapType = std::unordered_map<std::string, std::string>;

  static bool openFile(std::string const name = "");
  static void readFile();

  static bool hasSpec();
  static SpecIndex numEntries() { return num_entries_; }
  static SpecEntry* entry(SpecIndex const& idx);
  static LBType getLB(SpecIndex const& idx);
  static ParamMapType parseParams(std::vector<std::string> params);
  static SpecEntry makeSpecFromParams(std::string params);
  static void clear();

private:
  static bool read_complete_;
  static std::string filename;
  static SpecIndex num_entries_;
  static SpecMapType spec_mod_;
  static SpecMapType spec_exact_;
  static std::vector<SpecIndex> spec_prec_;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_READ_LB_H*/
