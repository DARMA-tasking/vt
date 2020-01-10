/*
//@HEADER
// *****************************************************************************
//
//                                stats_lb_reader.cc
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
#include "vt/vrt/collection/balance/stats_lb_reader.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/manager.h"
#include "vt/timing/timing.h"
#include "vt/configs/arguments/args.h"
#include "vt/runtime/runtime.h"

#include <cstdio>
#include <deque>
#include <map>
#include <vector>

#include "fmt/format.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/*static*/
std::deque<std::set<ElementIDType>> StatsLBReader::user_specified_map_ = {};

/*static*/
std::deque< std::vector<ElementIDType> > StatsLBReader::moveList = {};

/*static*/
std::vector<size_t> StatsLBReader::msgReceived = {};

/*static*/
std::deque<std::map<ElementIDType, std::pair<NodeType, NodeType>>>
  StatsLBReader::totalMove = {};

/*static*/
std::vector<bool> StatsLBReader::phase_changed_map_ = {};

/*static*/ objgroup::proxy::Proxy<StatsLBReader> StatsLBReader::proxy_ = {};

/*static*/ void StatsLBReader::init() {
  // Create the new class dedicated to the input reader
  StatsLBReader::proxy_ = theObjGroup()->makeCollective<StatsLBReader>();
  StatsLBReader::inputStatsFile();
  StatsLBReader::loadPhaseChangedMap();
}

/*static*/ void StatsLBReader::destroy() {
  theObjGroup()->destroyCollective(StatsLBReader::proxy_);
}

/*static*/ void StatsLBReader::clearStats() {
  StatsLBReader::user_specified_map_.clear();
}

/*static*/ void StatsLBReader::inputStatsFile() {

  using ArgType = vt::arguments::ArgConfig;

  auto const node = theContext()->getNode();
  auto const base_file = std::string(ArgType::vt_lb_stats_file_in);
  auto const dir = std::string(ArgType::vt_lb_stats_dir_in);
  auto const file = fmt::format("{}.{}.out", base_file, node);
  auto const file_name = fmt::format("{}/{}", dir, file);

  vt_print(lb, "inputStatFile: file={}, iter={}\n", file_name, 0);

  std::FILE *pFile = std::fopen(file_name.c_str(), "r");
  if ((pFile == nullptr) or (errno == ENOENT)) {
    vtAssert(pFile, "File opening failed");
  }

  std::set<ElementIDType> buffer;

  // Load: Format of a line :size_t,ElementIDType,TimeType
  size_t c1;
  ElementIDType c2;
  TimeType c3;
  CommBytesType c4;
  using E = typename std::underlying_type<CommCategory>::type;
  E c5;
  char separator;
  fpos_t pos;
  bool finished = false;
  size_t c1PreviousValue = 0;
  while (!finished) {
    if (fscanf(pFile, "%zu %c %lli %c %lf", &c1, &separator, &c2,
            &separator, &c3) > 0) {
      fgetpos (pFile,&pos);
      fscanf (pFile, "%c", &separator);
      if (separator == ',') {
        // COM detected, read the end of line and do nothing else
        int res = fscanf (pFile, "%lf %c %hhi", &c4, &separator, &c5);
        vtAssertExpr(res == 3);
      } else {
        // Load detected, create the new element
        fsetpos (pFile,&pos);
        if (c1PreviousValue != c1) {
          c1PreviousValue = c1;
          StatsLBReader::user_specified_map_.push_back(buffer);
          buffer.clear();
        }
        buffer.insert(c2);
      }
    } else {
      finished = true;
    }
  }

  if (!buffer.empty()) {
    StatsLBReader::user_specified_map_.push_back(buffer);
  }

  std::fclose(pFile);
}

