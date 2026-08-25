/*
//@HEADER
// *****************************************************************************
//
//                               location.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_TOPOS_LOCATION_LOCATION_IMPL_H
#define INCLUDED_VT_TOPOS_LOCATION_LOCATION_IMPL_H

#include "vt/config.h"
#include "vt/topos/location/location_common.h"
#include "vt/topos/location/location.h"
#include "vt/topos/location/manager.h"
#include "vt/topos/location/manager.fwd.h"
#include "vt/topos/location/utility/entity.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/runnable/make_runnable.h"
#include "vt/termination/term_common.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace vt { namespace location {

template <typename EntityID>
int LocCommunicator<EntityID>::getRank() const {
  return theContext()->getNode();
}

template <typename EntityID>
template <typename Instance>
auto LocCommunicator<EntityID>::registerInstanceCollective(Instance* instance)
  -> HandleType<Instance>
{
  static_assert(std::is_same_v<Instance, typename OwnerType::ResolverType>);
  vtAssert(owner_ != nullptr, "loc communicator must have an owner");
  vtAssert(instance != nullptr, "loc coordinator must be valid");
  return owner_->proxy_;
}

template <typename EntityID>
template <auto Handler, typename... Args>
void LocCommunicator<EntityID>::send(
  ::loc::NodeType node, objgroup::proxy::Proxy<OwnerType> handle,
  Args&&... args
) {
  using Owner = OwnerType;
  handle[node].template send<
    &Owner::template locControlHandler<Handler, std::decay_t<Args>...>
  >(
    MsgProps().asLocationMsg(), std::forward<Args>(args)...
  );
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::registerEntity(
  EntityID const& id, NodeType const& home, LocMsgActionType msg_action,
  bool const& migrated
) {
  auto reg_iter = local_registered_.find(id);

  vtAssert(
    reg_iter == local_registered_.end(),
    "EntityLocationCoord entity should not already be registered"
  );

  vt_debug_print(
    terse, location,
    "EntityLocationCoord: registerEntity: home={}, migrated={}, "
    "id={}\n",
    home, migrated, id
  );

  local_registered_.insert(id);

  if (msg_action != nullptr) {
    // vtAssert(
    //   local_registered_msg_han_.find(id) == local_registered_msg_han_.end(),
    //   "Entity should not exist in local registered msg handler"
    // );
    local_registered_msg_han_.emplace(id, LocEntityMsg{id, msg_action});
  }

  vt_debug_print(
    normal, location,
    "EntityLocationCoord: registerEntity: pending lookups size={}, this={}, "
    "id={}\n",
    pending_lookups_.size(), print_ptr(this), id
  );

  // trigger any pending actions upon registration
  {
    auto const& node = theContext()->getNode();
    int action = 0;
    if (auto lookups = pending_lookups_.extract(id); lookups) {
      for (auto&& pending_action : lookups.mapped()) {
        vt_debug_print(
          verbose, location,
          "EntityLocationCoord: registerEntity: running pending action {}\n",
          action
        );
        action++;
        pending_action(node);
      }
    }
  }

  vtAssert(home != uninitialized_destination, "Must have home node info");
  vtAssert(resolver_ != nullptr, "loc resolver must be initialized");
  resolver_->registerEntity(id, home);
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::registerEntityRemote(
  EntityID const& id, NodeType const& home, NodeType const create_node,
  LocMsgActionType msg_action
) {
  auto reg_iter = local_registered_.find(id);
  vtAssert(
    reg_iter == local_registered_.end(),
    "EntityLocationCoord entity should not already be registered"
  );

  vt_debug_print(
    normal, location,
    "EntityLocationCoord: registerEntityRemote: home={}, "
    "create_node={}, id={}\n",
    home, create_node, id
  );

  auto const this_node = theContext()->getNode();
  vtAssert(home == this_node, "Must be registered on home node");

  vtAssert(resolver_ != nullptr, "loc resolver must be initialized");
  resolver_->updateHome(id, create_node);

  if (msg_action != nullptr) {
    local_registered_msg_han_.emplace(id, LocEntityMsg{id, msg_action});
  }
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::unregisterEntity(EntityID const& id) {
  auto reg_iter = local_registered_.find(id);

  vtAssert(
    reg_iter != local_registered_.end(),
    "EntityLocationCoord entity must be registered"
  );

  vt_debug_print(
    normal, location,
    "EntityLocationCoord: unregisterEntity\n"
  );

  local_registered_.erase(reg_iter);

  vtAssert(resolver_ != nullptr, "loc resolver must be initialized");
  resolver_->unregisterEntity(id);

  auto reg_msg_han_iter = local_registered_msg_han_.find(id);
  if (reg_msg_han_iter != local_registered_msg_han_.end()) {
    local_registered_msg_han_.erase(reg_msg_han_iter);
  }
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::startMigrations() {
  migrations_ongoing_ = true;
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::doneMigrations() {
  if (keep_cache_updated_) {
    clearCache();
  }
  migrations_ongoing_ = false;
}

template <typename EntityID>
bool EntityLocationCoord<EntityID>::entityExistsLocal(EntityID const& id) const {
  return
    local_registered_.find(id) != local_registered_.end() or
    local_registered_msg_han_.find(id) != local_registered_msg_han_.end();
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::entityExists(
  EntityID const& id, NodeType const& home_node,
  ExistsNodeActionType const& action
) {
  vtAssert(resolver_ != nullptr, "loc resolver must be initialized");
  resolver_->entityExists(id, home_node, action);
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::entityEmigrated(
  EntityID const& id, NodeType const& new_node
) {
  vtAssert(
    anytime_migration_ or migrations_ongoing_, "Migrations must be allowed"
  );

  vt_debug_print(
    normal, location,
    "EntityLocationCoord: entityEmigrated: id={}, new_node={}\n",
    id, new_node
  );

  auto reg_iter = local_registered_.find(id);

  if (reg_iter != local_registered_.end()) {
    local_registered_.erase(reg_iter);
  }
  vtAssert(resolver_ != nullptr, "loc resolver must be initialized");
  resolver_->entityEmigrated(id, new_node);
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::entityImmigrated(
  EntityID const& id, NodeType const& home_node,
  [[maybe_unused]] NodeType const& from, LocMsgActionType msg_action
) {
  vtAssert(
    anytime_migration_ or migrations_ongoing_, "Migrations must be allowed"
  );

  // @todo: currently `from' is unused, but is passed to this method in case we
  // need it in the future
  return registerEntity(id, home_node, msg_action, true);
}

template <typename EntityID>
bool EntityLocationCoord<EntityID>::isCached(EntityID const& id) const {
  vtAssert(resolver_ != nullptr, "loc resolver must be initialized");
  return resolver_->isCached(id);
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::clearCache() {
  vtAssert(resolver_ != nullptr, "loc resolver must be initialized");
  resolver_->clearCache();
}

namespace detail {

template <typename MsgT, typename=void>
struct IsSerializable {
  static constexpr bool const is_ser = false;
};

template <typename MsgT>
struct IsSerializable<
  MsgT,
  typename std::enable_if_t<true
    and ::vt::messaging::msg_defines_serialize_mode<MsgT>::value
    and ::vt::messaging::msg_serialization_mode<MsgT>::required
  >
>
{
  static constexpr bool const is_ser = true;
};

} /* end namespace detail */

