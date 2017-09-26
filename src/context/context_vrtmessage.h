
#if !defined INCLUDED_CONTEXT_VRT_MESSAGE
#define INCLUDED_CONTEXT_VRT_MESSAGE

#include "messaging/message.h"
#include "config.h"

namespace vt { namespace vrt {

struct VrtContextMessage : vt::Message {
  VrtContext_IdType to;

  VrtContextMessage() : Message() {}
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_CONTEXT_VRT_MESSAGE*/