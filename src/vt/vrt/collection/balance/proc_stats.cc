/*
//@HEADER
// *****************************************************************************
//
//                                proc_stats.cc
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
#include "vt/vrt/collection/balance/proc_stats.h"
#include "vt/vrt/collection/balance/proc_stats.util.h"
#include "vt/vrt/collection/manager.h"
#include "vt/timing/timing.h"
#include "vt/configs/arguments/args.h"
#include "vt/runtime/runtime.h"

#include <vector>
#include <unordered_map>
#include <cstdio>
#include <sys/stat.h>

#include "fmt/format.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/*static*/
std::vector<std::unordered_map<ElementIDType,TimeType>>
  ProcStats::proc_data_ = {};

/*static*/ std::vector<CommMapType> ProcStats::proc_comm_ = {};

/*static*/
std::unordered_map<ElementIDType,ProcStats::MigrateFnType>
  ProcStats::proc_migrate_ = {};

/*static*/ std::unordered_map<ElementIDType,ElementIDType>
  ProcStats::proc_temp_to_perm_ =  {};

/*static*/ std::unordered_map<ElementIDType,ElementIDType>
  ProcStats::proc_perm_to_temp_ =  {};

/*static*/ ElementIDType ProcStats::next_elm_ = 1;

/*static*/ FILE* ProcStats::stats_file_ = nullptr;

/*static*/ bool ProcStats::created_dir_ = false;

/*static*/ std::deque< std::vector<ElementIDType> >
  ProcStats::proc_move_list_ = {};

/*static*/ std::vector< bool > ProcStats::proc_phase_runs_LB_ = {};

/*static*/ StatsRestartReader *ProcStats::proc_reader_ = nullptr;

StatsRestartReader::~StatsRestartReader() {
  if (proxy.getProxy() != no_obj_group) {
    theObjGroup()->destroyCollective(proxy);
  }
}

/*static*/
void StatsRestartReader::readStats(const std::string &fileName) {

  // Read the input files
  std::deque< std::set<ElementIDType> > elements_history;
  inputStatsFile(fileName, elements_history);
  if (elements_history.empty()) {
    vtWarn("No element history provided");
    return;
  }

  ProcStats::proc_reader_->proxy =
    theObjGroup()->makeCollective<StatsRestartReader>();

  const auto num_iters = elements_history.size() - 1;
  ProcStats::proc_move_list_.resize(num_iters);
  ProcStats::proc_phase_runs_LB_.resize(num_iters, true);
  if (theContext()->getNode() == 0) {
    ProcStats::proc_reader_->msgsReceived.resize(num_iters, 0);
    ProcStats::proc_reader_->totalMove.resize(num_iters);
  }

  // Communicate the migration information
  createMigrationInfo(elements_history);
}

