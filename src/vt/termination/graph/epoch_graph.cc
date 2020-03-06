/*
//@HEADER
// *****************************************************************************
//
//                                epoch_graph.cc
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
#include "vt/termination/graph/epoch_graph.h"
#include "vt/termination/term_common.h"
#include "vt/epoch/epoch_headers.h"
#include "vt/configs/arguments/args.h"

#include <sys/stat.h>
#include <cstdio>

namespace vt { namespace termination { namespace graph {

void EpochGraph::detectCyclesImpl(std::list<EpochType>& stack) {
  bool cycle = false;
  std::string cycle_diagnostic = "";
  for (auto&& p : stack) {
    // We have encountered the same epoch previously in the stack, thus a
    // cycle
    if (p == epoch_) {
      cycle = true;
    }
    if (cycle) {
      cycle_diagnostic += fmt::format("{:x}->", p);
    }
  }
  if (cycle) {
    cycle_diagnostic += fmt::format("{:x}", epoch_);
    std::string cycle_print = fmt::format(
      "Cycle detected in epoch graph:\nCycle: {}\n",
      cycle_diagnostic
    );
    vtAbort(cycle_print);
    return;
  }

  stack.push_back(epoch_);
  for (auto const& s : successors_) {
    s->detectCyclesImpl(stack);
  }
  vtAssert(stack.back() == epoch_, "Must match after pop!");
  stack.pop_back();
}

void EpochGraph::detectCycles() {
  // DFS on graph, maintaining a stack to detect any cycles (which are an
  // error, and will cause a hang). This could be made more efficient by
  // tracking visited nodes
  std::list<EpochType> stack;
  detectCyclesImpl(stack);
}

void EpochGraph::outputImpl(
  std::set<std::tuple<EpochType, std::string, EpochType, std::string>> &links
)  {
  for (auto const& s : successors_) {
    links.insert(
      std::make_tuple(epoch_, user_label_, s->epoch_, s->user_label_)
    );
    s->outputImpl(links);
  }
}

EpochGraph::EpFormat EpochGraph::formatDOTEpoch(
  EpochType epoch, std::string label
) {
  if (epoch == term::any_epoch_sentinel) {
    return std::make_tuple(epoch, static_cast<NodeType>(-1), true, "Global");
  } else {
    std::string label_format = "";
    if (label != "") {
      label_format = fmt::format("{}{}{}", "\\\"", label, "\\\"\\n");
    }
    if (epoch::EpochManip::isRooted(epoch)) {
      auto const ds_epoch = epoch::eEpochCategory::DijkstraScholtenEpoch;
      auto const epoch_category = epoch::EpochManip::category(epoch);
      auto const is_ds = epoch_category == ds_epoch;
      auto const ep_node = epoch::EpochManip::node(epoch);
      auto const ep_seq = epoch::EpochManip::seq(epoch);
      if (is_ds) {
        auto str = fmt::format("{}{:x}-DS-{}", label_format, ep_seq, ep_node);
        return std::make_tuple(epoch, ep_node, false, str);
      } else {
        auto str = fmt::format("{}{:x}-Wave-{}", label_format, ep_seq, ep_node);
        return std::make_tuple(epoch, ep_node, false, str);
      }
    } else {
      auto str = fmt::format("{}{:x}-C", label_format, epoch);
      return std::make_tuple(epoch, static_cast<NodeType>(-1), true, str);
    }
  }
}

std::string EpochGraph::outputDOT(bool verbose) {
  std::unordered_map<EpochType, EpFormat> eps;
  std::set<std::tuple<EpochType, std::string, EpochType, std::string>> links;
  std::string builder = "";
  outputImpl(links);

  for (auto&& elm : links) {
    if (eps.find(std::get<0>(elm)) == eps.end()) {
      eps[std::get<0>(elm)] = formatDOTEpoch(std::get<0>(elm), std::get<1>(elm));
    }
    if (eps.find(std::get<2>(elm)) == eps.end()) {
      eps[std::get<2>(elm)] = formatDOTEpoch(std::get<2>(elm), std::get<3>(elm));
    }
  }

  /*
   * This is a very slow/inefficient std::string builder...but this is
   * completely off the critical path
   */
  builder = "digraph graphname {\n";
  builder += "\tlabelloc = \"b\"\n";
  builder += "\trankdir = \"TB\"\n";
  builder += "\tranksep = \"1\"\n";
  builder += "\tedge[\n";
  builder += "\t\t\tpenwidth=2\n";
  builder += "\t]\n";
  builder += "\tnode[\n";
  builder += "\t\tfontname=Monaco,\n";
  builder += "\t\tpenwidth=1,\n";
  builder += "\t\tfontsize=10,\n";
  if (verbose) {
    builder += "\t\tmargin=.2,\n";
  } else {
    builder += "\t\tmargin=.3,\n";
  }
  builder += "\t\tshape=box,\n";
  builder += "\t\tfillcolor=lightblue,\n";
  builder += "\t\tstyle=\"rounded,filled\"\n";
  builder += "\t]\n";
  for (auto&& elm : eps) {
    EpochType ep = std::get<0>(elm.second);
    NodeType node = std::get<1>(elm.second);
    bool collective = std::get<2>(elm.second);
    std::string str = std::get<3>(elm.second);
    if (not arguments::ArgConfig::vt_epoch_graph_terse or verbose) {
      if (collective) {
        builder += fmt::format(
          "\t{} [shape=record height=1"
          " label=\"{}|{} collective | Hex: {:x} {}\"]\n",
          elm.first, str, "{", ep, "}"
        );
      } else {
        builder += fmt::format(
          "\t{} [shape=record height=1"
          " label=\"{}|{} rooted | Hex: {:x} | node={} {}\"]\n",
          elm.first, str, "{", ep, node, "}"
        );
      }
    } else {
      builder += fmt::format("\t{} [label=\"{}\"]\n", elm.first, str);
    }
  }
  for (auto&& elm : links) {
    builder += fmt::format("\t{}->{};\n", std::get<0>(elm), std::get<2>(elm));
  }
  builder += "}\n";
  return builder;
}

