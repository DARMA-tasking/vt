
#if ! defined __RUNTIME_TRANSPORT_LOCATION__
#define __RUNTIME_TRANSPORT_LOCATION__

#include "config.h"
#include "context.h"
#include "location_common.h"
#include "location_msg.h"
#include "location_record.h"
#include "location_cache.h"
#include "location_pending.h"
#include "location_entity.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>

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
   * unregisterEntity should be called; if the entity is migrated,
   * `entityMigrated' should be invoked where the registration occurred. The
   * function `msg_action' gets triggered when a message arrives that is
   * designated for the entity `id' that is registered. A message may arrive for
   * the entity by a location coordinator calling `routeMsg' to the associated
   * entity.
   */
  void registerEntity(EntityID const& id, LocMsgActionType msg_action = nullptr);
  void unregisterEntity(EntityID const& id);
  void entityMigrated(EntityID const& id, NodeType const& new_node);
  void getLocation(
    EntityID const& id, NodeType const& home_node, NodeActionType const& action
  );
  void printCurrentCache() const;

  template <typename MessageT>
  void routeMsg(
    EntityID const& id, NodeType const& home_node, MessageT* m,
    ActionType action = nullptr
  );

  void updatePendingRequest(LocEventID const& event_id, NodeType const& node);

private:
  template <typename MessageT>
  static void msgHandler(MessageT* msg);
  static void getLocationHandler(LocMsgType* msg);
  static void updateLocation(LocMsgType* msg);

  template <typename MessageT>
  void routeMsgEager(
    EntityID const& id, NodeType const& home_node, MessageT* msg,
    ActionType action = nullptr
  );

  template <typename MessageT>
  void routeMsgNode(
    EntityID const& id, NodeType const& home_node, NodeType const& to_node,
    MessageT* msg, ActionType action = nullptr
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
  using LocCoordPtrType = LocationCoord*;
  using LocInstContainerType = std::vector<LocCoordPtrType>;
  using VirtualLocMan = EntityLocationCoord<int32_t>;
  using VirtualContextLocMan = EntityLocationCoord<VrtContext_ProxyType>;

  std::unique_ptr<VirtualLocMan> virtual_loc = std::make_unique<VirtualLocMan>();
  std::unique_ptr<VirtualContextLocMan> vrtContextLoc =
    std::make_unique<VirtualContextLocMan>();

  static void insertInstance(int const i, LocCoordPtrType const& ptr);
  static LocCoordPtrType getInstance(int const inst);

private:
  static LocInstContainerType loc_insts;
};

}} // end namespace vt::location

namespace vt {

extern std::unique_ptr<location::LocationManager> theLocMan;

}

#include "location.impl.h"

#endif /*__RUNTIME_TRANSPORT_LOCATION__*/
