
#include "config.h"
#include "lb/instrumentation/centralized/collect.h"
#include "lb/instrumentation/centralized/collect_msg.h"
#include "lb/lb_types.h"
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
  for (auto&& elm : msg2->entries_) {
    auto const& entity = elm.first;
    auto entry_iter = msg2->entries_.find(entity);
    assert(entry_iter == msg2->entries_.end());
    msg2->entries_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(entity),
      std::forward_as_tuple(elm.second)
    );
  }
}

/*static*/ void CentralCollect::collectFinished(
  LBPhaseType const& phase, CollectMsg::ContainerType const& entries
) {
  debug_print(
    lb, node,
    "collectFinished: phase=%llu, size=%ld\n", phase, entries.size()
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
  auto msg = makeSharedMessage<CollectMsg>(phase);
  auto const& entity_list = Entity::entities_;
  for (auto&& elm : entity_list) {
    auto const& entity = elm.first;
    auto const& db = elm.second;
    auto phase_iter = db.phase_timings_.find(phase);
    if (phase_iter != db.phase_timings_.end()) {
      auto msg_entry_iter = msg->entries_.find(entity);
      if (msg_entry_iter == msg->entries_.end()) {
        msg->entries_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(entity),
          std::forward_as_tuple(CollectMsg::EntryListType{})
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
