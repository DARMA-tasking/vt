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
#include "vt/vrt/collection/balance/lb_invoke/invoke.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/manager.h"
#include "vt/timing/timing.h"
#include "vt/configs/arguments/args.h"
#include "vt/runtime/runtime.h"

#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "fmt/format.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/*static*/
std::deque<std::map<ElementIDType,TimeType>>
    StatsLBReader::user_specified_map_changed_ = {};

/*static*/
std::deque<std::set<ElementIDType>> StatsLBReader::user_specified_map_ = {};

/*static*/
std::deque< std::vector<ElementIDType> > StatsLBReader::mmList = {};

/*static*/
std::vector<size_t> StatsLBReader::mmReceived = {};

/*static*/
std::vector<bool> StatsLBReader::mmBool = {};

/*static*/
std::deque<std::map<ElementIDType, std::pair<NodeType, NodeType>>>
  StatsLBReader::mmTotal = {};

/*static*/
StatsLBReader::VectorDiffPhase StatsLBReader::phase_changed_map_ = {};

/*static*/ FILE* StatsLBReader::stats_file_ = nullptr;

/*static*/ bool StatsLBReader::created_dir_ = false;

/*static*/ objgroup::proxy::Proxy<StatsLBReader> StatsLBReader::proxy_ = {};

/*static*/ std::vector<ElementIDType> StatsLBReader::migrateList = {};
/*static*/ size_t StatsLBReader::numReceived = 0;

/*static*/ void StatsLBReader::init() {
  // Create the new class dedicated to the input reader
  StatsLBReader::inputStatsFile();
  StatsLBReader::proxy_ = theObjGroup()->makeCollective<StatsLBReader>();
  StatsLBReader::loadPhaseChangedMap();
//  StatsLBReader::proxy_.get()->doReduce();
}

/*static*/ void StatsLBReader::destroy() {
  theObjGroup()->destroyCollective(StatsLBReader::proxy_);
}

/*static*/ void StatsLBReader::clearStats() {
  StatsLBReader::user_specified_map_changed_.clear();
  StatsLBReader::user_specified_map_.clear();
}

/*static*/ void StatsLBReader::closeStatsFile() {
  if (stats_file_) {
    fclose(stats_file_);
    stats_file_  = nullptr;
  }
}

/*static*/ void StatsLBReader::inputStatsFile() {
  using ArgType = vt::arguments::ArgConfig;

  // todo if File exist
  auto const node = theContext()->getNode();
  auto const base_file = std::string(ArgType::vt_lb_stats_file_in);
  auto const dir = std::string(ArgType::vt_lb_stats_dir_in);
  auto const file = fmt::format("{}.{}.out", base_file, node);
  auto const file_name = fmt::format("{}/{}", dir, file);

  vt_print(lb, "inputStatFile: file={}, iter={}\n", file_name, 0);

  std::FILE *pFile = std::fopen(file_name.c_str(), "r");
  if (pFile == nullptr) {
    vtAssert(pFile, "File opening failed");
  }
  else if (errno == ENOENT) {
    vtAssert(errno != ENOENT, "File does not exist");
  }

  auto elements = std::map<ElementIDType,TimeType> ();
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
          StatsLBReader::user_specified_map_changed_.push_back(elements);
          elements.clear();
          StatsLBReader::user_specified_map_.push_back(buffer);
          buffer.clear();
        }
        elements.emplace (c2, c3);
        buffer.insert(c2);
      }
    } else {
      finished = true;
    }
  }

  if (!buffer.empty()) {
    StatsLBReader::user_specified_map_changed_.push_back(elements);
    StatsLBReader::user_specified_map_.push_back(buffer);
  }

  std::fclose(pFile);
}

