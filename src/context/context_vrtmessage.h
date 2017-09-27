
#if !defined INCLUDED_CONTEXT_VRT_MESSAGE
#define INCLUDED_CONTEXT_VRT_MESSAGE

#include "messaging/message.h"
#include "config.h"

namespace vt { namespace vrt {

template <typename MessageT>
using RoutedMessageType = LocationRoutedMsg<VrtContext_ProxyType, MessageT>;

template <typename MessageT>
struct VrtContextMessage : RoutedMessageType<MessageT> {
  VrtContextMessage() = default;
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_CONTEXT_VRT_MESSAGE*/