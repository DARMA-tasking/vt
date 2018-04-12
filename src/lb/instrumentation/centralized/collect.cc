
#include "config.h"
#include "lb/instrumentation/centralized/collect.h"
#include "lb/instrumentation/centralized/collect_msg.h"
#include "lb/lb_types.h"
#include "lb/lb_types_internal.h"
#include "lb/instrumentation/entity.h"
#include "lb/instrumentation/database.h"
#include "collective/collective_alg.h"

#include <unordered_map>
#include <vector>
#include <cassert>

namespace vt { namespace lb { namespace instrumentation {

/*static*/ LBPhaseType CentralCollect::cur_lb_phase_ = fst_phase;
/*static*/ NodeType CentralCollect::collect_root_ = 0;

/*static*/ void CentralCollect::combine(CollectMsg* msg1, CollectMsg* msg2) {
  assert(msg1->phase_ == msg2->phase_ && "Phases must be identical");

  // Runtime validity check to ensure that nodes are unique
  #if backend_check_enabled(runtime_checks) || 1
  for (auto&& elm1 : msg1->entries_) {
    for (auto&& elm2 : msg2->entries_) {
      assert(
        elm1.first != elm2.first &&
        "CollectMsg combine must have unique entries"
      );
    }
  }
  #endif

  msg1->entries_.insert(msg2->entries_.begin(), msg2->entries_.end());
}

/*static*/ void CentralCollect::collectFinished(
  LBPhaseType const& phase, ProcContainerType const& entries
) {
  debug_print(
    lb, node,
    "collectFinished: phase={}, size={}\n", phase, entries.size()
  );
}

/*static*/ void CentralCollect::centralizedCollect(CollectMsg* msg) {
  if (msg->is_root) {
    return collectFinished(msg->phase_, msg->entries_);
  } else {
    CollectMsg* fst_msg = msg;
    CollectMsg* cur_msg = msg->next ? static_cast<CollectMsg*>(msg->next) : nullptr;
    while (cur_msg != nullptr) {
      // Combine msgs
      CentralCollect::combine(fst_msg, cur_msg);
      cur_msg = cur_msg->next ? static_cast<CollectMsg*>(cur_msg->next) : nullptr;
    }
  }
}

/*static*/ void CentralCollect::reduceCurrentPhase() {
  auto const& phase = CentralCollect::currentPhase();
  CentralCollect::nextPhase();
  return startReduce(phase);
}

/*static*/ CollectMsg* CentralCollect::collectStats(LBPhaseType const& phase) {
  auto const& node = theContext()->getNode();
  auto msg = makeSharedMessage<CollectMsg>(phase);
  auto node_cont_iter = msg->entries_.find(node);
  assert(
    node_cont_iter == msg->entries_.end() &&
    "Entries must not exist for this node"
  );
  msg->entries_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(node),
    std::forward_as_tuple(ContainerType{})
  );
  node_cont_iter = msg->entries_.find(node);
  assert(
    node_cont_iter != msg->entries_.end() &&
    "Entries must exist here for this node"
  );
  auto const& entity_list = Entity::entities_;
  for (auto&& elm : entity_list) {
    auto const& entity = elm.first;
    auto const& db = elm.second;
    auto phase_iter = db.phase_timings_.find(phase);
    if (phase_iter != db.phase_timings_.end()) {
      auto msg_entry_iter = node_cont_iter->second.find(entity);
      if (msg_entry_iter == node_cont_iter->second.end()) {
        node_cont_iter->second.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(entity),
          std::forward_as_tuple(EntryListType{})
        );
      }
      msg_entry_iter->second = phase_iter->second;
    }
  }
  return msg;
}

/*static*/ void CentralCollect::startReduce(LBPhaseType const& phase) {
  auto const& root = CentralCollect::collect_root_;
  auto msg = CentralCollect::collectStats(phase);
  theCollective()->reduce<CollectMsg, CentralCollect::centralizedCollect>(
    root, msg
  );
}

/*static*/ LBPhaseType CentralCollect::currentPhase() {
  return CentralCollect::cur_lb_phase_;
}

/*static*/ void CentralCollect::nextPhase() {
  CentralCollect::cur_lb_phase_++;
}

}}} /* end namespace vt::lb::instrumentation */
