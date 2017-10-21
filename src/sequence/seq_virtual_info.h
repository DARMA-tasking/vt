
#if !defined INCLUDED_SEQUENCE_SEQ_VIRTUAL_INFO_H
#define INCLUDED_SEQUENCE_SEQ_VIRTUAL_INFO_H

#include "config.h"
#include "vrt/context/context_vrt.h"
#include "vrt/context/context_vrtproxy.h"

namespace vt { namespace seq {

struct VirtualInfo {
  VirtualInfo(VirtualProxyType const& in_proxy)
    : proxy(in_proxy)
  { }

  VirtualProxyType proxy;
};

}} //end namespace vt::seq

#endif /*INCLUDED_SEQUENCE_SEQ_VIRTUAL_INFO_H*/
