
#if ! defined __RUNTIME_TRANSPORT_LOCATION_ENTITY__
#define __RUNTIME_TRANSPORT_LOCATION_ENTITY__

#include "config.h"
#include "message.h"
#include "location_common.h"

namespace vt { namespace location {

template <typename EntityID>
struct LocEntity {
  EntityID entity;
  LocMsgActionType msg_action = nullptr;

  LocEntity(
    EntityID const& in_entity, LocMsgActionType in_msg_action
  ) : entity(in_entity), msg_action(in_msg_action)
  { }

  template <typename MessageT>
  void applyRegisteredActionMsg(MessageT* msg) {
    msg_action(static_cast<BaseMessage*>(msg));
  }
};

}} // end namespace vt::location

#endif /*__RUNTIME_TRANSPORT_LOCATION_ENTITY__*/