template <typename EntityID>
template <typename MessageT>
bool EntityLocationCoord<EntityID>::useEagerProtocol(
  MsgSharedPtr<MessageT> const& msg
) const {
  if (detail::IsSerializable<MessageT>::is_ser) {
    return false;
  } else {
    return sizeof(*msg) < small_msg_max_size;
  }
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::insertPendingEntityAction(
  EntityID const& id, NodeActionType action
) {
  vt_debug_print(
    verbose, location,
    "EntityLocationCoord: insertPendingEntityAction, this={}, id={}\n",
    print_ptr(this), id
  );

  pending_lookups_[id].push_back(action);
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::getLocation(
  EntityID const& id, NodeType const& home_node, NodeActionType const& action
) {
  vtAssert(resolver_ != nullptr, "loc resolver must be initialized");
  resolver_->getLocation(id, home_node, action);
}

template <typename EntityID>
template <typename MessageT>
void EntityLocationCoord<EntityID>::routeMsgNode(
  EntityID const& id, NodeType const& home_node, NodeType const& to_node,
  MsgSharedPtr<MessageT> const& msg
) {
  auto const& this_node = theContext()->getNode();
  auto const epoch = theMsg()->getEpochContextMsg(msg);

  vt_debug_print(
    normal, location,
    "EntityLocationCoord: routeMsgNode: to_node={}, this_node={}: "
    "home_node={}, id={}, ref={}, from={}, msg={}, epoch={:x}\n",
    to_node, this_node, home_node, id,
    envelopeGetRef(msg->env), msg->getLocFromNode(), print_ptr(msg.get()),
    epoch
  );

  theMsg()->markAsLocationMessage(msg);

  if (to_node != this_node) {
    auto m = msg; //copy for msg thief
    // send to the node discovered by the location manager
    proxy_[to_node].template sendMsg<&ThisType::routedHandler<MessageT>>(m);
  } else {
    vt_debug_print(
      normal, location,
      "EntityLocationCoord: routeMsgNode: to_node={}, this_node={}: "
      "home_node={}, ref={}, from={}, epoch={:x}: apply here\n",
      to_node, this_node, home_node, envelopeGetRef(msg->env),
      msg->getLocFromNode(), epoch
    );

    theTerm()->produce(epoch);

    auto trigger_msg_handler_action = [this, msg](EntityID const& hid) {
      bool const& has_handler = msg->hasHandler();
      auto const& from = msg->getLocFromNode();
      if (has_handler) {
        auto const handler = msg->getHandler();

        vt_debug_print(
          verbose, location,
          "EntityLocationCoord: apply direct handler action: "
          "id={}, from={}, handler={}, ref={}\n",
          hid, from, handler, envelopeGetRef(msg->env)
        );

        runnable::makeRunnable(msg, true, handler, from)
          .withTDEpochFromMsg()
          .run();
      } else {
        auto reg_han_iter = local_registered_msg_han_.find(hid);
        vtAssert(
          reg_han_iter != local_registered_msg_han_.end(),
          "Message handler must exist for location manager routed msg"
        );
        vt_debug_print(
          verbose, location,
          "EntityLocationCoord: no direct handler: id={}\n", hid
        );
        reg_han_iter->second.applyRegisteredActionMsg(msg.get());
      }

      auto const route_epoch = msg->getRouteEpoch();

      // Release the caller's epoch that was manually held at origination for a
      // message whose envelope could not carry it across the routing hop.
      if (route_epoch != no_epoch) {
        theTerm()->consume(route_epoch);
      }
    };

    auto reg_iter = local_registered_.find(id);

    vt_debug_print(
      verbose, location,
      "EntityLocationCoord: routeMsgNode: size={}\n",
      local_registered_.size()
    );

    if (reg_iter != local_registered_.end()) {
      vt_debug_print(
        normal, location,
        "EntityLocationCoord: routeMsgNode: epoch={:x} running actions\n",
        epoch
      );

      theMsg()->pushEpoch(epoch);
      trigger_msg_handler_action(id);
      theMsg()->popEpoch(epoch);
      theTerm()->consume(epoch);
    } else {
      vt_debug_print(
        normal, location,
        "EntityLocationCoord: routeMsgNode: ref={}, buffering\n",
        envelopeGetRef(msg->env)
      );

      EntityID id_ = id;
      // buffer the message here, the entity will be registered in the future
      insertPendingEntityAction(id_, [this, id_, epoch, msg, home_node, trigger_msg_handler_action](NodeType resolved) {
        auto const& my_node = theContext()->getNode();

        vt_debug_print(
          normal, location,
          "EntityLocationCoord: routeMsgNode: trigger action: resolved={}, "
          "this_node={}, id={}, ref={}, epoch={:x}\n",
          resolved, my_node, id_, envelopeGetRef(msg->env), epoch
        );

        theMsg()->pushEpoch(epoch);
        if (resolved == my_node) {
          trigger_msg_handler_action(id_);
        } else {
          /*
           *  Recurse with the new updated node information. This occurs
           *  typically when an non-migrated registration occurs off the home
           *  node and messages are buffered, awaiting forwarding information.
           */
          routeMsgNode<MessageT>(id_, home_node, resolved,msg);
        }
        theMsg()->popEpoch(epoch);
        theTerm()->consume(epoch);
      });
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
  EntityID const& id, NodeType const& home_node,
  MsgSharedPtr<MessageT> const& msg
) {
  setupMessageForRouting<MessageT, f>(id, home_node, msg);

  routePreparedMsgHandler(msg);
}

template <typename EntityID>
template <typename MessageT>
void EntityLocationCoord<EntityID>::routePreparedMsgHandler(
  MsgSharedPtr<MessageT> const& msg
) {
  if (local_registered_.find(msg->getEntity()) == local_registered_.end()) {
    return routePreparedMsg(msg);
  } else {
    return routeMsgHandlerLocal(msg);
  }
}

template <typename EntityID>
template <typename MessageT, ActiveTypedFnType<MessageT> *f>
void EntityLocationCoord<EntityID>::setupMessageForRouting(
  EntityID const& id, NodeType const& home_node,
  MsgSharedPtr<MessageT> const& msg
) {
  using auto_registry::HandlerManagerType;

  auto handler = auto_registry::makeAutoHandler<MessageT,f>();

# if vt_check_enabled(trace_enabled)
  HandlerManagerType::setHandlerTrace(
    handler, envelopeGetTraceRuntimeEnabled(msg->env)
  );
# endif

  msg->setHandler(handler);
  msg->setEntity(id);
  msg->setHomeNode(home_node);
  msg->setLocFromNode(theContext()->getNode());
}

template <typename EntityID>
template <typename MessageT>
void EntityLocationCoord<EntityID>::routeMsgHandlerLocal(
  MsgSharedPtr<MessageT> const& msg
) {
  runnable::makeRunnable(msg, true, msg->getHandler(), theContext()->getNode())
    .withTDEpochFromMsg()
    .run();
}

template <typename EntityID>
template <typename MessageT>
void EntityLocationCoord<EntityID>::routePreparedMsg(
  MsgSharedPtr<MessageT> const& msg
) {
  auto const msg_size = sizeof(*msg);
  bool const use_eager = useEagerProtocol(msg);
  auto const epoch = theMsg()->getEpochContextMsg(msg);

  // Messages whose envelope cannot hold an epoch (e.g. short messages) lose the
  // caller's epoch as soon as they hop to another node: getEpochContextMsg
  // collapses to the "any epoch" sentinel even when the caller is inside a real
  // collective epoch. Detect that case at origination and manually hold the
  // caller's epoch across the whole route, releasing it at final delivery.
  // This keeps the routed message enclosed by the caller's epoch exactly as an
  // epoch-carrying (long) message already is via its envelope.
  if (msg->getRouteEpoch() == no_epoch and epoch == term::any_epoch_sentinel) {
    auto const cur_epoch = theMsg()->getEpoch();
    if (cur_epoch != term::any_epoch_sentinel and cur_epoch != no_epoch) {
      msg->setRouteEpoch(cur_epoch);
      theTerm()->produce(cur_epoch);
    }
  }

  vt_debug_print(
    verbose, location,
    "routeMsg: home={}, msg_size={}, is_large_msg={}, eager={}, "
    "msg={}, from={}, epoch={:x}\n",
    msg->getHomeNode(), msg_size, msg_size > small_msg_max_size, use_eager,
    print_ptr(msg.get()), msg->getLocFromNode(),
    epoch
  );

  // loc is a resolver rather than a forwarding router. Resolve first for both
  // message sizes, then keep VT's existing delivery and epoch semantics.
  theTerm()->produce(epoch);
  getLocation(
    msg->getEntity(), msg->getHomeNode(),
    [this, epoch, msg](NodeType node) {
      theMsg()->pushEpoch(epoch);
      routeMsgNode<MessageT>(msg->getEntity(), msg->getHomeNode(), node, msg);
      theMsg()->popEpoch(epoch);
      theTerm()->consume(epoch);
    }
  );
}

template <typename EntityID>
template <typename MessageT>
void EntityLocationCoord<EntityID>::routeMsg(
  EntityID const& id, NodeType const& home_node,
  MsgSharedPtr<MessageT> const& msg, NodeType from_node
) {
  auto const from =
    from_node == uninitialized_destination ? theContext()->getNode() :
    from_node;

  // set field for location routed message
  msg->setEntity(id);
  msg->setHomeNode(home_node);
  msg->setLocFromNode(from);

  routePreparedMsg(msg);
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::printCurrentCache() const {
  vt_debug_print(
    normal, location,
    "EntityLocationCoord: cache is managed by DARMA/loc\n"
  );
}

template <typename EntityID>
template <typename MessageT>
void EntityLocationCoord<EntityID>::routedHandler(MessageT *raw_msg) {
  // Message may be re-routed (and sent) again from subsequent routeMsg.
  envelopeUnlockForForwarding(raw_msg->env);

  auto msg = promoteMsg(raw_msg);
  auto const entity_id = msg->getEntity();
  auto const home_node = msg->getHomeNode();
  auto const from_node = msg->getLocFromNode();
  auto const epoch = theMsg()->getEpochContextMsg(msg);

  msg->incHops();

  vt_debug_print(
    verbose, location,
    "routedHandler: msg={}, ref={}, id={}, from={}, "
    "epoch={:x}, hops={}, ask={}\n",
    print_ptr(msg.get()), envelopeGetRef(msg->env), entity_id,
    from_node, epoch, msg->getHops(), msg->getAskNode()
  );

  routeMsg(entity_id, home_node, msg, from_node);
}

template <typename EntityID>
std::vector<EntityID> EntityLocationCoord<EntityID>::getLocalEntities() const {
  std::vector<EntityID> local;
  for (auto const& elm : local_registered_) {
    local.push_back(elm);
  }
  for (auto const& [elm, _] : local_registered_msg_han_) {
    local.push_back(elm);
  }
  return local;
}

template <typename EntityID>
std::unordered_map<EntityID, NodeType>
EntityLocationCoord<EntityID>::buildGlobalMap() {
  std::unordered_map<EntityID, NodeType> local_map;

  for (auto const& key : local_registered_) {
    local_map[key] = theContext()->getNode();
  }

  waiting_global_map_handler_ = true;

  proxy_.template allreduce<
    &EntityLocationCoord<EntityID>::globalMapHandler, collective::PlusOp
  >(local_map);

  theSched()->runSchedulerWhile([&]{ return waiting_global_map_handler_; });

  auto global_map = std::move(global_map_temp_);
  global_map_temp_ = {};
  return global_map;
}

template <typename EntityID>
void EntityLocationCoord<EntityID>::globalMapHandler(
  std::unordered_map<EntityID, NodeType> const& global_map
) {
  global_map_temp_ = global_map;
  waiting_global_map_handler_ = false;
}

}}  // end namespace vt::location

#endif /*INCLUDED_VT_TOPOS_LOCATION_LOCATION_IMPL_H*/
