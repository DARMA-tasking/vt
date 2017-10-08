
#if ! defined __RUNTIME_TRANSPORT_SEQUENCER_VIRTUAL_INFO__
#define __RUNTIME_TRANSPORT_SEQUENCER_VIRTUAL_INFO__

#include "config.h"
#include "context/context_vrt.h"
#include "context/context_vrtproxy.h"

namespace vt { namespace seq {

struct VirtualInfo {
  VirtualInfo(VirtualProxyType const& in_proxy)
    : proxy(in_proxy)
  { }

  VirtualProxyType proxy;
};

}} //end namespace vt::seq

#endif /*__RUNTIME_TRANSPORT_SEQUENCER_VIRTUAL_INFO__*/