/*static*/ void StatsLBReader::loadPhaseChangedMap() {
  auto const num_iters = StatsLBReader::user_specified_map_.size() - 1;
  vt_print(lb, "StatsLBReader::loadPhaseChangedMap size : {}\n", num_iters);

  auto myNodeID = static_cast<ElementIDType>(theContext()->getNode());

  StatsLBReader::moveList.resize(num_iters + 1);
  StatsLBReader::phase_changed_map_.resize(num_iters);

  if (myNodeID == 0) {
    StatsLBReader::msgReceived.resize(num_iters, 0);
    StatsLBReader::totalMove.resize(num_iters);
  }

  for (size_t ii = 0; ii < num_iters; ++ii) {
    auto elms = StatsLBReader::user_specified_map_[ii];
    auto elmsNext = StatsLBReader::user_specified_map_[ii + 1];
    std::set<ElementIDType> diff;
    std::set_difference(elmsNext.begin(), elmsNext.end(), elms.begin(),
      elms.end(), std::inserter(diff, diff.begin()));
    const size_t qi = diff.size();
    const size_t pi = elms.size() - (elmsNext.size() - qi);
    StatsLBReader::moveList[ii].reserve(3 * (pi + qi) + 1);
    //--- Store the iteration number
    StatsLBReader::moveList[ii].push_back(static_cast<ElementIDType>(ii));
    for (auto iEle : diff) {
      StatsLBReader::moveList[ii].push_back(iEle);  //--- permID to receive
      StatsLBReader::moveList[ii].push_back(no_element_id); // node moving from
      StatsLBReader::moveList[ii].push_back(myNodeID); // node moving to
    }
    diff.clear();
    std::set_difference(elms.begin(), elms.end(), elmsNext.begin(),
      elmsNext.end(), std::inserter(diff, diff.begin()));
    for (auto iEle : diff) {
      StatsLBReader::moveList[ii].push_back(iEle);  //--- permID to send
      StatsLBReader::moveList[ii].push_back(myNodeID); // node migrating from
      StatsLBReader::moveList[ii].push_back(no_element_id); // node migrating to
    }
    //
    // Create a message storing the vector
    //
    auto msg = makeSharedMessage<lb::VecMsg>(StatsLBReader::moveList[ii]);
    StatsLBReader::proxy_[0].send<lb::VecMsg, &StatsLBReader::doSend>(msg);
  }

}

void StatsLBReader::doSend(lb::VecMsg *msg) {
  auto sendVec = msg->getTransfer();
  ElementIDType iter = sendVec[0];
  //
  // --- Combine the different pieces of information
  //
  StatsLBReader::msgReceived[iter] += 1;
  auto &migrate = StatsLBReader::totalMove[iter];
  for (size_t ii = 1; ii < sendVec.size(); ii += 3) {
    auto permID = sendVec[ii];
    auto nodeFrom = static_cast<NodeType>(sendVec[ii+1]);
    auto nodeTo = static_cast<NodeType>(sendVec[ii+2]);
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
  auto numNodes = theContext()->getNumNodes();
  if (StatsLBReader::msgReceived[iter] < numNodes)
    return;
  //
  //--- Distribute the information when everything has been received
  //
  for (NodeType in = 0; in < numNodes; ++in) {
    size_t iCount = 0;
    for (auto iNode : migrate) {
      if (iNode.second.first == in)
        iCount += 1;
    }
    std::vector<ElementIDType> toMove(2 * iCount + 2);
    iCount = 0;
    toMove[iCount++] = iter;
    toMove[iCount++] = static_cast<ElementIDType>(migrate.size());
    for (auto iNode : migrate) {
      if (iNode.second.first == in) {
        toMove[iCount] = iNode.first;
        toMove[iCount + 1] = static_cast<ElementIDType>(iNode.second.second);
        iCount += 2;
      }
    }
    if (in > 0) {
      auto msg2 = makeSharedMessage<lb::VecMsg>(toMove);
      StatsLBReader::proxy_[in].send<lb::VecMsg,&StatsLBReader::scatterSend>
        (msg2);
    } else {
      StatsLBReader::moveList[iter].resize(toMove.size() - 1);
      std::copy(&toMove[1], &toMove[0] + toMove.size(),
                StatsLBReader::moveList[iter].begin());
    }
  }
  migrate.clear();
}

void StatsLBReader::scatterSend(lb::VecMsg *msg) {
  auto recvVec = msg->getTransfer();
  ElementIDType iter = recvVec[0];
  StatsLBReader::phase_changed_map_[iter] = (recvVec[1] > 0);
  auto &myList = StatsLBReader::moveList[iter];
  if (recvVec.size() <= 2) {
    myList.clear();
    return;
  }
  //
  myList.resize(recvVec.size() - 1);
  std::copy(&recvVec[1], &recvVec[0]+recvVec.size(), myList.begin());
}

}}}} /* end namespace vt::vrt::collection::balance */