/*static*/ void StatsLBReader::loadPhaseChangedMap() {
//  auto const num_iters = StatsLBReader::user_specified_map_changed_.size() - 1;
  auto const num_iters = StatsLBReader::user_specified_map_.size() - 1;
  vt_print(lb, "StatsLBReader::loadPhaseChangedMap size : {}\n", num_iters);

  StatsLBReader::phase_changed_map_.vec_.assign(num_iters, false);
  StatsLBReader::mmBool.assign(num_iters, false);

  auto myNodeID = static_cast<ElementIDType>(theContext()->getNode());

  StatsLBReader::mmList.resize(num_iters+1);

  if (myNodeID == 0) {
    StatsLBReader::mmReceived.resize(num_iters, 0);
    StatsLBReader::mmTotal.resize(num_iters);
  }

  for (size_t ii = 0; ii < num_iters; ++ii) {
    auto elms = StatsLBReader::user_specified_map_[ii];
    auto elmsNext = StatsLBReader::user_specified_map_[ii + 1];
    std::set<ElementIDType> diff;
    std::set_difference(elmsNext.begin(), elmsNext.end(), elms.begin(), elms.end(),
                        std::inserter(diff, diff.begin()));
    size_t qi = diff.size();
    size_t pi = elms.size() - (elmsNext.size() - qi);
    //--- Prepare a list to communicate
    auto &newMigrateList = StatsLBReader::mmList[ii];
    newMigrateList.reserve(3 * (pi + qi) + 1);
    //--- Store the iteration number
    newMigrateList.push_back(static_cast<ElementIDType>(ii));
    for (auto iEle : diff) {
      newMigrateList.push_back(iEle);  //--- permID to receive
      newMigrateList.push_back(no_element_id); // node migrating from
      newMigrateList.push_back(myNodeID); // node migrating to
    }
    diff.clear();
    std::set_difference(elms.begin(), elms.end(), elmsNext.begin(), elmsNext.end(),
                        std::inserter(diff, diff.begin()));
    for (auto iEle : diff) {
      newMigrateList.push_back(iEle);  //--- permID to send
      newMigrateList.push_back(myNodeID); // node migrating from
      newMigrateList.push_back(no_element_id); // node migrating to
    }
    //
    // Create a message storing the vector
    //
    auto msg = makeSharedMessage<lb::TransferMsg<std::vector<ElementIDType> > >(newMigrateList);
    StatsLBReader::proxy_[0].send<lb::TransferMsg<std::vector<ElementIDType> >, &StatsLBReader::doSend>(msg);
  }

  size_t maxMove = 0;
  for (auto jList : StatsLBReader::user_specified_map_changed_) {
      maxMove = std::max<size_t>(maxMove, jList.size());
  }

  StatsLBReader::numReceived = 1;
  StatsLBReader::migrateList.reserve(4 * num_iters * maxMove);

  for (size_t i = 0; i < num_iters; i++) {
    auto &unordered_elms = StatsLBReader::user_specified_map_changed_.at(i);
    auto elms = std::map<ElementIDType,TimeType>(unordered_elms.begin(),
                                                 unordered_elms.end());
    unordered_elms = StatsLBReader::user_specified_map_changed_.at(i + 1);
    auto elmsNext = std::map<ElementIDType,TimeType>(unordered_elms.begin(),
      unordered_elms.end());
    std::map<ElementIDType,TimeType> diff;
    //
    std::set_difference(elmsNext.begin(), elmsNext.end(), elms.begin(), elms.end(),
            std::inserter(diff, diff.begin()), [](std::pair<ElementIDType, TimeType> a,
                    std::pair<ElementIDType, TimeType> b) {
                return a.first < b.first;
            });
    for (auto iEle : diff) {
      StatsLBReader::migrateList.push_back(i+1);  // iteration
      StatsLBReader::migrateList.push_back(iEle.first);  // perm ID to migrate
      StatsLBReader::migrateList.push_back(no_element_id); // node migrating from
      StatsLBReader::migrateList.push_back(myNodeID); // node migrating to
    }
    //
    diff.clear();
    //
    std::set_difference(elms.begin(), elms.end(), elmsNext.begin(), elmsNext.end(),
            std::inserter(diff, diff.begin()), [](std::pair<ElementIDType, TimeType> a,
                    std::pair<ElementIDType, TimeType> b) {
                return a.first < b.first;
            });
    for (auto iEle : diff) {
      StatsLBReader::migrateList.push_back(i+1); // iteration
      StatsLBReader::migrateList.push_back(iEle.first); // permID to migrate
      StatsLBReader::migrateList.push_back(myNodeID); // node migrating from
      StatsLBReader::migrateList.push_back(no_element_id); // node migrating to
    }
    //
//    StatsLBReader::phase_changed_map_.vec_[i] =
//      !( (elmsNext.size() == elms.size()) and
//      std::equal(elms.begin(), elms.end(), elmsNext.begin(),
//        [](auto a, auto b) { return (a.first == b.first); }) );
  }

   /*
  //
  // Create a message storing the vector
  //
  if (myNodeID != 0) {
      auto msg = makeSharedMessage< lb::TransferMsg< std::vector<ElementIDType> > >(StatsLBReader::migrateList);
      StatsLBReader::proxy_[0].send< lb::TransferMsg< std::vector<ElementIDType> >, &StatsLBReader::doSend2>(msg);
  }
*/

}

