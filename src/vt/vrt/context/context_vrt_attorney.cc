
#if !defined INCLUDED_VRT_CONTEXT_CONTEXT_VRT_ATTORNEY_CC
#define INCLUDED_VRT_CONTEXT_CONTEXT_VRT_ATTORNEY_CC

#include "vt/config.h"
#include "vt/vrt/context/context_vrt_attorney.h"

namespace vt { namespace vrt {

/*static*/ void VirtualContextAttorney::setProxy(
  VirtualContext* vc, VirtualProxyType in_proxy
) {
  vc->setProxy(in_proxy);
}

}} /* end namespace vt::vrt */

#endif /*INCLUDED_VRT_CONTEXT_CONTEXT_VRT_ATTORNEY_CC*/
