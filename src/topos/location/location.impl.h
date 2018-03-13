
#if !defined INCLUDED_TOPOS_LOCATION_LOCATION_IMPL_H
#define INCLUDED_TOPOS_LOCATION_LOCATION_IMPL_H

#include "config.h"
#include "topos/location/location_common.h"
#include "topos/location/location.h"
#include "topos/location/manager.h"
#include "topos/location/manager.fwd.h"
#include "topos/location/utility/entity.h"
#include "context/context.h"
#include "messaging/active.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace vt { namespace location {

template <typename EntityID>
EntityLocationCoord<EntityID>::EntityLocationCoord()
  : EntityLocationCoord<EntityID>(theLocMan()->cur_loc_inst++)
{ }

template <typename EntityID>
EntityLocationCoord<EntityID>::EntityLocationCoord(
  collection_lm_tag_t, LocInstType identifier
) : EntityLocationCoord<EntityID>(identifier)
{ }

template <typename EntityID>
EntityLocationCoord<EntityID>::EntityLocationCoord(LocInstType const identifier)
  : this_inst(identifier), recs_(default_max_cache_size)
{
  debug_print(
    location, node,
    "EntityLocationCoord constructor: inst=%llu, this=%p\n",
    this_inst, this
  );

  LocationManager::insertInstance<EntityLocationCoord<EntityID>>(
    this_inst, this
  );
}