/*static*/
void StatsRestartReader::inputStatsFile(
  const std::string &fileName,
  std::deque< std::set<ElementIDType> > &element_history
)
{
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
    if (fscanf(pFile, "%zu %c %llu %c %lf", &phaseID, &separator, &elmID,
               &separator, &tval) > 0) {
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

/*static*/
void StatsRestartReader::createMigrationInfo(
  std::deque< std::set<ElementIDType> > &element_history
)
{
  const auto num_iters = element_history.size() - 1;
  const auto myNodeID = static_cast<ElementIDType>(theContext()->getNode());
  auto myProxy = ProcStats::proc_reader_->proxy;

  for (size_t ii = 0; ii < num_iters; ++ii) {
    auto &elms = element_history[ii];
    auto &elmsNext = element_history[ii + 1];
    std::set<ElementIDType> diff;
    std::set_difference(elmsNext.begin(), elmsNext.end(), elms.begin(),
                        elms.end(), std::inserter(diff, diff.begin()));
    const size_t qi = diff.size();
    const size_t pi = elms.size() - (elmsNext.size() - qi);
    auto &myList = ProcStats::proc_move_list_[ii];
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
    auto msg = makeSharedMessage<VecMsg>(myList);
    myProxy[0].send<VecMsg, &StatsRestartReader::gatherMsgs>(msg);
    //
    // Clear old distribution of elements
    //
    elms.clear();
  }

}

void StatsRestartReader::gatherMsgs(VecMsg *msg) {
  auto sentVec = msg->getTransfer();
  vtAssert(sentVec.size() % 3 == 1, "Expecting vector of length 3n+1");
  const ElementIDType phaseID = sentVec[0];
  //
  // --- Combine the different pieces of information
  //
  ProcStats::proc_reader_->msgsReceived[phaseID] += 1;
  auto &migrate = ProcStats::proc_reader_->totalMove[phaseID];
  for (size_t ii = 1; ii < sentVec.size(); ii += 3) {
    const auto permID = sentVec[ii];
    const auto nodeFrom = static_cast<NodeType>(sentVec[ii + 1]);
    const auto nodeTo = static_cast<NodeType>(sentVec[ii + 2]);
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
  if (ProcStats::proc_reader_->msgsReceived[phaseID] < static_cast<std::size_t>(numNodes))
    return;
  //
  //--- Distribute the information when everything has been received
  //
  auto myProxy = ProcStats::proc_reader_->proxy;
  const size_t header = 2;
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
      auto msg2 = makeSharedMessage<VecMsg>(toMove);
      myProxy[in].send<VecMsg, &StatsRestartReader::scatterMsgs>(msg2);
    } else {
      ProcStats::proc_phase_runs_LB_[phaseID] = (!migrate.empty());
      auto &myList = ProcStats::proc_move_list_[phaseID];
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
  ProcStats::proc_phase_runs_LB_[phaseID] = static_cast<bool>(recvVec[1] > 0);
  auto &myList = ProcStats::proc_move_list_[phaseID];
  if (!ProcStats::proc_phase_runs_LB_[phaseID]) {
    myList.clear();
    return;
  }
  //--- Copy the migration information
  myList.resize(recvVec.size() - header);
  std::copy(&recvVec[header], &recvVec[0]+recvVec.size(), myList.begin());
}

/*static*/ void ProcStats::readRestartInfo(const std::string &fileName) {
  if (ProcStats::proc_reader_ == nullptr) {
    ProcStats::proc_reader_ = new StatsRestartReader;
  }
  ProcStats::proc_reader_->readStats(fileName);
}

/*static*/ void ProcStats::clearStats() {
  ProcStats::proc_comm_.clear();
  ProcStats::proc_data_.clear();
  ProcStats::proc_migrate_.clear();
  ProcStats::proc_temp_to_perm_.clear();
  ProcStats::proc_perm_to_temp_.clear();
  next_elm_ = 1;
  ProcStats::proc_move_list_.clear();
  ProcStats::proc_phase_runs_LB_.clear();
  delete ProcStats::proc_reader_;
}

/*static*/ void ProcStats::startIterCleanup() {
  // Convert the temp ID proc_data_ for the last iteration into perm ID for
  // stats output
  auto const phase = proc_data_.size() - 1;
  auto const prev_data = std::move(proc_data_[phase]);
  std::unordered_map<ElementIDType,TimeType> new_data;
  for (auto& elm : prev_data) {
    auto iter = proc_temp_to_perm_.find(elm.first);
    vtAssert(iter != proc_temp_to_perm_.end(), "Temp ID must exist");
    auto perm_id = iter->second;
    new_data[perm_id] = elm.second;
  }
  proc_data_[phase] = std::move(new_data);

  // Create migrate lambdas and temp to perm map since LB is complete
  ProcStats::proc_migrate_.clear();
  ProcStats::proc_temp_to_perm_.clear();
  ProcStats::proc_perm_to_temp_.clear();
}

/*static*/ ElementIDType ProcStats::getNextElm() {
  auto const& this_node = theContext()->getNode();
  auto elm = next_elm_++;
  return (elm << 32) | this_node;
}

/*static*/ void ProcStats::releaseLB() {
  using MsgType = CollectionPhaseMsg;
  auto msg = makeMessage<MsgType>();
  theMsg()->broadcastMsg<MsgType,CollectionManager::releaseLBPhase>(msg.get());
  CollectionManager::releaseLBPhase(msg.get());
}

/*static*/ void ProcStats::createStatsFile() {
  using ArgType = vt::arguments::ArgConfig;
  auto const node = theContext()->getNode();
  auto const base_file = std::string(ArgType::vt_lb_stats_file);
  auto const dir = std::string(ArgType::vt_lb_stats_dir);
  auto const file = fmt::format("{}.{}.out", base_file, node);
  auto const file_name = fmt::format("{}/{}", dir, file);

  debug_print(
    lb, node,
    "ProcStats: createStatsFile file={}\n", file_name
  );

  // Node 0 creates the directory
  if (not created_dir_ and node == 0) {
    mkdir(dir.c_str(), S_IRWXU);
    created_dir_ = true;
  }

  // Barrier: wait for node 0 to create directory before trying to put a file in
  // the stats destination directory
  if (curRT) {
    curRT->systemSync();
  } else {
    // Something is wrong
    vtAssert(false, "Trying to dump stats when VT runtime is deallocated?");
  }

  stats_file_ = fopen(file_name.c_str(), "w+");
}

/*static*/ void ProcStats::closeStatsFile() {
  if (stats_file_) {
    fclose(stats_file_);
    stats_file_  = nullptr;
  }
}

/*static*/ void ProcStats::outputStatsFile() {
  if (stats_file_ == nullptr) {
    createStatsFile();
  }

  vtAssertExpr(stats_file_ != nullptr);

  auto const num_iters = ProcStats::proc_data_.size();

  vt_print(lb, "outputStatsFile: file={}, iter={}\n", print_ptr(stats_file_), num_iters);

  for (size_t i = 0; i < num_iters; i++) {
    for (auto&& elm : ProcStats::proc_data_.at(i)) {
      auto obj_str = fmt::format("{},{},{}\n", i, elm.first, elm.second);
      fprintf(stats_file_, "%s", obj_str.c_str());
    }
    for (auto&& elm : ProcStats::proc_comm_.at(i)) {
      using E = typename std::underlying_type<CommCategory>::type;

      auto const& key = elm.first;
      auto const& val = elm.second;
      auto const cat = static_cast<E>(key.cat_);

      if (
        key.cat_ == CommCategory::SendRecv or
        key.cat_ == CommCategory::Broadcast
      ) {
        auto const to   = key.toObj();
        auto const from = key.fromObj();
        auto obj_str = fmt::format("{},{},{},{},{}\n", i, to, from, val, cat);
        fprintf(stats_file_, "%s", obj_str.c_str());
      } else if (
        key.cat_ == CommCategory::NodeToCollection or
        key.cat_ == CommCategory::NodeToCollectionBcast
      ) {
        auto const to   = key.toObj();
        auto const from = key.fromNode();
        auto obj_str = fmt::format("{},{},{},{},{}\n", i, to, from, val, cat);
        fprintf(stats_file_, "%s", obj_str.c_str());
      } else if (
        key.cat_ == CommCategory::CollectionToNode or
        key.cat_ == CommCategory::CollectionToNodeBcast
      ) {
        auto const to   = key.toNode();
        auto const from = key.fromObj();
        auto obj_str = fmt::format("{},{},{},{},{}\n", i, to, from, val, cat);
        fprintf(stats_file_, "%s", obj_str.c_str());
      } else {
        vtAssert(false, "Invalid balance::CommCategory enum value");
      }
    }
  }
  fflush(stats_file_);

  closeStatsFile();
}

}}}} /* end namespace vt::vrt::collection::balance */
