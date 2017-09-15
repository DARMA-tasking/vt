
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
#include <unordered_set>
#include <unordered_map>

namespace vt { namespace location {

template <typename EntityID>
struct EntityLocationCoord {
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

  EntityLocationCoord() : recs_(default_max_cache_size) { }

  void registerEntity(EntityID const& id, LocMsgActionType msg_action = nullptr);
  void unregisterEntity(EntityID const& id);
  void entityMigrated(EntityID const& id, NodeType const& new_node);
  void getLocation(
    EntityID const& id, NodeType const& home_node, NodeActionType const& action
  );
  void printCurrentCache() const;

  template <typename MessageT>
  void routeMsg(
    EntityID const& id, NodeType const& home_node, MessageT* m
  );

  template <typename MessageT>
  static void msgHandler(MessageT* msg);
  static void getLocationHandler(LocMsgType* msg);
  static void updateLocation(LocMsgType* msg);

private:
  void insertPendingEntityAction(EntityID const& id, NodeActionType action);
  void updatePendingRequest(LocEventID const& event_id, NodeType const& node);

private:
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
  // @todo: something like this with the type for virtual context
  using VirtualLocMan = EntityLocationCoord<int32_t>;

  std::unique_ptr<VirtualLocMan> virtual_loc = std::make_unique<VirtualLocMan>();
};

}} // end namespace vt::location

namespace vt {

extern std::unique_ptr<location::LocationManager> theLocMan;

}

#include "location.impl.h"

#endif /*__RUNTIME_TRANSPORT_LOCATION__*/
