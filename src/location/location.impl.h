
#if ! defined __RUNTIME_TRANSPORT_LOCATION_IMPL__
#define __RUNTIME_TRANSPORT_LOCATION_IMPL__

#include "config.h"
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
  auto const& this_node = theContext->getNode();
  auto reg_iter = local_registered_.find(id);

  assert(
    reg_iter == local_registered_.end() and
    "EntityLocationCoord entity should not already be registered"
  );

  debug_print(
    location, node,
    "EntityLocationCoord: registerEntity: id=%d\n", id
  );

  local_registered_.insert(id);

  recs_.insert(id, LocRecType{id, eLocState::Local, this_node});
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::unregisterEntity(EntityID const& id) {
  auto reg_iter = local_registered_.find(id);

  assert(
    reg_iter != local_registered_.end() and
    "EntityLocationCoord entity must be registered"
  );

  debug_print(
    location, node,
    "EntityLocationCoord: unregisterEntity: id=%d\n", id
  );

  local_registered_.erase(reg_iter);

  bool const& rec_exists = recs_.exists(id);
  if (rec_exists) {
    recs_.remove(id);
  }
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::entityMigrated(
  EntityID const& id, NodeType const& new_node
) {
  auto reg_iter = local_registered_.find(id);

  if (reg_iter != local_registered_.end()) {
    local_registered_.erase(reg_iter);
  }

  recs_.insert(id, LocRecType{id, eLocState::Remote, new_node});
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::getLocation(
  EntityID const& id, NodeType const& home_node, NodeActionType const& action
) {
  auto const& this_node = theContext->getNode();

  auto reg_iter = local_registered_.find(id);

  if (reg_iter != local_registered_.end()) {
    debug_print(
      location, node,
      "EntityLocationCoord: getLocation: id=%d, entity is local\n", id
    );

    action(this_node);
    recs_.insert(id, LocRecType{id, eLocState::Local, this_node});
    return;
  } else {
    bool const& rec_exists = recs_.exists(id);

    debug_print(
      location, node,
      "EntityLocationCoord: getLocation: id=%d, home_node=%d, rec_exists=%s\n",
      id, home_node, print_bool(rec_exists)
    );

    if (not rec_exists) {
      if (home_node != this_node) {
        auto const& event_id = fst_location_event_id++;
        auto msg = new LocMsgType(
          LocManInstType::VirtualLocManInst, id, event_id, this_node, home_node
        );
        theMsg->sendMsg<LocMsgType, getLocationHandler>(
          home_node, msg, [=]{ delete msg; }
        );
        // save a pending action when information about location arrives
        pending_actions_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(event_id),
          std::forward_as_tuple(PendingType{id,action})
        );
      } else {
        // this is the home node and there is no record on this entity
        auto pending_iter = pending_lookups_.find(id);
        if (pending_iter != pending_lookups_.end()) {
          pending_iter->second.push_back(action);
        } else {
          pending_lookups_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(id),
            std::forward_as_tuple(ActionListType{action})
          );
        }
      }
    } else {
      auto const& rec = recs_.get(id);

      if (rec.isLocal()) {
        assert(0 and "Should be registered if this is the case!");
        action(this_node);
      } else if (rec.isRemote()) {
        debug_print(
          location, node,
          "EntityLocationCoord: getLocation: id=%d, entity is remote\n", id
        );

        action(rec.getRemoteNode());
      }
    }
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

  auto const& entity = pending_iter->second.entity;

  debug_print(
    location, node,
    "EntityLocationCoord: updatePendingRequest: event_id=%lld, entity=%d, "
    "node=%d\n",
    event_id, entity, node
  );

  recs_.insert(entity, LocRecType{entity, eLocState::Remote, node});

  pending_iter->second.applyNodeAction(node);

  pending_actions_.erase(pending_iter);
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::printCurrentCache() const {
  recs_.printCache();
}

template <typename EntityID>
/*static*/ void EntityLocationCoord<EntityID>::getLocationHandler(LocMsgType* msg) {
  auto const& event_id = msg->loc_event;
  auto const& inst = msg->loc_man_inst;
  auto const& entity = msg->entity;
  auto const& home_node = msg->home_node;
  auto const& ask_node = msg->ask_node;

  if (inst == LocManInstType::VirtualLocManInst) {
    auto const& loc = theLocMan->virtual_loc;
    loc->getLocation(entity, home_node, [=](NodeType node) {
      auto msg = new LocMsgType(
        LocManInstType::VirtualLocManInst, entity, event_id, ask_node, home_node
      );
      msg->setResolvedNode(node);
      theMsg->sendMsg<LocMsgType, updateLocation>(
        ask_node, msg, [=]{ delete msg; }
      );
    });
  }
}

template <typename EntityID>
/*static*/ void EntityLocationCoord<EntityID>::updateLocation(LocMsgType* msg) {
  auto const& event_id = msg->loc_event;
  auto const& inst = msg->loc_man_inst;
  auto const& entity = msg->entity;

  if (inst == LocManInstType::VirtualLocManInst) {
    auto const& loc = theLocMan->virtual_loc;
    loc->updatePendingRequest(event_id, msg->resolved_node);
  }
}

}} // end namespace vt::location

#endif /*__RUNTIME_TRANSPORT_LOCATION_IMPL__*/
