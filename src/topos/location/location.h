
#if !defined INCLUDED_TOPOS_LOCATION
#define INCLUDED_TOPOS_LOCATION

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "config.h"
#include "context.h"
#include "location_common.h"
#include "location_msg.h"
#include "location_record.h"
#include "location_cache.h"
#include "location_pending.h"
#include "location_entity.h"

namespace vt { namespace location {

// General base class for the location coords to erase templated types
struct LocationCoord {
  int data;
};

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
  void registerEntity(EntityID const& id, LocMsgActionType msg_action = nullptr);
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

  template <typename MessageT>
  void routeMsg(
      EntityID const& id, NodeType const& home_node, MessageT *m,
      ActionType action = nullptr
  );

  void updatePendingRequest(LocEventID const& event_id, NodeType const& node);
  void printCurrentCache() const;

 private:
  template <typename MessageT>
  static void msgHandler(MessageT *msg);
  static void getLocationHandler(LocMsgType *msg);
  static void updateLocation(LocMsgType *msg);

  template <typename MessageT>
  void routeMsgEager(
      EntityID const& id, NodeType const& home_node, MessageT *msg,
      ActionType action = nullptr
  );

  template <typename MessageT>
  void routeMsgNode(
      EntityID const& id, NodeType const& home_node, NodeType const& to_node,
      MessageT *msg, ActionType action = nullptr
  );

  void insertPendingEntityAction(EntityID const& id, NodeActionType action);

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

struct LocationManager {
  using LocCoordPtrType = LocationCoord *;
  using LocInstContainerType = std::vector<LocCoordPtrType>;
  using VirtualLocMan = EntityLocationCoord<int32_t>;
  using VirtualContextLocMan = EntityLocationCoord<VirtualProxyType>;

  std::unique_ptr<VirtualLocMan>
      virtual_loc = std::make_unique<VirtualLocMan>();
  std::unique_ptr<VirtualContextLocMan> vrtContextLoc =
      std::make_unique<VirtualContextLocMan>();

  static void insertInstance(int const i, LocCoordPtrType const& ptr);
  static LocCoordPtrType getInstance(int const inst);

 private:
  static LocInstContainerType loc_insts;
};

}}  // end namespace vt::location

namespace vt {

extern std::unique_ptr<location::LocationManager> theLocMan;

}  // end namespace vt

#include "location.impl.h"

#endif  /*INCLUDED_TOPOS_LOCATION*/
