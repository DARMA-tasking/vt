
#include "config.h"
#include "group/group_common.h"
#include "group/global/group_default.h"
#include "messaging/active.h"
#include "messaging/message.h"
#include "collective/tree/tree.h"

#include <memory>
#include <cassert>

namespace vt { namespace group { namespace global {

/*static*/ void DefaultGroup::localSync(PhaseType const& phase) {
  DefaultGroup::buildDefaultTree(phase);
  default_group_->sync_count_[phase]++;
  DefaultGroup::sendUpTree(phase);
}

/*static*/ void DefaultGroup::setupDefaultTree() {
  PhaseType const& initial_phase = 0;
  DefaultGroup::localSync(initial_phase);

  // Wait for startup to complete all phases before initializing other
  // components, which may broadcast, requiring the spanning tree to be setup
  while (default_group_->cur_phase_ < num_phases) {
    theMsg()->scheduler();
  }
}

/*static*/ void DefaultGroup::newPhaseSendChildren(PhaseType const& phase) {
   // Initialize spanning tree for default group `default_group'
  assert(
    default_group_->spanning_tree_ != nullptr &&
    "Must have valid tree when entering new phase"
  );
  if (default_group_->spanning_tree_->getNumChildren() > 0) {
    default_group_->spanning_tree_->foreachChild([&](NodeType child) {
      DefaultGroup::sendPhaseMsg<GroupSyncMsg, newPhaseHandler>(phase, child);
    });
  }
  newPhase(phase);
}

/*static*/ void DefaultGroup::newPhaseHandler(GroupSyncMsg* msg) {
  auto const& phase = envelopeGetTag(msg->env);
  return newPhaseSendChildren(phase);
}

/*static*/ void DefaultGroup::syncHandler(GroupSyncMsg* msg) {
  auto const& phase = envelopeGetTag(msg->env);
  DefaultGroup::localSync(phase);
}

/*static*/ void DefaultGroup::buildDefaultTree(PhaseType const& phase) {
  // Initialize spanning tree for default group `default_group'
  if (default_group_->spanning_tree_ == nullptr) {
    default_group_->spanning_tree_ = std::make_unique<TreeType>(
      collective::tree::tree_cons_tag_t
    );
    default_group_->this_node_ = theContext()->getNode();
    assert(phase == 0 && "Must be first phase when initializing spanning tree");
  }
}

/*static*/ void DefaultGroup::newPhase(PhaseType const& phase) {
  assert(default_group_->cur_phase_ == phase && "Must be on current phase");

  PhaseType const& new_phase = phase + 1;
  default_group_->cur_phase_ = new_phase;
  localSync(new_phase);
}

/*static*/ void DefaultGroup::sendUpTree(PhaseType const& phase) {
  assert(
    default_group_->spanning_tree_ != nullptr && "Must have valid tree"
  );

  auto const& count = default_group_->spanning_tree_->getNumChildren() + 1;

  debug_print(
    group, node,
    "sendUpTree: count=%d, default_group_->sync_count_[%hu]=%d, is_root=%s, "
    "phase=%hu\n",
    count, phase, default_group_->sync_count_[phase],
    print_bool(default_group_->spanning_tree_->isRoot()), phase
  );

  if (default_group_->sync_count_[phase] == count) {
    if (!default_group_->spanning_tree_->isRoot()) {
      auto const& parent = default_group_->spanning_tree_->getParent();
      DefaultGroup::sendPhaseMsg<GroupSyncMsg, syncHandler>(phase, parent);
    } else {
      debug_print(
        group, node,
        "DefaultGroup::sendUpTree: root node: phase=%hu, num_phase=%hu\n",
        phase, num_phases
      );
      if (phase < num_phases) {
        newPhaseSendChildren(phase);
      }
    }
  }
}

/*static*/ EventType DefaultGroup::broadcast(
  BaseMessage* base, NodeType const& from, MsgSizeType const& size,
  bool const is_root, ActionType action
) {
  // By default use the default_group_->spanning_tree_
  auto const& msg = reinterpret_cast<ShortMessage* const>(base);
  // Destination is where the broadcast originated from
  auto const& dest = envelopeGetDest(msg->env);
  auto const& num_children = default_group_->spanning_tree_->getNumChildren();
  auto const& node = theContext()->getNode();
  NodeType const& root_node = 0;
  bool const& send_to_root = is_root && node != root_node;
  EventType event = no_event;

  debug_print(
    broadcast, node,
    "DefaultGroup::broadcast msg=%p, size=%d, from=%d, dest=%d, is_root=%s\n",
    base, size, from, dest, print_bool(is_root)
  );

  if (num_children > 0 || send_to_root) {
    auto const& is_shared = isSharedMessage(msg);
    bool const& has_action = action != nullptr;
    EventRecordType* parent = nullptr;
    auto const& send_tag = static_cast<MPI_TagType>(MPITag::ActiveMsgTag);

    if (has_action) {
      event = theEvent()->createParentEvent(node);
      auto& holder = theEvent()->getEventHolder(event);
      parent = holder.get_event();
    }

    if (is_shared) {
      messageRef(msg);
    }

    // Send to child nodes in the spanning tree
    if (num_children > 0) {
      default_group_->spanning_tree_->foreachChild([&](NodeType child) {
        bool const& send = child != dest;

        debug_print(
          broadcast, node,
          "DefaultGroup::broadcast *send* size=%d, from=%d, child=%d, send=%s, "
          "msg=%p\n",
          size, from, child, print_bool(send), msg
        );

        if (send) {
          auto const put_event = theMsg()->sendMsgBytesWithPut(
            child, base, size, send_tag, action
          );
          if (has_action) {
            parent->addEventToList(put_event);
          }
        }
      });
    }

    // If not the root of the spanning tree, send to the root to propagate to
    // the rest of the tree
    if (send_to_root) {
      debug_print(
        broadcast, node,
        "DefaultGroup::broadcast *send* is_root=%s, root_node=%d, dest=%d, "
        "msg=%p\n",
        print_bool(is_root), root_node, dest, msg
      );

      auto const put_event = theMsg()->sendMsgBytesWithPut(
        root_node, base, size, send_tag, action
      );
      if (has_action) {
        parent->addEventToList(put_event);
      }
    }

    if (is_shared) {
      messageDeref(msg);
    }
  }

  return event;
}

std::unique_ptr<DefaultGroup> default_group_ = std::make_unique<DefaultGroup>();

}}} /* end namespace vt::group::global */
