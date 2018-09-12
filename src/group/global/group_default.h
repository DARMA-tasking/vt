
#if !defined INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_H
#define INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_H

#include "config.h"
#include "group/group_common.h"
#include "group/global/group_default_msg.h"
#include "group/group_manager.fwd.h"
#include "activefn/activefn.h"
#include "messaging/message.h"
#include "messaging/message/smart_ptr.h"
#include "collective/tree/tree.h"

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
    MsgSizeType const& size, bool const is_root, ActionType action
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
  CountType sync_count_[num_phases] = { 0 };
  NodeType this_node_ = uninitialized_destination;
};

extern std::unique_ptr<DefaultGroup> default_group_;

}}} /* end namespace vt::group::global */

#include "group/global/group_default.impl.h"

#endif /*INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_H*/
