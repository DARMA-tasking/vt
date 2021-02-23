/*
//@HEADER
// *****************************************************************************
//
//                               group_default.h
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

#if !defined INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_H
#define INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_H

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/global/group_default_msg.h"
#include "vt/group/group_manager.fwd.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/message.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/collective/tree/tree.h"

#include <memory>
#include <cstdlib>

namespace vt { namespace group { namespace global {

using PhaseType = int16_t;

static constexpr PhaseType const num_phases = 2;

struct DefaultGroup {
  using CountType = int32_t;
  using TreeType = collective::tree::Tree;
  using TreePtrType = std::unique_ptr<TreeType>;

  DefaultGroup() = default;

  friend struct ::vt::group::GroupManager;

  // Interface for collection communication within the default group
public:
  static EventType broadcast(
    MsgSharedPtr<BaseMsgType> const& base, NodeType const& from,
    MsgSizeType const& size, bool const is_root, bool* const deliver
  );

private:
  template <typename MsgT, ActiveTypedFnType<MsgT>* handler>
  static void sendPhaseMsg(PhaseType const& phase, NodeType const& node);

  // Setup for default group
  static void setupDefaultTree();
  static void syncHandler(GroupSyncMsg* msg);
  static void newPhaseHandler(GroupSyncMsg* msg);
  static void newPhaseSendChildren(PhaseType const& phase);
  static void sendUpTree(PhaseType const& phase);
  static void buildDefaultTree(PhaseType const& phase);
  static void localSync(PhaseType const& phase);
  static void newPhase(PhaseType const& phase);

private:
  TreePtrType spanning_tree_ = nullptr;
  bool finished_startup_ = false;
  PhaseType cur_phase_ = 0;
  CountType sync_count_[num_phases + 1] = { 0, 0, 0 };
  NodeType this_node_ = uninitialized_destination;
};

extern std::unique_ptr<DefaultGroup> default_group_;

}}} /* end namespace vt::group::global */

#include "vt/group/global/group_default.impl.h"

#endif /*INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_H*/
