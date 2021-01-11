/*
//@HEADER
// *****************************************************************************
//
//                                epoch_graph.h
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

#if !defined INCLUDED_VT_TERMINATION_GRAPH_EPOCH_GRAPH_H
#define INCLUDED_VT_TERMINATION_GRAPH_EPOCH_GRAPH_H

#include "vt/config.h"

#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <unordered_map>
#include <list>
#include <set>

namespace vt { namespace termination { namespace graph {

struct EpochGraph {
  using EpFormat = std::tuple<EpochType, NodeType, bool, std::string>;

  EpochGraph() = default;
  EpochGraph(EpochGraph&&) = default;
  EpochGraph(EpochGraph const&) = default;
  EpochGraph& operator=(EpochGraph const&) = default;

  EpochGraph(EpochType in_epoch, std::string user_label)
    : epoch_(in_epoch),
      user_label_(user_label)
  { }

public:
  bool hasSuccessors() const { return successors_.size() != 0; }
  void addSuccessor(EpochGraph&& t) {
    successors_.push_back(std::make_shared<EpochGraph>(std::move(t)));
  }
  void addSuccessor(std::shared_ptr<EpochGraph> t) {
    successors_.push_back(t);
  }

private:
  void detectCyclesImpl(std::list<EpochType>& stack);

public:
  void detectCycles();

private:
  void outputImpl(
    std::set<std::tuple<EpochType, std::string, EpochType, std::string>> &links
  );
  EpFormat formatDOTEpoch(EpochType epoch, std::string label);

public:
  std::string outputDOT(bool verbose = false);
  void writeToFile(std::string const& dot, bool global = false, std::string tag = "");

public:
  // Merge the EpochGraph `a2` with `a1` recursively to combine localized
  // sub-graphs data up the reduction tree
  friend EpochGraph operator+(EpochGraph a1, EpochGraph const& a2);

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | epoch_;
    s | user_label_;
    std::size_t nc = successors_.size();
    s | nc;
    for (std::size_t i = 0; i < nc; i++) {
      // This is inefficient because it potentially serializes vertices multiple
      // times (if the structure is not a tree). But, given that this only
      // executes as a diagnostic, it's not worth the trouble of keeping a
      // lookup table
      if (s.isUnpacking()) {
        EpochGraph et;
        s | et;
        successors_.push_back(std::make_shared<EpochGraph>(std::move(et)));
      } else {
        s | *successors_[i];
      }
    }
  }

private:
  EpochType epoch_ = no_epoch;
  std::string user_label_ = "";
  std::vector<std::shared_ptr<EpochGraph>> successors_ = {};
};

}}} /* end namespace vt::termination::graph */

#endif /*INCLUDED_VT_TERMINATION_GRAPH_EPOCH_GRAPH_H*/
