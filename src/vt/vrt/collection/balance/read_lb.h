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

struct SpecEntry {
  SpecEntry(
    SpecIndex const in_idx, std::string const in_name,
    double const in_lb_min, double const in_lb_max
  ) : idx_(in_idx), lb_name_(in_name), lb_min_(in_lb_min), lb_max_(in_lb_max)
  {}

  SpecIndex getIdx() const { return idx_; }
  std::string getName() const { return lb_name_; }
  bool hasMin() const { return lb_min_ != 0.0f; }
  bool hasMax() const { return lb_max_ != 0.0f; }
  double min() const { return lb_min_; }
  double max() const { return lb_max_; }
  LBType getLB() const {
    for (auto&& elm : lb_names_) {
      if (lb_name_ == elm.second) {
        return elm.first;
      }
    }
    return LBType::NoLB;
  }

private:
  SpecIndex idx_;
  std::string lb_name_;
  double lb_min_ = 0.0f, lb_max_ = 0.0f;
};

struct ReadLBSpec {
  using SpecMapType = std::unordered_map<SpecIndex,SpecEntry>;

  static bool openFile(std::string const name = "balance.in");
  static void readFile();

  static bool hasSpec() { return has_spec_; }
  static SpecMapType const& entries() { return spec_; }
  static SpecIndex numEntries() { return num_entries_; }
  static SpecEntry const* entry(SpecIndex const& idx);
  static LBType getLB(SpecIndex const& idx);

private:
  static bool has_spec_;
  static bool read_complete_;
  static std::string filename;
  static SpecIndex num_entries_;
  static SpecMapType spec_;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_READ_LB_H*/
