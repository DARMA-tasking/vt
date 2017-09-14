
#if ! defined __RUNTIME_TRANSPORT_LOCATION_IMPL__
#define __RUNTIME_TRANSPORT_LOCATION_IMPL__

#include "configs/types/types_common.h"
#include "context.h"
#include "active.h"
#include "location_common.h"
#include "location.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace vt { namespace location {

template <typename EntityID>
void EntityLocationCoord<EntityID>::registerEntity(EntityID const& id) {
  auto rec_iter = recs_.find(id);

  assert(
    rec_iter == recs_.end() and "EntityLocationCoord entity should not exist"
  );

  auto const& this_node = theContext->getNode();

  recs_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(LocRecType{eLocState::Local, this_node})
  );
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::entityMigrated(
  EntityID const& id, NodeType const& new_node
) {
  auto rec_iter = recs_.find(id);
  if (rec_iter == recs_.end()) {
    recs_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(id),
      std::forward_as_tuple(LocRecType{eLocState::Remote, new_node})
    );
  } else {
    rec_iter->second.updateNode(new_node);
  }
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::getLocation(
  EntityID const& id, NodeType const& home_node, NodeActionType const& action
) {
  auto const& this_node = theContext->getNode();

  auto rec_iter = recs_.find(id);

  if (rec_iter == recs_.end()) {
    if (home_node != this_node) {
      auto const& event_id = fst_location_event_id++;
      auto msg = new LocMsgType(
        LocManInstType::VirtualLocManInst, id, event_id, this_node, home_node
      );
      theMsg->sendMsg<LocMsgType, getLocationHandler>(
        home_node, msg, [=]{ delete msg; }
      );
      pending_actions_[event_id] = action;
    } else {
      // this is the home node and there is no record on this entity
    }
  } else if (rec_iter->second.isLocal()) {
    action(this_node);
  } else if (rec_iter->second.isRemote()) {
    action(rec_iter->second.getRemoteNode());
  }
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::updatePendingRequest(
  LocEventID const& event_id, NodeType const& node
) {
  auto pending_iter = pending_actions_.find(event_id);

  assert(
    pending_iter != pending_actions_.end() and "Event must exist in pending"
  );

  pending_iter->second(node);

  pending_actions_.erase(pending_iter);
}

template <typename EntityID>
/*static*/ void EntityLocationCoord<EntityID>::getLocationHandler(LocMsgType* msg) {
  auto const& event_id = msg->event_id;
  auto const& inst = msg->loc_man_inst;
  auto const& entity = msg->entity;
  auto const& home_node = msg->home_node;
  auto const& ask_node = msg->ask_node;

  if (inst == LocManInstType::VirtualLocManInst) {
    auto const& loc = theLocMan->virtual_loc;
    loc.getLocation(entity, home_node, [](NodeType node) {
      auto msg = new LocMsgType(
        LocManInstType::VirtualLocManInst, entity, event_id, ask_node, home_node
      );
      msg->setResolved_node(node);
      theMsg->sendMsg<LocMsgType, updateLocation>(
        ask_node, msg, [=]{ delete msg; }
      );
    });
  }
}

template <typename EntityID>
/*static*/ void EntityLocationCoord<EntityID>::updateLocation(LocMsgType* msg) {
  auto const& event_id = msg->event_id;
  auto const& inst = msg->loc_man_inst;
  auto const& entity = msg->entity;

  if (inst == LocManInstType::VirtualLocManInst) {
    auto const& loc = theLocMan->virtual_loc;
    loc.updatePendingRequest(event_id, msg->resolved_node);
  }
}

}} // end namespace vt::location

#endif /*__RUNTIME_TRANSPORT_LOCATION_IMPL__*/
