
#if !defined INCLUDED_TOPOS_LOCATION_UTILITY_ENTITY_H
#define INCLUDED_TOPOS_LOCATION_UTILITY_ENTITY_H

#include "vt/config.h"
#include "vt/topos/location/location_common.h"
#include "vt/topos/location/location.fwd.h"
#include "vt/messaging/message.h"

namespace vt { namespace location {

template <typename EntityID>
struct LocEntity {
  LocEntity(EntityID const& in_entity, LocMsgActionType in_msg_action)
    : entity_(in_entity), msg_action_(in_msg_action)
  { }

  template <typename MessageT>
  void applyRegisteredActionMsg(MessageT *msg) {
    msg_action_(static_cast<BaseMessage *>(msg));
  }

  friend struct EntityLocationCoord<EntityID>;

private:
  EntityID entity_;
  LocMsgActionType msg_action_ = nullptr;
};

}}  // end namespace vt::location

#endif /*INCLUDED_TOPOS_LOCATION_UTILITY_ENTITY_H*/
