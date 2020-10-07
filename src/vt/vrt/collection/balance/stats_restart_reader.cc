/*
//@HEADER
// *****************************************************************************
//
//                           stats_restart_reader.cc
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_RESTART_READER_CC
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_RESTART_READER_CC

#include "vt/config.h"
#include "vt/vrt/collection/balance/stats_restart_reader.h"
#include "vt/objgroup/manager.h"
#include "vt/vrt/collection/balance/lb_comm.h"

#include <cinttypes>

namespace vt { namespace vrt { namespace collection { namespace balance {

void StatsRestartReader::setProxy(
  objgroup::proxy::Proxy<StatsRestartReader> in_proxy
) {
  proxy_ = in_proxy;
}

/*static*/ std::unique_ptr<StatsRestartReader> StatsRestartReader::construct() {
  auto ptr = std::make_unique<StatsRestartReader>();
  auto proxy = theObjGroup()->makeCollective<StatsRestartReader>(ptr.get());
  proxy.get()->setProxy(proxy);
  return ptr;
}

void StatsRestartReader::startup() {
  auto const node = theContext()->getNode();
  std::string const& base_file = theConfig()->vt_lb_stats_file_in;
  std::string const& dir = theConfig()->vt_lb_stats_dir_in;
  auto const file = fmt::format("{}.{}.out", base_file, node);
  auto const file_name = fmt::format("{}/{}", dir, file);
  readStats(file_name);
}

std::vector<ElementIDType> const&
StatsRestartReader::getMoveList(PhaseType phase) const {
  vtAssert(proc_move_list_.size() > phase, "Phase must exist");
  return proc_move_list_.at(phase);
}

void StatsRestartReader::clearMoveList(PhaseType phase) {
  if (proc_move_list_.size() > phase) {
    proc_move_list_.at(phase).clear();
  }
}

bool StatsRestartReader::needsLB(PhaseType phase) const {
  if (proc_phase_runs_LB_.size() > phase) {
    return proc_phase_runs_LB_.at(phase);
  } else {
    return false;
  }
}

std::deque<std::vector<ElementIDType>> const&
StatsRestartReader::getMigrationList() const {
  return proc_move_list_;
}

void StatsRestartReader::readStats(std::string const& fileName) {

  // Read the input files
  std::deque<std::set<ElementIDType>> elements_history;
  inputStatsFile(fileName, elements_history);
  if (elements_history.empty()) {
    vtWarn("No element history provided");
    return;
  }

  auto const num_iters = elements_history.size() - 1;
  proc_move_list_.resize(num_iters);
  proc_phase_runs_LB_.resize(num_iters, true);

  if (theContext()->getNode() == 0) {
    msgsReceived.resize(num_iters, 0);
    totalMove.resize(num_iters);
  }

  // Communicate the migration information
  createMigrationInfo(elements_history);
}

void StatsRestartReader::inputStatsFile(
  std::string const& fileName,
  std::deque<std::set<ElementIDType>>& element_history
) {
  std::FILE *pFile = std::fopen(fileName.c_str(), "r");
  if (pFile == nullptr) {
    vtAssert(pFile, "File opening failed");
  }

  std::set<ElementIDType> buffer;

  // Load: Format of a line :size_t, ElementIDType, TimeType
  size_t phaseID = 0, prevPhaseID = 0;
  ElementIDType elmID;
  TimeType tval;
  CommBytesType d_buffer;
  using vtCommType = typename std::underlying_type<CommCategory>::type;
  vtCommType typeID;
  char separator;
  fpos_t pos;
  bool finished = false;
  while (!finished) {
    if (fscanf(pFile, "%zu %c %" PRIu64 " %c %lf",
               &phaseID, &separator, &elmID, &separator, &tval) > 0) {
      fgetpos (pFile,&pos);
      fscanf (pFile, "%c", &separator);
      if (separator == ',') {
        // COM detected, read the end of line and do nothing else
        int res = fscanf (pFile, "%lf %c %hhi", &d_buffer, &separator, &typeID);
        vtAssertExpr(res == 3);
      } else {
        // Load detected, create the new element
        fsetpos (pFile,&pos);
        if (prevPhaseID != phaseID) {
          prevPhaseID = phaseID;
          element_history.push_back(buffer);
          buffer.clear();
        }
        buffer.insert(elmID);
      }
    } else {
      finished = true;
    }
  }

  if (!buffer.empty()) {
    element_history.push_back(buffer);
  }

  std::fclose(pFile);
}

