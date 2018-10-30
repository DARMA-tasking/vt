
#if !defined INCLUDED_VRT_CONTEXT_CONTEXT_VRT_ATTORNEY_H
#define INCLUDED_VRT_CONTEXT_CONTEXT_VRT_ATTORNEY_H

#include "config.h"
#include "vrt/context/context_vrt.h"
#include "vrt/context/context_vrt_fwd.h"

namespace vt { namespace vrt {

struct VirtualContextAttorney {
  // Only allow access to modify a vrt context proxy for these runtime entities
  template <typename VrtContextT, typename... Args>
  friend struct VirtualMakeClosure;
  friend struct VirtualContextManager;

private:
  static void setProxy(VirtualContext* vc, VirtualProxyType in_proxy);
};

}} /* end namespace vt::vrt */

#endif /*INCLUDED_VRT_CONTEXT_CONTEXT_VRT_ATTORNEY_H*/
