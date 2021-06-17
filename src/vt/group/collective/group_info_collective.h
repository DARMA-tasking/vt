/*
//@HEADER
// *****************************************************************************
//
//                           group_info_collective.h
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

#if !defined INCLUDED_GROUP_GROUP_INFO_COLLECTIVE_H
#define INCLUDED_GROUP_GROUP_INFO_COLLECTIVE_H

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/base/group_info_base.h"
#include "vt/group/collective/group_collective.h"
#include "vt/group/collective/group_collective_msg.h"
#include "vt/group/collective/group_collective_reduce_msg.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/messaging/message.h"

#include <memory>
#include <list>

#include <mpi.h>

namespace vt { namespace group {

struct InfoColl : virtual InfoBase {
  using GroupCollectiveType    = GroupCollective;
  using GroupCollectivePtrType = std::unique_ptr<GroupCollective>;
  using ReduceType             = collective::reduce::Reduce;
  using ReducePtrType          = ReduceType*;
  using GroupCollMsgPtrType    = MsgSharedPtr<GroupCollectiveMsg>;

  explicit InfoColl(bool const in_is_in_group, MPI_Comm default_comm, bool make_mpi_group)
    : is_in_group(in_is_in_group),
      default_comm_(default_comm),
      mpi_group_comm_(default_comm),
      make_mpi_group_(make_mpi_group)
  {
  }

private:
  /*
   *  Inner struct used as functor reduce target after the collective group is
   *  fully created a operational: this will be triggered.
   */
  struct CollSetupFinished {
    void operator()(FinishedReduceMsg* msg);
  };

public:
  ReducePtrType getReduce() const;
  NodeType getRoot() const;
  bool isGroupDefault() const;
  TreeType* getTree() const;
  bool inGroup() const;
  bool isReady() const;
  void readyAction(ActionType const action);
  MPI_Comm getComm() const;
  void freeComm();

protected:
  void setupCollective();
  void setupCollectiveSingular();

  static void upHan(GroupCollectiveMsg* msg);
  static void downHan(GroupCollectiveMsg* msg);
  static void newRootHan(GroupCollectiveMsg* msg);
  static void downFinishedHan(GroupOnlyMsg* msg);
  static void finalizeHan(GroupOnlyMsg* msg);
  static void newTreeHan(GroupOnlyMsg* msg);
  static void tree(GroupOnlyMsg* msg);

private:
  void upTree();
  void atRoot();
  void downTree(GroupCollectiveMsg* msg);
  void collectiveFn(MsgSharedPtr<GroupCollectiveMsg> msg);
  void newRoot(GroupCollectiveMsg* msg);
  void downTreeFinished(GroupOnlyMsg* msg);
  void finalizeTree(GroupOnlyMsg* msg);
  void finalize();
  void sendDownNewTree();
  void newTree(NodeType const& parent);
  RemoteOperationIDType makeCollectiveContinuation(GroupType const group_);

protected:
  bool is_in_group                       = false;
  bool finished_init_                    = false;
  bool in_phase_two_                     = false;
  GroupCollectivePtrType collective_     = nullptr;
  WaitCountType coll_wait_count_         = 0;
  std::vector<GroupCollMsgPtrType> msgs_ = {};
  uint32_t arrived_count_                = 0;
  uint32_t extra_count_                  = 0;
  uint32_t extra_arrived_count_          = 0;
  uint32_t send_down_                    = 0;
  uint32_t send_down_finished_           = 0;
  NodeType known_root_node_              = uninitialized_destination;
  bool is_new_root_                      = false;
  bool has_root_                         = false;
  bool is_default_group_                 = false;
  std::size_t subtree_                   = 0;

private:
  RemoteOperationIDType down_tree_cont_     = no_op_id;
  RemoteOperationIDType down_tree_fin_cont_ = no_op_id;
  RemoteOperationIDType up_tree_cont_       = no_op_id;
  RemoteOperationIDType finalize_cont_      = no_op_id;
  RemoteOperationIDType new_tree_cont_      = no_op_id;
  RemoteOperationIDType new_root_cont_      = no_op_id;

private:
  std::list<ActionType> pending_ready_actions_ = {};

private:
  //< Default, NON-OWNED, communicator.
  MPI_Comm default_comm_   = MPI_COMM_NULL;
  MPI_Comm mpi_group_comm_ = MPI_COMM_NULL;
  bool make_mpi_group_     = false;
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_INFO_COLLECTIVE_H*/
