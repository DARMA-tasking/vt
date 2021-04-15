/*
//@HEADER
// *****************************************************************************
//
//                           location.routing.impl.h
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

#if !defined INCLUDED_VT_TOPOS_LOCATION_LOCATION_ROUTING_IMPL_H
#define INCLUDED_VT_TOPOS_LOCATION_LOCATION_ROUTING_IMPL_H

#include "vt/topos/location/location.h"
#include "vt/messaging/active.h"
#include "vt/topos/location/manager.h"
#include "vt/termination/termination.h"
#include "vt/runnable/make_runnable.h"

namespace vt { namespace location {

template <typename EntityID>
template <typename MessageT>
bool EntityLocationCoord<EntityID>::useEagerProtocol(
  MsgSharedPtr<MessageT> msg
) const {
  bool const is_small = sizeof(*msg) < small_msg_max_size;
  // could change according to entity type or another criterion
  return is_small;
}

template <typename EntityID>
template <typename MessageT, ActiveTypedFnType<MessageT> *f>
void EntityLocationCoord<EntityID>::routeMsgHandler(
  EntityID const& id, NodeType const& home_node, MsgSharedPtr<MessageT> m
) {
  using auto_registry::HandlerManagerType;

  auto handler = auto_registry::makeAutoHandler<MessageT,f>();

# if vt_check_enabled(trace_enabled)
  HandlerManagerType::setHandlerTrace(
    handler, envelopeGetTraceRuntimeEnabled(m->env)
  );
# endif

  m->setHandler(handler);
  return routeMsg<MessageT>(id,home_node,m);
}

template <typename EntityID>
template <typename MessageT>
void EntityLocationCoord<EntityID>::routeMsg(
  EntityID const& id, NodeType const& home_node, MsgSharedPtr<MessageT> msg,
  NodeType from_node
) {
  auto const from =
    from_node == uninitialized_destination ? theContext()->getNode() :
    from_node;

  // set field for location routed message
  msg->setEntity(id);
  msg->setHomeNode(home_node);
  msg->setLocFromNode(from);

  auto const msg_size = sizeof(*msg);
  bool const use_eager = useEagerProtocol(msg);
  auto const epoch = theMsg()->getEpochContextMsg(msg);

  vt_debug_print(
    verbose, location,
    "routeMsg: inst={}, home={}, msg_size={}, is_large_msg={}, eager={}, "
    "in_from={}, from={}, msg{}, msg from={}, epoch={:x}\n",
    this_inst, home_node, msg_size, msg_size > small_msg_max_size, use_eager,
    from_node, from, print_ptr(msg.get()), msg->getLocFromNode(), epoch
  );

  msg->setLocInst(this_inst);

  if (use_eager) {
    theMsg()->pushEpoch(epoch);
    routeMsgEager<MessageT>(id, home_node, msg);
    theMsg()->popEpoch(epoch);
  } else {
    theTerm()->produce(epoch);
    // non-eager protocol: get location first then send message after resolution
    getLocation(id, home_node, [=](NodeType node) {
      theMsg()->pushEpoch(epoch);
      routeMsgNode<MessageT>(id, home_node, node, msg);
      theMsg()->popEpoch(epoch);
      theTerm()->consume(epoch);
    });
  }
}

template <typename EntityID>
template <typename MessageT>
void EntityLocationCoord<EntityID>::routeMsgEager(
  EntityID const& id, NodeType const& home_node, MsgSharedPtr<MessageT> msg
) {
  auto const& this_node = theContext()->getNode();
  NodeType route_to_node = uninitialized_destination;

  auto reg_iter = local_registered_.find(id);
  bool const found = reg_iter != local_registered_.end();

  vt_debug_print(
    normal, location,
    "EntityLocationCoord: routeMsgEager: found={}, home_node={}, "
    "route_to_node={}, id={}\n",
    found, home_node, route_to_node, id
  );

  if (found) {
    recs_.insert(id, home_node, LocRecType{id, eLocState::Local, this_node});
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

  vtAssert(
    route_to_node != uninitialized_destination,
    "Node to route to must be set by this point"
  );

  vt_debug_print(
    normal, location,
    "EntityLocationCoord: routeMsgEager: home_node={}, route_node={}, id={}\n",
    home_node, route_to_node, id
  );

  return routeMsgNode<MessageT>(id,home_node,route_to_node,msg);
}

template <typename EntityID>
template <typename MessageT>
void EntityLocationCoord<EntityID>::routeMsgNode(
  EntityID const& id, NodeType const& home_node, NodeType const& to_node,
  MsgSharedPtr<MessageT> msg
) {
  auto const& this_node = theContext()->getNode();
  auto const epoch = theMsg()->getEpochContextMsg(msg);

  vt_debug_print(
    normal, location,
    "EntityLocationCoord: routeMsgNode: to_node={}, this_node={}: inst={}, "
    "home_node={}, id={}, ref={}, from={}, msg={}, epoch={:x}\n",
    to_node, this_node, this_inst, home_node, id,
    envelopeGetRef(msg->env), msg->getLocFromNode(), print_ptr(msg.get()),
    epoch
  );

  theMsg()->markAsLocationMessage(msg);

  if (to_node != this_node) {
    // Get the current ask node, which is the from node for the first hop
    auto ask_node = msg->getAskNode();
    if (ask_node != uninitialized_destination) {
      // Insert into the ask list for a later update when information is known
      loc_asks_[id].insert(ask_node);
    }

    // Update the new asking node, as this node is will be the next to ask
    msg->setAskNode(this_node);

    // set the instance on the message to deliver to the correct manager
    msg->setLocInst(this_inst);

    // send to the node discovered by the location manager
    theMsg()->sendMsg<MessageT, routedHandler>(to_node, msg);
  } else {
    vt_debug_print(
      normal, location,
      "EntityLocationCoord: routeMsgNode: to_node={}, this_node={}: "
      "home_node={}, ref={}, from={}, epoch={:x}: apply here\n",
      to_node, this_node, home_node, envelopeGetRef(msg->env),
      msg->getLocFromNode(), epoch
    );

    theTerm()->produce(epoch);

    auto trigger_msg_handler_action = [=](EntityID const& hid) {
      bool const& has_handler = msg->hasHandler();
      auto const& from = msg->getLocFromNode();
      if (has_handler) {
        auto const& handler = msg->getHandler();

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

      auto ask_node = msg->getAskNode();

      if (ask_node != uninitialized_destination) {
        auto delivered_node = theContext()->getNode();
        sendEagerUpdate(hid, ask_node, home_node, delivered_node);
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
      insertPendingEntityAction(id_, [=](NodeType resolved) {
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
          routeMsgNode<MessageT>(id_,home_node,resolved,msg);
        }
        theMsg()->popEpoch(epoch);
        theTerm()->consume(epoch);
      });
    }
  }
}

template <typename EntityID>
template <typename MessageT>
/*static*/ void EntityLocationCoord<EntityID>::routedHandler(MessageT *raw_msg) {
  // Message may be re-routed (and sent) again from subsequent routeMsg.
  envelopeUnlockForForwarding(raw_msg->env);

  auto msg = promoteMsg(raw_msg);
  auto const entity_id = msg->getEntity();
  auto const home_node = msg->getHomeNode();
  auto const inst = msg->getLocInst();
  auto const from_node = msg->getLocFromNode();
  auto const epoch = theMsg()->getEpochContextMsg(msg);

  msg->incHops();

  vt_debug_print(
    verbose, location,
    "routedHandler: msg={}, ref={}, loc_inst={}, id={}, from={}, "
    "epoch={:x}, hops={}, ask={}\n",
    print_ptr(msg.get()), envelopeGetRef(msg->env), inst, entity_id,
    from_node, epoch, msg->getHops(), msg->getAskNode()
  );

  theTerm()->produce(epoch);
  LocationManager::applyInstance<EntityLocationCoord<EntityID>>(
    inst, [=](EntityLocationCoord<EntityID>* loc) {
      theMsg()->pushEpoch(epoch);
      loc->routeMsg(entity_id, home_node, msg, from_node);
      theMsg()->popEpoch(epoch);
      theTerm()->consume(epoch);
    }
  );
}

}} /* end namespace vt::location */

#endif /*INCLUDED_VT_TOPOS_LOCATION_LOCATION_ROUTING_IMPL_H*/