void StatsLBReader::doSend(lb::TransferMsg<std::vector<ElementIDType>> *msg) {
  auto sendVec = msg->getTransfer();
  const ElementIDType iter = sendVec[0];
  //
  StatsLBReader::mmReceived[iter] += 1;
  auto &migrate = StatsLBReader::mmTotal[iter];
  for (size_t ii = 1; ii < sendVec.size(); ii += 3) {
    const auto permID = sendVec[ii];
    const auto nodeFrom = static_cast<NodeType>(sendVec[ii+1]);
    const auto nodeTo = static_cast<NodeType>(sendVec[ii+2]);
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
  // --- Check whether all the messages have been received
  const auto numNodes = theContext()->getNumNodes();
  if (StatsLBReader::mmReceived[iter] < numNodes)
    return;
  //--- Distribute the information when everything has been received
  using VecMsg = lb::TransferMsg<std::vector<ElementIDType>>;
  for (NodeType in = 0; in < numNodes; ++in) {
    size_t iCount = 0;
    for (auto iNode : migrate) {
      if (iNode.second.first == in)
        iCount += 1;
    }
    const size_t header = 2;
    std::vector<ElementIDType> toMove(2 * iCount + header);
    iCount = 0;
    //--- Header values = iteration index, total number of migrations
    toMove[iCount++] = iter;
    toMove[iCount++] = migrate.size();
    //--- Copy information about migrations
    for (auto iNode : migrate) {
      if (iNode.second.first == in) {
        toMove[iCount++] = iNode.first;
        toMove[iCount++] = static_cast<ElementIDType>(iNode.second.second);
      }
    }
    if (in > 0) {
      auto msg2 = makeSharedMessage<VecMsg>(toMove);
      StatsLBReader::proxy_[in].send<VecMsg, &StatsLBReader::scatterSend>(msg2);
    } else {
      StatsLBReader::mmBool[iter] = (migrate.size() > 0);
      StatsLBReader::mmList[iter].resize(toMove.size() - header);
      std::copy(&toMove[header], &toMove[0] + toMove.size(),
                StatsLBReader::mmList[iter].begin());
    }
  }
  migrate.clear();
}

void StatsLBReader::scatterSend(
  lb::TransferMsg< std::vector<ElementIDType> > *msg
) {
  const size_t header = 2;
  auto recvVec = msg->getTransfer();
  ElementIDType iter = recvVec[0];
//  phase_changed_map_.vec_[iter] = (recvVec[1] > 0);
  StatsLBReader::mmBool[iter] = (recvVec[1] > 0);
  auto &myList = StatsLBReader::mmList[iter];
  if (recvVec.size() <= header) {
    myList.clear();
    return;
  }
  //
  myList.resize(recvVec.size() - header);
  std::copy(&recvVec[header], &recvVec[0]+recvVec.size(), myList.begin());
//  //////////
//  for (size_t in = 0; in < myList.size(); in += 2) {
//    std::cout << " scatterSend ** NODE " << theContext()->getNode() << " >> " << iter << " "
//              << myList[in] << " " << myList[in+1] << "\n";
//  }
//  //////////
}

void StatsLBReader::doSend2( lb::TransferMsg< std::vector<ElementIDType> > *msg) {
    StatsLBReader::numReceived += 1;
    auto sendVec = msg->getTransfer();
    std::deque<ElementIDType> toInsert;
    for (size_t ii = 0; ii < sendVec.size(); ii += 4) {
        auto iter = sendVec[ii];
        auto permID = sendVec[ii + 1];
        bool isFound = false;
        for (size_t jj = 0; jj < StatsLBReader::migrateList.size(); jj += 4) {
            if ((StatsLBReader::migrateList[jj] == iter) && (StatsLBReader::migrateList[jj+1] == permID)) {
                if (StatsLBReader::migrateList[jj+2] == 0)
                    StatsLBReader::migrateList[jj+2] = sendVec[ii+2];
                if (StatsLBReader::migrateList[jj+3] == 0)
                    StatsLBReader::migrateList[jj+3] = sendVec[ii+3];
                isFound = true;
                break;
            }
        }
        if (!isFound) {
            toInsert.push_back(iter);
            toInsert.push_back(permID);
            toInsert.push_back(sendVec[ii + 2]);
            toInsert.push_back(sendVec[ii + 3]);
        }
    }
    StatsLBReader::migrateList.insert(StatsLBReader::migrateList.end(), toInsert.begin(), toInsert.end());
    auto numNodes = theContext()->getNumNodes();
    if (StatsLBReader::numReceived == numNodes) {
        std::cout << "\n !!! AAAAA !!! \n\n";
        for (size_t jj = 0; jj < StatsLBReader::migrateList.size(); jj += 4) {
            std::cout << StatsLBReader::migrateList[jj] << " " << StatsLBReader::migrateList[jj+1]
            << " " << StatsLBReader::migrateList[jj+2] << " " << StatsLBReader::migrateList[jj+3] << "\n";
        }
        for (NodeType in = 1; in < numNodes; ++in) {
            auto msg2 = makeSharedMessage< lb::TransferMsg< std::vector<ElementIDType> > >(StatsLBReader::migrateList);
            StatsLBReader::proxy_[in].send< lb::TransferMsg< std::vector<ElementIDType> >, &StatsLBReader::scatterSend2>(msg2);
        }
    }
}

void StatsLBReader::scatterSend2( lb::TransferMsg< std::vector<ElementIDType> > *msg) {
    auto nodeID = theContext()->getNode();
    StatsLBReader::migrateList.clear();
    auto sendVec = msg->getTransfer();
    for (size_t in = 0; in < sendVec.size(); in += 4) {
        if (sendVec[in+2] == nodeID) {
            StatsLBReader::migrateList.push_back(sendVec[in]);
            StatsLBReader::migrateList.push_back(sendVec[in+1]);
            StatsLBReader::migrateList.push_back(sendVec[in+2]);
            StatsLBReader::migrateList.push_back(sendVec[in+3]);
        }
    }
    std::cout << " scatterSend2 >> node " << nodeID << " vec length "
              << msg->getTransfer().size()
              << " migrateList size " << StatsLBReader::migrateList.size() << std::endl;
    for (size_t in = 0; in < StatsLBReader::migrateList.size(); in += 4) {
        std::cout << " NODE " << nodeID << " >> " << StatsLBReader::migrateList[in] << " "
        << StatsLBReader::migrateList[in+1] << " "
                << StatsLBReader::migrateList[in+2] << " "
                << StatsLBReader::migrateList[in+3] << "\n";
    }
}

void StatsLBReader::doneReduce(VecPhaseMsg *msg) {
  vt_print(lb, "StatsLBReader::doneReduce with msg of size {}\n",
    msg->getConstVal().vec_.size());
}

void StatsLBReader::doReduce() {
  vt_print(lb, "StatsLBReader::doReduce {} \n",
  StatsLBReader::phase_changed_map_.vec_.size());

  auto cb = theCB()->makeBcast<StatsLBReader,VecPhaseMsg,&StatsLBReader::doneReduce>
    (StatsLBReader::proxy_);
  auto msg = makeMessage<VecPhaseMsg>(StatsLBReader::phase_changed_map_);
  StatsLBReader::proxy_.reduce<collective::OrOp<VectorDiffPhase>>(msg.get(),cb);
}

}}}} /* end namespace vt::vrt::collection::balance */
