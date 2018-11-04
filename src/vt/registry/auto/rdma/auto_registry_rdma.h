
#if !defined INCLUDED_REGISTRY_AUTO_RDMA_AUTO_REGISTRY_RDMA_H
#define INCLUDED_REGISTRY_AUTO_RDMA_AUTO_REGISTRY_RDMA_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/registry/registry.h"
#include "vt/activefn/activefn.h"
#include "vt/vrt/collection/active/active_funcs.h"

namespace vt { namespace auto_registry {

AutoActiveRDMAGetType getAutoHandlerRDMAGet(HandlerType const& handler);
AutoActiveRDMAPutType getAutoHandlerRDMAPut(HandlerType const& handler);

template <typename MsgT, ActiveTypedRDMAPutFnType<MsgT>* f>
HandlerType makeAutoHandlerRDMAPut(MsgT* const msg);

template <typename MsgT, ActiveTypedRDMAGetFnType<MsgT>* f>
HandlerType makeAutoHandlerRDMAGet(MsgT* const msg);

}} /* end namespace vt::auto_registry */

#include "vt/registry/auto/rdma/auto_registry_rdma.impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_RDMA_AUTO_REGISTRY_RDMA_H*/