void EpochGraph::writeToFile(std::string const& dot, bool global, std::string tag) {
  auto const node = theContext()->getNode();
  auto const base_file = "epoch_graph";
  if (tag != "") {
    tag = tag + ".";
  }
  std::string file = "";
  if (global) {
    file = fmt::format("{}.global.{}dot", base_file, tag);
  } else {
    file = fmt::format("{}.{}.{}dot", base_file, node, tag);
  }
  fmt::print("file={}\n", file);
  auto fd = fopen(file.c_str(), "w+");
  fmt::print("file desc={}\n", print_ptr(fd));
  fprintf(fd, "%s", dot.c_str());
  fclose(fd);
}

/*friend*/ EpochGraph operator+(EpochGraph a1, EpochGraph const& a2) {
  vtAssert(a1.epoch_ == a2.epoch_, "Trees should match");
  std::vector<std::shared_ptr<EpochGraph>> new_successors;
  for (auto& c2 : a2.successors_) {
    // This could be more efficient O(n) vs O(n^2) if this was stored with a
    // constant time lookup data structure
    std::shared_ptr<EpochGraph> c1_match = nullptr;
    for (auto& c1 : a1.successors_) {
      if (c1->epoch_ == c2->epoch_) {
        c1_match = c1;
        break;
      }
    }

    if (c1_match != nullptr) {
      // Recurse down this matching node, merging the matching successors
      (*c1_match) = (*c1_match) + (*c2);
    } else {
      new_successors.push_back(c2);
    }
  }

  // Add new successors that a1 did not have to a1's successors list
  for (auto const& child : new_successors) {
    a1.successors_.push_back(child);
  }

  return a1;
}

}}} /* end namespace vt::termination::graph */
