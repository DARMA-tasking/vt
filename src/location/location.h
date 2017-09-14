
#if ! defined __RUNTIME_TRANSPORT_LOCATION__
#define __RUNTIME_TRANSPORT_LOCATION__

#include "common.h"
#include "context.h"
#include "location_common.h"
#include "location_msg.h"
#include "location_record.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace vt { namespace location {

template <typename EntityID>
struct EntityLocationCoord {
  using LocRecType = LocRecord;
  using RecContainerType = std::unordered_map<EntityID, LocRecType>;
  using ActionContainerType = std::unordered_map<LocEventID, NodeActionType>;
  using LocMsgType = LocationMsg<EntityID>;

  void registerEntity(EntityID const& id);
  void entityMigrated(EntityID const& id, NodeType const& new_node);
  void getLocation(
    EntityID const& id, NodeType const& home_node, NodeActionType const& action
  );
  void updatePendingRequest(LocEventID const& event_id, NodeType const& node);

  static void getLocationHandler(LocMsgType* msg);
  static void updateLocation(LocMsgType* msg);

private:
  // @todo: this should be only allowed to grow so large
  RecContainerType recs_;

  ActionContainerType pending_actions_;
};

struct LocationManager {
  // @todo: something like this with the type for virtual context
  using VirtualLocMan = EntityLocationCoord<int32_t>;

  VirtualLocMan virtual_loc;
};

}} // end namespace vt::location

namespace vt {

extern std::unique_ptr<location::LocationManager> theLocMan;

}

#include "location.impl.h"

#endif /*__RUNTIME_TRANSPORT_LOCATION__*/
