/*
//@HEADER
// *****************************************************************************
//
//                                  location.h
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

#if !defined INCLUDED_TOPOS_LOCATION_LOCATION_H
#define INCLUDED_TOPOS_LOCATION_LOCATION_H

#include "vt/config.h"
#include "vt/topos/location/location_common.h"
#include "vt/topos/location/location.fwd.h"
#include "vt/topos/location/utility/pending.h"
#include "vt/topos/location/utility/entity.h"
#include "vt/topos/location/utility/coord.h"
#include "vt/topos/location/message/msg.h"
#include "vt/topos/location/lookup/lookup.h"
#include "vt/topos/location/record/record.h"
#include "vt/topos/location/record/state.h"
#include "vt/context/context.h"
#include "vt/activefn/activefn.h"
#include "vt/vrt/vrt_common.h"

#include <checkpoint/checkpoint.h>

#include <cstdint>
#include <memory>
#include <vector>
#include <list>
#include <unordered_set>
#include <unordered_map>

namespace vt { namespace location {

struct collection_lm_tag_t {};

/**
 * \struct EntityLocationCoord
 *
 * \brief Part of a core VT component that manages the distributed location of
 * virtual entities.
 *
 * Allows general registration of an \c EntityID that is tracked across the
 * system as it migrates. Routes messages to the appropriate node by inheriting
 * from \c EntityMsg and sending it through the routing algorithm. Manages a
 * distributed table of entities and their location allowing an entity to be
 * found anywhere in the system. Caches locations to speed up resolution once a
 * location is known. Allows migration of an entity at any time; forwards
 * messages if they miss in the cache.
 *
 * The \c registerEntity method allows an external component to locally register
 * an entity as existing on this node. If the entity is deleted, \c
 * unregisterEntity should be called; if the entity is migrated, \c
 * entityMigrated should be invoked on the node from which the entity is
 * emigrating. A message may arrive for the entity by a location coordinator
 * calling \c routeMsg to the associated entity.
 *
 */
template <typename EntityID>
struct EntityLocationCoord : LocationCoord {
  using LocRecType = LocRecord<EntityID>;
  using LocCacheType = LocLookup<EntityID, LocRecType>;
  using LocEntityMsg = LocEntity<EntityID>;
  using LocalRegisteredContType = std::unordered_set<EntityID>;
  using LocalRegisteredMsgContType = std::unordered_map<EntityID, LocEntityMsg>;
  using ActionListType = std::list<NodeActionType>;
  using PendingType = PendingLocationLookup<EntityID>;
  using PendingLocLookupsType = std::unordered_map<EntityID, ActionListType>;
  using ActionContainerType = std::unordered_map<LocEventID, PendingType>;
  using LocMsgType = LocationMsg<EntityID>;
  using LocAsksType = std::unordered_map<EntityID, std::unordered_set<NodeType>>;

  template <typename MessageT>
  using EntityMsgType = EntityMsg<EntityID, MessageT>;

  /**
   * \internal \brief System call to construct a new entity coordinator
   */
  EntityLocationCoord();

  /**
   * \internal \brief System call to construct a new entity coordinator for
   * collections
   *
   * \param[in] collection_lm_tag_t tag
   * \param[in] identifier the entity class identifier
   */
  EntityLocationCoord(collection_lm_tag_t, LocInstType identifier);

  /**
   * \internal \brief System call to construct a new entity coordinator
   *
   * \param[in] identifier the entity class identifier
   */
  explicit EntityLocationCoord(LocInstType const identifier);

  virtual ~EntityLocationCoord();

  /**
   * \brief Register a new entity
   *
   * \param[in] id the entity ID
   * \param[in] home the home node for this entity
   * \param[in] msg_action function to trigger when message arrives for it
   * \param[in] migrated whether it migrated in: \c entityMigrated is preferred
   */
  void registerEntity(
    EntityID const& id, NodeType const& home,
    LocMsgActionType msg_action = nullptr, bool const& migrated = false
  );

  /**
   * \brief Unregister an entity
   *
   * \param[in] id the entity ID
   */
  void unregisterEntity(EntityID const& id);

  /**
   * \brief Tell coordinator that the entity has migrated to another node
   *
   *
   * \param[in] id the entity ID
   * \param[in] new_node the node it was migrated to
   */
  void entityEmigrated(EntityID const& id, NodeType const& new_node);

