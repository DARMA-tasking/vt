
#if !defined INCLUDED_TOPOS_LOCATION_LOCATION_H
#define INCLUDED_TOPOS_LOCATION_LOCATION_H

#include "config.h"
#include "topos/location/location_common.h"
#include "topos/location/location.fwd.h"
#include "topos/location/utility/pending.h"
#include "topos/location/utility/entity.h"
#include "topos/location/utility/coord.h"
#include "topos/location/message/msg.h"
#include "topos/location/cache/cache.h"
#include "topos/location/record/record.h"
#include "topos/location/record/state.h"
#include "context/context.h"
#include "activefn/activefn.h"
#include "vrt/vrt_common.h"

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
  using LocCacheType = LocationCache<EntityID, LocRecType>;
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
   *   3) Node 1: registerEntityMigrated(my_id, 0, ...);
   */
  void registerEntityMigrated(
    EntityID const& id, NodeType const& __attribute__((unused)) from,
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
    EntityID const& id,
    NodeType const& home_node,
    NodeActionType const& action
  );

  template <typename MessageT, ActiveTypedFnType<MessageT> *f>
  void routeMsgHandler(
    EntityID const& id, NodeType const& home_node, MessageT *m,
    ActionType action = nullptr
  );

  template <typename MessageT, ActiveTypedFnType<MessageT> *f>
  void routeMsgSerializeHandler(
    EntityID const& id, NodeType const& home_node, MessageT *m,
    ActionType action = nullptr
  );

  template <typename MessageT>
  void routeMsg(
    EntityID const& id, NodeType const& home_node, MessageT *m,
    ActionType action = nullptr, bool const serialize = false
  );

  template <typename MessageT>
  void routeMsgSerialize(
    EntityID const& id, NodeType const& home_node, MessageT *m,
    ActionType action = nullptr
  );

  void routeNonEagerAction(
    EntityID const& id, NodeType const& home_node, ActionNodeType action
  );

  void updatePendingRequest(
    LocEventID const& event_id, EntityID const& id, NodeType const& node
  );
  void printCurrentCache() const;

private:
  template <typename MessageT>
  static void msgHandler(MessageT *msg);
  static void getLocationHandler(LocMsgType *msg);
  static void updateLocation(LocMsgType *msg);

  template <typename MessageT>
  void routeMsgEager(
    bool const serialize, EntityID const& id, NodeType const& home_node,
    MessageT *msg, ActionType action = nullptr
  );

  template <typename MessageT>
  void routeMsgNode(
    bool const serialize, EntityID const& id, NodeType const& home_node,
    NodeType const& to_node, MessageT *msg, ActionType action = nullptr
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

#include "topos/location/location.impl.h"

#endif /*INCLUDED_TOPOS_LOCATION_LOCATION_H*/