void StatsRestartReader::createMigrationInfo(
  std::deque<std::set<ElementIDType>>& element_history
) {
  const auto num_iters = element_history.size() - 1;
  const auto myNodeID = static_cast<ElementIDType>(theContext()->getNode());

  for (size_t ii = 0; ii < num_iters; ++ii) {
    auto& elms = element_history[ii];
    auto& elmsNext = element_history[ii + 1];
    std::set<ElementIDType> diff;
    std::set_difference(elmsNext.begin(), elmsNext.end(), elms.begin(),
                        elms.end(), std::inserter(diff, diff.begin()));
    const size_t qi = diff.size();
    const size_t pi = elms.size() - (elmsNext.size() - qi);
    auto& myList = proc_move_list_[ii];
    myList.reserve(3 * (pi + qi) + 1);
    //--- Store the iteration number
    myList.push_back(static_cast<ElementIDType>(ii));
    //--- Store partial migration information (i.e. nodes moving in)
    for (auto iEle : diff) {
      myList.push_back(iEle);  //--- permID to receive
      myList.push_back(no_element_id); // node moving from
      myList.push_back(myNodeID); // node moving to
    }
    diff.clear();
    //--- Store partial migration information (i.e. nodes moving out)
    std::set_difference(elms.begin(), elms.end(), elmsNext.begin(),
                        elmsNext.end(), std::inserter(diff, diff.begin()));
    for (auto iEle : diff) {
      myList.push_back(iEle);  //--- permID to send
      myList.push_back(myNodeID); // node migrating from
      myList.push_back(no_element_id); // node migrating to
    }
    //
    // Create a message storing the vector
    //
    proxy_[0].send<VecMsg, &StatsRestartReader::gatherMsgs>(myList);
    //
    // Clear old distribution of elements
    //
    elms.clear();
  }

}

void StatsRestartReader::gatherMsgs(VecMsg *msg) {
  auto sentVec = msg->getTransfer();
  vtAssert(sentVec.size() % 3 == 1, "Expecting vector of length 3n+1");
  ElementIDType const phaseID = sentVec[0];
  //
  // --- Combine the different pieces of information
  //
  msgsReceived[phaseID] += 1;
  auto& migrate = totalMove[phaseID];
  for (size_t ii = 1; ii < sentVec.size(); ii += 3) {
    auto const permID = sentVec[ii];
    auto const nodeFrom = static_cast<NodeType>(sentVec[ii + 1]);
    auto const nodeTo = static_cast<NodeType>(sentVec[ii + 2]);
    auto iptr = migrate.find(permID);
    if (iptr == migrate.end()) {
      migrate.insert(std::make_pair(permID, std::make_pair(nodeFrom, nodeTo)));
    }
    else {
      auto &nodePair = iptr->second;
      nodePair.first = std::max(nodePair.first, nodeFrom);
      nodePair.second = std::max(nodePair.second, nodeTo);
    }
  }
  //
  // --- Check whether all the messages have been received
  //
  const NodeType numNodes = theContext()->getNumNodes();
  if (msgsReceived[phaseID] < static_cast<std::size_t>(numNodes))
    return;
  //
  //--- Distribute the information when everything has been received
  //
  size_t const header = 2;
  for (NodeType in = 0; in < numNodes; ++in) {
    size_t iCount = 0;
    for (auto iNode : migrate) {
      if (iNode.second.first == in)
        iCount += 1;
    }
    std::vector<ElementIDType> toMove(2 * iCount + header);
    iCount = 0;
    toMove[iCount++] = phaseID;
    toMove[iCount++] = static_cast<ElementIDType>(migrate.size());
    for (auto iNode : migrate) {
      if (iNode.second.first == in) {
        toMove[iCount++] = iNode.first;
        toMove[iCount++] = static_cast<ElementIDType>(iNode.second.second);
      }
    }
    if (in > 0) {
      proxy_[in].send<VecMsg, &StatsRestartReader::scatterMsgs>(toMove);
    } else {
      proc_phase_runs_LB_[phaseID] = (!migrate.empty());
      auto& myList = proc_move_list_[phaseID];
      myList.resize(toMove.size() - header);
      std::copy(&toMove[header], &toMove[0] + toMove.size(),
                myList.begin());
    }
  }
  migrate.clear();
}

void StatsRestartReader::scatterMsgs(VecMsg *msg) {
  const size_t header = 2;
  auto recvVec = msg->getTransfer();
  vtAssert((recvVec.size() -header) % 2 == 0,
    "Expecting vector of length 2n+2");
  //--- Get the iteration number associated with the message
  const ElementIDType phaseID = recvVec[0];
  //--- Check whether some migration will be done
  proc_phase_runs_LB_[phaseID] = static_cast<bool>(recvVec[1] > 0);
  auto &myList = proc_move_list_[phaseID];
  if (!proc_phase_runs_LB_[phaseID]) {
    myList.clear();
    return;
  }
  //--- Copy the migration information
  myList.resize(recvVec.size() - header);
  std::copy(&recvVec[header], &recvVec[0]+recvVec.size(), myList.begin());
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_RESTART_READER_CC*/