  /**
   * \brief Register a migrated entity on new node
   *
   *
   * This should be called after the entity is migrated when it arrived on the
   * new node: order of operations:
   *
   *   1) Node 0: registerEntity(my_id, ...);
   *   2) Node 0: entityMigrated(my_id, 1);
   *   3) Node 1: registerEntityMigrated(my_id, <home>, 0, ...);
   *
   * \param[in] id the entity ID
   * \param[in] home_node the home node for the entity
   * \param[in] msg_action function to trigger when message arrives for it
   */
  void entityImmigrated(
    EntityID const& id, NodeType const& home_node,
    NodeType const& __attribute__((unused)) from_node,
    LocMsgActionType msg_action = nullptr
  );

  /**
   * \brief Get the location of an entity
   *
   * Get the location of a entity: the `action' is triggered when the location
   * of the entity is resolved. This is an asynchronous call that may send
   * messages to discover the location of the entity `id'. To resolve the
   * location the method uses the following algorithm:
   *
   *   1) Check locally for the entity's existence
   *   2) If not local, search for a cache entry with location info
   *   3) If no cache information available, send resolution message to home node.
   *     a) The home node applies the same algorithm, starting with (1)
   *     b) On step 3, if no information is known, the manager buffers the
   *        request, waiting to the entity to be registered in the future.
   *
   * \note Migrations may make this information inaccurate; the node delivered
   * to `action' reflects the current known state, which may be remote.
   *
   * \param[in] id the entity ID
   * \param[in] home_node the home node for the entity
   * \param[in] action the action to trigger with the discovered location
   */
  void getLocation(
    EntityID const& id, NodeType const& home_node, NodeActionType const& action
  );

  /**
   * \brief Route a message with a custom handler
   *
   * \param[in] id the entity ID
   * \param[in] home_node home node for entity
   * \param[in] m pointer to the message
   */
  template <typename MessageT, ActiveTypedFnType<MessageT> *f>
  void routeMsgHandler(
    EntityID const& id, NodeType const& home_node, MessageT *m
  );

  /**
   * \brief Route a serialized message with a custom handler
   *
   * \param[in] id the entity ID
   * \param[in] home_node home node for entity
   * \param[in] msg pointer to the message
   */
  template <typename MessageT, ActiveTypedFnType<MessageT> *f>
  void routeMsgSerializeHandler(
    EntityID const& id, NodeType const& home_node, MsgSharedPtr<MessageT> msg
  );

  /**
   * \brief Route a message to the default handler
   *
   * \param[in] id the entity ID
   * \param[in] home_node home node for the entity
   * \param[in] msg pointer to the message
   * \param[in] serialize_msg whether it should be serialized (optional)
   * \param[in] from_node the sending node (optional)
   */
  template <typename MessageT>
  void routeMsg(
    EntityID const& id, NodeType const& home_node, MsgSharedPtr<MessageT> msg,
    bool const serialize_msg = false,
    NodeType from_node = uninitialized_destination
  );

  /**
   * \brief  Route a message to the default handler
   *
   * \param[in] id the entity ID
   * \param[in] home_node home node for the entity
   * \param[in] msg pointer to the message
   */
  template <typename MessageT>
  void routeMsgSerialize(
    EntityID const& id, NodeType const& home_node, MsgSharedPtr<MessageT> msg
  );

  /**
   * \internal \brief Route a message with non-eager protocol
   *
   * \param[in] id the entity ID
   * \param[in] home_node home node for the entity
   * \param[in] action action once entity is found
   */
  void routeNonEagerAction(
    EntityID const& id, NodeType const& home_node, ActionNodeType action
  );

  /**
   * \internal \brief Update location
   *
   * \param[in] event_id the event ID waiting on the location
   * \param[in] id the entity ID
   * \param[in] resolved_node the node reported to have the entity
   * \param[in] home_node the home node for the entity
   */
  void updatePendingRequest(
    LocEventID const& event_id, EntityID const& id,
    NodeType const& resolved_node, NodeType const& home_node
  );

  /**
   * \internal \brief Output the current cache state
   */
  void printCurrentCache() const;

  /**
   * \internal \brief Check if the purported location of an entity is cached
   *
   * \param[in] id the entity ID
   *
   * \return whether it is cached
   */
  bool isCached(EntityID const& id) const;

  /**
   * \internal \brief Clear the cache
   */
  void clearCache();