template <typename EntityID>
/*virtual*/ EntityLocationCoord<EntityID>::~EntityLocationCoord() {
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::registerEntity(
  EntityID const& id, LocMsgActionType msg_action
) {
  auto const& this_node = theContext()->getNode();
  auto reg_iter = local_registered_.find(id);

  assert(
    reg_iter == local_registered_.end() &&
    "EntityLocationCoord entity should not already be registered"
  );

  debug_print(
    location, node,
    "EntityLocationCoord: registerEntity: inst=%llu\n", this_inst
  );

  local_registered_.insert(id);

  recs_.insert(id, LocRecType{id, eLocState::Local, this_node});

  if (msg_action != nullptr) {
    assert(
      local_registered_msg_han_.find(id) == local_registered_msg_han_.end() &&
      "Entitiy should not exist in local registered msg handler"
    );
    local_registered_msg_han_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(id),
      std::forward_as_tuple(LocEntityMsg{id, msg_action})
    );
  }

  // trigger any pending actions upon registration
  auto pending_lookup_iter = pending_lookups_.find(id);

  debug_print(
    location, node,
    "EntityLocationCoord: registerEntity: pending lookups size=%lu, this=%p\n",
    pending_lookups_.size(), this
  );

  if (pending_lookup_iter != pending_lookups_.end()) {
    auto const& node = theContext()->getNode();
    int action = 0;
    for (auto&& pending_action : pending_lookup_iter->second) {
      debug_print(
        location, node,
        "EntityLocationCoord: registerEntity: running pending action %d\n",
        action
      );
      action++;
      pending_action(node);
    }
    pending_lookups_.erase(pending_lookup_iter);
  }
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::unregisterEntity(EntityID const& id) {
  auto reg_iter = local_registered_.find(id);

  assert(
    reg_iter != local_registered_.end() &&
    "EntityLocationCoord entity must be registered"
  );

  debug_print(
    location, node,
    "EntityLocationCoord: unregisterEntity\n"
  );

  local_registered_.erase(reg_iter);

  bool const& rec_exists = recs_.exists(id);
  if (rec_exists) {
    recs_.remove(id);
  }

  auto reg_msg_han_iter = local_registered_msg_han_.find(id);
  if (reg_msg_han_iter != local_registered_msg_han_.end()) {
    local_registered_msg_han_.erase(reg_msg_han_iter);
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
void EntityLocationCoord<EntityID>::registerEntityMigrated(
  EntityID const& id, NodeType const& from, LocMsgActionType msg_action
) {
  // @todo: currently `from' is unused, but is passed to this method in case we
  // need it in the future
  return registerEntity(id, msg_action);
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::insertPendingEntityAction(
  EntityID const& id, NodeActionType action
) {
  debug_print(
    location, node,
    "EntityLocationCoord: insertPendingEntityAction, this=%p\n",
    this
  );

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

template <typename EntityID>
template <typename MessageT>
void EntityLocationCoord<EntityID>::routeMsgEager(
  EntityID const& id, NodeType const& home_node, MessageT *msg,
  ActionType action
) {
  auto const& this_node = theContext()->getNode();
  NodeType route_to_node = uninitialized_destination;

  auto reg_iter = local_registered_.find(id);
  bool const found = reg_iter != local_registered_.end();

  debug_print(
    location, node,
    "EntityLocationCoord: routeMsgEager: found=%s\n", print_bool(found)
  );

  if (found) {
    recs_.insert(id, LocRecType{id, eLocState::Local, this_node});
    route_to_node = this_node;
  } else {
    bool const& rec_exists = recs_.exists(id);

    if (not rec_exists) {
      if (home_node != this_node) {
        route_to_node = home_node;
      } else {
        route_to_node = this_node;
      }
    } else {
      auto const& rec = recs_.get(id);

      if (rec.isLocal()) {
        route_to_node = this_node;
      } else if (rec.isRemote()) {
        route_to_node = rec.getRemoteNode();
      }
    }
  }

  assert(
    route_to_node != uninitialized_destination &&
    "Node to route to must be set by this point"
  );

  debug_print(
    location, node,
    "EntityLocationCoord: routeMsgEager: home_node=%d, route_node=%d\n",
    home_node, route_to_node
  );

  return routeMsgNode<MessageT>(id, home_node, route_to_node, msg, action);
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::getLocation(
  EntityID const& id, NodeType const& home_node, NodeActionType const& action
) {
  auto const& this_node = theContext()->getNode();

  auto reg_iter = local_registered_.find(id);

  if (reg_iter != local_registered_.end()) {
    debug_print(
      location, node,
      "EntityLocationCoord: getLocation: entity is local\n"
    );

    action(this_node);
    recs_.insert(id, LocRecType{id, eLocState::Local, this_node});
    return;
  } else {
    bool const& rec_exists = recs_.exists(id);

    debug_print(
      location, node,
      "EntityLocationCoord: getLocation: home_node=%d, rec_exists=%s\n",
      home_node, print_bool(rec_exists)
    );

    if (not rec_exists) {
      if (home_node != this_node) {
        auto const& event_id = fst_location_event_id++;
        auto msg = new LocMsgType(this_inst, id, event_id, this_node, home_node);
        theMsg()->sendMsg<LocMsgType, getLocationHandler>(
          home_node, msg, [=] { delete msg; }
        );
        // save a pending action when information about location arrives
        pending_actions_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(event_id),
          std::forward_as_tuple(PendingType{id, action})
        );
      } else {
        // this is the home node and there is no record on this entity
        insertPendingEntityAction(id, action);
      }
    } else {
      auto const& rec = recs_.get(id);

      if (rec.isLocal()) {
        assert(0 && "Should be registered if this is the case!");
        action(this_node);
      } else if (rec.isRemote()) {
        debug_print(
          location, node,
          "EntityLocationCoord: getLocation: entity is remote\n"
        );

        action(rec.getRemoteNode());
      }
    }
  }
}

template <typename EntityID>
template <typename MessageT>
void EntityLocationCoord<EntityID>::routeMsgNode(
  EntityID const& id, NodeType const& home_node, NodeType const& to_node,
  MessageT *msg, ActionType action
) {
  auto const& this_node = theContext()->getNode();

  debug_print(
    location, node,
    "EntityLocationCoord: routeMsgNode: to_node=%d, node=%d: inst=%llu\n",
    to_node, this_node, this_inst
  );

  if (to_node != this_node) {
    // set the instance on the message to deliver to the correct manager
    msg->setLocInst(this_inst);
    // send to the node discovered by the location manager
    theMsg()->sendMsg<MessageT, msgHandler>(to_node, msg, action);
  } else {
    if (msg->hasHandler()) {
      auto const& handler = msg->getHandler();
      auto active_fn = auto_registry::getAutoHandler(handler);
      active_fn(reinterpret_cast<BaseMessage*>(msg));
    } else {
      auto trigger_msg_handler_action = [=](EntityID const& id) {
        auto reg_han_iter = local_registered_msg_han_.find(id);
        assert(
          reg_han_iter != local_registered_msg_han_.end() and
          "Message handler must exist for location manager routed msg"
        );
        reg_han_iter->second.applyRegisteredActionMsg(msg);
      };

      auto reg_iter = local_registered_.find(id);

      debug_print(
        location, node,
        "EntityLocationCoord: routeMsgNode: size=%ld\n",
        local_registered_.size()
      );

      if (reg_iter != local_registered_.end()) {
        debug_print(
          location, node,
          "EntityLocationCoord: routeMsgNode: running actions\n"
        );

        trigger_msg_handler_action(id);
        if (action) {
          action();
        }
      } else {
        debug_print(
          location, node,
          "EntityLocationCoord: routeMsgNode: buffering\n"
        );

        // buffer the message here, the entity will be registered in the future
        insertPendingEntityAction(id, [=](NodeType) {
          trigger_msg_handler_action(id);
          if (action) {
            action();
          }
        });
      }
    }
  }
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::routeNonEagerAction(
  EntityID const& id, NodeType const& home_node, ActionNodeType action
) {
  getLocation(id, home_node, [=](NodeType node) {
    action(node);
  });
}

template <typename EntityID>
template <typename MessageT, ActiveTypedFnType<MessageT> *f>
void EntityLocationCoord<EntityID>::routeMsgHandler(
  EntityID const& id, NodeType const& home_node, MessageT *m, ActionType action
) {
  HandlerType const& han = auto_registry::makeAutoHandler<MessageT, f>(nullptr);
  m->setHandler(han);

  return routeMsg<MessageT>(id, home_node, m, action);
}

template <typename EntityID>
template <typename MessageT>
void EntityLocationCoord<EntityID>::routeMsg(
  EntityID const& id, NodeType const& home_node, MessageT *msg, ActionType act
) {
  // set field for location routed message
  msg->setEntity(id);
  msg->setHomeNode(home_node);

  auto const& msg_size = sizeof(*msg);
  bool const& is_large_msg = msg_size > small_msg_max_size;
  bool const& use_eager = not is_large_msg;

  debug_print(
    location, node,
    "routeMsg: inst=%llu, home=%d, msg_size=%ld, is_large_msg=%s, eager=%s\n",
    this_inst, home_node, msg_size, print_bool(is_large_msg),
    print_bool(use_eager)
  );

  msg->setLocInst(this_inst);

  if (use_eager) {
    routeMsgEager<MessageT>(id, home_node, msg, act);
  } else {
    // non-eager protocol: get location first then send message after resolution
    getLocation(id, home_node, [=](NodeType node) {
      routeMsgNode<MessageT>(id, home_node, node, msg, act);
    });
  }
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::updatePendingRequest(
  LocEventID const& event_id, NodeType const& node
) {
  auto pending_iter = pending_actions_.find(event_id);

  assert(
    pending_iter != pending_actions_.end() && "Event must exist in pending"
  );

  auto const& entity = pending_iter->second.entity_;

  debug_print(
    location, node,
    "EntityLocationCoord: updatePendingRequest: event_id=%lld, node=%d\n",
    event_id, node
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
LocInstType EntityLocationCoord<EntityID>::getInst() const {
  return this_inst;
}

template <typename EntityID>
template <typename MessageT>
/*static*/ void EntityLocationCoord<EntityID>::msgHandler(MessageT *msg) {
  auto const& entity_id = msg->getEntity();
  auto const& home_node = msg->getHomeNode();
  auto const& inst = msg->getLocInst();

  debug_print(
    location, node,
    "msgHandler: msg=%p, ref=%d, loc_inst=%llu\n",
    msg, envelopeGetRef(msg->env), inst
  );

  messageRef(msg);
  LocationManager::applyInstance<EntityLocationCoord<EntityID>>(
    inst, [=](EntityLocationCoord<EntityID>* loc) {
      loc->routeMsg(entity_id, home_node, msg);
      messageDeref(msg);
    }
  );
}

template <typename EntityID>
/*static*/ void EntityLocationCoord<EntityID>::getLocationHandler(LocMsgType *msg) {
  auto const& event_id = msg->loc_event;
  auto const& inst = msg->loc_man_inst;
  auto const& entity = msg->entity;
  auto const& home_node = msg->home_node;
  auto const& ask_node = msg->ask_node;

  messageRef(msg);
  LocationManager::applyInstance<EntityLocationCoord<EntityID>>(
    inst, [=](EntityLocationCoord<EntityID>* loc) {
      loc->getLocation(entity, home_node, [=](NodeType node) {
        auto msg = new LocMsgType(inst, entity, event_id, ask_node, home_node);
        msg->setResolvedNode(node);
        theMsg()->sendMsg<LocMsgType, updateLocation>(
          ask_node, msg, [=] { delete msg; }
        );
      });
      messageDeref(msg);
    }
  );
}

template <typename EntityID>
/*static*/ void EntityLocationCoord<EntityID>::updateLocation(LocMsgType *msg) {
  auto const& event_id = msg->loc_event;
  auto const& inst = msg->loc_man_inst;
  auto const& entity = msg->entity;

  messageRef(msg);
  LocationManager::applyInstance<EntityLocationCoord<EntityID>>(
    inst, [=](EntityLocationCoord<EntityID>* loc) {
      loc->updatePendingRequest(event_id, msg->resolved_node);
      messageDeref(msg);
    }
  );
}

}}  // end namespace vt::location

#endif /*INCLUDED_TOPOS_LOCATION_LOCATION_IMPL_H*/
