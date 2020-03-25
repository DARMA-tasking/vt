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

#include <cstdint>
#include <memory>
#include <vector>
#include <list>
#include <unordered_set>
#include <unordered_map>

namespace vt { namespace location {

struct collection_lm_tag_t {};

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

  template <typename MessageT>
  using EntityMsgType = EntityMsg<EntityID, MessageT>;

  EntityLocationCoord();
  EntityLocationCoord(collection_lm_tag_t, LocInstType identifier);
  explicit EntityLocationCoord(LocInstType const identifier);

  virtual ~EntityLocationCoord();

  /*
   * The `registerEntity' method allows an external component to locally
   * register an entity as existing on this node. If the entity is deleted,
   * `unregisterEntity' should be called; if the entity is migrated,
   * `entityMigrated' should be invoked on the node where the entity is
   * migrating from. The function `msg_action' gets triggered when a message
   * arrives that is designated for the entity `id' that is registered. A
   * message may arrive for the entity by a location coordinator calling
   * `routeMsg' to the associated entity.
   */
  void registerEntity(
    EntityID const& id, NodeType const& home,
    LocMsgActionType msg_action = nullptr, bool const& migrated = false
  );
  void unregisterEntity(EntityID const& id);
  void entityMigrated(EntityID const& id, NodeType const& new_node);

  /*
   * Should be called after the entity is migrated when it arrived on the new
   * node: order of operations:
   *
   *   1) Node 0: registerEntity(my_id, ...);
   *   2) Node 0: entityMigrated(my_id, 1);
   *   3) Node 1: registerEntityMigrated(my_id, <home>, 0, ...);
   */
  void registerEntityMigrated(
    EntityID const& id, NodeType const& home_node,
    NodeType const& __attribute__((unused)) from_node,
    LocMsgActionType msg_action = nullptr
  );

  /*
   * Get the location of a entity: the `action' is triggered when the location
   * of the entity is resolved. This is an asynchronous call that may send
   * messages to discover the location of the entity `id'. To resolve the
   * location the method uses the following algorithm:
   *
   *   1) Check locally for the entity's existence
   *.  2) If not local, search for a cache entry with location info
   *   3) If no cache information available, send resolution message to home node.
   *     a) The home node applies the same algorithm, starting with (1)
   *. .  b) On step 3, if no information is known, the manager buffers the
   *.       request, waiting to the entity to be registered in the future.
   *
   * Note: migrations may make this information inaccurate; the node delivered
   * to `action' reflects the current known state, which may be remote.
   */
  void getLocation(
    EntityID const& id, NodeType const& home_node, NodeActionType const& action
  );

  template <typename MessageT, ActiveTypedFnType<MessageT> *f>
  void routeMsgHandler(
    EntityID const& id, NodeType const& home_node, MessageT *m
  );

  template <typename MessageT, ActiveTypedFnType<MessageT> *f>
  void routeMsgSerializeHandler(
    EntityID const& id, NodeType const& home_node, MsgSharedPtr<MessageT> msg
  );

  template <typename MessageT>
  void routeMsg(
    EntityID const& id, NodeType const& home_node, MsgSharedPtr<MessageT> msg,
    bool const serialize = false,
    NodeType from_node = uninitialized_destination
  );

  template <typename MessageT>
  void routeMsgSerialize(
    EntityID const& id, NodeType const& home_node, MsgSharedPtr<MessageT> msg
  );

  void routeNonEagerAction(
    EntityID const& id, NodeType const& home_node, ActionNodeType action
  );

  void updatePendingRequest(
    LocEventID const& event_id, EntityID const& id,
    NodeType const& resolved_node, NodeType const& home_node
  );
  void printCurrentCache() const;

  bool isCached(EntityID const& id) const;
  void clearCache();

  template <typename MessageT>
  bool useEagerProtocol(MsgSharedPtr<MessageT> msg) const;

private:
  template <typename MessageT>
  static void msgHandler(MessageT *msg);
  static void getLocationHandler(LocMsgType *msg);
  static void updateLocation(LocMsgType *msg);

  template <typename MessageT>
  void routeMsgEager(
    bool const serialize, EntityID const& id, NodeType const& home_node,
    MsgSharedPtr<MessageT> msg
  );

  template <typename MessageT>
  void routeMsgNode(
    bool const serialize, EntityID const& id, NodeType const& home_node,
    NodeType const& to_node, MsgSharedPtr<MessageT> msg
  );

  void insertPendingEntityAction(EntityID const& id, NodeActionType action);

public:
  LocInstType getInst() const;

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
};

}}  // end namespace vt::location

#include "vt/topos/location/location.impl.h"

#endif /*INCLUDED_TOPOS_LOCATION_LOCATION_H*/