  /**
   * \internal \brief Send back an eager update on a discovered location
   *
   * \param[in] id the entity ID
   * \param[in] ask_node the asking node
   * \param[in] home_node the home node
   * \param[in] deliver_node the node discovered which delivered the message
   */
  void sendEagerUpdate(
    EntityID const& id, NodeType ask_node, NodeType home_node,
    NodeType deliver_node
  );

  /**
   * \internal \brief Update cache on eager update received
   *
   * \param[in] id the entity ID
   * \param[in] home_node the home node
   * \param[in] deliver_node the node discovered which delivered the message
   */
  void handleEagerUpdate(
    EntityID const& id, NodeType home_node, NodeType deliver_node
  );

  /**
   * \internal \brief Check if the eager or rendezvous protocol should be used
   *
   * The eager protocol, typically used for small messages, forwards the message
   * even if the location is stale or unknown (to the home node). The rendezvous
   * protocol, typically used for large messages, will send a control message to
   * determine the location of the entity before sending the actual data. The
   * threshold between these two modes is controlled by \c small_msg_max_size
   *
   * \param[in] msg the message to check
   *
   * \return whether it is of eager size
   */
  template <typename MessageT>
  bool useEagerProtocol(MsgSharedPtr<MessageT> msg) const;

private:
  /**
   * \internal \brief Handle relocation on different node.
   *
   * \param[in] msg the message
   */
  template <typename MessageT>
  static void routedHandler(MessageT *msg);

  /**
   * \internal \brief Request location handler from this node
   *
   * \param[in] msg the location request message
   */
  static void getLocationHandler(LocMsgType *msg);

  /**
   * \internal \brief Update the location on this node
   *
   * \param[in] msg the location update message
   */
  static void updateLocation(LocMsgType *msg);

  /**
   * \internal \brief Receive an eager location update
   *
   * \param[in] msg the location update message
   */
  static void recvEagerUpdate(LocMsgType *msg);

  /**
   * \internal \brief Route a message to destination with eager protocol
   *
   * \param[in] is_serialized whether it is serialized
   * \param[in] id the entity ID
   * \param[in] home_node the home node
   * \param[in] msg the message to route
   */
  template <typename MessageT>
  void routeMsgEager(
    bool const is_serialized, EntityID const& id, NodeType const& home_node,
    MsgSharedPtr<MessageT> msg
  );

  /**
   * \internal \brief Route a message to destination with rendezvous protocol
   *
   * \param[in] is_serialized whether it is serialized
   * \param[in] id the entity ID
   * \param[in] home_node the home node
   * \param[in] to_node destination node
   * \param[in] msg the message to route
   */
  template <typename MessageT>
  void routeMsgNode(
    bool const is_serialized, EntityID const& id, NodeType const& home_node,
    NodeType const& to_node, MsgSharedPtr<MessageT> msg
  );

  /**
   * \internal \brief Insert a pending entity action
   *
   * Add actions that are waiting on an entity to be registered on this
   * node. Once \c registerEntity is called, these actions will get triggered.
   *
   * \param[in] id the entity ID
   * \param[in] action action to execute
   */
  void insertPendingEntityAction(EntityID const& id, NodeActionType action);

public:
  /**
   * \internal \brief Get the instance identifier for this location manager
   *
   * \return the instance ID
   */
  LocInstType getInst() const;

  template <
    typename SerializerT,
    typename = std::enable_if_t<
      std::is_same<SerializerT, checkpoint::Footprinter>::value
    >
  >
  void serialize(SerializerT& s) {
    s | this_inst
      | local_registered_msg_han_
      | local_registered_
      | recs_
      | pending_actions_
      | pending_lookups_
      | loc_asks_;
  }

private:
  LocInstType this_inst = no_loc_inst;

  // message handlers for local registrations
  LocalRegisteredMsgContType local_registered_msg_han_;

  // registered entities
  LocalRegisteredContType local_registered_;

  // the cached location records
  LocCacheType recs_;

  // hold pending actions that this location manager is waiting on
  ActionContainerType pending_actions_;

  // pending lookup requests where this manager is the home node
  PendingLocLookupsType pending_lookups_;

  // List of nodes that inquire about an entity that require an update
  LocAsksType loc_asks_;
};

}}  // end namespace vt::location

#include "vt/topos/location/location.impl.h"

#endif /*INCLUDED_TOPOS_LOCATION_LOCATION_H*/
