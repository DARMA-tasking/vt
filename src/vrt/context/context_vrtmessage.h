
#if !defined INCLUDED_CONTEXT_VRT_MESSAGE
#define INCLUDED_CONTEXT_VRT_MESSAGE

#include <cassert>

#include "config.h"
#include "messaging/message.h"
#include "topos/location/location_msg.h"

namespace vt { namespace vrt {

template <typename MessageT>
using RoutedMessageType = LocationRoutedMsg<VirtualProxyType, MessageT>;

struct VirtualMessage : RoutedMessageType<vt::Message> {
  VirtualMessage() = default;

  void setVrtHandler(HandlerType const& in_handler) {
    vt_sub_handler = in_handler;
  }

  HandlerType getVrtHandler() const {
    assert(vt_sub_handler != uninitialized_handler and "Must have a valid handler");
    return vt_sub_handler;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    RoutedMessageType<vt::Message>::serialize(s);
    s | vt_sub_handler;
  }

 private:
  HandlerType vt_sub_handler = uninitialized_handler;
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_CONTEXT_VRT_MESSAGE*/
